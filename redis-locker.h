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
#ifndef REDIS_LOCKER_H
#define REDIS_LOCKER_H
#include <hiredis/hiredis.h>

#define REDIS_LOCK_STATE_BUSY     0 /** Already locked */
#define REDIS_LOCK_STATE_LOCKED   1 /** Locked */
#define REDIS_LOCK_STATE_EXPIRED  2 /** Locked an expired lock */

#ifdef __cplusplus
# define REDIS_LOCKER_CDECLS_BEGIN extern "C" {
# define REDIS_LOCKER_CDECLS_END   };
#else
# define REDIS_LOCKER_CDECLS_BEGIN
# define REDIS_LOCKER_CDECLS_END
#endif


typedef struct {
    char lock_key[100];
    size_t lock_key_len;

    char data_key[100];
    size_t data_key_len;

    char timestamp[30];
    size_t timestamp_len;
    char expire_at[30];
    size_t expire_at_len;

    redisReply *error_reply;

    int is_locked;
    int state;
} redis_lock_t;

REDIS_LOCKER_CDECLS_BEGIN

/**
 * Initialize redis lock.
 * @param lock_key
 * @param lock_key_len length of lock_key or -1
 * @return zero on success
 */
int redis_lock_init(redis_lock_t *lock,
                    const char *lock_key, ssize_t lock_key_len);
/** Destroy lock object */
void redis_lock_destroy(redis_lock_t *lock);

/** Set data key for locker */
int redis_lock_set_data(redis_lock_t *lock,
                        const char *data_key, ssize_t data_key_len);

/** Update lock's timestamp and expire at */
int redis_lock_set_time(redis_lock_t *lock, double timestamp, double timeout);


int redis_lock_acquire(redis_lock_t *lock, redisContext *context);
int redis_lock_acquire_data(redis_lock_t *lock, redisContext *context,
                            redisReply **data_reply);

int redis_lock_release(redis_lock_t *lock, redisContext *context);
int redis_lock_release_data(redis_lock_t *lock, redisContext *context,
                            const char *data, size_t data_len);
int redis_lock_release_data_delete(redis_lock_t *lock, redisContext *context);


/**
 * Load lua scripts into redis.
 * @returns zero on success
 **/
int redis_lock_init_context(redisContext *context, redisReply **reply_error);


/**
 * Try to lock redis key without retrieving data.
 *
 * @return redis reply as returned by lock script
 **/
redisReply *redis_lock_script(
    redisContext *c,
    const char *lock_key, size_t lock_key_len,
    const char *timestamp, size_t timestamp_len,
    const char *expire_at, size_t expire_at_len);

/**
 * Try to lock redis key retrievning data.
 *
 * @return redis reply as returned by lock script
 **/
redisReply *redis_lock_script_data(
    redisContext *c,
    const char *lock_key, size_t lock_key_len,
    const char *data_key, size_t data_key_len,
    const char *timestamp, size_t timestamp_len,
    const char *expire_at, size_t expire_at_len);

/**
 * Unlock redis key without without data modification.
 *
 * @return redis reply as returned by unlock script
 **/
redisReply *redis_unlock_script(
    redisContext *c,
    const char *lock_key, size_t lock_key_len,
    const char *expire_at, size_t expire_at_len);

/**
 * Unlock redis key deleting data.
 *
 * @return redis reply as returned by unlock script
 **/
redisReply *redis_unlock_script_data_delete(
    redisContext *c,
    const char *lock_key, size_t lock_key_len,
    const char *data_key, size_t data_key_len,
    const char *expire_at, size_t expire_at_len);

/**
 * Unlock redis key writing data back.
 *
 * @return redis reply as returned by unlock script
 **/
redisReply *redis_unlock_script_data(
    redisContext *c,
    const char *lock_key, size_t lock_key_len,
    const char *data_key, size_t data_key_len,
    const char *expire_at, size_t expire_at_len,
    const char *data, size_t data_len);

REDIS_LOCKER_CDECLS_END
#endif /* REDIS_LOCKER_H */
