#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "xb_types.h"
#include "util.h"


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


gboolean
all_members_present(manager *mgr) {
  if (g_hash_table_size(mgr->members) != mgr->member_count) {
    return FALSE;
  }
  gboolean retval = TRUE;
  iterate_members(mgr, g_member_present, &retval, FALSE);
  return retval;
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
  if (memb->buf.base != NULL) {
    // printf("assume_buffer: freeing buf.base for %s\n", memb->name);
    free(memb->buf.base);
  }
  memb->buf.base = base;
  memb->buf.len = len;
}


void
buffer_dispose(member *memb) {
  // printf("buffer_dispose for %s\n", memb->name);
  if (memb->buf.base != NULL) {
    // printf("freeing buf.base for %s\n", memb->name);
    free(memb->buf.base);
    memb->buf.base = NULL;
  }
  memb->buf.len = 0;
}


void
assume_payload(manager *mgr, payload *pload) {
  if (mgr->payload != NULL) {
    free(mgr->payload);
  }
  mgr->payload = pload;
  pload = NULL;
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
