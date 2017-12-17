
#pragma once

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include <evhtp/evhtp.h>

#define REDIS_CTX_SET_DATA(redis_ctx, userdata) \
                            redis_ctx->data = (void*)(userdata);
#define REDIS_CTX_GET_DATA(redis_ctx) \
                            redis_ctx->data

struct app_info {
    evbase_t* evbase;
    evhtp_t* evhtp;

    char* redis_host;
    uint16_t redis_port;
};

struct thread_info {
    struct app_info* parent;
    evbase_t* evbase;

    bool redis_connected;
    redisAsyncContext* redis;

    event_t* timer_ev;
    struct timeval timer_tv;
};


