/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef ART_RUNTIME_CLASS_REFERENCE_H_
#define ART_RUNTIME_CLASS_REFERENCE_H_

#include <stdint.h>
#include <utility>

namespace art {

class DexFile;

// A class is uniquely located by its DexFile and the class_defs_ table index into that DexFile
typedef std::pair<const DexFile*, uint32_t> ClassReference;

inline bool operator<(const ClassReference& lhs, const ClassReference& rhs) {
  if (lhs.second < rhs.second) {
    return true;
  } else if (lhs.second > rhs.second) {
    return false;
  } else {
    return (lhs.first < rhs.first);
  }
}

}  // namespace art

#endif  // ART_RUNTIME_CLASS_REFERENCE_H_
