#ifndef XB_CLIENT_TYPES_H
#define XB_CLIENT_TYPES_H

#include <glib.h>
#include <uv.h>

#define CONTENT_SIZE 256
#define SCHED_SIZE 100
#define NAME_SIZE 32

struct member;

typedef unsigned short sched_t;
typedef void (*after_read_cb)(struct member *);

typedef enum {
  SCHEDULE, READY, START, ROUND
} payload_type;


typedef struct payload {
  payload_type type;
  int is_important;
  sched_t modulo;
  char content[CONTENT_SIZE];
} payload;


typedef struct member {
  char name[NAME_SIZE];
  sched_t schedule[SCHED_SIZE];
  gboolean message_processed;

  /* uv types */
  uv_connect_t connection;
  uv_stream_t *server;
  uv_tcp_t client;
  uv_work_t work;
  uv_buf_t buf;

  after_read_cb callback;

} member;


member *
member_new();

void
member_dispose(member *memb);

void
digest_broadcast(member *memb);

void
deserialize_payload(struct payload *pload, void *buf, size_t len);


void
process_schedule(member *memb, payload *pload);

void
process_ready(member *memb, payload *pload);

void
process_start(member *memb, payload *pload);

void
process_round(member *memb, payload *pload);

#endif