#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include <uv.h>

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
  gint id;
  sched_t schedule[SCHED_SIZE];
  uv_tcp_t handle;
  uv_work_t work;
  uv_buf_t buf;
} member;


manager *
manager_new() {
  manager *mgr = malloc(sizeof(manager));
  mgr->members = g_hash_table_new(g_direct_hash, g_direct_equal);
  mgr->current_round = 0;
  return mgr;
}


void
manager_dispose(manager *mgr) {
  free(mgr);
}


member *
member_new(const char *name) {
  member *memb = malloc(sizeof(member));
  member->work.data = (void *)memb;
  snprintf(memb->name, sizeof(memb->name), "%s" name);
  return memb;  
}


void
member_dispose(member *memb) {
  free(memb);
}


char *
serialize_payload(const payload *pload) {

}




static void
processMemberMessage(manager *mgr, int memb_id, payload *pload);

static void
processMemberMessage(manager *mgr, int memb_id, payload *pload) {
  
  /* Can client broadcast?
   * If so, verify message.
   * If message is okay, is the message important?
   * If so, set manager round message and broadcast with next round modulo.
   * If not, broadcast no message with next round modulo. 
   */

  uv_mutex_lock(&classMutex_);

  Member& member = members[memberId];

  if (memberCanTransmit(mgr, &member)) {        
    
    setRoundMessage(payload.is_important() ?
      payload.content() : "whatever");  
    flags.messagesDelivered = false;
  }

  member.messageProcessed = true;  

  broadcastRoundIfNecessary();  

  uv_mutex_unlock(&classMutex_);
}


static void
broadcastRoundIfNecessary(manager *mgr) {
  if (allMessagesProcessed()) {
    getMessageBuffers();

    broadcast(false);

    ++currentRound_;
    flags.resetRound();
  }
  else {
    cout << "all messages not processed\n";
  }
}