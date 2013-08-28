#include <glib.h>

#define CONTENT_SIZE 128
#define SCHED_SIZE 100
#define NAME_SIZE 32

typedef unsigned sched_t;

typedef struct {
  int is_important;
  char content[CONTENT_SIZE];
} payload;


typedef struct {
  GHashTable* members;
  int current_round;
} manager;


typedef struct {
  char name[NAME_SIZE];
  int id;
  sched_t schedule[SCHED_SIZE];
} member;


manager *
manager_new();

void
manager_dispose(manager *mgr);

member *
member_new();

void
member_dispose(member *memb);

static void
make_name(member *memb);

static void
test_group_init(manager *mgr);

static void
print_members(gpointer key, gpointer value, gpointer data);
