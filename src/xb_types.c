#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "xb_types.h"


manager *
manager_new() {
  manager *mgr = malloc(sizeof(*mgr));
  assert(mgr != NULL);
  assert(uv_mutex_init(&mgr->mutex) == 0);
  mgr->members = g_hash_table_new_full(
    g_direct_hash, g_direct_equal, NULL, g_member_dispose);
  mgr->current_round = 0;
  return mgr;
}


void
manager_dispose(manager *mgr) {
  g_hash_table_destroy(mgr->members);
  uv_mutex_destroy(&mgr->mutex);
  free(mgr);
}


void
iterate_members(manager *mgr, GHFunc func, gpointer *data, gboolean lock) {
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
  return g_hash_table_size(mgr->members) < mgr->member_count ? TRUE : FALSE;
}


member *
member_new() {
  member *memb = malloc(sizeof(*memb));
  assert(memb != NULL);
  memb->present = FALSE;
  memb->work.data = memb;
  memb->handle.data = memb;
  return memb;  
}


void
member_dispose(member *memb) {
  free(memb);
}


void
g_member_dispose(gpointer data) {
  member *memb = (member *)data;
  printf("calling dispose for %s...\n", memb->name);
  member_dispose(memb);
}
