#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t data[256];
  size_t len;
  uint8_t mode;
} tx_item_t;

typedef struct {
  tx_item_t* buf;
  size_t cap, mask;
  volatile size_t head, tail;
} ringbuf_t;

int  ringbuf_init(ringbuf_t* rb, size_t capacity_pow2);
void ringbuf_free(ringbuf_t* rb);
int  ringbuf_push(ringbuf_t* rb, const tx_item_t* item);
int  ringbuf_pop(ringbuf_t* rb, tx_item_t* out);
