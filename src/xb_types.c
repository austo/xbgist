#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "xb_types.h"
#include "util.h"


// extern void
// on_write(uv_write_t *req, int status);

manager *
manager_new() {
  manager *mgr = xb_malloc(sizeof(*mgr));
  mgr->payload = xb_malloc(sizeof(*(mgr->payload)));
  assert(uv_mutex_init(&mgr->mutex) == 0);
  mgr->members = g_hash_table_new_full(
    g_direct_hash, g_direct_equal, NULL, g_member_dispose);
  mgr->current_round = 0;
  mgr->round_finished = FALSE;
  mgr->schedules_sent = FALSE;
  mgr->chat_started = FALSE;
  return mgr;
}


void
manager_dispose(manager *mgr) {
  g_hash_table_destroy(mgr->members);
  uv_mutex_destroy(&mgr->mutex);
  free(mgr->payload);
  free(mgr);
  mgr = NULL;
}


void
iterate_members(manager *mgr, GHFunc func, gpointer data, gboolean lock) {
  if (lock) {
    uv_mutex_lock(&mgr->mutex);
  }

  g_hash_table_foreach(mgr->members, func, data);
  
  if (lock) {
    uv_mutex_unlock(&mgr->mutex);
  }
}


void
insert_member(manager *mgr, guint memb_id, member *memb) {
  uv_mutex_lock(&mgr->mutex);
  g_hash_table_insert(mgr->members, GINT_TO_POINTER(memb_id), memb);
  memb->mgr = mgr;
  memb->present = TRUE;
  uv_mutex_unlock(&mgr->mutex);
}


void
remove_member(manager *mgr, guint memb_id) {
  uv_mutex_lock(&mgr->mutex);
  g_hash_table_remove(mgr->members, GINT_TO_POINTER(memb_id));
  uv_mutex_unlock(&mgr->mutex);
}


gboolean
has_room(manager *mgr) {
  guint tsize = g_hash_table_size(mgr->members);
  // fprintf(stdout, "g_hash_table_size: %d\n", tsize);
  return tsize < mgr->member_count ? TRUE : FALSE;
}


gboolean
member_can_transmit(manager *mgr, member *memb) {
  return mgr->modulo == memb->schedule[mgr->current_round] ? TRUE : FALSE;
}


gboolean
all_messages_processed(manager *mgr) {
  gboolean retval = TRUE;
  iterate_members(mgr, g_message_processed, &retval, FALSE);
  return retval;
}


gboolean
all_schedules_delivered(manager *mgr) {
  gboolean retval = TRUE;
  iterate_members(mgr, g_schedule_delivered, &retval, FALSE);
  return retval;
}


gboolean
all_members_need_schedule(manager *mgr) {
  gboolean retval = TRUE;
  iterate_members(mgr, g_member_needs_schedule, &retval, FALSE);
  return retval;
}


gboolean
all_members_present(manager *mgr) {
  if (g_hash_table_size(mgr->members) != mgr->member_count) {
    return FALSE;
  }
  gboolean retval = TRUE;
  iterate_members(mgr, g_member_present, &retval, FALSE);
  return retval;
}


void
g_message_processed(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  gboolean *processed = (gboolean *)data;
  if (*processed) {
    *processed = memb->message_processed;
  }
}


void
g_schedule_delivered(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  gboolean *delivered = (gboolean *)data;
  if (*delivered) {
    *delivered = memb->schedule_delivered;
  }
}


void
g_get_schedule_addr(gpointer key, gpointer value, gpointer data) {

  // use phony member id to calculate array index

  int idx = GPOINTER_TO_INT(key) - 1;
  assert(idx >= 0);
  member *memb = (member *)value;
  ((sched_t**)data)[idx] = &memb->schedule[0];
}


void
g_member_present(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  gboolean *present = (gboolean *)data;
  if (*present) {
    *present = memb->present;
  }
}


void
g_member_needs_schedule(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  gboolean *need_sched = (gboolean *)data;
  if (*need_sched) {
    *need_sched = !memb->schedule_delivered;
  }
}


void
g_unicast_payload(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  
  char buf[ALLOC_BUF_SIZE];

  /* copy member's schedule into manager's payload */
  char *content = (char *)data; // memb->mgr->payload->content
  memcpy(content, memb->schedule, sizeof(memb->schedule));
  serialize_payload(memb->mgr->payload, buf, ALLOC_BUF_SIZE);
  unicast(memb, buf);
}


void
g_unicast(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  char *msg = (char *)data;
  unicast(memb, msg);
}


void
calculate_modulo(manager *mgr) {
  mgr->payload->modulo = simple_random(mgr->member_count);
}


void
reset_round(manager *mgr) {
  if (mgr->round_finished) {
    fprintf(stdout, "Round %d finished. ", mgr->current_round);
    ++(mgr->current_round);
    fprintf(stdout, "Incremented round to %d\n", mgr->current_round);
    mgr->round_finished = FALSE;
  }
}


void
fill_start_payload(manager *mgr) {
  mgr->payload->type = START;
  mgr->payload->is_important = 1;
  mgr->payload->modulo = 0;
  fill_random_msg(mgr->payload->content, CONTENT_SIZE);
}


void
fill_member_schedules(manager *mgr, sched_t **schedules) {
  size_t n_sched = g_hash_table_size(mgr->members);
  gboolean free_sched = FALSE;

  if (schedules == NULL) { // allocate schedules if not provided by client
    schedules = xb_malloc(sizeof(sched_t *) * n_sched);
    free_sched = TRUE;
  }

  iterate_members(mgr, g_get_schedule_addr, schedules, FALSE);

  fill_disjoint_arrays(schedules, n_sched, SCHED_SIZE);

  if (free_sched) {
    free(schedules);
  }
}


void
maybe_broadcast_schedules(manager *mgr, gboolean lock,
  gboolean(*test)(manager *)) {
  if (lock) { uv_mutex_lock(&mgr->mutex); }
  if (!mgr->schedules_sent && test(mgr)) {
    usleep(1000);
    printf("broadcasting schedules...\n");
    broadcast_schedules(mgr);
  }
  if (lock) { uv_mutex_unlock(&mgr->mutex); }
}


void
maybe_broadcast_start(manager *mgr, gboolean lock) {
  printf("entering maybe_broadcast_start\n");

  if (mgr->chat_started) { return; }

  if (lock) { uv_mutex_lock(&mgr->mutex); }

  if (all_schedules_delivered(mgr)) {
    
    printf("all schedules delivered, filling payload\n");
    fill_start_payload(mgr);

    printf("broadcasting payload\n");
    broadcast_payload(mgr);

    mgr->chat_started = TRUE;
  }
  if (lock) { uv_mutex_unlock(&mgr->mutex); }
}


void
broadcast_schedules(manager *mgr) {
  /* set schedule && serialize payload for each member */
  mgr->payload->type = SCHEDULE;
  mgr->payload->is_important = 1;
  mgr->payload->modulo = 0;

  fill_member_schedules(mgr, NULL);
  iterate_members(mgr, g_unicast_payload, mgr->payload->content, FALSE);
  mgr->schedules_sent = TRUE;
  mgr->chat_started = FALSE;
}


void
broadcast_payload(manager *mgr) {
  char msg[ALLOC_BUF_SIZE];
  serialize_payload(mgr->payload, msg, ALLOC_BUF_SIZE);
  iterate_members(mgr, g_unicast, msg, FALSE);
}


void
unicast(struct member *memb, const char *msg) {
  // size_t len = strlen(msg);
  uv_write_t *req = xb_malloc(sizeof(*req) + ALLOC_BUF_SIZE);
  req->data = memb;
  void *addr = req + 1;
  memcpy(addr, msg, ALLOC_BUF_SIZE);  
  uv_buf_t buf = uv_buf_init(addr, ALLOC_BUF_SIZE);
  uv_write(req, (uv_stream_t*) &memb->handle, &buf, 1, on_write);
  memb->message_processed = FALSE;
}


member *
member_new() {
  member *memb = xb_malloc(sizeof(*memb));
  memb->present = FALSE;
  memb->message_processed = FALSE;
  memb->work.data = memb;
  memb->handle.data = memb;
  memb->buf.base = NULL;
  return memb;  
}


void
member_dispose(member *memb) {
  free(memb);
  memb = NULL;
}


void
g_member_dispose(gpointer data) {
  member *memb = (member *)data;
  printf("calling dispose for %s...\n", memb->name);
  member_dispose(memb);
}


void
assume_buffer(member *memb, void *base, size_t len) {
  free(memb->buf.base);
  memb->buf.base = base;
  memb->buf.len = len;
}


void
buffer_dispose(member *memb) {
  free(memb->buf.base);
  memb->buf.base = NULL;
  memb->buf.len = 0;
}


void
assume_payload(manager *mgr, payload *pload) {
  free(mgr->payload);
  mgr->payload = pload;
}


payload *
payload_new(
  payload_type type, int is_important, sched_t modulo, char *msg) {
  payload *pload = xb_malloc(sizeof(*pload));
  pload->type = type;
  pload->is_important = is_important;
  pload->modulo = modulo;

  if (msg == NULL) {
    fill_random_msg(pload->content, CONTENT_SIZE);
  }
  else {
    strcpy(pload->content, msg);
  }
  return pload;
}


void
payload_set(
  payload *pload, payload_type type,
  int is_important, sched_t modulo, char *msg) {

  pload->type = type;
  pload->is_important = is_important;
  pload->modulo = modulo;

  if (msg == NULL) {
    fill_random_msg(pload->content, CONTENT_SIZE);
  }
  else {
    strcpy(pload->content, msg);
  }
}


/* member after_read_cb functions */

void
unicast_buffer(struct member *memb) {
  printf("unicasting buffer for %s\n", memb->name);
  assert(memb->buf.len != 0);
  printf("memb->buf.len: %zu\n", memb->buf.len);
  uv_write_t *req = xb_malloc(sizeof(*req) + memb->buf.len);
  req->data = memb;
  void *addr = req + 1;
  memmove(addr, memb->buf.base, memb->buf.len);
  printf("after memmove\n");  
  uv_buf_t buf = uv_buf_init(addr, memb->buf.len);
  printf("after uv_buf_init\n");
  uv_write(req, (uv_stream_t*) &memb->handle, &buf, 1, on_write);
  printf("after uv_write\n");
  memb->message_processed = FALSE;
  memb->callback = NULL;
}


void
attempt_broadcast_schedules(struct member *memb) {
  printf("attempting broadcast schedules\n");
  maybe_broadcast_schedules(memb->mgr, TRUE, all_members_need_schedule);
  memb->callback = NULL;
}


void
attempt_broadcast_start(struct member *memb) {
  maybe_broadcast_start(memb->mgr, TRUE);
  memb->callback = NULL;
}


void
attempt_broadcast_round(struct member *memb) {
  uv_mutex_lock(&memb->mgr->mutex);

  if (all_messages_processed(memb->mgr)) {
    calculate_modulo(memb->mgr);
    memb->mgr->payload->type = ROUND;

    broadcast_payload(memb->mgr);

    memb->mgr->round_finished = TRUE;
    reset_round(memb->mgr);
  }
  else {
    memb->mgr->round_finished = FALSE;
  }
  uv_mutex_unlock(&memb->mgr->mutex);
  memb->callback = NULL;
}


/* end member after_read_cb functions */
