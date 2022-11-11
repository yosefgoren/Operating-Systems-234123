#ifndef __REQUEST_H__
#define __REQUEST_H__
#include <sys/time.h>

struct request_t{
    double arrival;
    double dispatch;
    int connfd;
};
typedef struct request_t request;

typedef struct ThreadData_t {
    int id;
    int num_http_handled; //the number of reuqests handled by this thread.
    int num_static_handled;
    int num_dynamic_handled;
} ThreadData;

void requestHandle(request req, ThreadData* thread_data);

#endif
