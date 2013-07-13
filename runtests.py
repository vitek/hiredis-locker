import unittest

import redis

from redis_locker_tests import (
    RedisContext, RedisLocker, AlreadyLocked, StolenLock)


REDIS_HOSTNAME = 'localhost'
REDIS_PORT = 6379


LOCK_KEY_NAME = 'test_redis_locker_lock'
DATA_KEY_NAME = 'test_redis_locker_data'

class TestCaseMixIn(object):
    def setUp(self):
        self.redis = redis.Redis(REDIS_HOSTNAME, REDIS_PORT)
        self.locker = RedisContext(REDIS_HOSTNAME, REDIS_PORT)
        self.redis.delete(LOCK_KEY_NAME)
        self.redis.delete(DATA_KEY_NAME)


class TestLockerNoData(TestCaseMixIn, unittest.TestCase):
    def test_simple_locking(self):
        # No time specified
        lock = self.locker.lock(LOCK_KEY_NAME)
        lock.acquire()
        self.assertEqual(lock.state, 1)
        self.assertTrue(lock.is_locked)
        self.assertEqual(self.redis.get(LOCK_KEY_NAME), "1")
        lock.release()
        self.assertFalse(lock.is_locked)
        self.assertEqual(self.redis.get(LOCK_KEY_NAME), None)

    def test_busy(self):
        lock1 = self.locker.lock(LOCK_KEY_NAME)
        lock1.acquire()

        lock2 = self.locker.lock(LOCK_KEY_NAME)
        with self.assertRaises(AlreadyLocked):
            lock2.acquire()

        lock1.release()
        lock2.acquire()

    def test_stolen(self):
        lock = self.locker.lock(LOCK_KEY_NAME)
        lock.acquire()
        self.redis.set(LOCK_KEY_NAME, 'stolen')
        with self.assertRaises(StolenLock):
            lock.release()

    def test_timedout(self):
        self.redis.set(LOCK_KEY_NAME, "100")
        lock = self.locker.lock(LOCK_KEY_NAME)
        lock.set_time(200, 1)
        lock.acquire()
        self.assertEqual(lock.state, 2)


class TestLockerData(TestCaseMixIn, unittest.TestCase):
    def test_data_get_non_existing(self):
        lock = self.locker.lock(LOCK_KEY_NAME)
        lock.set_data_key(DATA_KEY_NAME)
        data = lock.acquire_get()
        self.assertEqual(data, None)
        self.assertTrue(lock.is_locked)
        self.assertEqual(lock.state, 1)
        lock.release()
        self.assertFalse(lock.is_locked)

    def test_data_get(self):
        self.redis.set(DATA_KEY_NAME, "I am here")
        lock = self.locker.lock(LOCK_KEY_NAME)
        lock.set_data_key(DATA_KEY_NAME)
        data = lock.acquire_get()
        self.assertEqual(data, "I am here")
        self.assertTrue(lock.is_locked)
        self.assertEqual(lock.state, 1)
        lock.release()
        self.assertFalse(lock.is_locked)

    def test_data_release_write_back(self):
        lock = self.locker.lock(LOCK_KEY_NAME)
        lock.set_data_key(DATA_KEY_NAME)
        lock.acquire()
        lock.release_write_back('hello, world')
        self.assertFalse(lock.is_locked)
        data = self.redis.get(DATA_KEY_NAME)
        self.assertEqual(data, "hello, world")

    def test_data_release_write_delete(self):
        self.redis.set(DATA_KEY_NAME, 'foo')
        lock = self.locker.lock(LOCK_KEY_NAME)
        lock.set_data_key(DATA_KEY_NAME)
        lock.acquire()
        lock.release_delete()
        self.assertFalse(lock.is_locked)
        data = self.redis.get(DATA_KEY_NAME)
        self.assertEqual(data, None)

    def test_data_release_write_back_expired(self):
        lock = self.locker.lock(LOCK_KEY_NAME)
        lock.set_data_key(DATA_KEY_NAME)
        lock.acquire()
        self.redis.set(LOCK_KEY_NAME, 'expired')
        with self.assertRaises(StolenLock):
            lock.release_write_back('hello, world')
        data = self.redis.get(DATA_KEY_NAME)
        self.assertEqual(data, None)

    def test_data_release_delete_expired(self):
        self.redis.set(DATA_KEY_NAME, 'foo')
        lock = self.locker.lock(LOCK_KEY_NAME)
        lock.set_data_key(DATA_KEY_NAME)
        lock.acquire()
        self.redis.set(LOCK_KEY_NAME, 'expired')
        with self.assertRaises(StolenLock):
            lock.release_delete()
        data = self.redis.get(DATA_KEY_NAME)
        self.assertEqual(data, "foo")

if __name__ == "__main__":
    unittest.main()
