/* Adapted from "libuv-chat" by Ben Noordhuis
 * https://github.com/bnoordhuis/libuv-chat
 */

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h> // offsetof
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h> // usleep


#include "generic_queue.h"
#include "server.h"
#include "util.h"

#define SERVER_ADDR "0.0.0.0" // a.k.a. "all interfaces"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))


static void* users[2];


int
main(int argc, char** argv) {

  if (argc != 2){
    fprintf(stderr, "usage: %s port\n", argv[0]);
    return 1;
  }

  int s_port = atoi(argv[1]);

  QUEUE_INIT(&users);

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

  return 0;
}


static void
on_connection(uv_stream_t* server_handle, int status) {
  assert(status == 0);

  // hurray, a new user!
  struct user *user = xmalloc(sizeof(*user));
  user->work.data = (void*)user;

  uv_tcp_init(uv_default_loop(), &user->handle);

  if (uv_accept(server_handle, (uv_stream_t*) &user->handle)){
    fatal("uv_accept");
  }

  status = uv_queue_work(
    uv_default_loop(),
    &user->work,
    new_user_work,
    (uv_after_work_cb)new_user_after);

  assert(status == 0);  
}


static void
new_user_work(uv_work_t *req) {
  struct user *user = (struct user*)req->data;  

  // add user to the list
  QUEUE_INSERT_TAIL(&users, &user->queue);
  make_user_id(user);
}


static void 
new_user_after(uv_work_t *req) {
  struct user *user = (struct user*)req->data;

  // now tell everyone & start listening
  broadcast("* %s joined from %s\n", user->id, addr_and_port(user));
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
  struct user *user = container_of(handle, struct user, handle);

  if (nread == -1) {
  // user disconnected
    QUEUE_REMOVE(&user->queue);
    uv_close((uv_handle_t*) &user->handle, on_close);
    broadcast("* %s has left the building\n", user->id);
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
  struct user *user = (struct user*)req->data;
  broadcast("%s said: %.*s", user->id, (int)user->buf.len, user->buf.base);
}


static void 
broadcast_after(uv_work_t *req) {
  struct user *user = (struct user*)req->data;

  fprintf(stdout, "Broadcast \"%.*s\" from %s.\n",
  (int)(user->buf.len - 1), user->buf.base, user->id);
}


static void
on_write(uv_write_t *req, int status) {
  free(req);
}


static void
on_close(uv_handle_t* handle) {
  struct user *user = container_of(handle, struct user, handle);
  free(user);
}


static void
fatal(const char *what) {
  uv_err_t err = uv_last_error(uv_default_loop());
  fprintf(stderr, "%s: %s\n", what, uv_strerror(err));
  exit(1);
}


static void
broadcast(const char *fmt, ...) {
  QUEUE* q;
  char msg[512];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  int n_users = 0;

  QUEUE_FOREACH(q, &users) {
    struct user *user = container_of(q, struct user, queue);
    unicast(user, msg);
    ++n_users;
  }
  if (n_users > 1) {
    sched_t **schedules = malloc(sizeof(sched_t *) * n_users);
    n_users = 0;

    QUEUE_FOREACH(q, &users) {
      struct user *user = container_of(q, struct user, queue);
      schedules[n_users++] = &user->schedule[0];
    }
    fprintf(stdout, "calling fill_disjoint_arrays with %d users.\n", n_users);
    fill_disjoint_arrays(schedules, n_users, SCHED_SIZE);

    int i, j;
    for (i = 0; i < n_users; ++i) {
      for (j = 0; j < SCHED_SIZE; ++j) {
        if (j > 0) {
          fprintf(stdout, ", ");
        }
        fprintf(stdout, "%d", schedules[i][j]);
      }
      fprintf(stdout, "\n");
    }
    free(schedules);
  }
}


static void
unicast(struct user *user, const char *msg) {
  size_t len = strlen(msg);
  uv_write_t *req = xmalloc(sizeof(*req) + len);
  void *addr = req + 1;
  memcpy(addr, msg, len);
  uv_buf_t buf = uv_buf_init(addr, len);
  uv_write(req, (uv_stream_t*) &user->handle, &buf, 1, on_write);
}


static void
make_user_id(struct user *user) {
  // most popular baby names in Alabama in 2011
  static const char *names[] = {
    "Mason", "Ava", "James", "Madison", "Jacob", "Olivia", "John",
    "Isabella", "Noah", "Addison", "Jayden", "Chloe", "Elijah",
    "Elizabeth", "Jackson", "Abigail"
  };
  static unsigned int index0 = 0;
  static unsigned int index1 = 1;

  snprintf(user->id, sizeof(user->id), "%s %s", names[index0], names[index1]);
  index0 = (index0 + 3) % ARRAY_SIZE(names);
  index1 = (index1 + 7) % ARRAY_SIZE(names);

  usleep(10000); //simulate blocking thread
}


static const char *
addr_and_port(struct user *user) {
  struct sockaddr_in name;
  int namelen = sizeof(name);
  if (uv_tcp_getpeername(
    &user->handle, (struct sockaddr*) &name, &namelen)){
    fatal("uv_tcp_getpeername");
  }

  char addr[16];
  static char buf[32];
  uv_inet_ntop(AF_INET, &name.sin_addr, addr, sizeof(addr));
  snprintf(buf, sizeof(buf), "%s:%d", addr, ntohs(name.sin_port));

  return buf;
}


static void *
xmalloc(size_t len) {
  void *ptr = malloc(len);
  assert(ptr != NULL);
  return ptr;
}
