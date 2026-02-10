//
// Created by Josemar Carvalho on 10/02/26.
//

#ifndef SHARED_LIBS_NONCECACHE_H
#define SHARED_LIBS_NONCECACHE_H

#pragma once
#include <Arduino.h>

/**
 * @file NonceCache.h
 * @brief Small in-memory replay cache for request nonces.
 *
 * The gateway uses this to reject replayed requests within a TTL window.
 */

/**
 * @brief Nonce cache entry.
 */
struct NonceEntry {
    uint32_t ts;  ///< Stored timestamp in seconds.
    String nonce; ///< Stored nonce string.
};

/**
 * @brief Fixed-capacity nonce cache with TTL eviction.
 *
 * This class is designed for constrained devices. It keeps up to @p cap entries.
 * Entries older than @p ttlSec are removed during cache operations.
 */
class NonceCache {
public:
    /**
     * @brief Construct a nonce cache.
     *
     * @param cap Maximum number of entries kept.
     * @param ttlSec Time-to-live (seconds) for each entry.
     */
    NonceCache(size_t cap, uint32_t ttlSec);

    /**
     * @brief Destroy cache and release memory.
     */
    ~NonceCache();

    /**
     * @brief Check whether a nonce has been seen recently (within TTL).
     *
     * @param now Current time in seconds.
     * @param nonce Nonce to check.
     * @return true if the nonce is present; false otherwise.
     */
    bool seenRecently(uint32_t now, const String& nonce);

    /**
     * @brief Store a nonce as seen at time @p now.
     *
     * If the cache is full, the oldest entry is replaced.
     *
     * @param now Current time in seconds.
     * @param nonce Nonce to store.
     */
    void remember(uint32_t now, const String& nonce);

private:
    /**
     * @brief Evict expired entries.
     *
     * @param now Current time in seconds.
     */
    void cleanup(uint32_t now);

    size_t _cap;
    uint32_t _ttl;
    NonceEntry* _entries;
};

#endif //SHARED_LIBS_NONCECACHE_H