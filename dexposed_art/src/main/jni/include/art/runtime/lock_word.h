/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ART_RUNTIME_LOCK_WORD_H_
#define ART_RUNTIME_LOCK_WORD_H_

#include <iosfwd>
#include <stdint.h>

#include "base/logging.h"
#include "utils.h"

namespace art {
namespace mirror {
  class Object;
}  // namespace mirror

class Monitor;

/* The lock value itself as stored in mirror::Object::monitor_.  The two most significant bits of
 * the state. The three possible states are fat locked, thin/unlocked, and hash code.
 * When the lock word is in the "thin" state and its bits are formatted as follows:
 *
 *  |33|22222222221111|1111110000000000|
 *  |10|98765432109876|5432109876543210|
 *  |00| lock count   |thread id owner |
 *
 * When the lock word is in the "fat" state and its bits are formatted as follows:
 *
 *  |33|222222222211111111110000000000|
 *  |10|987654321098765432109876543210|
 *  |01| MonitorId                    |
 *
 * When the lock word is in hash state and its bits are formatted as follows:
 *
 *  |33|222222222211111111110000000000|
 *  |10|987654321098765432109876543210|
 *  |10| HashCode                     |
 */
class LockWord {
 public:
  enum {
    // Number of bits to encode the state, currently just fat or thin/unlocked or hash code.
    kStateSize = 2,
    // Number of bits to encode the thin lock owner.
    kThinLockOwnerSize = 16,
    // Remaining bits are the recursive lock count.
    kThinLockCountSize = 32 - kThinLockOwnerSize - kStateSize,
    // Thin lock bits. Owner in lowest bits.

    kThinLockOwnerShift = 0,
    kThinLockOwnerMask = (1 << kThinLockOwnerSize) - 1,
    // Count in higher bits.
    kThinLockCountShift = kThinLockOwnerSize + kThinLockOwnerShift,
    kThinLockCountMask = (1 << kThinLockCountSize) - 1,
    kThinLockMaxCount = kThinLockCountMask,

    // State in the highest bits.
    kStateShift = kThinLockCountSize + kThinLockCountShift,
    kStateMask = (1 << kStateSize) - 1,
    kStateThinOrUnlocked = 0,
    kStateFat = 1,
    kStateHash = 2,
    kStateForwardingAddress = 3,

    // When the state is kHashCode, the non-state bits hold the hashcode.
    kHashShift = 0,
    kHashSize = 32 - kStateSize,
    kHashMask = (1 << kHashSize) - 1,
  };

  static LockWord FromThinLockId(uint32_t thread_id, uint32_t count) {
    CHECK_LE(thread_id, static_cast<uint32_t>(kThinLockOwnerMask));
    return LockWord((thread_id << kThinLockOwnerShift) | (count << kThinLockCountShift) |
                     (kStateThinOrUnlocked << kStateShift));
  }

  static LockWord FromForwardingAddress(size_t target) {
    DCHECK(IsAligned < 1 << kStateSize>(target));
    return LockWord((target >> kStateSize) | (kStateForwardingAddress << kStateShift));
  }

  static LockWord FromHashCode(uint32_t hash_code) {
    CHECK_LE(hash_code, static_cast<uint32_t>(kHashMask));
    return LockWord((hash_code << kHashShift) | (kStateHash << kStateShift));
  }

  enum LockState {
    kUnlocked,    // No lock owners.
    kThinLocked,  // Single uncontended owner.
    kFatLocked,   // See associated monitor.
    kHashCode,    // Lock word contains an identity hash.
    kForwardingAddress,  // Lock word contains the forwarding address of an object.
  };

  LockState GetState() const {
    if (UNLIKELY(value_ == 0)) {
      return kUnlocked;
    } else {
      uint32_t internal_state = (value_ >> kStateShift) & kStateMask;
      switch (internal_state) {
        case kStateThinOrUnlocked:
          return kThinLocked;
        case kStateHash:
          return kHashCode;
        case kStateForwardingAddress:
          return kForwardingAddress;
        default:
          DCHECK_EQ(internal_state, static_cast<uint32_t>(kStateFat));
          return kFatLocked;
      }
    }
  }

  // Return the owner thin lock thread id.
  uint32_t ThinLockOwner() const;

  // Return the number of times a lock value has been locked.
  uint32_t ThinLockCount() const;

  // Return the Monitor encoded in a fat lock.
  Monitor* FatLockMonitor() const;

  // Return the forwarding address stored in the monitor.
  size_t ForwardingAddress() const;

  // Default constructor with no lock ownership.
  LockWord();

  // Constructor a lock word for inflation to use a Monitor.
  explicit LockWord(Monitor* mon);

  bool operator==(const LockWord& rhs) const {
    return GetValue() == rhs.GetValue();
  }

  // Return the hash code stored in the lock word, must be kHashCode state.
  int32_t GetHashCode() const;

  uint32_t GetValue() const {
    return value_;
  }

 private:
  explicit LockWord(uint32_t val) : value_(val) {}

  // Only Object should be converting LockWords to/from uints.
  friend class mirror::Object;

  // The encoded value holding all the state.
  uint32_t value_;
};
std::ostream& operator<<(std::ostream& os, const LockWord::LockState& code);

}  // namespace art


#endif  // ART_RUNTIME_LOCK_WORD_H_
