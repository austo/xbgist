#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "xb_types.h"
#include "test.h"
#include "pack_tool.h"


#define TEST_GROUP_SIZE 5


static void
test_group_init(manager *mgr) {
  int i = 0, skipped = 0;

  sched_t **schedules = xb_malloc(sizeof(sched_t *) * TEST_GROUP_SIZE);

  srandom(time(NULL));

  for (; i < TEST_GROUP_SIZE; ++i) {
    member *memb = member_new();
    if (memb == NULL) {
      ++skipped;
      continue;
    }
    make_name(memb);
    memb->id = i;
    memb->mgr = mgr;
    g_hash_table_insert(mgr->members, GINT_TO_POINTER(i), memb);
    schedules[i] = &memb->schedule[0];
  }
  fill_disjoint_arrays(schedules, TEST_GROUP_SIZE - skipped, SCHED_SIZE);
  free(schedules);
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
  manager_dispose(mgr);
  return 0;
}