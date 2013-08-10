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
#ifndef REDIS_CONNECTION_H
#define REDIS_CONNECTION_H
#include <hiredis/hiredis.h>

#ifdef __cplusplus
# define REDIS_CONNECTION_CDECLS_BEGIN extern "C" {
# define REDIS_CONNECTION_CDECLS_END   };
#else
# define REDIS_CONNECTION_CDECLS_BEGIN
# define REDIS_CONNECTION_CDECLS_END
#endif

struct RedisConnection;

typedef int (*redis_connection_setup_t)(struct RedisConnection *connection,
                                        redisContext *context,
                                        void *data);

typedef struct RedisConnection {
    /* public */
    redis_connection_setup_t setup;
    void *data;

    double keepalive_interval;

    unsigned int max_fails;
    double fail_timeout;

    double connect_timeout;
    double operation_timeout;

    /* private */
    char *hostname;
    int port;

    char *error;
    redisContext *context;
    double last_alive;
    double first_failure;
    unsigned int failures;
} redisConnection;


REDIS_CONNECTION_CDECLS_BEGIN

redisConnection *redis_connection_new(const char *hostname, int port);
void redis_connection_free(redisConnection *connection);
void redis_connection_disconnect(redisConnection *connection);
redisContext *redis_connection_get(redisConnection *connection,
                                   double current_time);
int redis_connection_mark_alive(redisConnection *connection,
                                double current_time);
int redis_connection_ping(redisConnection *connection);

int redis_connection_set_error(redisConnection *connection,
                               const char *error);

REDIS_CONNECTION_CDECLS_END
#endif /* REDIS_CONNECTION_H */
