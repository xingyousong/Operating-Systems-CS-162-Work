#include <stdlib.h>
#include "wq.h"
#include "utlist.h"

/* Initializes a work queue WQ. */
void wq_init(wq_t *wq) {

  /* TODO: Make me thread-safe! */
  pthread_mutex_init(&wq-> wq_mutex, NULL);
  pthread_cond_init(&wq-> wq_empty, NULL);
  wq->size = 0;
  wq->head = NULL;
}

/* Remove an item from the WQ. This function should block until there
 * is at least one item on the queue. */
int wq_pop(wq_t *wq) {

  /* TODO: Make me blocking and thread-safe! */
  

  // lock wq
  pthread_mutex_lock(&wq->wq_mutex);

  while(wq-> size <1){
    pthread_cond_wait(&wq->wq_empty, &wq-> wq_mutex); //release lock if fail 
  }

  

  wq_item_t *wq_item = wq->head;
  int client_socket_fd = wq->head->client_socket_fd;
  wq->size--;
  DL_DELETE(wq->head, wq->head);

  free(wq_item);

  pthread_mutex_unlock(&wq->wq_mutex);
  //unlock wq

  return client_socket_fd;
}

/* Add ITEM to WQ. */
void wq_push(wq_t *wq, int client_socket_fd) {

  /* TODO: Make me thread-safe! */
  
  wq_item_t *wq_item = calloc(1, sizeof(wq_item_t));
  wq_item->client_socket_fd = client_socket_fd;

  //lock wq
  pthread_mutex_lock(&wq->wq_mutex);

  DL_APPEND(wq->head, wq_item);
  wq->size++;

  pthread_cond_signal(&wq->wq_empty);

  pthread_mutex_unlock(&wq->wq_mutex);
  //unlock wq
}


