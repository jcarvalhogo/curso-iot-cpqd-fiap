#include "HexUtils.h"

String hexEncode(const uint8_t* data, size_t len) {
  static const char* hex = "0123456789abcdef";
  String out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    out += hex[(data[i] >> 4) & 0xF];
    out += hex[data[i] & 0xF];
  }
  return out;
}

bool hexDecodeFixed(const String& hexStr, uint8_t* out, size_t outLen) {
  if (hexStr.length() != (int)(outLen * 2)) return false;
  for (size_t i = 0; i < outLen; i++) {
    unsigned int v = 0;
    if (sscanf(hexStr.substring(i * 2, i * 2 + 2).c_str(), "%02x", &v) != 1) return false;
    out[i] = (uint8_t)v;
  }
  return true;
}

bool isHexStringEven(const String& s) {
  if ((s.length() % 2) != 0) return false;
  for (size_t i = 0; i < (size_t)s.length(); i++) {
    char c = s[i];
    bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    if (!ok) return false;
  }
  return true;
}
