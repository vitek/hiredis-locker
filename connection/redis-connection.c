/*
 * Copyright (c) 2013, Vitja Makarov <vitja.makarov@gmail.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "redis-connection.h"

#include <stdlib.h>
#include <string.h>

static
void double_to_timeval(struct timeval *tv, double time)
{
    tv->tv_sec = time;
    tv->tv_usec = (time - (int) time) * 1000000;
}

redisConnection *redis_connection_new(const char *hostname, int port)
{
    redisConnection *connection =
        (redisConnection *) calloc(1, sizeof(redisConnection));
    if (!connection)
        return NULL;

    connection->hostname = strdup(hostname);
    connection->port = port;

    if (!connection->hostname) {
        redis_connection_free(connection);
        return NULL;
    }

    return connection;
}

void redis_connection_free(redisConnection *connection)
{
    if (connection->context)
        redisFree(connection->context);
    if (connection->hostname)
        free(connection->hostname);
    if (connection->error)
        free(connection->error);
    free(connection);
}

void redis_connection_disconnect(redisConnection *connection)
{
    if (connection->context) {
        if (!connection->error && connection->context->err)
            redis_connection_set_error(connection, connection->context->errstr);
        redisFree(connection->context);
        connection->context = NULL;
    }
}

int redis_connection_set_error(redisConnection *connection, const char *error)
{
    if (connection->error) {
        free(connection->error);
        connection->error = NULL;
    }

    if (error)
        connection->error = strdup(error);
    return 0;
}

static
void redis_connection_clear_error(redisConnection *connection)
{
    if (connection->error) {
        free(connection->error);
        connection->error = NULL;
    }
}

int redis_connection_ping(redisConnection *connection)
{
    redisReply *reply;
    int retval = -1;

    if (!connection->context)
        return -1;

    reply = redisCommand(connection->context, "PING");

    if (!reply) {
        redis_connection_set_error(connection, "PING failed");
        redis_connection_disconnect(connection);
        return -1;
    }

    if (reply->type == REDIS_REPLY_STATUS) {
        retval = 0;
    } else {
        redis_connection_set_error(connection, "PING failed");
        redis_connection_disconnect(connection);
    }

    freeReplyObject(reply);
    return retval;
}

static int redis_connection_connect(redisConnection *connection)
{
    if (connection->context)
        return -1;

    redis_connection_clear_error(connection);

    if (connection->connect_timeout > 0) {
        struct timeval tv;
        double_to_timeval(&tv, connection->connect_timeout);
        connection->context = redisConnectWithTimeout(
            connection->hostname, connection->port, tv);
    } else {
        connection->context = redisConnect(connection->hostname,
                                           connection->port);
    }

    if (!connection->context)
        return -1;

    if (connection->context->err) {
        redis_connection_disconnect(connection);
        return -1;
    }

    if (connection->operation_timeout > 0) {
        struct timeval tv;
        double_to_timeval(&tv, connection->operation_timeout);
        redisSetTimeout(connection->context, tv);
    }

    if (connection->setup && connection->setup(connection, connection->context,
                                               connection->data)) {
        redis_connection_disconnect(connection);
        return -1;
    }

    return 0;
}

redisContext *redis_connection_get(redisConnection *connection,
                                   double current_time)
{
    if (connection->context) {
        /* broken connection */
        if (connection->context->err) {
            redis_connection_disconnect(connection);
        } else if (connection->keepalive_interval > 0 &&
                current_time > (connection->last_alive
                                + connection->keepalive_interval)) {
            if (!redis_connection_ping(connection))
                connection->last_alive = current_time;
        }
    }

    if (!connection->context) {
        if (connection->fail_timeout > 0) {
            if (current_time > (connection->first_failure +
                                connection->fail_timeout)) {
                connection->failures = 0;
            } else if (connection->failures >= connection->max_fails) {
                redis_connection_set_error(connection,
                                           "Server is down");
                return NULL;
            }
        }

        if (redis_connection_connect(connection)) {
            if (!connection->failures)
                connection->first_failure = current_time;
            connection->failures++;
            return NULL;
        }

        connection->failures = 0;
        connection->first_failure = 0;
        connection->last_alive = current_time;
        redis_connection_set_error(connection, NULL);
    }

    return connection->context;
}

int redis_connection_mark_alive(redisConnection *connection,
                                double current_time)
{
    if (!connection->context)
        return 0;

    if (connection->context->err) {
        redis_connection_disconnect(connection);
        return -1;
    }

    connection->last_alive = current_time;
    return 0;
}
