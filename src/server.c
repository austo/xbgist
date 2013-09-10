#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "server.h"
#include "util.h"
#include "xb_types.h"

#define SERVER_ADDR "127.0.0.1"

#define N_TEST_MEMBERS 5

uv_loop_t *loop;
manager *xb_manager;
uv_mutex_t xb_mutex;


int
main(int argc, char **argv) {

  if (argc != 2){
    fprintf(stderr, "usage: %s port\n", argv[0]);
    exit(1);
  }

  /* initialize rng for sentences & filler */
  srandom(time(NULL));

  int s_port = atoi(argv[1]);

  xb_manager = manager_new();
  xb_manager->member_count = N_TEST_MEMBERS;

  uv_mutex_init(&xb_mutex);

  loop = uv_default_loop();

  uv_tcp_t server_handle;
  uv_tcp_init(loop, &server_handle);

  struct sockaddr_in addr;
  assert(0 == uv_ip4_addr(SERVER_ADDR, s_port, &addr));

  int uv_res;

  uv_res = uv_tcp_bind(&server_handle, (const struct sockaddr*) &addr);
  if (uv_res) {
    fatal(uv_res, "uv_tcp_bind");
  }

  const int backlog = 128;
  uv_res = uv_listen((uv_stream_t*)&server_handle, backlog, on_connection);
  if (uv_res) {
    fatal(uv_res, "uv_listen");
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

  int uv_res = uv_accept(server_handle, (uv_stream_t*) &memb->handle);
  if (uv_res){
    fatal(uv_res, "uv_accept");
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
  if (!has_room(xb_manager)) { 
    printf("no room in the chat\n");
    return;
  }

  uv_mutex_lock(&xb_mutex);

  // phony member id based on arrival order
  if (has_room(xb_manager)) {
    memb->id = g_hash_table_size(xb_manager->members) + 1;
    make_name(memb);
    fprintf(stdout, "adding %s to group\n", memb->name);

    insert_member(xb_manager, memb->id, memb);

    payload pload;
    payload_set(&pload, WELCOME, 1, 0, memb->name);    

    char *buf = xb_malloc(ALLOC_BUF_SIZE);
    serialize_payload(&pload, buf, ALLOC_BUF_SIZE);
    assume_buffer(memb, buf, ALLOC_BUF_SIZE);
    memb->callback = unicast_buffer;
  }

  uv_mutex_unlock(&xb_mutex);
}


static void 
new_member_after(uv_work_t *req, int status) {
  member *memb = (member*)req->data;

  /* disconnect unwelcome members */
  if (!memb->present) {
    uv_close((uv_handle_t*)&memb->handle, on_close);
    return;
  }
  do_callback(memb);
  uv_read_start((uv_stream_t*) &memb->handle, on_alloc, on_read);
  maybe_broadcast_schedules(memb->mgr, TRUE, all_members_present);
}


static void 
on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t *buf) {
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

  assume_buffer(memb, buf->base, nread);  

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

  deserialize_payload(temp_pload, memb->buf.base, memb->buf.len);

  /* process_*_work functions are guaranteed to free
   * their payloads, set the proper member callback,
   * and not molest the event loop. 
   */
  switch(temp_pload->type) {
    case WELCOME: {
      process_welcome_work(memb, temp_pload);
      break;
    }
    case SCHEDULE: {
      process_schedule_work(memb, temp_pload);
      break;
    }
    case READY: {
      process_ready_work(memb, temp_pload);
      break;
    }
    case START: {
      process_start_work(memb, temp_pload);
      break;
    }
    case ROUND: {
      process_round_work(memb, temp_pload);
      break;
    }    
  }  

  uv_mutex_unlock(&memb->mgr->mutex);
}


static void 
read_after(uv_work_t *req, int status) {
  member *memb = (member*)req->data;
  do_callback(memb);
  buffer_dispose(memb); 
}


/* ROUND WORK FUNCTIONS
 * all process_* functions must free
 * (or otherwise handle) their payloads */

static void
process_welcome_work(member *memb, payload *pload) {
  /* as of now, client should not be sending this... */
  printf("recieved WELCOME from %s\n", memb->name);
  
  free(pload);
  memb->callback = NULL;
  assert(0 == 1);
}


static void
process_schedule_work(member *memb, payload *pload) {
  printf("recieved SCHEDULE from %s\n", memb->name);

  /* TODO: implement as client schedule request
   * Do we have a request from all members?
   * If so, send them all a schedule and wait for ready response,
   * then begin another round.
   */
  memb->mgr->schedules_sent = FALSE;
  memb->schedule_delivered = FALSE;
  
  free(pload);
  memb->callback = attempt_broadcast_schedules;
}


static void
process_ready_work(member *memb, payload *pload) {
  /* when all ready messages recieved, send start */
  printf("recieved READY from %s\n", memb->name);
  memb->schedule_delivered = TRUE;
  
  free(pload);
  memb->callback = attempt_broadcast_start;
}


static void
process_start_work(member *memb, payload *pload) {
  /* as of now, client should not be sending this... */
  printf("recieved START from %s\n", memb->name);

  free(pload);
  memb->callback = NULL;
  assert(0 == 1);
}


static void
process_round_work(member *memb, payload *pload) {

  /* does user have right to broadcast?
   * does user want to broadcast?
   * set buffer for group
   * broadcast
   */
  printf("recieved ROUND from %s\n", memb->name);
 
  if (member_can_transmit(memb->mgr, memb)) {
    assume_payload(memb->mgr, pload);

    if (memb->mgr->payload->is_important) {
      printf("%s says: %s\n", memb->name, memb->mgr->payload->content);
    }
  }
  else {
    free(pload);
  }

  memb->message_processed = TRUE;
  memb->callback = attempt_broadcast_round;  
}


/* END ROUND WORK FUNCTIONS */


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
  char msg[ALLOC_BUF_SIZE];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  iterate_members(mgr, g_unicast, msg, FALSE);
}