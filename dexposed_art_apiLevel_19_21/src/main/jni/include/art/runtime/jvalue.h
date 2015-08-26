/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ART_RUNTIME_JVALUE_H_
#define ART_RUNTIME_JVALUE_H_

#include "base/macros.h"

#include <stdint.h>

namespace art {
namespace mirror {
class Object;
}  // namespace mirror

union PACKED(4) JValue {
  // We default initialize JValue instances to all-zeros.
  JValue() : j(0) {}

  int8_t GetB() const { return b; }
  void SetB(int8_t new_b) {
    i = ((static_cast<int32_t>(new_b) << 24) >> 24);  // Sign-extend.
  }

  uint16_t GetC() const { return c; }
  void SetC(uint16_t new_c) { c = new_c; }

  double GetD() const { return d; }
  void SetD(double new_d) { d = new_d; }

  float GetF() const { return f; }
  void SetF(float new_f) { f = new_f; }

  int32_t GetI() const { return i; }
  void SetI(int32_t new_i) { i = new_i; }

  int64_t GetJ() const { return j; }
  void SetJ(int64_t new_j) { j = new_j; }

  mirror::Object* GetL() const { return l; }
  void SetL(mirror::Object* new_l) { l = new_l; }

  int16_t GetS() const { return s; }
  void SetS(int16_t new_s) {
    i = ((static_cast<int32_t>(new_s) << 16) >> 16);  // Sign-extend.
  }

  uint8_t GetZ() const { return z; }
  void SetZ(uint8_t new_z) { z = new_z; }

 private:
  uint8_t z;
  int8_t b;
  uint16_t c;
  int16_t s;
  int32_t i;
  int64_t j;
  float f;
  double d;
  mirror::Object* l;
};

}  // namespace art

#endif  // ART_RUNTIME_JVALUE_H_
