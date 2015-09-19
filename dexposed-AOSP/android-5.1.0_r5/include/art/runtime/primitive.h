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

#ifndef ART_RUNTIME_PRIMITIVE_H_
#define ART_RUNTIME_PRIMITIVE_H_

#include <sys/types.h>

#include "base/logging.h"
#include "base/macros.h"
#include "mirror/object_reference.h"

namespace art {
namespace mirror {
class Object;
}  // namespace mirror

class Primitive {
 public:
  enum Type {
    kPrimNot = 0,
    kPrimBoolean,
    kPrimByte,
    kPrimChar,
    kPrimShort,
    kPrimInt,
    kPrimLong,
    kPrimFloat,
    kPrimDouble,
    kPrimVoid,
  };

  static Type GetType(char type) {
    switch (type) {
      case 'B':
        return kPrimByte;
      case 'C':
        return kPrimChar;
      case 'D':
        return kPrimDouble;
      case 'F':
        return kPrimFloat;
      case 'I':
        return kPrimInt;
      case 'J':
        return kPrimLong;
      case 'S':
        return kPrimShort;
      case 'Z':
        return kPrimBoolean;
      case 'V':
        return kPrimVoid;
      default:
        return kPrimNot;
    }
  }

  static size_t ComponentSize(Type type) {
    switch (type) {
      case kPrimVoid:    return 0;
      case kPrimBoolean:
      case kPrimByte:    return 1;
      case kPrimChar:
      case kPrimShort:   return 2;
      case kPrimInt:
      case kPrimFloat:   return 4;
      case kPrimLong:
      case kPrimDouble:  return 8;
      case kPrimNot:     return sizeof(mirror::HeapReference<mirror::Object>);
      default:
        LOG(FATAL) << "Invalid type " << static_cast<int>(type);
        return 0;
    }
  }

  static size_t FieldSize(Type type) {
    return ComponentSize(type) <= 4 ? 4 : 8;
  }

  static const char* Descriptor(Type type) {
    switch (type) {
      case kPrimBoolean:
        return "Z";
      case kPrimByte:
        return "B";
      case kPrimChar:
        return "C";
      case kPrimShort:
        return "S";
      case kPrimInt:
        return "I";
      case kPrimFloat:
        return "F";
      case kPrimLong:
        return "J";
      case kPrimDouble:
        return "D";
      case kPrimVoid:
        return "V";
      default:
        LOG(FATAL) << "Primitive char conversion on invalid type " << static_cast<int>(type);
        return NULL;
    }
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Primitive);
};

std::ostream& operator<<(std::ostream& os, const Primitive::Type& state);

}  // namespace art

#endif  // ART_RUNTIME_PRIMITIVE_H_
