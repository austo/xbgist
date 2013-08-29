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

  uv_tcp_t server_handle;
  uv_tcp_init(uv_default_loop(), &server_handle);

  const struct sockaddr_in addr = uv_ip4_addr(SERVER_ADDR, s_port);
  if (uv_tcp_bind(&server_handle, addr)){
    fatal("uv_tcp_bind");
  }

  const int backlog = 128;
  if (uv_listen((uv_stream_t*) &server_handle, backlog, on_connection)) {
    fatal("uv_listen");
  }

  printf("Listening at %s:%d\n", SERVER_ADDR, s_port);
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  uv_mutex_destroy(&xb_mutex);

  return 0;
}


static void
on_connection(uv_stream_t* server_handle, int status) {
  assert(status == 0);

  member *memb = member_new();

  uv_tcp_init(uv_default_loop(), &memb->handle);

  if (uv_accept(server_handle, (uv_stream_t*) &memb->handle)){
    fatal("uv_accept");
  }

  status = uv_queue_work(
    uv_default_loop(),
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

  if (!memb->present) {}
  broadcast(memb->mgr, "* %s joined from %s\n", memb->name, addr_and_port(memb));
  uv_read_start((uv_stream_t*) &user->handle, on_alloc, on_read);
}


static uv_buf_t
on_alloc(uv_handle_t* handle, size_t suggested_size) {
  // Return a buffer that wraps a static buffer.
  // Safe because our on_read() allocations never overlap.
  static char buf[512];
  return uv_buf_init(buf, sizeof(buf));
}


static void 
on_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
  member *memb = (member*)handle->data;
  if (nread == -1) {
  // user disconnected
    if (memb->present) {
      broadcast(memb->mgr, "* %s has left the building\n", memb->name);
    }
    uv_close((uv_handle_t*) &memb->handle, on_close);
    return;
  }

  user->buf = buf;
  user->buf.len = nread;

  int status = uv_queue_work(
    uv_default_loop(),
    &user->work,
    broadcast_work,
    (uv_after_work_cb)broadcast_after);

  assert(status == 0);  
}


static void
broadcast_work(uv_work_t *req) {
  member *memb = (member*)req->data;
  broadcast(memb->mgr, "%s said: %.*s", memb->name, (int)memb->buf.len, memb->buf.base);
}


static void 
broadcast_after(uv_work_t *req) {
  member *memb = (member*)req->data;

  fprintf(stdout, "Broadcast \"%.*s\" from %s.\n",
    (int)(user->buf.len - 1), user->buf.base, user->id);
}


static void
on_write(uv_write_t *req, int status) {
  free(req);
}


static void
on_close(uv_handle_t* handle) {
  member *memb = (member*)handle->data;
  if (memb->present) {
    remove_member(manager *mgr, memb->id)
  }
}


static void
broadcast(struct manager *mgr, const char *fmt, ...) {
  char msg[512];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  iterate_members(mgr, g_unicast, msg, TRUE);  
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
}

