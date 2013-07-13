#ifndef BENCH_COMMON_H
#define BENCH_COMMON_H
#include <hiredis/hiredis.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TIMES 100000

static inline
double gettime(void)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL))
        return 0;
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

static inline
void check_reply(redisReply *reply)
{
    if (!reply) {
        fprintf(stderr, "null reply\n");
        exit(1);
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        fprintf(stderr, "error reply: %s\n", reply->str);
        exit(1);
    }

}
#endif /* BENCH_COMMON_H */
