#include <hiredis/hiredis.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "redis-locker.h"
#include "bench_common.h"

int main(int argc, char **argv)
{
    redisContext *c = redisConnect("localhost", 6379);
    redisReply *reply;
    double t0, t1;
    int i;
    redisLock lock;

    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }


    if (redis_lock_init_context(c, &reply)) {
        fprintf(stderr, "redis_locker_init() failed: error=%s\n",
                reply? reply->str: "unknown");
        if (reply)
            freeReplyObject(reply);
        redisFree(c);
        return -1;
    }


    if (redis_lock_init(&lock, "lock", -1)) {
        printf("redis_lock_init(): failed\n");
        exit(1);
    }

    reply = redisCommand(c, "DEL lock");
    check_reply(reply);
    freeReplyObject(reply);

    reply = redisCommand(c, "DEL counter");
    check_reply(reply);
    freeReplyObject(reply);

    t0 = gettime();

    redis_lock_set_data(&lock, "counter", -1);
    redis_lock_set_time(&lock, gettime(), .1);

    for (i = 0; i < TIMES; i++) {
        redisReply *data_reply;
        int ret;
        long counter = 0;
        char buf[100];
        int len;

        ret = redis_lock_acquire_data(&lock, c, &data_reply);

        if (ret < 0)
            break;

        if (ret == REDIS_LOCK_STATE_BUSY) {
            fprintf(stderr, "oops, lock is busy\n");
        }

        if (data_reply->str)
            counter = atoi(data_reply->str);
        freeReplyObject(data_reply);

        len = snprintf(buf, sizeof(buf), "%ld", counter + 1);

        ret = redis_lock_release_data(&lock, c, buf, len);
        if (ret != 1)
            fprintf(stderr, "unlock script returned %d\n", ret);
    }
    t1 = gettime();

    printf("%s: %.3lf locks/sec\n", argv[0], TIMES / (t1 - t0));
    return 0;
}

