#include "kvs_clock.h"
#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**Struct Definitions**/
typedef struct queue_node {
  char *key;
  char *value;
  bool dirty;
  int reference_bit;  // 0 or 1
  struct queue_node *next;
  struct queue_node *prev;
} queue_node;

typedef struct {
  queue_node *front;  // oldest
  queue_node *rear;   // newest
  queue_node *cursor;
  int size;  // curr num of nodes in the queue
} queue_t;

struct kvs_clock {
  // TODO: add necessary variables
  kvs_base_t *kvs_base;
  int capacity;
  queue_t *queue;
};
/**Helper Functions**/
static queue_node *create_node(const char *key, const char *value) {
  queue_node *node = malloc(sizeof(queue_node));
  if (node == NULL) {
    return NULL;
  }

  node->key = strdup(key);
  node->value = strdup(value);
  node->dirty = false;
  node->reference_bit = 1;
  node->prev = NULL;
  node->next = NULL;

  if (node->key == NULL || node->value == NULL) {
    free(node->key);
    free(node->value);
    free(node);
    return NULL;
  }

  return node;
}

static void free_node(queue_node *node) {
  if (node) {
    free(node->key);
    free(node->value);
    free(node);
  }
}

static int enqueue(queue_t *queue, queue_node *node) {
  // queue_node *node = create(key, value);
  if (node == NULL) {
    return 0;
  }
  // node->dirty = dirty;
  node->reference_bit = 1;

  if (queue->rear != NULL) {
    queue->rear->next = node;
    node->prev = queue->rear;
  } else {
    queue->front = node;
  }
  queue->rear = node;
  queue->size++;

  if (queue->cursor == NULL) {
    queue->cursor = node;
  }

  return 1;
}

static queue_node *dequeue(queue_t *queue) {
  if (queue->front == NULL) return NULL;

  queue_node *node = queue->front;
  queue->front = queue->front->next;

  if (queue->front == NULL) {
    queue->rear = NULL;
    queue->cursor = NULL;
  } else {
    queue->front->prev = NULL;
  }
  queue->size--;

  return node;
}

static queue_node *clock_evict(queue_t *queue) {
  int fullCycle = 0;

  while (fullCycle < 2) {
    if (queue->cursor->reference_bit == 0) {
      queue_node *evict = queue->cursor;

      if (evict == queue->front) queue->front = evict->next;
      if (evict == queue->rear) queue->rear = evict->prev;
      if (evict->next) evict->next->prev = evict->prev;
      if (evict->prev) evict->prev->next = evict->next;

      if (queue->cursor->next) {
        queue->cursor = queue->cursor->next;
      } else {
        queue->cursor = queue->front;
        fullCycle++;
      }

      return evict;
    } else {
      queue->cursor->reference_bit = 0;
      queue->cursor = queue->cursor->next ? queue->cursor->next : queue->front;
    }
    fullCycle = queue->cursor == queue->front ? fullCycle + 1 : fullCycle;
  }
  return NULL;
}
/**kvs_clock Function Definitions**/
kvs_clock_t *kvs_clock_new(kvs_base_t *kvs, int capacity) {
  kvs_clock_t *kvs_clock = malloc(sizeof(kvs_clock_t));
  if (kvs_clock == NULL) {
    return NULL;
  }
  kvs_clock->kvs_base = kvs;
  kvs_clock->capacity = capacity;

  // TODO: initialize other variables
  kvs_clock->queue = malloc(sizeof(queue_t));
  if (kvs_clock->queue == NULL) {
    free(kvs_clock);
    return NULL;
  }

  kvs_clock->queue->front = NULL;
  kvs_clock->queue->rear = NULL;
  kvs_clock->queue->cursor = NULL;
  kvs_clock->queue->size = 0;
  return kvs_clock;
}

void kvs_clock_free(kvs_clock_t **ptr) {
  // TODO: free dynamically allocated memory
  if (ptr == NULL || *ptr == NULL) return;

  queue_node *curr = (*ptr)->queue->front;
  while (curr != NULL) {
    queue_node *next = curr->next;
    free_node(curr);
    curr = next;
  }

  free((*ptr)->queue);
  free(*ptr);
  *ptr = NULL;
}

int kvs_clock_set(kvs_clock_t *kvs_clock, const char *key, const char *value) {
  // TODO: implement this function
  if (kvs_clock == NULL || kvs_clock->queue == NULL) {
    return FAILURE;
  }

  // 1. check if the key exist and update it if it does
  queue_node *curr = kvs_clock->queue->front;
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      free(curr->value);
      curr->value = strdup(value);
      if (curr->value == NULL) {
        return FAILURE;
      }
      curr->dirty = true;
      curr->reference_bit = 1;
      return SUCCESS;
    }
    curr = curr->next;
  }

  if (kvs_clock->queue->size >= kvs_clock->capacity) {
    queue_node *evict = clock_evict(kvs_clock->queue);
    if (evict->dirty == true) {
      kvs_base_set(kvs_clock->kvs_base, evict->key, evict->value);
    }
    free_node(evict);
  }
  queue_node *node = create_node(key, value);
  if (node == NULL) {
    return FAILURE;
  }
  node->dirty = true;
  if (!enqueue(kvs_clock->queue, node)) {
    free_node(node);
    return FAILURE;
  }

  return SUCCESS;
}

int kvs_clock_get(kvs_clock_t *kvs_clock, const char *key, char *value) {
  // TODO: implement this function
  if (kvs_clock == NULL || kvs_clock->queue == NULL || key == NULL ||
      value == NULL) {
    return FAILURE;
  }

  queue_node *curr = kvs_clock->queue->front;
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      strcpy(value, curr->value);
      curr->reference_bit = 1;
      return SUCCESS;
    }
    curr = curr->next;
  }

  char *temp = malloc(KVS_VALUE_MAX);
  if (temp == NULL) {
    return FAILURE;
  }

  if (kvs_base_get(kvs_clock->kvs_base, key, temp) != 0) {
    free(temp);
    return FAILURE;
  }

  if (kvs_clock->queue->size >= kvs_clock->capacity) {
    queue_node *evict = dequeue(kvs_clock->queue);
    if (evict) {
      if (evict->dirty) {
        kvs_base_set(kvs_clock->kvs_base, evict->key, evict->value);
      }
      free_node(evict);
    };
  }

  queue_node *node = create_node(key, temp);
  if (node == NULL) {
    free(temp);
    return FAILURE;
  }
  node->dirty = false;
  if (!enqueue(kvs_clock->queue, node)) {
    free_node(node);
    free(temp);
    return FAILURE;
  }
  strcpy(value, temp);
  free(temp);
  // printf("Get retuns: %s\n", value);
  return SUCCESS;
}

int kvs_clock_flush(kvs_clock_t *kvs_clock) {
  // TODO: implement this function
  if (kvs_clock == NULL || kvs_clock->queue == NULL) {
    return FAILURE;
  }

  queue_node *curr = kvs_clock->queue->front;
  int flush = SUCCESS;

  while (curr != NULL) {
    if (curr->dirty == true &&
        (kvs_base_set(kvs_clock->kvs_base, curr->key, curr->value) != 0)) {
      flush = FAILURE;
    }
    curr = curr->next;
  }

  while (kvs_clock->queue->front != NULL) {
    queue_node *evict = dequeue(kvs_clock->queue);
    free_node(evict);
  }

  return flush;
}
