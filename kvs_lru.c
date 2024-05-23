#include "kvs_lru.h"
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>

typedef struct Node {
  char* key;
  char* value;
  struct Node* prev;
  struct Node* next;
} Node;

struct kvs_lru {
  // TODO: add necessary variables
  kvs_base_t* kvs_base;
  int capacity;
  int size;
  Node* head;
  Node* tail;
};

Node* create_node(const char* key, const char* value) {
  Node* new_node = malloc(sizeof(Node));
  if (new_node == NULL) return NULL;

  new_node->key = strdup(key);
  new_node->value = strdup(value);
  new_node->prev = new_node->next = NULL;
  return new_node;
}

void detach_node(Node* node, kvs_lru_t* cache) {
  if (node->prev) {
    node->prev->next = node->next;
  }
  if (node->next) {
    node->next->prev = node->prev;
  }
  if (node == cache->tail) {
    cache->tail = node->prev;
  }
}

void move_front(kvs_lru_t* cache, Node* node) {
  if (node == cache->head) {
    return;
  }

  if (node->next) {
    node->next->prev = node->prev;
  }
  if (node->prev) {
    node->prev->next = node->next;
  }
  if (node == cache->tail) {
    cache->tail = node->prev;
  }

  node->next = cache->head;
  node->prev = NULL;
  if (cache->head) {
    cache->head->prev = node;
  }
  cache->head = node;
}

kvs_lru_t* kvs_lru_new(kvs_base_t* kvs, int capacity) {
  kvs_lru_t* kvs_lru = malloc(sizeof(kvs_lru_t));
  kvs_lru->kvs_base = kvs;
  kvs_lru->capacity = capacity;

  // TODO: initialize other variables
  kvs_lru->size = 0;
  kvs_lru->head = NULL;
  kvs_lru->tail = NULL;
  return kvs_lru;
}

void kvs_lru_free(kvs_lru_t** ptr) {
  // TODO: free dynamically allocated memory
  if (ptr == NULL || *ptr == NULL) return;

  Node* curr = (*ptr)->head;
  while (curr != NULL) {
    Node* next = curr->next;
    free(curr->key);
    free(curr->value);
    free(curr);
    curr = next;
  }
  free(*ptr);
  *ptr = NULL;
}

int kvs_lru_set(kvs_lru_t* kvs_lru, const char* key, const char* value) {
  // TODO: implement this function
  if (kvs_lru == NULL) return FAILURE;

  // search the key in cache
  Node* curr = kvs_lru->head;
  while (curr) {
    if (strcmp(curr->key, key) == 0) {
      free(curr->value);
      curr->value = strdup(value);
      if (!curr->value) return FAILURE;

      if (curr != kvs_lru->head) {
        detach_node(curr, kvs_lru);
        curr->next = kvs_lru->head;
        curr->prev = NULL;
        if (kvs_lru->head) {
          kvs_lru->head->prev = curr;
        }
        kvs_lru->head = curr;
      }
      return SUCCESS;
    }
    curr = curr->next;
  }

  // if key not found && cache full -> evict the lru
  if (kvs_lru->size == kvs_lru->capacity) {
    Node* evict = kvs_lru->tail;
    detach_node(evict, kvs_lru);
    // kvs_lru->tail = evict->prev;
    free(evict->key);
    free(evict->value);
    free(evict);
    kvs_lru->size--;
  }

  Node* N = create_node(key, value);
  if (!N) {
    return FAILURE;
  }

  // add to the front of the list
  N->next = kvs_lru->head;
  if (kvs_lru->head) {
    kvs_lru->head->prev = N;
  }
  kvs_lru->head = N;
  if (kvs_lru->tail == NULL) {
    kvs_lru->tail = N;
  }
  kvs_lru->size++;

  return SUCCESS;
}

int kvs_lru_get(kvs_lru_t* kvs_lru, const char* key, char* value) {
  // TODO: implement this function
  // 1. search cache 2. move to front 3. fetch base 4. cache full 5. add cache
  if (kvs_lru == NULL || key == NULL || value == NULL) {
    return FAILURE;
  }

  Node* curr = kvs_lru->head;
  while (curr != NULL) {
    if (strcmp(curr->key, key) == 0) {
      strcpy(value, curr->value);

      move_front(kvs_lru, curr);

      return SUCCESS;
    }
    curr = curr->next;
  }

  char temp[KVS_VALUE_MAX];
  if (kvs_base_get(kvs_lru->kvs_base, key, temp) == FAILURE) {
    return FAILURE;
  }

  if (kvs_lru->size >= kvs_lru->capacity) {
    Node* evict = kvs_lru->tail;
    if (evict->prev) {
      evict->prev->next = NULL;
    }
    kvs_lru->tail = evict->prev;
    free(evict->key);
    free(evict->value);
    free(evict);
    kvs_lru->size--;
  }

  Node* N = create_node(key, temp);
  if (N == NULL) {
    return FAILURE;
  }
  N->next = kvs_lru->head;
  if (kvs_lru->head) {
    kvs_lru->head->prev = N;
  }
  kvs_lru->head = N;
  if (kvs_lru->tail == NULL) {
    kvs_lru->tail = N;
  }
  kvs_lru->size++;
  strcpy(value, temp);

  return SUCCESS;
}

int kvs_lru_flush(kvs_lru_t* kvs_lru) {
  // TODO: implement this function
  if (kvs_lru == NULL) {
    return FAILURE;
  }

  Node* curr = kvs_lru->head;
  int flush_stats = SUCCESS;

  while (curr != NULL) {
    if (kvs_base_set(kvs_lru->kvs_base, curr->key, curr->value) == FAILURE) {
      flush_stats = FAILURE;
    }
    curr = curr->next;
  }
  return flush_stats;
}
