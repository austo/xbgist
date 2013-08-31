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
	tpl_node *tn = tpl_map("S(iivc#)", &pload, CONTENT_SIZE);
  tpl_load(tn, TPL_MEM|TPL_EXCESS_OK, &memb->buf, &memb->buf.len);
  tpl_unpack(tn, 0);
  tpl_free(tn);
}
