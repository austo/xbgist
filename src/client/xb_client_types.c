#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "tpl.h"
#include "xb_client_types.h"
#include "util.h"
#include "sentence_util.h"


extern void
write_payload(struct member *memb);


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


member *
member_new() {

  // member *memb = xb_malloc(sizeof(*memb));
  member *memb = calloc(1, sizeof(*memb));
  assert(memb != NULL);
  memb->work.data = memb;
  memb->client.data = memb;
  memb->connection.data = memb;
  memb->write.data = memb;
  memb->payload = NULL;
  memb->current_round = 0;
  return memb;  
}


void
member_dispose(member *memb) {
  if (memb->payload != NULL) {
    free(memb->payload);
  }
  free(memb);
  memb = NULL;
}


void
assume_payload(member *memb, payload *pload) {
  if (memb->payload != NULL) {
    free(memb->payload);
  }
  memb->payload = pload;
  pload = NULL;
}


int
flip_coin() {
  return simple_random(USHRT_MAX) % 2;
}


void
display_message(payload *pload) {
  if (pload->type != ROUND || !pload->is_important) { return; }
  printf("%s\n", pload->content);
}


void
digest_broadcast(member *memb) {
  /* deserialize payload */
  payload pload;

  printf("payload memb->buf.len: %d\n",  (int)memb->buf.len);

  printf("payload memb->buf.base: %.*s\n",
    (int)(memb->buf.len - 1), memb->buf.base);

  deserialize_payload(&pload, &memb->buf.base, memb->buf.len);

  switch(pload.type) {
    case WELCOME: {
      printf("%s\n", pload.content);
      break; 
    }
    case SCHEDULE: {
      process_schedule(memb, &pload);
      break;
    }
    case READY: {
      process_ready(memb, &pload);
      break;
    }
    case START:
    case ROUND: {
      process_round(memb, &pload);
      break;
    }
  }

}


void
process_schedule(member *memb, payload *pload) {
  /* add schedule to member, respond with ready message */
  memcpy(memb->schedule, pload->content, CONTENT_SIZE);

  payload *temp_pload = payload_new(READY, 1, 0, NULL);
  assume_payload(memb, temp_pload);
  memb->callback = write_payload;
}


void
process_ready(member *memb, payload *pload) {
  // as of now, the server shouldn't be sending this...
  assert(0 == 1);
}


void
process_start(member *memb, payload *pload) {
  /* can actually call process_round */

  assert(0 == 1);
}


void
process_round(member *memb, payload *pload) {
  assert(memb->payload != NULL);

  /* 1. is it my turn?
   * 2. do I have anything to say?
   * 3. build payload
   * 4. send & set callback
   */


  if (memb->schedule[memb->current_round] == pload->modulo) {
    int important = flip_coin();
    payload_set(memb->payload, ROUND, important, 0,
      important ? get_sentence() : NULL);
  }
  else {
    payload_set(memb->payload, ROUND, 0, 0, NULL);
  }
  memb->callback = write_payload;
}

