#ifndef XB_TYPES_H
#define XB_TYPES_H

#include <glib.h>
#include <uv.h>

#define CONTENT_SIZE 256
#define SCHED_SIZE 100
#define NAME_SIZE 32


typedef unsigned short sched_t;

typedef enum {
  SCHEDULE, READY, START, ROUND
} payload_type;


typedef struct payload {
  payload_type type;
  int is_important;
  sched_t modulo;
  char content[CONTENT_SIZE];
} payload;


typedef struct manager {
	guint member_count;
  GHashTable* members;
  int current_round;
  gboolean round_finished;
  sched_t modulo;
  payload *payload;
  uv_work_t work;
  uv_mutex_t mutex;
} manager;


typedef struct member {
	manager *mgr;
  char name[NAME_SIZE];
  guint id;
  sched_t schedule[SCHED_SIZE];
  gboolean present;
  gboolean message_processed;
  uv_tcp_t handle;
  uv_work_t work;
  uv_buf_t buf;
} member;


manager *
manager_new();

void
manager_dispose(manager *mgr);

void
iterate_members(manager *mgr, GHFunc func, gpointer data, gboolean lock);

void
insert_member(manager *mgr, guint memb_id, member *memb);

void
remove_member(manager *mgr, guint memb_id);

gboolean
has_room(manager *mgr);

gboolean
member_can_transmit(manager *mgr, member *memb);

gboolean
all_messages_processed(manager *mgr);

void
g_message_processed(gpointer key, gpointer value, gpointer data);

void
g_get_schedule_addr(gpointer key, gpointer value, gpointer data);

void
calculate_modulo(manager *mgr);

void
fill_member_schedules(manager *mgr);

void
assume_payload(manager *mgr, payload *pload);

member *
member_new();

void
member_dispose(member *memb);

void
g_member_dispose(gpointer data);


#endif
