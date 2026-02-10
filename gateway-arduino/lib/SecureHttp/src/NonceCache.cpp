#include "NonceCache.h"

NonceCache::NonceCache(size_t cap, uint32_t ttlSec)
  : _cap(cap), _ttl(ttlSec) {
  _entries = new NonceEntry[_cap];
  for (size_t i = 0; i < _cap; i++) _entries[i] = {0, ""};
}

NonceCache::~NonceCache() {
  delete[] _entries;
}

void NonceCache::cleanup(uint32_t now) {
  for (size_t i = 0; i < _cap; i++) {
    if (_entries[i].ts != 0 && (now - _entries[i].ts) > _ttl) {
      _entries[i].ts = 0;
      _entries[i].nonce = "";
    }
  }
}

bool NonceCache::seenRecently(uint32_t now, const String& nonce) {
  cleanup(now);
  for (size_t i = 0; i < _cap; i++) {
    if (_entries[i].ts != 0 && _entries[i].nonce == nonce) return true;
  }
  return false;
}

void NonceCache::remember(uint32_t now, const String& nonce) {
  cleanup(now);

  size_t idx = 0;
  uint32_t oldest = UINT32_MAX;

  for (size_t i = 0; i < _cap; i++) {
    if (_entries[i].ts == 0) { idx = i; oldest = 0; break; }
    if (_entries[i].ts < oldest) { oldest = _entries[i].ts; idx = i; }
  }

  _entries[idx].ts = now;
  _entries[idx].nonce = nonce;
}
