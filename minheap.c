#include "minheap.h"
#include <stdlib.h>
#include <string.h>

/* ── hash table (open addressing, linear probing) ── */

static char TOMBSTONE_MARKER;
#define TOMBSTONE (&TOMBSTONE_MARKER)

typedef struct hash_entry {
    char *key;   /* NULL=empty, TOMBSTONE=deleted, otherwise owned string */
    int index;   /* 1-based heap index */
} hash_entry;

typedef struct hash_table {
    hash_entry *buckets;
    int capacity;
    int occupied;  /* active entries only */
} hash_table;

static unsigned long hash_str(const char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) h = ((h << 5) + h) + c;
    return h;
}

static hash_table *ht_new(int cap) {
    hash_table *ht = malloc(sizeof(*ht));
    if (!ht) return NULL;
    ht->buckets = calloc((size_t)cap, sizeof(hash_entry));
    if (!ht->buckets) { free(ht); return NULL; }
    ht->capacity = cap;
    ht->occupied = 0;
    return ht;
}

static void ht_free(hash_table *ht) {
    if (!ht) return;
    for (int i = 0; i < ht->capacity; i++) {
        char *k = ht->buckets[i].key;
        if (k && k != TOMBSTONE) free(k);
    }
    free(ht->buckets);
    free(ht);
}

static int ht_find_slot(hash_table *ht, const char *key, int *found) {
    unsigned long h = hash_str(key);
    int first_tombstone = -1;
    for (int i = 0; i < ht->capacity; i++) {
        int idx = (int)((h + (unsigned long)i) % (unsigned long)ht->capacity);
        char *k = ht->buckets[idx].key;
        if (k == NULL) {
            *found = 0;
            return first_tombstone >= 0 ? first_tombstone : idx;
        }
        if (k == TOMBSTONE) {
            if (first_tombstone < 0) first_tombstone = idx;
        } else if (strcmp(k, key) == 0) {
            *found = 1;
            return idx;
        }
    }
    *found = 0;
    return first_tombstone;  /* table full of tombstones */
}

static int ht_resize(hash_table *ht, int new_cap);

static void ht_put(hash_table *ht, const char *key, int index) {
    if (ht->occupied >= ht->capacity * 3 / 4) {
        if (ht_resize(ht, ht->capacity * 2) != 0) return;
    }
    int found;
    int slot = ht_find_slot(ht, key, &found);
    if (found) {
        ht->buckets[slot].index = index;
    } else {
        char *copy = malloc(strlen(key) + 1);
        if (!copy) return;
        strcpy(copy, key);
        if (ht->buckets[slot].key != TOMBSTONE)
            ht->occupied++;
        ht->buckets[slot].key = copy;
        ht->buckets[slot].index = index;
    }
}

static int ht_get(hash_table *ht, const char *key) {
    unsigned long h = hash_str(key);
    for (int i = 0; i < ht->capacity; i++) {
        int idx = (int)((h + (unsigned long)i) % (unsigned long)ht->capacity);
        char *k = ht->buckets[idx].key;
        if (k == NULL) return 0;        /* not found */
        if (k == TOMBSTONE) continue;   /* skip tombstone */
        if (strcmp(k, key) == 0) return ht->buckets[idx].index;
    }
    return 0;
}

static void ht_remove(hash_table *ht, const char *key) {
    unsigned long h = hash_str(key);
    for (int i = 0; i < ht->capacity; i++) {
        int idx = (int)((h + (unsigned long)i) % (unsigned long)ht->capacity);
        char *k = ht->buckets[idx].key;
        if (k == NULL) return;
        if (k == TOMBSTONE) continue;
        if (strcmp(k, key) == 0) {
            free(k);
            ht->buckets[idx].key = TOMBSTONE;
            ht->occupied--;
            return;
        }
    }
}

static void ht_clear(hash_table *ht) {
    for (int i = 0; i < ht->capacity; i++) {
        char *k = ht->buckets[i].key;
        if (k && k != TOMBSTONE) free(k);
        ht->buckets[i].key = NULL;
        ht->buckets[i].index = 0;
    }
    ht->occupied = 0;
}

static int ht_resize(hash_table *ht, int new_cap) {
    hash_table *new_ht = ht_new(new_cap);
    if (!new_ht) return -1;
    for (int i = 0; i < ht->capacity; i++) {
        char *k = ht->buckets[i].key;
        if (k && k != TOMBSTONE)
            ht_put(new_ht, k, ht->buckets[i].index);
    }
    hash_entry *old_buckets = ht->buckets;
    int old_cap = ht->capacity;
    ht->buckets = new_ht->buckets;
    ht->capacity = new_ht->capacity;
    ht->occupied = new_ht->occupied;
    new_ht->buckets = old_buckets;
    new_ht->capacity = old_cap;
    ht_free(new_ht);  /* frees old buckets (keys already moved) */
    /* patch: new_ht keys are now owned by ht; prevent double-free */
    /* We moved the buckets array ownership above, so ht_free on new_ht
       won't free the keys — they're in ht now. The old keys were
       consumed by ht_put (strdup'd). So the old buckets' key pointers
       are still around. ht_free will free them — but they were strdup'd
       by ht_put into ht already. So the original key strings in the
       old buckets should be freed by ht_free. This is actually correct
       since ht_put copies keys. */
    return 0;
}

/* ── min-heap ── */

#define HEAP_INIT_CAP 16

typedef struct {
    char *key;       /* owned */
    double priority;
} heap_entry;

struct minheap {
    heap_entry *array;  /* 1-based; array[0] unused */
    int capacity;
    int size;
    hash_table *ht;
};

minheap *minheap_new(void) {
    minheap *h = malloc(sizeof(*h));
    if (!h) return NULL;
    h->capacity = HEAP_INIT_CAP;
    h->array = calloc((size_t)h->capacity, sizeof(heap_entry));
    if (!h->array) { free(h); return NULL; }
    h->size = 0;
    h->ht = ht_new(HEAP_INIT_CAP);
    if (!h->ht) { free(h->array); free(h); return NULL; }
    return h;
}

void minheap_free(minheap *h) {
    if (!h) return;
    for (int i = 1; i <= h->size; i++) free(h->array[i].key);
    free(h->array);
    ht_free(h->ht);
    free(h);
}

/* grow array by doubling */
static int grow(minheap *h) {
    int new_cap = h->capacity * 2;
    heap_entry *new_arr = realloc(h->array, (size_t)new_cap * sizeof(heap_entry));
    if (!new_arr) return -1;
    h->array = new_arr;
    h->capacity = new_cap;
    return 0;
}

static void swap(minheap *h, int i, int j) {
    heap_entry tmp = h->array[i];
    h->array[i] = h->array[j];
    h->array[j] = tmp;
    ht_put(h->ht, h->array[i].key, i);
    ht_put(h->ht, h->array[j].key, j);
}

static int less(minheap *h, int i, int j) {
    return h->array[i].priority < h->array[j].priority;
}

static void sift_up(minheap *h, int i) {
    while (i > 1) {
        int p = i / 2;
        if (less(h, i, p)) {
            swap(h, i, p);
            i = p;
        } else {
            break;
        }
    }
}

static void sift_down(minheap *h, int i) {
    for (;;) {
        int smallest = i;
        int left = 2 * i;
        int right = 2 * i + 1;
        if (left <= h->size && less(h, left, smallest)) smallest = left;
        if (right <= h->size && less(h, right, smallest)) smallest = right;
        if (smallest == i) break;
        swap(h, i, smallest);
        i = smallest;
    }
}

void minheap_push(minheap *h, const char *key, double priority) {
    if (h->size + 1 >= h->capacity) {
        if (grow(h) != 0) return;
    }
    int i = h->size + 1;
    h->array[i].key = malloc(strlen(key) + 1);
    if (!h->array[i].key) return;
    strcpy(h->array[i].key, key);
    h->array[i].priority = priority;
    h->size++;
    ht_put(h->ht, key, i);
    sift_up(h, i);
}

int minheap_pop(minheap *h, const char **key, double *priority) {
    if (h->size == 0) return 0;
    heap_entry root = h->array[1];
    ht_remove(h->ht, root.key);
    if (h->size == 1) {
        h->size = 0;
    } else {
        h->array[1] = h->array[h->size];
        ht_put(h->ht, h->array[1].key, 1);
        h->size--;
        sift_down(h, 1);
    }
    *key = root.key;       /* caller must free */
    *priority = root.priority;
    return 1;
}

int minheap_peek(minheap *h, const char **key, double *priority) {
    if (h->size == 0) return 0;
    *key = h->array[1].key;
    *priority = h->array[1].priority;
    return 1;
}

int minheap_empty(const minheap *h) {
    return h->size == 0;
}

int minheap_size(const minheap *h) {
    return h->size;
}

void minheap_clear(minheap *h) {
    for (int i = 1; i <= h->size; i++) free(h->array[i].key);
    ht_clear(h->ht);
    h->size = 0;
}

void minheap_remove(minheap *h, const char *key) {
    int idx = ht_get(h->ht, key);
    if (idx == 0) return;  /* not found */
    ht_remove(h->ht, key);
    free(h->array[idx].key);
    if (idx == h->size) {
        h->size--;
        return;
    }
    /* replace with last element */
    h->array[idx] = h->array[h->size];
    ht_put(h->ht, h->array[idx].key, idx);
    h->size--;
    /* fix heap property: sift up or down as needed */
    if (idx > 1 && less(h, idx, idx / 2))
        sift_up(h, idx);
    else
        sift_down(h, idx);
}

int minheap_contains(const minheap *h, const char *key) {
    return ht_get(h->ht, key) != 0;
}

void minheap_update(minheap *h, const char *key, double new_priority) {
    int idx = ht_get(h->ht, key);
    if (idx == 0) return;
    double old = h->array[idx].priority;
    h->array[idx].priority = new_priority;
    if (new_priority < old)
        sift_up(h, idx);
    else if (new_priority > old)
        sift_down(h, idx);
}
