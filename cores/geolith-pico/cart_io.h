#ifndef GEO_CART_IO_H
#define GEO_CART_IO_H
#include <stddef.h>
#include <stdint.h>
/* Caller owns handle and destination; read_at returns bytes or negative error. */
typedef int (*geo_cart_read_at)(void *,uint32_t,void *,uint32_t);
int geo_cart_load(geo_cart_read_at read_at,void *ctx,uint32_t size,void *dst,size_t capacity);
#endif
