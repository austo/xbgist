#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

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