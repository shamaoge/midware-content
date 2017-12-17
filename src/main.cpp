
#include <string>
#include <thread>
#include <iostream>

#include "conf.h"
#include "common.h"
#include "http_handler.h"

void connectCallback(const redisAsyncContext *c, int status) {
    struct thread_info* thrd_info = (struct thread_info*)REDIS_CTX_GET_DATA(c);
    if (status != REDIS_OK) {
        printf("connectCallback Error: %s\n", c->errstr);
        thrd_info->redis_connected = false;
    } else {
        thrd_info->redis_connected = true;
    }
    printf("connectCallback Connect... %p %d %d\n", c, status, thrd_info->redis_connected);
}

void disconnectCallback(const redisAsyncContext* c, int status) {
    printf("disconnectCallback Disconnected... %p %d %d\n", c, status, REDIS_OK);
    struct thread_info* thrd_info = (struct thread_info*)c->data;
    thrd_info->redis_connected = false;

    if (status != REDIS_OK) {
        printf("disconnectCallback Error: %s\n", c->errstr);
        return;
    }   
}

static void redis_reconnect(struct thread_info* thrd_info) {
    if(!thrd_info->redis_connected) {
        redisAsyncContext* _redis = redisAsyncConnect(thrd_info->parent->redis_host, thrd_info->parent->redis_port);
        REDIS_CTX_SET_DATA(_redis, thrd_info);
        if(_redis->err) {
            thrd_info->redis_connected = false;
            //redisAsyncDisconnect(_redis);
            //redisAsyncFree(_redis);
            fprintf(stderr, "redis connect error %s:%d\n", thrd_info->parent->redis_host, thrd_info->parent->redis_port);
            //LOG_ERROR<<"redis connect error"<<parent->redis_host<<":"<<parent->redis_port;
        } else {
            //if(thrd_info->redis) {
            //    //redisAsyncDisconnect(thrd_info->redis);   // coredump
            //    //redisAsyncFree(thrd_info->redis);         // coredump
            //}
            thrd_info->redis = _redis;
            redisLibeventAttach(thrd_info->redis, thrd_info->evbase);
            redisAsyncSetConnectCallback(thrd_info->redis, connectCallback);
            redisAsyncSetDisconnectCallback(thrd_info->redis, disconnectCallback);
        }
    }
}

static void redis_connection_checker(evutil_socket_t sock, short which, void * arg) {
    struct thread_info* thrd_info = (struct thread_info*)arg;
    fprintf(stderr, "check connection status %p %d\n", thrd_info->redis, thrd_info->redis_connected);
    redis_reconnect(thrd_info);
    thrd_info->timer_tv.tv_sec = rand()%10 + 2;
    thrd_info->timer_tv.tv_usec = 0;
    evtimer_add(thrd_info->timer_ev, &thrd_info->timer_tv);
}

void gateway_init_thread(evhtp_t* evhtp, evthr_t* thread, void* arg) {

    srand(time(NULL) + pthread_self());

    struct app_info* parent = (struct app_info*)arg;
    struct thread_info* thrd_info = new struct thread_info;
    
    thrd_info->parent = parent;
    thrd_info->evbase = evthr_get_base(thread);
    thrd_info->redis_connected = false;
    thrd_info->timer_tv.tv_sec = 5;
    thrd_info->timer_tv.tv_usec = 0;
    thrd_info->timer_ev = evtimer_new(thrd_info->evbase, redis_connection_checker, (void*)thrd_info);

    evtimer_add(thrd_info->timer_ev, &thrd_info->timer_tv); // 定时检查redis连接

    redis_reconnect(thrd_info);

    evthr_set_aux(thread, thrd_info);
}

void run_http_service(void) {
    Config& config = Config::instance();

    int http_threads    = config.getThreads();
    std::string baddr   = config.getHttpAddr();
    uint16_t bport      = config.getHttpPort();
    int backlog         = config.getHttpBacklog();
    int nodelay         = config.getHttpNodelay();
    int defer_accept    = config.getHttpDeferAccept();
    int reuse_port      = config.getHttpReusePort();

    struct event_base* evbase = event_base_new();
    evhtp_t* evhtp = evhtp_new(evbase, NULL);

    evhtp_set_parser_flags(evhtp, EVHTP_PARSE_QUERY_FLAG_LENIENT);
    if(nodelay)
        evhtp_enable_flag(evhtp, EVHTP_FLAG_ENABLE_NODELAY);

    if(defer_accept)
        evhtp_enable_flag(evhtp, EVHTP_FLAG_ENABLE_DEFER_ACCEPT);

    if(reuse_port)
        evhtp_enable_flag(evhtp, EVHTP_FLAG_ENABLE_REUSEPORT);

    for(request_handler_t* p=request_handlers; p && p->path; p++) {
        evhtp_set_cb(evhtp, p->path, p->handler, p->arg);
    }

#ifndef EVHTP_DISABLE_EVTHR
    if(http_threads > 0) {
        struct app_info* app_p = new struct app_info;
        app_p->evbase = evbase;
        app_p->evhtp = evhtp;
        app_p->redis_host = (char*)"127.0.0.1";
        app_p->redis_port = 6380;
        evhtp_use_threads_wexit(evhtp, gateway_init_thread, NULL, http_threads, app_p);
    }
#else
#error 'EVHTP_DISABLE_EVTHR'
#endif

    evhtp_bind_socket(evhtp, baddr.c_str(), bport, backlog);
    event_base_loop(evbase, 0);
}

#include <Ice/Ice.h>
#include <api_interface.h>

class SrvImpl : public api::idl::Srv {
public:

    virtual void Call_async(const ::api::idl::AMD_Srv_CallPtr& cb,
                            const std::string& rqstid,
                            const std::string& method,
                            const std::string& rqst,
                            const ::Ice::Current& = ::Ice::Current()) override {
        cb->ice_response(true, "{}");
    }
};

class IceServer : public Ice::Application {
public:

    virtual int run(int argc, char* argv[]) {

        std::string cfg_file = "config.json";
        if(argc > 1) {
            cfg_file = argv[1];
        }
        Config& config = Config::instance();
        bool res = config.load(cfg_file);
        if(!res) {
            std::cerr<<"load config failed "<<cfg_file<<std::endl;
            return EXIT_FAILURE;
        }

        //const std::string logfile = config.getLogFile();
        //bool debug = config.getLogLevel();
        //logging_init(logfile, debug);
    
        // statsd
        //bool statsd_enable = config.getStatsdEnable();
        //statsd::enable(statsd_enable);
        //if(statsd_enable) {
        //    std::string statsd_addr = config.getStatsdAddr();
        //    int32_t statsd_port = config.getStatsdPort();
        //    std::string statsd_namespace = config.getStatsdNamespace();
        //    statsd::set(statsd_addr, statsd_port, statsd_namespace);
        //}
        //// 统一日志
        //int log_level = config.getXLogLevel();
        //std::string service_name = config.getXLogName();
        //bool xlog_enabled = config.getXLogEnabled();
        //std::string xlog_addr = config.getXLogAddr();
        //int xlog_port = config.getXLogPort();
        //XLOGGING::XLOG_INIT(service_name, log_level, false, xlog_enabled, xlog_addr, xlog_port, false);

        //int succ = Api::Init();
        //if(succ != 0) {
        //    LOG_ERROR<<"init failed";
        //    return 3;
        //}

        //int32_t max_timeout = config.getWorkTaskMaxTimeout();
        //int32_t wait = config.getWorkQueueThreadSleepTimeMs();
        //int32_t work_queue_threads = config.getWorkQueueThreads();
        //work_queue_threads = _MAX_(1, _MIN_(work_queue_threads, 1000));
        //WorkQueuePtr _workQueues[1000];
        //for(int i=0; i<work_queue_threads; i++) {
        //    _workQueues[i] = new WorkQueue(i, wait, max_timeout);
        //    _workQueues[i]->start();
        //}
   
        std::thread http_thrd(run_http_service);
        http_thrd.detach();

        //LOG_INFO<<"system start";
        Ice::PropertiesPtr properties = communicator()->getProperties();
        Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter(
                                            properties->getProperty("AdapterIdentity"));
        adapter->add(new SrvImpl, communicator()->stringToIdentity(
                                            properties->getProperty("ServiceIdentity")));
        adapter->activate();
        communicator()->waitForShutdown();


        //Api::Destroy();

        //for(int i=0; i<work_queue_threads; i++) {
        //    _workQueues[i]->destroy();
        //    _workQueues[i]->getThreadControl().join();
        //}
 
        //LOG_INFO<<"system exit";

        return EXIT_SUCCESS;
    }

private:
};


int main(int argc, char ** argv) {
    IceServer application;
    _exit(application.main(argc, argv));
}

