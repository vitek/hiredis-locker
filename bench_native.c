#include <hiredis/hiredis.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench_common.h"

int main(int argc, char **argv)
{
    redisContext *c = redisConnect("localhost", 6379);
    redisReply *reply;
    long counter = 0;
    double t0, t1;
    int i;

    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    reply = redisCommand(c, "DEL lock");
    check_reply(reply);
    freeReplyObject(reply);

    reply = redisCommand(c, "DEL counter");
    check_reply(reply);
    freeReplyObject(reply);

    t0 = gettime();
    /* simulate locking with native redis commands */
    for (i = 0; i < TIMES; i++) {
        reply = redisCommand(c, "SETNX lock 0");
        check_reply(reply);
        freeReplyObject(reply);

        reply = redisCommand(c, "GET counter");
        check_reply(reply);
        if (reply->str)
            counter = atol(reply->str);
        freeReplyObject(reply);

        reply = redisCommand(c, "SET counter %d", counter + 1);
        check_reply(reply);
        freeReplyObject(reply);

        reply = redisCommand(c, "GET lock");
        check_reply(reply);
        freeReplyObject(reply);

        reply = redisCommand(c, "DEL lock");
        check_reply(reply);
        freeReplyObject(reply);
    }
    t1 = gettime();

    printf("%s: %.3lf locks/sec\n", argv[0], TIMES / (t1 - t0));
    return 0;
}
