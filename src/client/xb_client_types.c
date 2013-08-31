#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "tpl.h"
#include "xb_client_types.h"

member *
member_new() {
  member *memb = malloc(sizeof(*memb));
  assert(memb != NULL);
  memb->message_processed = FALSE;
  memb->work.data = memb;
  memb->client.data = memb;
  memb->connection.data = memb;
  return memb;  
}


void
member_dispose(member *memb) {
  free(memb);
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
deserialize_payload(struct payload *pload, void *buf, size_t len) {
  tpl_node *tn = tpl_map("S(iivc#)", pload, CONTENT_SIZE);
  tpl_load(tn, TPL_MEM|TPL_EXCESS_OK, buf, len);
  tpl_unpack(tn, 0);
  tpl_free(tn);
}


void
process_schedule(member *memb, payload *pload) {
  assert(0 == 1);
}


void
process_ready(member *memb, payload *pload) {
  assert(0 == 1);
}


void
process_start(member *memb, payload *pload) {
  assert(0 == 1);
}


void
process_round(member *memb, payload *pload) {
  assert(0 == 1);
}
