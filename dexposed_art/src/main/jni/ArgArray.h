//
// Created by liangwei on 15/8/25.
//

#ifndef DEXPOSED_ARGARRAY_H
#define DEXPOSED_ARGARRAY_H

#include <jni.h>
#include "class_linker.h"
#include "common_throws.h"
#include "dex_file-inl.h"
#include "jni_internal.h"
#include "method_helper-inl.h"
#include "mirror/art_field-inl.h"
#include "mirror/art_method-inl.h"
#include "mirror/class-inl.h"
#include "mirror/class.h"
#include "mirror/object_array-inl.h"
#include "mirror/object_array.h"
#include "scoped_thread_state_change.h"
#include "stack.h"
#include "well_known_classes.h"

namespace art {
    class ArgArray {
    public:
        explicit ArgArray(const char *shorty, uint32_t shorty_len)
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

        uint32_t *GetArray() {
            return arg_array_;
        }

        uint32_t GetNumBytes() {
            return num_bytes_;
        }

        void Append(uint32_t value) {
            arg_array_[num_bytes_ / 4] = value;
            num_bytes_ += 4;
        }

        void Append(mirror::Object *obj) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
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

        void BuildArgArrayFromVarArgs(const ScopedObjectAccessAlreadyRunnable &soa,
                                      mirror::Object *receiver, va_list ap)
        SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
            // Set receiver if non-null (method is not static)
            if (receiver != nullptr) {
                Append(receiver);
            }
            for (size_t i = 1; i < shorty_len_; ++i) {
                switch (shorty_[i]) {
                    case 'Z':
                    case 'B':
                    case 'C':
                    case 'S':
                    case 'I':
                        Append(va_arg(ap, jint));
                        break;
                    case 'F':
                        AppendFloat(va_arg(ap, jdouble));
                        break;
                    case 'L':
                        Append(soa.Decode<mirror::Object *>(va_arg(ap, jobject)));
                        break;
                    case 'D':
                        AppendDouble(va_arg(ap, jdouble));
                        break;
                    case 'J':
                        AppendWide(va_arg(ap, jlong));
                        break;
#ifndef NDEBUG
                        default:
                          LOG(FATAL) << "Unexpected shorty character: " << shorty_[i];
#endif
                }
            }
        }

        void BuildArgArrayFromJValues(const ScopedObjectAccessAlreadyRunnable &soa,
                                      mirror::Object *receiver, jvalue *args)
        SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
            // Set receiver if non-null (method is not static)
            if (receiver != nullptr) {
                Append(receiver);
            }
            for (size_t i = 1, args_offset = 0; i < shorty_len_; ++i, ++args_offset) {
                switch (shorty_[i]) {
                    case 'Z':
                        Append(args[args_offset].z);
                        break;
                    case 'B':
                        Append(args[args_offset].b);
                        break;
                    case 'C':
                        Append(args[args_offset].c);
                        break;
                    case 'S':
                        Append(args[args_offset].s);
                        break;
                    case 'I':
                    case 'F':
                        Append(args[args_offset].i);
                        break;
                    case 'L':
                        Append(soa.Decode<mirror::Object *>(args[args_offset].l));
                        break;
                    case 'D':
                    case 'J':
                        AppendWide(args[args_offset].j);
                        break;
#ifndef NDEBUG
                        default:
                          LOG(FATAL) << "Unexpected shorty character: " << shorty_[i];
#endif
                }
            }
        }

        void BuildArgArrayFromFrame(ShadowFrame *shadow_frame, uint32_t arg_offset)
        SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
            // Set receiver if non-null (method is not static)
            size_t cur_arg = arg_offset;
            if (!shadow_frame->GetMethod()->IsStatic()) {
                Append(shadow_frame->GetVReg(cur_arg));
                cur_arg++;
            }
            for (size_t i = 1; i < shorty_len_; ++i) {
                switch (shorty_[i]) {
                    case 'Z':
                    case 'B':
                    case 'C':
                    case 'S':
                    case 'I':
                    case 'F':
                    case 'L':
                        Append(shadow_frame->GetVReg(cur_arg));
                        cur_arg++;
                        break;
                    case 'D':
                    case 'J':
                        AppendWide(shadow_frame->GetVRegLong(cur_arg));
                        cur_arg++;
                        cur_arg++;
                        break;
#ifndef NDEBUG
                        default:
                          LOG(FATAL) << "Unexpected shorty character: " << shorty_[i];
#endif
                }
            }
        }

        static void ThrowIllegalPrimitiveArgumentException(const char *expected,
                                                           const char *found_descriptor)
        SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
            ThrowIllegalArgumentException(nullptr,
                                          StringPrintf("Invalid primitive conversion from %s to %s",
                                                       expected,
                                                       PrettyDescriptor(
                                                               found_descriptor).c_str()).c_str());
        }

        bool BuildArgArrayFromObjectArray(const ScopedObjectAccessAlreadyRunnable &soa,
                                          mirror::Object *receiver,
                                          mirror::ObjectArray<mirror::Object> *args,
                                          MethodHelper &mh)
        SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
            const DexFile::TypeList *classes = mh.GetMethod()->GetParameterTypeList();
            // Set receiver if non-null (method is not static)
            if (receiver != nullptr) {
                Append(receiver);
            }
            for (size_t i = 1, args_offset = 0; i < shorty_len_; ++i, ++args_offset) {
                mirror::Object *arg = args->Get(args_offset);
                if (((shorty_[i] == 'L') && (arg != nullptr)) ||
                    ((arg == nullptr && shorty_[i] != 'L'))) {
                    mirror::Class *dst_class =
                            mh.GetClassFromTypeIdx(classes->GetTypeItem(args_offset).type_idx_);
                    if (UNLIKELY(arg == nullptr || !arg->InstanceOf(dst_class))) {
                        ThrowIllegalArgumentException(nullptr,
                                                      StringPrintf(
                                                              "method %s argument %zd has type %s, got %s",
                                                              PrettyMethod(mh.GetMethod(),
                                                                           false).c_str(),
                                                              args_offset +
                                                              1,  // Humans don't count from 0.
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
                    case 'Z': DO_FIRST_ARG("Ljava/lang/Boolean;", GetBoolean, Append)
                    DO_FAIL("boolean")
                        break;
                    case 'B': DO_FIRST_ARG("Ljava/lang/Byte;", GetByte, Append)
                    DO_FAIL("byte")
                        break;
                    case 'C': DO_FIRST_ARG("Ljava/lang/Character;", GetChar, Append)
                    DO_FAIL("char")
                        break;
                    case 'S': DO_FIRST_ARG("Ljava/lang/Short;", GetShort, Append)
                        DO_ARG("Ljava/lang/Byte;", GetByte, Append)
                    DO_FAIL("short")
                        break;
                    case 'I': DO_FIRST_ARG("Ljava/lang/Integer;", GetInt, Append)
                        DO_ARG("Ljava/lang/Character;", GetChar, Append)
                        DO_ARG("Ljava/lang/Short;", GetShort, Append)
                        DO_ARG("Ljava/lang/Byte;", GetByte, Append)
                    DO_FAIL("int")
                        break;
                    case 'J': DO_FIRST_ARG("Ljava/lang/Long;", GetLong, AppendWide)
                        DO_ARG("Ljava/lang/Integer;", GetInt, AppendWide)
                        DO_ARG("Ljava/lang/Character;", GetChar, AppendWide)
                        DO_ARG("Ljava/lang/Short;", GetShort, AppendWide)
                        DO_ARG("Ljava/lang/Byte;", GetByte, AppendWide)
                    DO_FAIL("long")
                        break;
                    case 'F': DO_FIRST_ARG("Ljava/lang/Float;", GetFloat, AppendFloat)
                        DO_ARG("Ljava/lang/Long;", GetLong, AppendFloat)
                        DO_ARG("Ljava/lang/Integer;", GetInt, AppendFloat)
                        DO_ARG("Ljava/lang/Character;", GetChar, AppendFloat)
                        DO_ARG("Ljava/lang/Short;", GetShort, AppendFloat)
                        DO_ARG("Ljava/lang/Byte;", GetByte, AppendFloat)
                    DO_FAIL("float")
                        break;
                    case 'D': DO_FIRST_ARG("Ljava/lang/Double;", GetDouble, AppendDouble)
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
        enum {
            kSmallArgArraySize = 16
        };
        const char *const shorty_;
        const uint32_t shorty_len_;
        uint32_t num_bytes_;
        uint32_t *arg_array_;
        uint32_t small_arg_array_[kSmallArgArraySize];
        std::unique_ptr<uint32_t[]> large_arg_array_;
    };
}
#endif //DEXPOSED_ARGARRAY_H
