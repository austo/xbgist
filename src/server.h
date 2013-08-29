#ifndef SERVER_H
#define SERVER_H

#include <glib.h>
#include <uv.h>

#include "xb_types.h"

static void
unicast(struct member *memb, const char *msg);

static void
g_unicast(gpointer key, gpointer value, gpointer data);

static void
broadcast(const char *fmt, ...);

static uv_buf_t
on_alloc(uv_handle_t* handle, size_t suggested_size);

static void
on_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf);

static void
on_write(uv_write_t *req, int status);

static void
on_close(uv_handle_t* handle);

static void
on_connection(uv_stream_t* server_handle, int status);

static void
new_member_work(uv_work_t *req);

static void 
new_member_after(uv_work_t *req);

static void
broadcast_work(uv_work_t *req);

static void 
broadcast_after(uv_work_t *req);


#endif