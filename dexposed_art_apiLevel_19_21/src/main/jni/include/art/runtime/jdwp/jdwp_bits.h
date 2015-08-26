/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_RUNTIME_JDWP_JDWP_BITS_H_
#define ART_RUNTIME_JDWP_JDWP_BITS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

namespace art {

namespace JDWP {

static inline uint32_t Get4BE(unsigned char const* pSrc) {
  return (pSrc[0] << 24) | (pSrc[1] << 16) | (pSrc[2] << 8) | pSrc[3];
}

static inline void Append1BE(std::vector<uint8_t>& bytes, uint8_t value) {
  bytes.push_back(value);
}

static inline void Append2BE(std::vector<uint8_t>& bytes, uint16_t value) {
  bytes.push_back(static_cast<uint8_t>(value >> 8));
  bytes.push_back(static_cast<uint8_t>(value));
}

static inline void Append4BE(std::vector<uint8_t>& bytes, uint32_t value) {
  bytes.push_back(static_cast<uint8_t>(value >> 24));
  bytes.push_back(static_cast<uint8_t>(value >> 16));
  bytes.push_back(static_cast<uint8_t>(value >> 8));
  bytes.push_back(static_cast<uint8_t>(value));
}

static inline void Append8BE(std::vector<uint8_t>& bytes, uint64_t value) {
  bytes.push_back(static_cast<uint8_t>(value >> 56));
  bytes.push_back(static_cast<uint8_t>(value >> 48));
  bytes.push_back(static_cast<uint8_t>(value >> 40));
  bytes.push_back(static_cast<uint8_t>(value >> 32));
  bytes.push_back(static_cast<uint8_t>(value >> 24));
  bytes.push_back(static_cast<uint8_t>(value >> 16));
  bytes.push_back(static_cast<uint8_t>(value >> 8));
  bytes.push_back(static_cast<uint8_t>(value));
}

static inline void AppendUtf16BE(std::vector<uint8_t>& bytes, const uint16_t* chars, size_t char_count) {
  Append4BE(bytes, char_count);
  for (size_t i = 0; i < char_count; ++i) {
    Append2BE(bytes, chars[i]);
  }
}

// @deprecated
static inline void Set1(uint8_t* buf, uint8_t val) {
  *buf = (uint8_t)(val);
}

// @deprecated
static inline void Set2BE(uint8_t* buf, uint16_t val) {
  *buf++ = (uint8_t)(val >> 8);
  *buf = (uint8_t)(val);
}

// @deprecated
static inline void Set4BE(uint8_t* buf, uint32_t val) {
  *buf++ = (uint8_t)(val >> 24);
  *buf++ = (uint8_t)(val >> 16);
  *buf++ = (uint8_t)(val >> 8);
  *buf = (uint8_t)(val);
}

// @deprecated
static inline void Set8BE(uint8_t* buf, uint64_t val) {
  *buf++ = (uint8_t)(val >> 56);
  *buf++ = (uint8_t)(val >> 48);
  *buf++ = (uint8_t)(val >> 40);
  *buf++ = (uint8_t)(val >> 32);
  *buf++ = (uint8_t)(val >> 24);
  *buf++ = (uint8_t)(val >> 16);
  *buf++ = (uint8_t)(val >> 8);
  *buf = (uint8_t)(val);
}

static inline void Write1BE(uint8_t** dst, uint8_t value) {
  Set1(*dst, value);
  *dst += sizeof(value);
}

static inline void Write2BE(uint8_t** dst, uint16_t value) {
  Set2BE(*dst, value);
  *dst += sizeof(value);
}

static inline void Write4BE(uint8_t** dst, uint32_t value) {
  Set4BE(*dst, value);
  *dst += sizeof(value);
}

static inline void Write8BE(uint8_t** dst, uint64_t value) {
  Set8BE(*dst, value);
  *dst += sizeof(value);
}

}  // namespace JDWP

}  // namespace art

#endif  // ART_RUNTIME_JDWP_JDWP_BITS_H_
