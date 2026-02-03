#include "ringbuf.h"
#include <stdlib.h>

int ringbuf_init(ringbuf_t* rb, size_t cap_pow2) {
  if ((cap_pow2 & (cap_pow2 - 1)) != 0 || cap_pow2 < 2) return -1;
  rb->buf = (tx_item_t*)calloc(cap_pow2, sizeof(tx_item_t));
  if (!rb->buf) return -1;
  rb->cap = cap_pow2; rb->mask = cap_pow2 - 1; rb->head = 0; rb->tail = 0;
  return 0;
}
void ringbuf_free(ringbuf_t* rb) { free(rb->buf); rb->buf = NULL; rb->cap = rb->mask = rb->head = rb->tail = 0; }

int ringbuf_push(ringbuf_t* rb, const tx_item_t* item) {
  size_t h = rb->head, t = rb->tail;
  if (((h + 1) & rb->mask) == (t & rb->mask)) return -1;
  rb->buf[h & rb->mask] = *item; rb->head = h + 1; return 0;
}
int ringbuf_pop(ringbuf_t* rb, tx_item_t* out) {
  size_t t = rb->tail, h = rb->head;
  if ((t & rb->mask) == (h & rb->mask)) return -1;
  *out = rb->buf[t & rb->mask]; rb->tail = t + 1; return 0;
}
