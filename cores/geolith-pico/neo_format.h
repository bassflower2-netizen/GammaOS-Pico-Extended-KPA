#ifndef PICO_NEO_FORMAT_H
#define PICO_NEO_FORMAT_H
#include <stddef.h>
#include <stdint.h>
/* Validate container bounds before the upstream parser gets a pointer.
 * Not a guarantee that ROM internals or protection metadata are valid. */
#define PICO_NEO_HEADER 4096u
#define PICO_NEO_MAX (128u*1024u*1024u)
int pico_neo_validate(const void *data,size_t size);
#endif
