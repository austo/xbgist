#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "test_iter.h"

/* TODO: Makefile:
 * gcc test_iter.c util.c -I/usr/local/include/glib-2.0 \
 * -I/usr/local/lib/glib-2.0/include -lglib-2.0 -o out/test_iter
 */


#define TEST_GROUP_SIZE 5


manager *
manager_new() {
  manager *mgr = malloc(sizeof(*mgr));
  if (mgr == NULL) { return NULL; }
  mgr->members = g_hash_table_new(g_direct_hash, g_direct_equal);
  mgr->current_round = 0;
  return mgr;
}


void
manager_dispose(manager *mgr) {
  g_hash_table_destroy(mgr->members);
  free(mgr);
}


member *
member_new() {
  member *memb = malloc(sizeof(*memb));
  return memb;  
}


void
member_dispose(member *memb) {
  free(memb);
}


void
test_group_init(manager *mgr) {
  int i = 0, skipped = 0;

  sched_t **schedules = malloc(sizeof(sched_t *) * TEST_GROUP_SIZE);

  srandom(time(NULL));

  for (; i < TEST_GROUP_SIZE; ++i) {
    member *memb = member_new();
    if (memb == NULL) {
      ++skipped;
      continue;
    }
    make_name(memb);
    memb->id = i;
    g_hash_table_insert(mgr->members, GINT_TO_POINTER(i), memb);
    schedules[i] = &memb->schedule[0];
  }
  fill_disjoint_arrays(schedules, TEST_GROUP_SIZE - skipped, SCHED_SIZE);
  free(schedules);
}


static void
make_name(member *memb) {
  // most popular baby names in Alabama in 2011
  static const char *names[] = {
    "Mason", "Ava", "James", "Madison", "Jacob", "Olivia", "John",
    "Isabella", "Noah", "Addison", "Jayden", "Chloe", "Elijah",
    "Elizabeth", "Jackson", "Abigail"
  };

  unsigned first_index = simple_random(ARRAY_SIZE(names));
  unsigned last_index = simple_random(ARRAY_SIZE(names) - 1);

  snprintf(memb->name, sizeof(memb->name), "%s %s",
    names[first_index], names[last_index]);  
}


static void
print_members(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  printf("member %d: %s\n", GPOINTER_TO_INT(key), memb->name);
  
  printf("schedule:\n");

  int j = 0;
  for (; j < SCHED_SIZE; ++j) {
    if (j > 0) {
      printf(", ");
    }
    printf("%d", memb->schedule[j]);
  }
  printf("\n\n");
}


int
main(int argc, char **argv) {
  manager *mgr = manager_new();
  test_group_init(mgr);
  g_hash_table_foreach(mgr->members, print_members, NULL);
}