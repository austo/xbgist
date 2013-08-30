#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

typedef unsigned short sched_t;
struct member;
struct payload;

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))

sched_t
simple_random(sched_t n);

void
init_shuffle(sched_t *array, size_t n);

void
shuffle(sched_t *array, size_t n);

void
fill_disjoint_arrays(sched_t **schedules, size_t n_sched, size_t sched_len);

void
serialize_payload(struct payload *pload, void *buf, size_t len);

void
deserialize_payload(struct payload *pload, void *buf, size_t len);

void
make_name(struct member *memb);

const char *
addr_and_port(struct member *memb);

void
fatal(const char *what);

void *
xb_malloc(size_t size);

#endif