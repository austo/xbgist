#ifndef XB_CLIENT_TYPES_H
#define XB_CLIENT_TYPES_H

#include <glib.h>
#include <uv.h>

#define CONTENT_SIZE 256
#define SCHED_SIZE 100
#define NAME_SIZE 32


typedef unsigned short sched_t;

typedef enum {
  SCHEDULE, START, BROADCAST
} payload_type;


typedef struct payload {
  payload_type type;
  int is_important;
  sched_t modulo;
  char content[CONTENT_SIZE];
} payload;


typedef struct member {
  manager *mgr;
  char name[NAME_SIZE];
  sched_t schedule[SCHED_SIZE];
  gboolean message_processed;

  /* uv types */
  uv_connect_t connection;
  uv_stream_t *server;
  uv_tcp_t client;
  uv_work_t work;
  uv_buf_t buf;

} member;


#endif