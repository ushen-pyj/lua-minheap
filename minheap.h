#pragma once

/* Opaque heap handle */
typedef struct minheap minheap;

minheap *minheap_new(void);
void minheap_free(minheap *h);

void minheap_push(minheap *h, const char *key, double priority);

/* pop: writes *key and *priority, returns 1; on empty returns 0 */
int minheap_pop(minheap *h, const char **key, double *priority);

/* peek: writes *key and *priority, returns 1; on empty returns 0 */
int minheap_peek(minheap *h, const char **key, double *priority);

int minheap_empty(const minheap *h);
int minheap_size(const minheap *h);
void minheap_clear(minheap *h);

/* remove by key; silently ignored if key not found */
void minheap_remove(minheap *h, const char *key);

/* contains: returns 1 if key exists, 0 otherwise */
int minheap_contains(const minheap *h, const char *key);

/* update priority; silently ignored if key not found */
void minheap_update(minheap *h, const char *key, double new_priority);
