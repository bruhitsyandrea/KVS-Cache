#include "kvs_fifo.h"
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
  struct queue_node *next;
} queue_node;

typedef struct {
  queue_node *front;  // oldest
  queue_node *rear;   // newest
  int size;           // curr num of nodes in the queue
} queue_t;

struct kvs_fifo {
  // TODO: add necessary variables
  kvs_base_t *kvs_base;  // storage disk
  int capacity;          // max num of items in the cache
  queue_t *queue;        // this is the queue
};

/**Helper Functions**/
static queue_node *create_node(const char *key, const char *value);
static void free_node(queue_node *node);

static int enqueue(queue_t *queue, const char *key, const char *value,
                   bool dirty) {
  queue_node *node = create_node(key, value);
  node->dirty = dirty;
  // printf("Enqueuing: %s\n", value);
  if (node == NULL) {
    return 0;
  }

  if (queue->rear != NULL) {
    queue->rear->next = node;
  } else {
    queue->front = node;
  }
  queue->rear = node;
  queue->size++;
  // printf("Finished enqueueing: %s\n", value);
  return 1;
}

static queue_node *dequeue(queue_t *queue) {
  if (queue->front == NULL) return NULL;

  queue_node *node = queue->front;
  queue->front = queue->front->next;

  if (queue->front == NULL) {
    queue->rear = NULL;
  }
  queue->size--;
  return node;
}

static queue_node *create_node(const char *key, const char *value) {
  queue_node *node = malloc(sizeof(queue_node));
  if (node == NULL) {
    return NULL;
  }

  node->key = strdup(key);
  node->value = strdup(value);

  if (node->key == NULL || node->value == NULL) {
    free(node->key);
    free(node->value);
    free(node);
    return NULL;
  }

  node->next = NULL;
  // free_node(node);
  return node;
}

static void free_node(queue_node *node) {
  if (node) {
    free(node->key);
    free(node->value);
    free(node);
  }
}

/**kvs_fifo Function Definitions**/
kvs_fifo_t *kvs_fifo_new(kvs_base_t *kvs, int capacity) {
  kvs_fifo_t *kvs_fifo = malloc(sizeof(kvs_fifo_t));
  if (kvs_fifo == NULL) {
    return NULL;
  }
  kvs_fifo->kvs_base = kvs;
  kvs_fifo->capacity = capacity;

  // TODO: initialize other variables
  kvs_fifo->queue = malloc(sizeof(queue_t));
  if (kvs_fifo->queue == NULL) {
    free(kvs_fifo);
    return NULL;
  }

  kvs_fifo->queue->front = NULL;
  kvs_fifo->queue->rear = NULL;
  kvs_fifo->queue->size = 0;

  return kvs_fifo;
}

void kvs_fifo_free(kvs_fifo_t **ptr) {
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

int kvs_fifo_set(kvs_fifo_t *kvs_fifo, const char *key, const char *value) {
  // TODO: implement this function
  // printf("Set is running...\n");
  if (kvs_fifo == NULL || kvs_fifo->queue == NULL) {
    return FAILURE;
  }

  // 1. check if the key exist and update it if it does
  queue_node *curr = kvs_fifo->queue->front;
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      free(curr->value);
      curr->value = strdup(value);
      if (curr->value == NULL) {
        return FAILURE;
      }
      curr->dirty = true;
      return SUCCESS;
    }
    curr = curr->next;
  }

  if (kvs_fifo->queue->size >= kvs_fifo->capacity) {
    queue_node *evict = dequeue(kvs_fifo->queue);

    if (evict->dirty == true) {
      kvs_base_set(kvs_fifo->kvs_base, evict->key, evict->value);
    }
    free_node(evict);
  }

  if (!enqueue(kvs_fifo->queue, key, value, true)) {
    return FAILURE;
  }
  // enqueue(kvs_fifo->queue, key, value);
  return SUCCESS;
  // return FAILURE;
}

int kvs_fifo_get(kvs_fifo_t *kvs_fifo, const char *key, char *value) {
  // TODO: implement this function
  // printf("Get is running...\n");
  if (kvs_fifo == NULL || kvs_fifo->queue == NULL || key == NULL ||
      value == NULL) {
    return FAILURE;
  }

  queue_node *curr = kvs_fifo->queue->front;
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      strcpy(value, curr->value);
      return SUCCESS;
    }
    curr = curr->next;
  }

  char *temp = malloc(KVS_VALUE_MAX);
  if (temp == NULL) {
    return FAILURE;
  }

  if (kvs_base_get(kvs_fifo->kvs_base, key, temp) != 0) {
    free(temp);
    return FAILURE;
    // strcpy(value, temp);
    // free(temp);
    // return SUCCESS;
    //  return FAILURE;
  }
  // return FAILURE;
  //  printf("Cummaster: %s\n", key);

  if (kvs_fifo->queue->size >= kvs_fifo->capacity) {
    queue_node *evict = dequeue(kvs_fifo->queue);
    if (evict->dirty == true)
      kvs_base_set(kvs_fifo->kvs_base, evict->key, evict->value);
    free_node(evict);
  }

  if (!enqueue(kvs_fifo->queue, key, temp, false)) {
    free(temp);
    return FAILURE;
  }

  strcpy(value, temp);
  free(temp);
  // printf("Get retuns: %s\n", value);
  return SUCCESS;
}

int kvs_fifo_flush(kvs_fifo_t *kvs_fifo) {
  // TODO: implement this function
  if (kvs_fifo == NULL || kvs_fifo->queue == NULL) {
    return FAILURE;
  }

  queue_node *curr = kvs_fifo->queue->front;
  int flush = SUCCESS;

  while (curr != NULL) {
    // if (kvs_base_get(kvs_fifo->kvs_base, curr->key, temp) != 0) {
    //   return FAILURE;
    // } else {

    //   }
    // }
    if (curr->dirty == true &&
        (kvs_base_set(kvs_fifo->kvs_base, curr->key, curr->value) != 0)) {
      flush = FAILURE;
    }
    curr = curr->next;
  }

  while (kvs_fifo->queue->front != NULL) {
    queue_node *evict = dequeue(kvs_fifo->queue);
    free_node(evict);
  }

  return flush;
}
