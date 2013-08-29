#ifndef XB_TYPES_H
#define XB_TYPES_H

#include <glib.h>
#include <uv.h>

#define CONTENT_SIZE 128
#define SCHED_SIZE 100
#define NAME_SIZE 32
#define MESSAGE_SIZE 512

typedef unsigned sched_t;


typedef struct payload {
  int is_important;
  char content[CONTENT_SIZE];
} payload;


typedef struct manager {
	guint member_count;
  GHashTable* members;
  int current_round;
  guint modulo;
  uv_buf_t buf;
  uv_mutex_t mutex;
} manager;


typedef struct member {
	manager *mgr;
  char name[NAME_SIZE];
  guint id;
  sched_t schedule[SCHED_SIZE];
  gboolean present;
  uv_tcp_t handle;
  uv_work_t work;
  uv_buf_t buf;
} member;


manager *
manager_new();

void
manager_dispose(manager *mgr);

void
iterate_members(manager *mgr, GHFunc func, gpointer *data, gboolean lock);

void
insert_member(manager *mgr, guint memb_id, member *memb);

void
remove_member(manager *mgr, guint memb_id);

gboolean
has_room(manager *mgr);

gboolean
member_can_transmit(manager *mgr, member *memb);

member *
member_new();

void
member_dispose(member *memb);

void
g_member_dispose(gpointer data);


#endif
