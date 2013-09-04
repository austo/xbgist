#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

uv_loop_t *loop;
uv_process_t child_req;
uv_process_options_t options;

char *client = "client";


static void
on_proc_exit(uv_process_t *req, int exit_status, int term_signal) {
    fprintf(stderr, "Process exited with status %d, signal %d\n",
      exit_status, term_signal);
    uv_close((uv_handle_t*) req, NULL);
}


int
main(int argc, char **argv) {

  if (argc != 4) {
    printf("usage: %s <hostname> <port> <sentence_file>\n", argv[0]);
  }

  loop = uv_default_loop();

  char* args[5];
  args[0] = client;
  args[1] = argv[1];
  args[2] = argv[2];
  args[3] = argv[3];
  args[4] = NULL;

  options.exit_cb = on_proc_exit;
  options.file = client;
  options.args = args;

  if (uv_spawn(loop, &child_req, options)) {
      fprintf(stderr, "%s\n", uv_strerror(uv_last_error(loop)));
      return 1;
  }

  return uv_run(loop, UV_RUN_DEFAULT);
}