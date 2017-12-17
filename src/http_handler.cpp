
#include "http_handler.h"

#include <string>

#include "utils.h"
#include "timer.h"
#include "jsons.h"

#include "conf.h"
#include "common.h"

static const char* ERR_ids_resp = "{\"code\":1,\"message\":\"invalid id\"}";
static const char* ERR_server_resp = "{\"code\":1,\"message\":\"Internal Server Error\"}";

struct redis_request_t {
    std::string rqstid;
    evhtp_request_t* request;
};

static evthr_t* get_request_thr(evhtp_request_t* request) {
    evhtp_connection_t* htpconn = evhtp_request_get_connection(request);
    return htpconn->thread;
}

static void redis_mget_cb(redisAsyncContext *c, void* r, void *privdata) {

    redis_request_t* rqst = (redis_request_t*)privdata;
    const std::string rqstid = rqst->rqstid;
    evhtp_request_t* request = rqst->request;
    delete rqst; rqst = nullptr;

    int code = 1;
    size_t count = 0;
    std::string err{"success"};

    rapidjson::Document doc{ rapidjson::kObjectType };
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    rapidjson::Value arr{rapidjson::kArrayType};

    redisReply* reply = (redisReply*)r;
    if(reply == NULL) {
        evbuffer_add_printf(request->buffer_out, "%s", ERR_server_resp);
        evhtp_send_reply(request, EVHTP_RES_SERVERR);
        evhtp_request_resume(request);
        return;
    }
    switch(reply->type) {
        case REDIS_REPLY_ARRAY: {
            size_t elements = reply->elements;
            //LOG_DEBUG<<rqstid<<" REDIS_REPLY_ARRAY: "<<elements;

            for(int i=0; i<elements; i++) {
                struct redisReply *elem = *(reply->element+i);

                switch(elem->type) {
                    case REDIS_REPLY_STRING: {
                        long long len = elem->len;
                        const char* str = elem->str;
#if 1
                        rapidjson::Document subdoc{ rapidjson::kObjectType };
                        if(!JSON::Parse(subdoc, elem->str, elem->len) || !subdoc.IsObject()) {
                            //LOG_ERROR<<rqstid<<" ERROR invaid json "<<std::string(elem->str, elem->len);
                            continue;
                        }
                        rapidjson::Value item{ rapidjson::kObjectType };
                        item.CopyFrom(subdoc, allocator);
                        arr.PushBack(item, allocator);
#endif
                        ++count;

                        break;
                    }
                    case REDIS_REPLY_NIL: {
                        err = "REDIS_REPLY_NIL";
                        //LOG_DEBUG<<rqstid<<" REDIS_REPLY_NIL "<<err;
                        break;
                    } 
                    default: {
                        err = "unknown redis err";
                        break;
                    }
                }
            }
            code = 0;
            break;
        }
        case REDIS_REPLY_STRING: {
            long long len = reply->len;
            const char* str = reply->str;

            rapidjson::Document subdoc{ rapidjson::kObjectType };
            if(!JSON::Parse(subdoc, reply->str, reply->len) || !subdoc.IsObject()) {
                //LOG_ERROR<<rqstid<<" ERROR invaid json "<<std::string(reply->str, reply->len);
                break;
            }
            rapidjson::Value item{ rapidjson::kObjectType };
            item.CopyFrom(subdoc, allocator);
            arr.PushBack(item, allocator);
            ++count;
            code = 0;

            break;
        }
        case REDIS_REPLY_STATUS: {
            long long val = reply->integer;
            //LOG_DEBUG<<rqstid<<" REDIS_REPLY_STATUS "<<val;
            break;
        }
        case REDIS_REPLY_INTEGER: {
            long long val = reply->integer;
            //LOG_DEBUG<<rqstid<<" REDIS_REPLY_INTEGER "<<val;
            break;
        }
        case REDIS_REPLY_ERROR: {
            err = std::string(reply->str, reply->len);
            //LOG_ERROR<<rqstid<<" REDIS_REPLY_ERROR "<<err;
            break;
        }
        case REDIS_REPLY_NIL: {
            err = "REDIS_REPLY_NIL";
            //LOG_ERROR<<rqstid<<" REDIS_REPLY_NIL "<<err;
            break;
        } 
        default: {
            err = "unknown redis err";
            break;
        }
    }

    JSON::AddMember(doc, "code", code, allocator);
    JSON::AddMember(doc, "message", err, allocator);
    doc.AddMember("count", count, allocator);
    doc.AddMember("data", arr, allocator);

    rapidjson::StringBuffer respbuf;
    rapidjson::Writer<rapidjson::StringBuffer> respwriter(respbuf);
    doc.Accept(respwriter);
    const char* resp = respbuf.GetString();
    size_t resp_len = respbuf.GetLength();

    evbuffer_add_printf(request->buffer_out, "%*s", resp_len, resp);
    evhtp_send_reply(request, EVHTP_RES_OK);
    evhtp_request_resume(request);
}

void redis_request_handler(evhtp_request_t* r, void* arg) {
    std::string rqstid{""};
    const evhtp_kv_t* param_rqstid = evhtp_kvs_find_kv(r->uri->query, "rqstid");
    if(param_rqstid != NULL) {
        rqstid = std::string(param_rqstid->val, param_rqstid->vlen);
    } else {
        rqstid = RQSTID();
    }

    std::string ids{""};
    const evhtp_kv_t* param_ids = evhtp_kvs_find_kv(r->uri->query, "ids");
    if(param_ids != NULL) {
        ids = std::string(param_ids->val, param_ids->vlen);
    }
    if(ids.empty()) {
        // LOG_ERROR<<rqstid<<" no ids";
        evbuffer_add_printf(r->buffer_out, "%s", ERR_ids_resp);
        evhtp_send_reply(r, EVHTP_RES_BADREQ);
        return;
    }
    // split ids
    std::vector<std::string> vids = sExplode(ids, ',');
    if(vids.empty()) {
        // LOG_ERROR<<rqstid<<" ids empty ";
        evbuffer_add_printf(r->buffer_out, "%s", ERR_ids_resp);
        evhtp_send_reply(r, EVHTP_RES_BADREQ);
        return;
    }

    ////const char* v = evhtp_header_find(r->headers_in, "Host");  // 忽略大小写

    ////std::string rqst{""};
    ////size_t body_len = evbuffer_get_length(r->buffer_in);
    ////const char* body = (const char *)evbuffer_pullup(r->buffer_in, body_len);
    ////if(body_len >= 0 && body != nullptr) {
    ////    rqst = std::string(body, body_len);
    ////}

    evthr_t* thrd = get_request_thr(r);
    struct thread_info* thrd_info = (struct thread_info*)evthr_get_aux(thrd);
    if(thrd_info->redis_connected && thrd_info->redis) {

        /* mget */
        static const std::string cmd_mget{"MGET"};

        int argc = 0;
        const int MAXPARAMS = 1000;
        const char* argv[MAXPARAMS] = {0};
        size_t argvlen[MAXPARAMS] = {0};

        argv[argc] = cmd_mget.c_str();
        argvlen[argc++] = cmd_mget.size();
            
        for(const auto& id : vids) {
            argv[argc] = id.c_str();
            argvlen[argc++] = id.size();
        }
        redis_request_t* rqst = new redis_request_t{rqstid, r};
        int res = redisAsyncCommandArgv(thrd_info->redis, 
                                redis_mget_cb, (void*)rqst, argc, argv, argvlen);
        if(res == REDIS_OK) {
            /* pause the request processing */
            evhtp_request_pause(r);
            //LOG_INFO<<rqstid<<
            return;
        }
    }
    thrd_info->redis_connected = false;

    // LOG_ERROR<<rqstid<<
    fprintf(stderr, "AAAAAAAAAAAAAAAAAAA\n");
    evbuffer_add_printf(r->buffer_out, "%s", ERR_server_resp);
    evhtp_send_reply(r, EVHTP_RES_SERVERR);
    return;
}

