#include "lua.h"
#include "lauxlib.h"
#include "minheap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MINHEAP_MT "minheap"

struct lua_minheap {
    minheap *h;
};

/* Convert Lua value at idx to a type-prefixed heap key (caller frees). */
static char *value_to_key(lua_State *L, int idx) {
    if (lua_isinteger(L, idx)) {
        lua_Integer n = lua_tointeger(L, idx);
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "I%lld", (long long)n);
        char *key = malloc((size_t)len + 1);
        if (!key) return NULL;
        memcpy(key, buf, (size_t)len + 1);
        return key;
    }
    if (lua_type(L, idx) == LUA_TSTRING) {
        size_t len;
        const char *s = lua_tolstring(L, idx, &len);
        char *key = malloc(len + 2);
        if (!key) return NULL;
        key[0] = 'S';
        memcpy(key + 1, s, len + 1);
        return key;
    }
    luaL_error(L, "value must be integer or string");
    return NULL;
}

/* Push the decoded Lua value from a type-prefixed key. */
static void key_to_value(lua_State *L, const char *key) {
    if (key[0] == 'I') {
        lua_pushinteger(L, (lua_Integer)atoll(key + 1));
    } else {
        lua_pushstring(L, key + 1);
    }
}

/* Push {value = ..., priority = ...} table. */
static void push_result(lua_State *L, const char *key, double priority) {
    lua_newtable(L);
    key_to_value(L, key);
    lua_setfield(L, -2, "value");
    lua_pushnumber(L, priority);
    lua_setfield(L, -2, "priority");
}

/* ── Lua methods ── */

static int lminheap_push(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    luaL_checkany(L, 2);
    luaL_checkany(L, 3);
    double priority = luaL_checknumber(L, 3);
    char *key = value_to_key(L, 2);
    if (key) {
        minheap_push(lh->h, key, priority);
        free(key);
    }
    return 0;
}

static int lminheap_pop(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    const char *key;
    double priority;
    if (!minheap_pop(lh->h, &key, &priority)) {
        lua_pushnil(L);
        return 1;
    }
    push_result(L, key, priority);
    free((void *)key);
    return 1;
}

static int lminheap_peek(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    const char *key;
    double priority;
    if (!minheap_peek(lh->h, &key, &priority)) {
        lua_pushnil(L);
        return 1;
    }
    push_result(L, key, priority);
    return 1;
}

static int lminheap_empty(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    lua_pushboolean(L, minheap_empty(lh->h));
    return 1;
}

static int lminheap_size(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    lua_pushinteger(L, minheap_size(lh->h));
    return 1;
}

static int lminheap_clear(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    minheap_clear(lh->h);
    return 0;
}

static int lminheap_remove(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    luaL_checkany(L, 2);
    char *key = value_to_key(L, 2);
    if (key) {
        minheap_remove(lh->h, key);
        free(key);
    }
    return 0;
}

static int lminheap_contains(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    luaL_checkany(L, 2);
    char *key = value_to_key(L, 2);
    int found = key ? minheap_contains(lh->h, key) : 0;
    free(key);
    lua_pushboolean(L, found);
    return 1;
}

static int lminheap_update(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    luaL_checkany(L, 2);
    double priority = luaL_checknumber(L, 3);
    char *key = value_to_key(L, 2);
    if (key) {
        minheap_update(lh->h, key, priority);
        free(key);
    }
    return 0;
}

/* ── metatable & module ── */

static int lminheap_gc(lua_State *L) {
    struct lua_minheap *lh = luaL_checkudata(L, 1, MINHEAP_MT);
    if (lh->h) {
        minheap_free(lh->h);
        lh->h = NULL;
    }
    return 0;
}

static int lminheap_new(lua_State *L) {
    struct lua_minheap *lh = lua_newuserdata(L, sizeof(*lh));
    lh->h = minheap_new();
    if (!lh->h) {
        luaL_error(L, "minheap_new: allocation failed");
    }
    luaL_getmetatable(L, MINHEAP_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static const luaL_Reg minheap_methods[] = {
    {"push",     lminheap_push},
    {"pop",      lminheap_pop},
    {"peek",     lminheap_peek},
    {"empty",    lminheap_empty},
    {"size",     lminheap_size},
    {"clear",    lminheap_clear},
    {"remove",   lminheap_remove},
    {"contains", lminheap_contains},
    {"update",   lminheap_update},
    {NULL, NULL}
};

static const luaL_Reg minheap_lib[] = {
    {"new", lminheap_new},
    {NULL, NULL}
};

int luaopen_minheap(lua_State *L) {
    luaL_newmetatable(L, MINHEAP_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, minheap_methods, 0);
    lua_pushcfunction(L, lminheap_gc);
    lua_setfield(L, -2, "__gc");
    luaL_newlib(L, minheap_lib);
    return 1;
}
