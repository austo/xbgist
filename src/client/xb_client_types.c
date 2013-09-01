#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "tpl.h"
#include "xb_client_types.h"
#include "util.h"


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


member *
member_new() {
  member *memb = xb_malloc(sizeof(*memb));
  memb->work.data = memb;
  memb->client.data = memb;
  memb->connection.data = memb;
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
}


void
assume_payload(member *memb, payload *pload) {
  if (memb->payload != NULL) {
    free(memb->payload);
  }
  memb->payload = pload;
  pload = NULL;
}


void
digest_broadcast(member *memb) {
	/* deserialize payload */
	payload pload;
  deserialize_payload(&pload, &memb->buf.base, memb->buf.len);

  switch(pload.type) {
    case SCHEDULE: {
      process_schedule(memb, &pload);
      break;
    }
    case READY: {
      process_ready(memb, &pload);
      break;
    }
    case START: {
      process_start(memb, &pload);
      break;
    }
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
  /* implement start chat: */

  assert(0 == 1);
}


void
process_round(member *memb, payload *pload) {
  assert(0 == 1);
}

