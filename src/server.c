#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#include "server.h"
#include "util.h"
#include "xb_types.h"

#define SERVER_ADDR "0.0.0.0" // a.k.a. "all interfaces"

#define N_TEST_MEMBERS 5

uv_loop_t *loop;
manager *xb_manager;
uv_mutex_t xb_mutex;


int
main(int argc, char** argv) {

  if (argc != 2){
    fprintf(stderr, "usage: %s port\n", argv[0]);
    return 1;
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

    char buf[ALLOC_BUF_SIZE];
    serialize_payload(&pload, buf, ALLOC_BUF_SIZE);

    unicast(memb, buf);
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
  uv_read_start((uv_stream_t*) &memb->handle, on_alloc, on_read);

  maybe_broadcast_schedules(memb->mgr);
}


static uv_buf_t
on_alloc(uv_handle_t* handle, size_t suggested_size) {
  return uv_buf_init(xb_malloc(suggested_size), suggested_size);
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

  assume_buffer(memb, buf.base, nread);  

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

  switch(temp_pload->type) {
    case WELCOME: {
      process_welcome(memb, temp_pload);
      break;
    }
    case SCHEDULE: {
      process_schedule(memb, temp_pload);
      break;
    }
    case READY: {
      process_ready(memb, temp_pload);
      break;
    }
    case START: {
      process_start(memb, temp_pload);
      break;
    }
    case ROUND: {
      process_round(memb, temp_pload);
      break;
    }    
  }

  /* TODO: make ^ functions responsible for freeing temp_pload,
   * or pass **payload and set to null */
  // if (temp_pload != NULL) {
  //   free(temp_pload);
  // }

  uv_mutex_unlock(&memb->mgr->mutex);
}


static void 
read_after(uv_work_t *req, int status) {
  member *memb = (member*)req->data;
  do_callback(memb->mgr);

  buffer_dispose(memb); 
}


/* ROUND WORK FUNCTIONS */

static void
process_welcome(member *memb, payload *pload) {
  printf("recieved WELCOME from %s\n", memb->name);
  /* as of now, client should not be sending this... */
  assert(0 == 1);
}

static void
process_schedule(member *memb, payload *pload) {
  printf("recieved SCHEDULE from %s\n", memb->name);

  /* TODO: implement as client schedule request */
  assert(0 == 1);
}


static void
process_ready(member *memb, payload *pload) {
  /* when all ready messages recieved, send start */
  printf("recieved READY from %s\n", memb->name);
  memb->schedule_delivered = TRUE;
  memb->mgr->callback = NULL;

  maybe_broadcast_start(memb->mgr);
}


static void
process_start(member *memb, payload *pload) {
  /* as of now, client should not be sending this... */
  printf("recieved START from %s\n", memb->name);
  assert(0 == 1);
}


static void
process_round(member *memb, payload *pload) {

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
    if (pload != NULL) {
      printf("payload != NULL, freeing payload\n");
      free(pload);
      pload = NULL;
    }
  }

  memb->message_processed = TRUE;

  if (all_messages_processed(memb->mgr)) {

    calculate_modulo(memb->mgr);
    memb->mgr->payload->type = ROUND;    

    broadcast_payload(memb->mgr);

    memb->mgr->round_finished = TRUE;
    memb->mgr->callback = reset_round;
  }
  else {
    memb->mgr->round_finished = FALSE;
    memb->mgr->callback = NULL;
  }
}


/* END ROUND WORK FUNCTIONS */


static void
on_write(uv_write_t *req, int status) {
  member *memb = (member *)req->data;
  printf("freeing write req for %s\n", memb->name);
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
maybe_broadcast_schedules(manager *mgr) {
  uv_mutex_lock(&mgr->mutex);
  if (!mgr->schedules_sent && all_members_present(mgr)) {
    usleep(1000);
    printf("broadcasting schedules...\n");
    broadcast_schedules(mgr);
  }
  uv_mutex_unlock(&mgr->mutex);
}


static void
maybe_broadcast_start(manager *mgr) {
  printf("entering maybe_broadcast_start\n");

  if (mgr->chat_started) { return; }

  if (all_schedules_delivered(mgr)) {
    
    printf("all schedules delivered, filling payload\n");
    fill_start_payload(mgr);

    printf("broadcasting payload\n");
    broadcast_payload(mgr);

    mgr->chat_started = TRUE;
    mgr->callback = NULL;
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


static void
broadcast_payload(manager *mgr) {
  char msg[ALLOC_BUF_SIZE];
  serialize_payload(mgr->payload, msg, ALLOC_BUF_SIZE);
  iterate_members(mgr, g_unicast, msg, FALSE);
}


static void
broadcast_schedules(manager *mgr) {
  /* set schedule && serialize payload for each member */
  mgr->payload->type = SCHEDULE;
  mgr->payload->is_important = 1;
  mgr->payload->modulo = 0;

  fill_member_schedules(mgr, NULL);
  iterate_members(mgr, g_unicast_payload, mgr->payload->content, FALSE);
  mgr->schedules_sent = TRUE;
}


static void
g_unicast_payload(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  
  char buf[ALLOC_BUF_SIZE];

  /* copy member's schedule into manager's payload */
  char *content = (char *)data; // memb->mgr->payload->content
  memcpy(content, memb->schedule, sizeof(memb->schedule));
  serialize_payload(memb->mgr->payload, buf, ALLOC_BUF_SIZE);
  unicast(memb, buf);
}


static void
g_unicast(gpointer key, gpointer value, gpointer data) {
  member *memb = (member *)value;
  char *msg = (char *)data;
  unicast(memb, msg);
}


static void
unicast(struct member *memb, const char *msg) {
  // size_t len = strlen(msg);
  uv_write_t *req = xb_malloc(sizeof(*req) + ALLOC_BUF_SIZE);
  req->data = memb;
  void *addr = req + 1;
  memcpy(addr, msg, ALLOC_BUF_SIZE);  
  uv_buf_t buf = uv_buf_init(addr, ALLOC_BUF_SIZE);
  uv_write(req, (uv_stream_t*) &memb->handle, &buf, 1, on_write);
  memb->message_processed = FALSE;
}

