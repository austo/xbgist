#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <uv.h>
#include <tpl.h>

#include "client.h"
#include "xb_client_types.h"

#define do_callback(m) m->callback(m)
#define MESSAGE_SIZE 256
#define ALLOC_BUF_SIZE 512


uv_loop_t *loop;

int
main(int argc, char **argv) {

  if (argc != 3){
    fprintf(stderr, "usage: %s port\n", argv[0]);
    return 1;
  }

  int port = atoi(argv[2]);
  struct sockaddr_in xb_addr = uv_ip4_addr(argv[1], port);

  loop = uv_default_loop();

  member *memb = member_new();

  uv_tcp_init(loop, &memb->client);

  int status = uv_tcp_connect(
    &memb->connection,
    &memb->client,
    xb_addr,
    on_connect
  );

  assert(status == 0);

  return uv_run(loop, UV_RUN_DEFAULT);
}


void
on_close(uv_handle_t* handle) {
  member *memb = (member*)handle->data;
  uv_stop(loop);
  member_dispose(memb);
}


static uv_buf_t
on_alloc(uv_handle_t* handle, size_t suggested_size) {
  /* Return buffer wrapping static buffer
   * TODO: assert on_read() allocations never overlap
  */
  static char buf[ALLOC_BUF_SIZE];
  return uv_buf_init(buf, sizeof(buf));
}


void
on_connect(uv_connect_t *req, int status) {
  if (status == -1) {
    fprintf(stderr, "Error connecting to xblab server: %s\n",
      uv_err_name(uv_last_error(loop)));
    return;
  }

  member *memb = (member*)req->data;
  memb->server = req->handle;

  uv_read_start((uv_stream_t*)memb->server, on_alloc, on_read);
}


void
on_read(uv_stream_t* server, ssize_t nread, uv_buf_t buf) {

  if (nread == -1) {
    if (uv_last_error(loop).code != UV_EOF) {
      fprintf(stderr, "Read error %s\n", 
        uv_err_name(uv_last_error(loop)));
    }
  }
  
  member *memb = (member*)server->data;

  if (nread == EOF){
    uv_close((uv_handle_t*)memb->server, on_close);
    return;
  }

  memb->buf = buf;
  memb->buf.len = nread;

  int status = uv_queue_work(
    loop,
    &memb->work,
    read_work,
    (uv_after_work_cb)read_after);
  assert(status == 0);   
}


void
read_work(uv_work_t *r) {
  member *memb = (member*)r->data;
  digest_broadcast(memb);
}


void
read_after(uv_work_t *r) {
  member *memb = (member*)r->data;
  if (memb->callback != NULL){
    do_callback(memb);
  }  
}


// void
// Client::onSendCredential(MemberBaton *baton) {
//   int status = uv_queue_work(
//     loop,
//     &baton->uvWork,
//     sendCredentialWork,
//     (uv_after_work_cb)afterSendCredential);
//   assert(status == 0);   
// }


// void
// Client::sendCredentialWork(uv_work_t *r) {
//   member *memb = (member*)r->data;
//   baton->packageCredential();
// }


// void
// Client::afterSendCredential(uv_work_t *r) {
//   member *memb = (member*)r->data;
//   baton->uvWriteCb = onWrite;

//   uv_write(
//     &baton->uvWrite,
//     (uv_stream_t*)&baton->uvClient,
//     &baton->uvBuf,
//     1,
//     baton->uvWriteCb
//   );
// }


void
on_write(uv_write_t *req, int status) {
  if (status == -1) {
    fprintf(stderr, "Write error %s\n",
      uv_err_name(uv_last_error(loop)));
    return;
  }  
}


static void
unicast(struct member *memb, const char *msg) {
  size_t len = strlen(msg);
  uv_write_t *req = xb_malloc(sizeof(*req) + len);
  void *addr = req + 1;
  memcpy(addr, msg, len);
  uv_buf_t buf = uv_buf_init(addr, len);
  uv_write(req, (uv_stream_t*) &memb->client, &buf, 1, on_write);
}



void
write_payload(struct member *memb) {
  // allocate & serialize payload
  char msg[MESSAGE_SIZE];
  unicast(memb, msg);
}


void *
xb_malloc(size_t size) {
  void *ptr = malloc(size);
  assert(ptr != NULL);
  return ptr;
}

