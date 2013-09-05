#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <assert.h>

#include <uv.h>

#include "util.h"
#include "tpl.h"
#include "xb_types.h"

sched_t
simple_random(sched_t n) {
  unsigned rnd, limit = RAND_MAX - RAND_MAX % n;
  
  do {
    rnd = random();
  } while (rnd >= limit);
  return rnd % n;
}


void
init_shuffle(sched_t *array, size_t n) {
  sched_t i, r;

  array[0] = 0;

  for (i = 1; i < n; ++i) {
    r = simple_random(i);      
    array[i] = array[r]; 
    array[r] = i;
  }
}


void
shuffle(sched_t *array, size_t n) {
  sched_t i, r, t;

  for (i = n - 1; i > 0; --i) {
    r = simple_random(i + 1);
    t = array[r];
    array[r] = array[i];
    array[i] = t;
  }
}

// Use FY shuffle to fill zero-initialized arrays one step at a time
void
fill_disjoint_arrays(
  sched_t **schedules, size_t n_sched, size_t sched_len) {

  // srandom(time(NULL));

  size_t i, j;

  sched_t *arr = xb_malloc(sizeof(sched_t) * n_sched);
  init_shuffle(arr, n_sched);

  for (i = 0; i < sched_len; ++i) {
    for (j = 0; j < n_sched; ++j) {
      schedules[j][i] = arr[j];
    }
    shuffle(arr, n_sched);
  }
  free(arr);
}


void
fill_random_msg(char *buf, size_t maxlen) {
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  static const size_t alen = sizeof(alphanum) - 1;

  int rlen = simple_random(maxlen - 1), i = 0;
  for (; i < rlen; ++i) {
    buf[i] = alphanum[random() % alen];
  }
  buf[rlen] = '\0';

  // clear remaining buffer
  if (rlen < maxlen - 1) {
    for (i = rlen + 1; i < maxlen; ++i) {
      buf[i] = 0;
    }
  } 
}


void
serialize_payload(struct payload *pload, void *to, size_t len) {
  // printf("serializing payload\n");
  // printf("%s\n", pload->content);

  tpl_node *tn = tpl_map("S(iivc#)", pload, CONTENT_SIZE);
  tpl_pack(tn, 0);
  tpl_dump(tn, TPL_MEM|TPL_PREALLOCD, to, len);
  tpl_free(tn);
}


size_t
serialize_payload_exact(struct payload *pload, void *to) {
  size_t len;
  tpl_node *tn = tpl_map("S(iivc#)", pload, CONTENT_SIZE);
  tpl_pack(tn, 0);
  tpl_dump(tn, TPL_MEM, to, &len);
  tpl_free(tn);
  return len;
}


void
deserialize_payload(struct payload *pload, void *from, size_t len) {
  tpl_node *tn = tpl_map("S(iivc#)", pload, CONTENT_SIZE);
  tpl_load(tn, TPL_MEM|TPL_EXCESS_OK, from, len);
  tpl_unpack(tn, 0);
  tpl_free(tn);
}


void
fatal(const char *what) {
  uv_err_t err = uv_last_error(uv_default_loop());
  fprintf(stderr, "%s: %s\n", what, uv_strerror(err));
  exit(1);
}


void *
xb_malloc(size_t size) {
  void *ptr = malloc(size);
  assert(ptr != NULL);
  return ptr;
}


void
make_name(struct member *memb) {
  // most popular baby names in Alabama in 2011
  static const char *names[] = {
    "Mason", "Ava", "James", "Madison", "Jacob", "Olivia", "John",
    "Isabella", "Noah", "Addison", "Jayden", "Chloe", "Elijah",
    "Elizabeth", "Jackson", "Abigail"
  };

  unsigned first_index = simple_random(G_N_ELEMENTS(names));
  unsigned last_index = simple_random(G_N_ELEMENTS(names) - 1);

  snprintf(memb->name, sizeof(memb->name), "%s %s",
    names[first_index], names[last_index]);  
}

#ifdef XBSERVER

const char *
addr_and_port(struct member *memb) {
  struct sockaddr_in name;
  int namelen = sizeof(name);
  if (uv_tcp_getpeername(
    &memb->handle, (struct sockaddr*) &name, &namelen)){
    fatal("uv_tcp_getpeername");
  }

  char addr[16];
  static char buf[32];
  uv_inet_ntop(AF_INET, &name.sin_addr, addr, sizeof(addr));
  snprintf(buf, sizeof(buf), "%s:%d", addr, ntohs(name.sin_port));

  return buf;
}

#endif

