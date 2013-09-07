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
fill_random_msg(char *buf, size_t maxlen);

void
serialize_payload(struct payload *pload, void *buf, size_t len);

size_t
serialize_payload_exact(struct payload *pload, void *to);

void
deserialize_payload(struct payload *pload, void *buf, size_t len);

void
fatal(int status, const char *what);

void *
xb_malloc(size_t size);


#ifdef XBSERVER

void
make_name(struct member *memb);

const char *
addr_and_port(struct member *memb);

#endif


#endif