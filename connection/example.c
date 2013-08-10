#include "redis-connection.h"

#include <unistd.h>

#include <stdlib.h>

static inline
double gettime(void)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL))
        return 0;
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

static
int connection_setup(redisConnection *connection,
                     redisContext *context, char *password)
{
    redisReply *reply = redisCommand(context, "AUTH %s", password);

    if (!reply)
        return -1;

    if (reply->type != REDIS_REPLY_STATUS) {
        redis_connection_set_error(connection, "AUTH failed");
        freeReplyObject(reply);
        return -1;
    }

    freeReplyObject(reply);
    return 0;
}


int main()
{
    redisConnection *connection = redis_connection_new("localhost", 6379);
    redisContext *context;

    connection->setup = (redis_connection_setup_t) connection_setup;
    connection->data = (void *) "foo";
    connection->connect_timeout = 0.001;
    connection->operation_timeout = 0.001;
    connection->keepalive_interval = 3;
    connection->fail_timeout = 10;
    connection->max_fails = 5;

    while (1) {
        double t0, t1;
        redisReply *reply;

        sleep(1);
        context = redis_connection_get(connection, gettime());
        if (!context) {
            fprintf(stderr, "redis_connection_get() failed, error = %s\n",
                    connection->error);
            fprintf(stderr,
                    "connection->failures = %u, "
                    "connection->first_failure = %.3lf\n",
                    connection->failures, connection->first_failure);
            continue;
        }

        t0 = gettime();
        reply = redisCommand(context, "INCR foo");
        t1 = gettime();

        if (!reply) {
            fprintf(stderr, "redisCommand() failed, dt = %.6lf\n", t1 - t0);
            continue;
        }

        if (reply->type != REDIS_REPLY_INTEGER) {
            freeReplyObject(reply);
            continue;
        }

        printf("foo = %lld, dt=%.6lf\n", reply->integer, t1 - t0);
        freeReplyObject(reply);

        redis_connection_mark_alive(connection, gettime());
    }

    redis_connection_disconnect(connection);
    return 0;
}
