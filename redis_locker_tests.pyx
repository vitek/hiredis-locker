from cpython.string cimport PyString_FromStringAndSize

cdef extern from "redis-locker.h":
    enum:
        REDIS_REPLY_STRING
        REDIS_REPLY_NIL

    ctypedef struct redisContext:
        int err
        char *errstr

    ctypedef struct redisReply:
        int type
        char *errstr
        char *str
        int len

    ctypedef struct redisReply:
         pass

    ctypedef struct redisLock:
         int state
         int is_locked

    redisContext *redisConnect(char *, int)
    void redisFree(redisContext *)
    void freeReplyObject(redisReply *)

    int redis_lock_init_context(redisContext *context, redisReply **reply_error)

    int redis_lock_init(redisLock *lock,
                        char *lock_key, ssize_t lock_key_len)

    int redis_lock_set_data(redisLock *lock, char *data_key,
                            ssize_t data_key_len)

    int redis_lock_set_time(redisLock *lock, double timestamp,
                            double timeout)

    int redis_lock_acquire(redisLock *lock, redisContext *context)
    int redis_lock_acquire_data(redisLock *lock, redisContext *context,
                            redisReply **data_reply)
    int redis_lock_release(redisLock *lock, redisContext *context)
    int redis_lock_release_data(redisLock *lock, redisContext *context,
                                char *data, size_t data_len)
    int redis_lock_release_data_delete(redisLock *lock,
                                       redisContext *context)


class LockerError(Exception):
      pass


cdef class RedisContext(object):
     cdef redisContext *context

     def __cinit__(self, hostname, port):
         cdef redisReply *reply_error

         self.context = redisConnect(hostname, port)

         if self.context == NULL:
             raise LockerError, "redisConnect() failed"

         if self.context.err:
             raise LockerError, "redisConnext(): %s" % self.context.errstr

         if redis_lock_init_context(self.context, &reply_error):
            raise LockerError, "redis_lock_init_context() failed"

     def __dealloc__(self):
          if self.context != NULL:
             redisFree(self.context)

     def lock(self, key):
         return RedisLocker(self, key)


class AlreadyLocked(LockerError):
    pass


class StolenLock(LockerError):
    pass


cdef class RedisLocker(object):
      cdef redisLock lock
      cdef RedisContext context

      def __cinit__(self, RedisContext context, key):
          redis_lock_init(&self.lock, key, len(key))
          self.context = context

      def set_data_key(self, key):
          redis_lock_set_data(&self.lock, key, len(key))

      def set_time(self, float timestamp, float timeout):
          redis_lock_set_time(&self.lock, timestamp, timeout)

      def acquire(self):
          ret = redis_lock_acquire(&self.lock, self.context.context)
          if ret < 0:
              raise LockerError
          if ret == 0:
              raise AlreadyLocked

      def acquire_get(self):
          cdef redisReply *reply
          ret = redis_lock_acquire_data(&self.lock, self.context.context, &reply)
          if ret < 0:
              raise LockerError
          if ret == 0:
              raise AlreadyLocked
          if reply.type == REDIS_REPLY_NIL:
              data = None
          elif reply.type == REDIS_REPLY_STRING:
              data = PyString_FromStringAndSize(reply.str, reply.len)
          freeReplyObject(reply)
          return data

      def release(self):
          ret = redis_lock_release(&self.lock, self.context.context)
          if ret == 0:
              raise StolenLock

      def release_write_back(self, data):
          ret = redis_lock_release_data(&self.lock, self.context.context,
                                        <char *>data, len(data))
          if ret == 0:
              raise StolenLock

      def release_delete(self):
          ret = redis_lock_release_data_delete(&self.lock, self.context.context)
          if ret == 0:
              raise StolenLock

      @property
      def state(self):
          return self.lock.state

      @property
      def is_locked(self):
          return bool(self.lock.is_locked)
