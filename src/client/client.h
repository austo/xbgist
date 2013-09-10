#ifndef CLIENT_H
#define CLIENT_H

#include <uv.h>
#include "xb_client_types.h"

void
on_close(uv_handle_t* handle);

void
on_connect(uv_connect_t *req, int status);

void
on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t *buf);

void
read_work(uv_work_t *r);

void
read_after(uv_work_t *r);

static void
unicast(struct member *memb, const char *msg);

void
write_payload(struct member *memb);

#endif