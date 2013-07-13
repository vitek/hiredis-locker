/* Don't edit this file it was automatically generated */
const char *script_lua_lock =
    "-- KEYS = {key_lock, [key_data]}\n"
    "-- ARGS = {timestamp, expire_at}\n"
    "if redis.call('SETNX', KEYS[1], ARGV[2]) == 1 then\n"
    "   -- ok\n"
    "   return {1, KEYS[2] and redis.call('GET', KEYS[2]) or false}\n"
    "else\n"
    "   local timestamp = tonumber(ARGV[1])\n"
    "   local lock_expires_at = tonumber(redis.call('GET', KEYS[1]) or 0)\n"
    "\n"
    "   if lock_expires_at < timestamp then\n"
    "      redis.call('SET', KEYS[1], ARGV[2])\n"
    "      -- expired\n"
    "      return {2, KEYS[2] and redis.call('GET', KEYS[2]) or false}\n"
    "   end\n"
    "end\n"
    "-- locked\n"
    "return {0, false}\n"
;
const char *script_lua_lock_sha1 = "e2a08ec9080b1a80320a445875a1a3165f0a6627";
const char *script_lua_unlock =
    "-- KEYS = {key_lock, [key_data]}\n"
    "-- ARGS = {expire_at, [data]}\n"
    "if redis.call('GET', KEYS[1]) ~= ARGV[1] then\n"
    "   return 0\n"
    "end\n"
    "if KEYS[2] then\n"
    "   if ARGV[2] then\n"
    "      redis.call('SET', KEYS[2], ARGV[2])\n"
    "   else\n"
    "      redis.call('DEL', KEYS[2])\n"
    "   end\n"
    "end\n"
    "redis.call('DEL', KEYS[1])\n"
    "return 1\n"
;
const char *script_lua_unlock_sha1 = "e5668bd1e8ea06d12822b0ff31386640fae4eafe";
