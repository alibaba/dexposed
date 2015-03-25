/*
 * Original work Copyright (c) 2005-2008, The Android Open Source Project
 *  Modified work Copyright (c) 2013, rovo89 and Tungstwenty
 *  Modified work Copyright (c) 2015, Alibaba Mobile Infrastructure (Android) Team
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
namespace art {

class ArgArray {
 public:
  explicit ArgArray(const char* shorty, uint32_t shorty_len)
      : shorty_(shorty), shorty_len_(shorty_len), num_bytes_(0) {
    size_t num_slots = shorty_len + 1;  // +1 in case of receiver.
    if (LIKELY((num_slots * 2) < kSmallArgArraySize)) {
      // We can trivially use the small arg array.
      arg_array_ = small_arg_array_;
    } else {
      // Analyze shorty to see if we need the large arg array.
      for (size_t i = 1; i < shorty_len; ++i) {
        char c = shorty[i];
        if (c == 'J' || c == 'D') {
          num_slots++;
        }
      }
      if (num_slots <= kSmallArgArraySize) {
        arg_array_ = small_arg_array_;
      } else {
        large_arg_array_.reset(new uint32_t[num_slots]);
        arg_array_ = large_arg_array_.get();
      }
    }
  }

  uint32_t* GetArray() {
    return arg_array_;
  }

  uint32_t GetNumBytes() {
    return num_bytes_;
  }

  void Append(uint32_t value) {
    arg_array_[num_bytes_ / 4] = value;
    num_bytes_ += 4;
  }

  void Append(mirror::Object* obj) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    Append(StackReference<mirror::Object>::FromMirrorPtr(obj).AsVRegValue());
  }

  void AppendWide(uint64_t value) {
    // For ARM and MIPS portable, align wide values to 8 bytes (ArgArray starts at offset of 4).
#if defined(ART_USE_PORTABLE_COMPILER) && (defined(__arm__) || defined(__mips__))
    if (num_bytes_ % 8 == 0) {
      num_bytes_ += 4;
    }
#endif
    arg_array_[num_bytes_ / 4] = value;
    arg_array_[(num_bytes_ / 4) + 1] = value >> 32;
    num_bytes_ += 8;
  }

  void AppendFloat(float value) {
    jvalue jv;
    jv.f = value;
    Append(jv.i);
  }

  void AppendDouble(double value) {
    jvalue jv;
    jv.d = value;
    AppendWide(jv.j);
  }

  static void ThrowIllegalPrimitiveArgumentException(const char* expected,
                                                     const char* found_descriptor)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    ThrowIllegalArgumentException(nullptr,
        StringPrintf("Invalid primitive conversion from %s to %s", expected,
                     PrettyDescriptor(found_descriptor).c_str()).c_str());
  }

  bool BuildArgArrayFromObjectArray(const ScopedObjectAccessAlreadyRunnable& soa,
                                    mirror::Object* receiver,
                                    mirror::ObjectArray<mirror::Object>* args, MethodHelper& mh)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    const DexFile::TypeList* classes = mh.GetMethod()->GetParameterTypeList();
    // Set receiver if non-null (method is not static)
    if (receiver != nullptr) {
      Append(receiver);
    }
    for (size_t i = 1, args_offset = 0; i < shorty_len_; ++i, ++args_offset) {
      mirror::Object* arg = args->Get(args_offset);
      if (((shorty_[i] == 'L') && (arg != nullptr)) || ((arg == nullptr && shorty_[i] != 'L'))) {
        mirror::Class* dst_class =
            mh.GetClassFromTypeIdx(classes->GetTypeItem(args_offset).type_idx_);
        if (UNLIKELY(arg == nullptr || !arg->InstanceOf(dst_class))) {
          ThrowIllegalArgumentException(nullptr,
              StringPrintf("method %s argument %zd has type %s, got %s",
                  PrettyMethod(mh.GetMethod(), false).c_str(),
                  args_offset + 1,  // Humans don't count from 0.
                  PrettyDescriptor(dst_class).c_str(),
                  PrettyTypeOf(arg).c_str()).c_str());
          return false;
        }
      }

#define DO_FIRST_ARG(match_descriptor, get_fn, append) { \
          if (LIKELY(arg != nullptr && arg->GetClass<>()->DescriptorEquals(match_descriptor))) { \
            mirror::ArtField* primitive_field = arg->GetClass()->GetIFields()->Get(0); \
            append(primitive_field-> get_fn(arg));

#define DO_ARG(match_descriptor, get_fn, append) \
          } else if (LIKELY(arg != nullptr && \
                            arg->GetClass<>()->DescriptorEquals(match_descriptor))) { \
            mirror::ArtField* primitive_field = arg->GetClass()->GetIFields()->Get(0); \
            append(primitive_field-> get_fn(arg));

#define DO_FAIL(expected) \
          } else { \
            if (arg->GetClass<>()->IsPrimitive()) { \
              std::string temp; \
              ThrowIllegalPrimitiveArgumentException(expected, \
                                                     arg->GetClass<>()->GetDescriptor(&temp)); \
            } else { \
              ThrowIllegalArgumentException(nullptr, \
                  StringPrintf("method %s argument %zd has type %s, got %s", \
                      PrettyMethod(mh.GetMethod(), false).c_str(), \
                      args_offset + 1, \
                      expected, \
                      PrettyTypeOf(arg).c_str()).c_str()); \
            } \
            return false; \
          } }

      switch (shorty_[i]) {
        case 'L':
          Append(arg);
          break;
        case 'Z':
          DO_FIRST_ARG("Ljava/lang/Boolean;", GetBoolean, Append)
          DO_FAIL("boolean")
          break;
        case 'B':
          DO_FIRST_ARG("Ljava/lang/Byte;", GetByte, Append)
          DO_FAIL("byte")
          break;
        case 'C':
          DO_FIRST_ARG("Ljava/lang/Character;", GetChar, Append)
          DO_FAIL("char")
          break;
        case 'S':
          DO_FIRST_ARG("Ljava/lang/Short;", GetShort, Append)
          DO_ARG("Ljava/lang/Byte;", GetByte, Append)
          DO_FAIL("short")
          break;
        case 'I':
          DO_FIRST_ARG("Ljava/lang/Integer;", GetInt, Append)
          DO_ARG("Ljava/lang/Character;", GetChar, Append)
          DO_ARG("Ljava/lang/Short;", GetShort, Append)
          DO_ARG("Ljava/lang/Byte;", GetByte, Append)
          DO_FAIL("int")
          break;
        case 'J':
          DO_FIRST_ARG("Ljava/lang/Long;", GetLong, AppendWide)
          DO_ARG("Ljava/lang/Integer;", GetInt, AppendWide)
          DO_ARG("Ljava/lang/Character;", GetChar, AppendWide)
          DO_ARG("Ljava/lang/Short;", GetShort, AppendWide)
          DO_ARG("Ljava/lang/Byte;", GetByte, AppendWide)
          DO_FAIL("long")
          break;
        case 'F':
          DO_FIRST_ARG("Ljava/lang/Float;", GetFloat, AppendFloat)
          DO_ARG("Ljava/lang/Long;", GetLong, AppendFloat)
          DO_ARG("Ljava/lang/Integer;", GetInt, AppendFloat)
          DO_ARG("Ljava/lang/Character;", GetChar, AppendFloat)
          DO_ARG("Ljava/lang/Short;", GetShort, AppendFloat)
          DO_ARG("Ljava/lang/Byte;", GetByte, AppendFloat)
          DO_FAIL("float")
          break;
        case 'D':
          DO_FIRST_ARG("Ljava/lang/Double;", GetDouble, AppendDouble)
          DO_ARG("Ljava/lang/Float;", GetFloat, AppendDouble)
          DO_ARG("Ljava/lang/Long;", GetLong, AppendDouble)
          DO_ARG("Ljava/lang/Integer;", GetInt, AppendDouble)
          DO_ARG("Ljava/lang/Character;", GetChar, AppendDouble)
          DO_ARG("Ljava/lang/Short;", GetShort, AppendDouble)
          DO_ARG("Ljava/lang/Byte;", GetByte, AppendDouble)
          DO_FAIL("double")
          break;
#ifndef NDEBUG
        default:
          LOG(FATAL) << "Unexpected shorty character: " << shorty_[i];
#endif
      }
#undef DO_FIRST_ARG
#undef DO_ARG
#undef DO_FAIL
    }
    return true;
  }

 private:
  enum { kSmallArgArraySize = 16 };
  const char* const shorty_;
  const uint32_t shorty_len_;
  uint32_t num_bytes_;
  uint32_t* arg_array_;
  uint32_t small_arg_array_[kSmallArgArraySize];
  std::unique_ptr<uint32_t[]> large_arg_array_;
};

static void CheckMethodArguments(mirror::ArtMethod* m, uint32_t* args)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
  const DexFile::TypeList* params = m->GetParameterTypeList();
  if (params == nullptr) {
    return;  // No arguments so nothing to check.
  }
  uint32_t offset = 0;
  uint32_t num_params = params->Size();
  size_t error_count = 0;
  if (!m->IsStatic()) {
    offset = 1;
  }
  // TODO: If args contain object references, it may cause problems
  Thread* self = Thread::Current();
  StackHandleScope<1> hs(self);
  Handle<mirror::ArtMethod> h_m(hs.NewHandle(m));
  MethodHelper mh(h_m);
  for (uint32_t i = 0; i < num_params; i++) {
    uint16_t type_idx = params->GetTypeItem(i).type_idx_;
    mirror::Class* param_type = mh.GetClassFromTypeIdx(type_idx);
    if (param_type == nullptr) {
      CHECK(self->IsExceptionPending());
      LOG(ERROR) << "Internal error: unresolvable type for argument type in JNI invoke: "
          << h_m->GetTypeDescriptorFromTypeIdx(type_idx) << "\n"
          << self->GetException(nullptr)->Dump();
      self->ClearException();
      ++error_count;
    } else if (!param_type->IsPrimitive()) {
      // TODO: check primitives are in range.
      // TODO: There is a compaction bug here since GetClassFromTypeIdx can cause thread suspension,
      // this is a hard to fix problem since the args can contain Object*, we need to save and
      // restore them by using a visitor similar to the ones used in the trampoline entrypoints.
      mirror::Object* argument = reinterpret_cast<mirror::Object*>(args[i + offset]);
      if (argument != nullptr && !argument->InstanceOf(param_type)) {
        LOG(ERROR) << "JNI ERROR (app bug): attempt to pass an instance of "
                   << PrettyTypeOf(argument) << " as argument " << (i + 1)
                   << " to " << PrettyMethod(h_m.Get());
        ++error_count;
      }
    } else if (param_type->IsPrimitiveLong() || param_type->IsPrimitiveDouble()) {
      offset++;
    }
  }
  if (error_count > 0) {
    // TODO: pass the JNI function name (such as "CallVoidMethodV") through so we can call JniAbort
    // with an argument.
    JniAbortF(nullptr, "bad arguments passed to %s (see above for details)",
              PrettyMethod(h_m.Get()).c_str());
  }
}

static void InvokeWithArgArray(const ScopedObjectAccessAlreadyRunnable& soa,
                               mirror::ArtMethod* method, ArgArray* arg_array, JValue* result,
                               const char* shorty)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
  uint32_t* args = arg_array->GetArray();
  if (UNLIKELY(soa.Env()->check_jni)) {
	  CheckMethodArguments(method, args);
  }
  method->Invoke(soa.Self(), args, arg_array->GetNumBytes(), result, shorty);
}

}  // namespace art
