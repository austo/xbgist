#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "server.h"
#include "util.h"
#include "xb_types.h"

#define SERVER_ADDR "0.0.0.0" // a.k.a. "all interfaces"
#define ALLOC_BUF_SIZE 512


uv_loop_t *loop;
manager *xb_manager;
uv_mutex_t xb_mutex;


int
main(int argc, char** argv) {

  if (argc != 2){
    fprintf(stderr, "usage: %s port\n", argv[0]);
    return 1;
  }

  int s_port = atoi(argv[1]);

  xb_manager = manager_new();

  uv_mutex_init(&xb_mutex);

  loop = uv_default_loop();

  uv_tcp_t server_handle;
  uv_tcp_init(loop, &server_handle);

  const struct sockaddr_in addr = uv_ip4_addr(SERVER_ADDR, s_port);
  if (uv_tcp_bind(&server_handle, addr)){
    fatal("uv_tcp_bind");
  }

  const int backlog = 128;
  if (uv_listen((uv_stream_t*)&server_handle, backlog, on_connection)) {
    fatal("uv_listen");
  }

  printf("Listening at %s:%d\n", SERVER_ADDR, s_port);
  uv_run(loop, UV_RUN_DEFAULT);

  uv_mutex_destroy(&xb_mutex);

  return 0;
}


static void
on_connection(uv_stream_t* server_handle, int status) {
  assert(status == 0);

  member *memb = member_new();

  uv_tcp_init(loop, &memb->handle);

  if (uv_accept(server_handle, (uv_stream_t*) &memb->handle)){
    fatal("uv_accept");
  }

  status = uv_queue_work(
    loop,
    &memb->work,
    new_member_work,
    (uv_after_work_cb)new_member_after);

  assert(status == 0);  
}


static void
new_member_work(uv_work_t *req) {
  member *memb = (member*)req->data;

  // add user to the list
  if (!has_room(xb_manager)) { return; }

  uv_mutex_lock(&xb_mutex);

  if (has_room(xb_manager)) {
    memb->id = g_hash_table_size(xb_manager->members) + 1;
    make_name(memb);
    insert_member(xb_manager, memb->id, memb);
  }

  uv_mutex_unlock(&xb_mutex);
}


static void 
new_member_after(uv_work_t *req) {
  member *memb = (member*)req->data;

  /* disconnect unwelcome members */
  if (!memb->present) {
    uv_close((uv_handle_t*)&memb->handle, on_close);
    return;
  }


  // TODO: if all members have connected, send schedule
  // broadcast(memb->mgr, "* %s joined from %s\n", memb->name, addr_and_port(memb));
  uv_read_start((uv_stream_t*) &memb->handle, on_alloc, on_read);
}


static uv_buf_t
on_alloc(uv_handle_t* handle, size_t suggested_size) {
  /* Return buffer wrapping static buffer
   * TODO: assert on_read() allocations never overlap
  */
  static char buf[ALLOC_BUF_SIZE];
  return uv_buf_init(buf, sizeof(buf));
}


static void 
on_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
  member *memb = (member*)handle->data;
  if (nread == -1) {
  /* user disconnected */
    if (memb->present) {
      broadcast(memb->mgr, "* %s has left the building\n", memb->name);
    }
    uv_close((uv_handle_t*)&memb->handle, on_close);
    return;
  }

  assert(memb->present);

  /* TODO:
   * does user have right to broadcast?
   * does user want to broadcast?
   * set buffer for group
   * broadcast
   */

  memb->buf = buf;
  memb->buf.len = nread;

  int status = uv_queue_work(
    loop,
    &memb->work,
    read_work,
    (uv_after_work_cb)read_after);

  assert(status == 0);  
}


/* broadcast a given member's message to the entire group */
static void
read_work(uv_work_t *req) {
  member *memb = (member*)req->data;
  uv_mutex_lock(&memb->mgr->mutex);

  payload *temp_pload = xb_malloc(sizeof(*temp_pload));

  deserialize_payload(temp_pload, &memb->buf, memb->buf.len);

  switch(temp_pload->type) {
    case SCHEDULE: {
      break;
    }
    case READY: {
      break;
    }
    case START: {
      break;
    }
    case ROUND: {
      process_round(memb, temp_pload);
      break;
    }
  }  

  uv_mutex_unlock(&memb->mgr->mutex);


}


static void 
read_after(uv_work_t *req) {
  member *memb = (member*)req->data;

  if (memb->mgr->round_finished) {
    ++memb->mgr->current_round;
    memb->mgr->round_finished = FALSE;
  }

  // TODO: diagnostic message based on type
  fprintf(stdout, "Broadcast \"%.*s\" from %s.\n",
    (int)(memb->buf.len - 1), memb->buf.base, memb->name);
}


/* ROUND WORK FUNCTIONS */

static void
process_schedule(member *memb, payload *pload) {

}



static void
process_round(member *memb, payload *pload) {

  if (member_can_transmit(memb->mgr, memb)) {
    assume_payload(memb->mgr, pload);
  }
  else {
    free(pload);
  }

  memb->message_processed = TRUE;

  if (all_messages_processed(memb->mgr)) {
    calculate_modulo(memb->mgr);

    memb->mgr->payload->type = ROUND;
    serialize_payload(memb->mgr->payload, &memb->buf, memb->buf.len);

    broadcast(memb->mgr, "%s said: %.*s", memb->name,
      (int)memb->buf.len, memb->buf.base);
    memb->mgr->round_finished = TRUE;
  }
}


/* END ROUND WORK FUNCTIONS */


static void
on_write(uv_write_t *req, int status) {
  free(req);
}


static void
on_close(uv_handle_t* handle) {
  member *memb = (member*)handle->data;
  if (memb->present) {
    remove_member(memb->mgr, memb->id);
  }
  else {
    member_dispose(memb);
  }
}


static void
broadcast(struct manager *mgr, const char *fmt, ...) {
  char msg[512];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  iterate_members(mgr, g_unicast, msg, FALSE);
}


static void
g_unicast(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  char *msg = (char *)data;
  unicast(memb, msg);
}


static void
unicast(struct member *memb, const char *msg) {
  size_t len = strlen(msg);
  uv_write_t *req = xb_malloc(sizeof(*req) + len);
  void *addr = req + 1;
  memcpy(addr, msg, len);
  uv_buf_t buf = uv_buf_init(addr, len);
  uv_write(req, (uv_stream_t*) &memb->handle, &buf, 1, on_write);
  memb->message_processed = FALSE;
}

