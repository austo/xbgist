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
g_unicast_payload(gpointer key, gpointer value, gpointer data);

static void
broadcast(manager *mgr, const char *fmt, ...);

static void
broadcast_payload(manager *mgr);

static void
broadcast_schedules(manager *mgr);

static void
maybe_broadcast_schedules(manager *mgr, gboolean lock,
	gboolean(*test)(manager *));

static void
maybe_broadcast_start(manager *mgr);

static void
on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t *buf);

static void
on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t *buf);

static void
on_close(uv_handle_t* handle);

static void
on_connection(uv_stream_t* server_handle, int status);

static void
new_member_work(uv_work_t *req);

static void 
new_member_after(uv_work_t *req, int status);

static void
read_work(uv_work_t *req);

static void 
read_after(uv_work_t *req, int status);


/*--------------------------------------------------*/


static void
process_welcome(member *memb, payload *pload);

static void
process_schedule(member *memb, payload *pload);

static void
process_ready(member *memb, payload *pload);

static void
process_start(member *memb, payload *pload);

static void
process_round(member *memb, payload *pload);




#endif