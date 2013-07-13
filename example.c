#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include <hiredis/hiredis.h>

#include "redis-locker.h"

static inline
double gettime(void)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL))
        return 0;
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

static
int redis_lock_acquire_data_wait(redis_lock_t *lock, redisContext *context,
                                 redisReply **data_reply,
                                 double lock_timeout,
                                 unsigned long usecs)
{
    int ret;

    while (1) {
        redis_lock_set_time(lock, gettime(), lock_timeout);
        ret = redis_lock_acquire_data(lock, context, data_reply);

        if (ret != REDIS_LOCK_STATE_BUSY)
            break;
        printf("Busy sleeping...\n");
        usleep(usecs);
    }

    return ret;
}



int main()
{
    redisContext *c = redisConnect("localhost", 6379);
    redis_lock_t lock;
    redisReply *data_reply, *reply;
    long counter;
    int ret, len;
    char buf[100];

    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    /* Upload LUA scripts */
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

    redis_lock_set_data(&lock, "counter", -1);

    ret = redis_lock_acquire_data_wait(&lock, c, &data_reply, .5, 1000);

    if (ret < 0) {
        fprintf(stderr, "Failed to lock key: %s",
                lock.error_reply? lock.error_reply->str: "unknown");
        exit(1);
    }

    if (ret == REDIS_LOCK_STATE_LOCKED)
        printf("Locked\n");
    else
        printf("Locked: expired lock found\n");

    if (data_reply->type == REDIS_REPLY_STRING)
        counter = atol(data_reply->str);
    else
        counter = 0;

    printf("counter = %ld\n", counter);

    len = snprintf(buf, sizeof(buf), "%ld", counter + 1);

    ret = redis_lock_release_data(&lock, c, buf, len);

    if (ret < 0) {
        fprintf(stderr, "Failed to unlock key: %s",
                lock.error_reply? lock.error_reply->str: "unknown");
        exit(1);
    }

    if (ret == 0)
        printf("oops lock was stolen\n");
    else
        printf("Unlocked\n");

    redis_lock_destroy(&lock);
    redisFree(c);
    return 0;
}
