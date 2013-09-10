#ifndef XB_TYPES_H
#define XB_TYPES_H

#include <glib.h>
#include <uv.h>

#define CONTENT_SIZE 256
#define SCHED_SIZE CONTENT_SIZE / 2
#define NAME_SIZE 32

#define ALLOC_BUF_SIZE CONTENT_SIZE * 2

#define do_callback(m) if (m->callback != NULL) m->callback(m)

struct member;
typedef unsigned short sched_t;
typedef void (*after_read_cb)(struct member *);

typedef enum {
  WELCOME, SCHEDULE, READY, START, ROUND
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

  /* flags */
  gboolean round_finished;
  gboolean schedules_sent;
  gboolean chat_started;

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

  /* flags */
  gboolean present;
  gboolean schedule_delivered;
  gboolean message_processed;

  uv_tcp_t handle;
  uv_work_t work;
  uv_buf_t buf;

  after_read_cb callback;

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
all_members_present(manager *mgr);

gboolean
all_schedules_delivered(manager *mgr);

gboolean
all_messages_processed(manager *mgr);

gboolean
all_members_need_schedule(manager *mgr);

void
g_message_processed(gpointer key, gpointer value, gpointer data);

void
g_member_present(gpointer key, gpointer value, gpointer data);

void
g_schedule_delivered(gpointer key, gpointer value, gpointer data);

void
g_member_needs_schedule(gpointer key, gpointer value, gpointer data);

void
g_get_schedule_addr(gpointer key, gpointer value, gpointer data);

void
g_unicast(gpointer key, gpointer value, gpointer data);

void
g_unicast_payload(gpointer key, gpointer value, gpointer data);

void
calculate_modulo(manager *mgr);

void
reset_round(manager *mgr);

void
fill_start_payload(manager *mgr);

void
fill_member_schedules(manager *mgr, sched_t **schedules);

void
maybe_broadcast_schedules(manager *mgr, gboolean lock,
  gboolean(*test)(manager *));

void
maybe_broadcast_start(manager *mgr, gboolean lock);

void
broadcast_schedules(manager *mgr);

void
broadcast_payload(manager *mgr);

void
unicast(struct member *memb, const char *msg);

void
assume_payload(manager *mgr, payload *pload);

member *
member_new();

void
member_dispose(member *memb);

void
g_member_dispose(gpointer data);

void
assume_buffer(member *memb, void *base, size_t len);

void
buffer_dispose(member *memb);

/* after_read_cb functions */

void
unicast_buffer(struct member *memb);

void
attempt_broadcast_schedules(struct member *memb);

void
attempt_broadcast_start(struct member *memb);

void
attempt_broadcast_round(struct member *memb);

/* end after_read_cb functions */


void
unicast_buffer(struct member *memb);

payload *
payload_new(
  payload_type type,
  int is_important, sched_t modulo, char *msg);

void
payload_set(
  payload *pload, payload_type type,
  int is_important, sched_t modulo, char *msg);

#endif
