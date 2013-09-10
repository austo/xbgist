#ifndef SERVER_H
#define SERVER_H

#include <glib.h>
#include <uv.h>

#include "xb_types.h"

static void
broadcast(manager *mgr, const char *fmt, ...);

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
process_welcome_work(member *memb, payload *pload);

static void
process_schedule_work(member *memb, payload *pload);

static void
process_ready_work(member *memb, payload *pload);

static void
process_start_work(member *memb, payload *pload);

static void
process_round_work(member *memb, payload *pload);




#endif