#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <uv.h>

#define NUM_TEST_USERS 5
#define CLIENT_ARGC 5
#define STDIO_COUNT 3

uv_loop_t *loop;
uv_process_t child_req[NUM_TEST_USERS];
uv_process_options_t options;

char *client = "out/client";


static void
on_proc_exit(uv_process_t *req, int64_t exit_status, int term_signal) {
  fprintf(stderr, "Process exited with status %" PRId64 "(%s), signal %d\n",
    exit_status, strerror(exit_status), term_signal);
  uv_close((uv_handle_t*) req, NULL);
}


int
main(int argc, char **argv) {

  if (argc != 4) {
    printf("usage: %s <hostname> <port> <sentence_file>\n", argv[0]);
  }

  loop = uv_default_loop();

  /* display stdio and stderr in main process */
  options.stdio_count = STDIO_COUNT;
  uv_stdio_container_t child_stdio[STDIO_COUNT];
  child_stdio[0].flags = UV_IGNORE;
  child_stdio[1].flags = UV_INHERIT_FD;
  child_stdio[1].data.fd = 1;
  child_stdio[2].flags = UV_INHERIT_FD;
  child_stdio[2].data.fd = 2;
  options.stdio = child_stdio;

  char *args[CLIENT_ARGC];
  args[0] = client;
  args[1] = argv[1];
  args[2] = argv[2];
  args[3] = argv[3];
  args[4] = NULL;

  options.exit_cb = on_proc_exit;
  options.file = client;
  options.args = args;

  int i, uv_res;
  for (i = 0; i < NUM_TEST_USERS; ++i) {
    uv_res = uv_spawn(loop, &child_req[i],
      (const uv_process_options_t*) &options);
    if (uv_res) {
      fprintf(stderr, "%s\n", uv_strerror(uv_res));
      return 1;
    }
  }

  return uv_run(loop, UV_RUN_DEFAULT);
}