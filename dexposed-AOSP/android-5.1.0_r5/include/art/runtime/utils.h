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

#ifndef ART_RUNTIME_UTILS_H_
#define ART_RUNTIME_UTILS_H_

#include <pthread.h>

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/mutex.h"
#include "globals.h"
#include "instruction_set.h"
#include "primitive.h"

#ifdef HAVE_ANDROID_OS
#include "cutils/properties.h"
#endif

namespace art {

class DexFile;

namespace mirror {
class ArtField;
class ArtMethod;
class Class;
class Object;
class String;
}  // namespace mirror

enum TimeUnit {
  kTimeUnitNanosecond,
  kTimeUnitMicrosecond,
  kTimeUnitMillisecond,
  kTimeUnitSecond,
};

template <typename T>
bool ParseUint(const char *in, T* out) {
  char* end;
  unsigned long long int result = strtoull(in, &end, 0);  // NOLINT(runtime/int)
  if (in == end || *end != '\0') {
    return false;
  }
  if (std::numeric_limits<T>::max() < result) {
    return false;
  }
  *out = static_cast<T>(result);
  return true;
}

template <typename T>
bool ParseInt(const char* in, T* out) {
  char* end;
  long long int result = strtoll(in, &end, 0);  // NOLINT(runtime/int)
  if (in == end || *end != '\0') {
    return false;
  }
  if (result < std::numeric_limits<T>::min() || std::numeric_limits<T>::max() < result) {
    return false;
  }
  *out = static_cast<T>(result);
  return true;
}

template<typename T>
static constexpr bool IsPowerOfTwo(T x) {
  return (x & (x - 1)) == 0;
}

template<int n, typename T>
static inline bool IsAligned(T x) {
  COMPILE_ASSERT((n & (n - 1)) == 0, n_not_power_of_two);
  return (x & (n - 1)) == 0;
}

template<int n, typename T>
static inline bool IsAligned(T* x) {
  return IsAligned<n>(reinterpret_cast<const uintptr_t>(x));
}

template<typename T>
static inline bool IsAlignedParam(T x, int n) {
  return (x & (n - 1)) == 0;
}

#define CHECK_ALIGNED(value, alignment) \
  CHECK(::art::IsAligned<alignment>(value)) << reinterpret_cast<const void*>(value)

#define DCHECK_ALIGNED(value, alignment) \
  DCHECK(::art::IsAligned<alignment>(value)) << reinterpret_cast<const void*>(value)

#define DCHECK_ALIGNED_PARAM(value, alignment) \
  DCHECK(::art::IsAlignedParam(value, alignment)) << reinterpret_cast<const void*>(value)

// Check whether an N-bit two's-complement representation can hold value.
static inline bool IsInt(int N, word value) {
  CHECK_LT(0, N);
  CHECK_LT(N, kBitsPerWord);
  word limit = static_cast<word>(1) << (N - 1);
  return (-limit <= value) && (value < limit);
}

static inline bool IsUint(int N, word value) {
  CHECK_LT(0, N);
  CHECK_LT(N, kBitsPerWord);
  word limit = static_cast<word>(1) << N;
  return (0 <= value) && (value < limit);
}

static inline bool IsAbsoluteUint(int N, word value) {
  CHECK_LT(0, N);
  CHECK_LT(N, kBitsPerWord);
  if (value < 0) value = -value;
  return IsUint(N, value);
}

static inline uint16_t Low16Bits(uint32_t value) {
  return static_cast<uint16_t>(value);
}

static inline uint16_t High16Bits(uint32_t value) {
  return static_cast<uint16_t>(value >> 16);
}

static inline uint32_t Low32Bits(uint64_t value) {
  return static_cast<uint32_t>(value);
}

static inline uint32_t High32Bits(uint64_t value) {
  return static_cast<uint32_t>(value >> 32);
}

// A static if which determines whether to return type A or B based on the condition boolean.
template <bool condition, typename A, typename B>
struct TypeStaticIf {
  typedef A type;
};

// Specialization to handle the false case.
template <typename A, typename B>
struct TypeStaticIf<false, A,  B> {
  typedef B type;
};

// Type identity.
template <typename T>
struct TypeIdentity {
  typedef T type;
};

// Like sizeof, but count how many bits a type takes. Pass type explicitly.
template <typename T>
static constexpr size_t BitSizeOf() {
  return sizeof(T) * CHAR_BIT;
}

// Like sizeof, but count how many bits a type takes. Infers type from parameter.
template <typename T>
static constexpr size_t BitSizeOf(T x) {
  return sizeof(T) * CHAR_BIT;
}

// For rounding integers.
template<typename T>
static constexpr T RoundDown(T x, typename TypeIdentity<T>::type n) WARN_UNUSED;

template<typename T>
static constexpr T RoundDown(T x, typename TypeIdentity<T>::type n) {
  return
      DCHECK_CONSTEXPR(IsPowerOfTwo(n), , T(0))
      (x & -n);
}

template<typename T>
static constexpr T RoundUp(T x, typename TypeIdentity<T>::type n) WARN_UNUSED;

template<typename T>
static constexpr T RoundUp(T x, typename TypeIdentity<T>::type n) {
  return RoundDown(x + n - 1, n);
}

// For aligning pointers.
template<typename T>
static inline T* AlignDown(T* x, uintptr_t n) WARN_UNUSED;

template<typename T>
static inline T* AlignDown(T* x, uintptr_t n) {
  return reinterpret_cast<T*>(RoundDown(reinterpret_cast<uintptr_t>(x), n));
}

template<typename T>
static inline T* AlignUp(T* x, uintptr_t n) WARN_UNUSED;

template<typename T>
static inline T* AlignUp(T* x, uintptr_t n) {
  return reinterpret_cast<T*>(RoundUp(reinterpret_cast<uintptr_t>(x), n));
}

namespace utils {
namespace detail {  // Private, implementation-specific namespace. Do not poke outside of this file.
template <typename T>
static constexpr inline T RoundUpToPowerOfTwoRecursive(T x, size_t bit) {
  return bit == (BitSizeOf<T>()) ? x: RoundUpToPowerOfTwoRecursive(x | x >> bit, bit << 1);
}
}  // namespace detail
}  // namespace utils

// Recursive implementation is from "Hacker's Delight" by Henry S. Warren, Jr.,
// figure 3-3, page 48, where the function is called clp2.
template <typename T>
static constexpr inline T RoundUpToPowerOfTwo(T x) {
  return art::utils::detail::RoundUpToPowerOfTwoRecursive(x - 1, 1) + 1;
}

// Implementation is from "Hacker's Delight" by Henry S. Warren, Jr.,
// figure 3-3, page 48, where the function is called clp2.
static inline uint32_t RoundUpToPowerOfTwo(uint32_t x) {
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}

// Find the bit position of the most significant bit (0-based), or -1 if there were no bits set.
template <typename T>
static constexpr ssize_t MostSignificantBit(T value) {
  return (value == 0) ? -1 : (MostSignificantBit(value >> 1) + 1);
}

// How many bits (minimally) does it take to store the constant 'value'? i.e. 1 for 1, 3 for 5, etc.
template <typename T>
static constexpr size_t MinimumBitsToStore(T value) {
  return static_cast<size_t>(MostSignificantBit(value) + 1);
}

template<typename T>
static constexpr int CLZ(T x) {
  static_assert(sizeof(T) <= sizeof(long long), "T too large, must be smaller than long long");  // NOLINT [runtime/int] [4]
  return (sizeof(T) == sizeof(uint32_t))
      ? __builtin_clz(x)  // TODO: __builtin_clz[ll] has undefined behavior for x=0
      : __builtin_clzll(x);
}

template<typename T>
static constexpr int CTZ(T x) {
  return (sizeof(T) == sizeof(uint32_t))
      ? __builtin_ctz(x)
      : __builtin_ctzll(x);
}

template<typename T>
static constexpr int POPCOUNT(T x) {
  return (sizeof(T) == sizeof(uint32_t))
      ? __builtin_popcount(x)
      : __builtin_popcountll(x);
}

static inline uint32_t PointerToLowMemUInt32(const void* p) {
  uintptr_t intp = reinterpret_cast<uintptr_t>(p);
  DCHECK_LE(intp, 0xFFFFFFFFU);
  return intp & 0xFFFFFFFFU;
}

static inline bool NeedsEscaping(uint16_t ch) {
  return (ch < ' ' || ch > '~');
}

// Interpret the bit pattern of input (type U) as type V. Requires the size
// of V >= size of U (compile-time checked).
template<typename U, typename V>
static inline V bit_cast(U in) {
  COMPILE_ASSERT(sizeof(U) <= sizeof(V), size_of_u_not_le_size_of_v);
  union {
    U u;
    V v;
  } tmp;
  tmp.u = in;
  return tmp.v;
}

std::string PrintableChar(uint16_t ch);

// Returns an ASCII string corresponding to the given UTF-8 string.
// Java escapes are used for non-ASCII characters.
std::string PrintableString(const char* utf8);

// Tests whether 's' starts with 'prefix'.
bool StartsWith(const std::string& s, const char* prefix);

// Tests whether 's' starts with 'suffix'.
bool EndsWith(const std::string& s, const char* suffix);

// Used to implement PrettyClass, PrettyField, PrettyMethod, and PrettyTypeOf,
// one of which is probably more useful to you.
// Returns a human-readable equivalent of 'descriptor'. So "I" would be "int",
// "[[I" would be "int[][]", "[Ljava/lang/String;" would be
// "java.lang.String[]", and so forth.
std::string PrettyDescriptor(mirror::String* descriptor)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
std::string PrettyDescriptor(const char* descriptor);
std::string PrettyDescriptor(mirror::Class* klass)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
std::string PrettyDescriptor(Primitive::Type type);

// Returns a human-readable signature for 'f'. Something like "a.b.C.f" or
// "int a.b.C.f" (depending on the value of 'with_type').
std::string PrettyField(mirror::ArtField* f, bool with_type = true)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
std::string PrettyField(uint32_t field_idx, const DexFile& dex_file, bool with_type = true);

// Returns a human-readable signature for 'm'. Something like "a.b.C.m" or
// "a.b.C.m(II)V" (depending on the value of 'with_signature').
std::string PrettyMethod(mirror::ArtMethod* m, bool with_signature = true)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
std::string PrettyMethod(uint32_t method_idx, const DexFile& dex_file, bool with_signature = true);

// Returns a human-readable form of the name of the *class* of the given object.
// So given an instance of java.lang.String, the output would
// be "java.lang.String". Given an array of int, the output would be "int[]".
// Given String.class, the output would be "java.lang.Class<java.lang.String>".
std::string PrettyTypeOf(mirror::Object* obj)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Returns a human-readable form of the type at an index in the specified dex file.
// Example outputs: char[], java.lang.String.
std::string PrettyType(uint32_t type_idx, const DexFile& dex_file);

// Returns a human-readable form of the name of the given class.
// Given String.class, the output would be "java.lang.Class<java.lang.String>".
std::string PrettyClass(mirror::Class* c)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Returns a human-readable form of the name of the given class with its class loader.
std::string PrettyClassAndClassLoader(mirror::Class* c)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

// Returns a human-readable size string such as "1MB".
std::string PrettySize(int64_t size_in_bytes);

// Returns a human-readable time string which prints every nanosecond while trying to limit the
// number of trailing zeros. Prints using the largest human readable unit up to a second.
// e.g. "1ms", "1.000000001s", "1.001us"
std::string PrettyDuration(uint64_t nano_duration, size_t max_fraction_digits = 3);

// Format a nanosecond time to specified units.
std::string FormatDuration(uint64_t nano_duration, TimeUnit time_unit,
                           size_t max_fraction_digits);

// Get the appropriate unit for a nanosecond duration.
TimeUnit GetAppropriateTimeUnit(uint64_t nano_duration);

// Get the divisor to convert from a nanoseconds to a time unit.
uint64_t GetNsToTimeUnitDivisor(TimeUnit time_unit);

// Performs JNI name mangling as described in section 11.3 "Linking Native Methods"
// of the JNI spec.
std::string MangleForJni(const std::string& s);

// Turn "java.lang.String" into "Ljava/lang/String;".
std::string DotToDescriptor(const char* class_name);

// Turn "Ljava/lang/String;" into "java.lang.String" using the conventions of
// java.lang.Class.getName().
std::string DescriptorToDot(const char* descriptor);

// Turn "Ljava/lang/String;" into "java/lang/String" using the opposite conventions of
// java.lang.Class.getName().
std::string DescriptorToName(const char* descriptor);

// Tests for whether 's' is a valid class name in the three common forms:
bool IsValidBinaryClassName(const char* s);  // "java.lang.String"
bool IsValidJniClassName(const char* s);     // "java/lang/String"
bool IsValidDescriptor(const char* s);       // "Ljava/lang/String;"

// Returns whether the given string is a valid field or method name,
// additionally allowing names that begin with '<' and end with '>'.
bool IsValidMemberName(const char* s);

// Returns the JNI native function name for the non-overloaded method 'm'.
std::string JniShortName(mirror::ArtMethod* m)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
// Returns the JNI native function name for the overloaded method 'm'.
std::string JniLongName(mirror::ArtMethod* m)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

bool ReadFileToString(const std::string& file_name, std::string* result);

// Returns the current date in ISO yyyy-mm-dd hh:mm:ss format.
std::string GetIsoDate();

// Returns the monotonic time since some unspecified starting point in milliseconds.
uint64_t MilliTime();

// Returns the monotonic time since some unspecified starting point in microseconds.
uint64_t MicroTime();

// Returns the monotonic time since some unspecified starting point in nanoseconds.
uint64_t NanoTime();

// Returns the thread-specific CPU-time clock in nanoseconds or -1 if unavailable.
uint64_t ThreadCpuNanoTime();

// Converts the given number of nanoseconds to milliseconds.
static constexpr inline uint64_t NsToMs(uint64_t ns) {
  return ns / 1000 / 1000;
}

// Converts the given number of milliseconds to nanoseconds
static constexpr inline uint64_t MsToNs(uint64_t ns) {
  return ns * 1000 * 1000;
}

#if defined(__APPLE__)
// No clocks to specify on OS/X, fake value to pass to routines that require a clock.
#define CLOCK_REALTIME 0xebadf00d
#endif

// Sleep for the given number of nanoseconds, a bad way to handle contention.
void NanoSleep(uint64_t ns);

// Initialize a timespec to either a relative time (ms,ns), or to the absolute
// time corresponding to the indicated clock value plus the supplied offset.
void InitTimeSpec(bool absolute, int clock, int64_t ms, int32_t ns, timespec* ts);

// Splits a string using the given separator character into a vector of
// strings. Empty strings will be omitted.
void Split(const std::string& s, char separator, std::vector<std::string>& result);

// Trims whitespace off both ends of the given string.
std::string Trim(std::string s);

// Joins a vector of strings into a single string, using the given separator.
template <typename StringT> std::string Join(std::vector<StringT>& strings, char separator);

// Returns the calling thread's tid. (The C libraries don't expose this.)
pid_t GetTid();

// Returns the given thread's name.
std::string GetThreadName(pid_t tid);

// Returns details of the given thread's stack.
void GetThreadStack(pthread_t thread, void** stack_base, size_t* stack_size, size_t* guard_size);

// Reads data from "/proc/self/task/${tid}/stat".
void GetTaskStats(pid_t tid, char* state, int* utime, int* stime, int* task_cpu);

// Returns the name of the scheduler group for the given thread the current process, or the empty string.
std::string GetSchedulerGroupName(pid_t tid);

// Sets the name of the current thread. The name may be truncated to an
// implementation-defined limit.
void SetThreadName(const char* thread_name);

// Dumps the native stack for thread 'tid' to 'os'.
void DumpNativeStack(std::ostream& os, pid_t tid, const char* prefix = "",
    mirror::ArtMethod* current_method = nullptr)
    NO_THREAD_SAFETY_ANALYSIS;

// Dumps the kernel stack for thread 'tid' to 'os'. Note that this is only available on linux-x86.
void DumpKernelStack(std::ostream& os, pid_t tid, const char* prefix = "", bool include_count = true);

// Find $ANDROID_ROOT, /system, or abort.
const char* GetAndroidRoot();

// Find $ANDROID_DATA, /data, or abort.
const char* GetAndroidData();
// Find $ANDROID_DATA, /data, or return nullptr.
const char* GetAndroidDataSafe(std::string* error_msg);

// Returns the dalvik-cache location, or dies trying. subdir will be
// appended to the cache location.
std::string GetDalvikCacheOrDie(const char* subdir, bool create_if_absent = true);
// Return true if we found the dalvik cache and stored it in the dalvik_cache argument.
// have_android_data will be set to true if we have an ANDROID_DATA that exists,
// dalvik_cache_exists will be true if there is a dalvik-cache directory that is present.
// The flag is_global_cache tells whether this cache is /data/dalvik-cache.
void GetDalvikCache(const char* subdir, bool create_if_absent, std::string* dalvik_cache,
                    bool* have_android_data, bool* dalvik_cache_exists, bool* is_global_cache);

// Returns the absolute dalvik-cache path for a DexFile or OatFile. The path returned will be
// rooted at cache_location.
bool GetDalvikCacheFilename(const char* file_location, const char* cache_location,
                            std::string* filename, std::string* error_msg);
// Returns the absolute dalvik-cache path for a DexFile or OatFile, or
// dies trying. The path returned will be rooted at cache_location.
std::string GetDalvikCacheFilenameOrDie(const char* file_location,
                                        const char* cache_location);

// Returns the system location for an image
std::string GetSystemImageFilename(const char* location, InstructionSet isa);

// Returns an .odex file name next adjacent to the dex location.
// For example, for "/foo/bar/baz.jar", return "/foo/bar/<isa>/baz.odex".
// Note: does not support multidex location strings.
std::string DexFilenameToOdexFilename(const std::string& location, InstructionSet isa);

// Check whether the given magic matches a known file type.
bool IsZipMagic(uint32_t magic);
bool IsDexMagic(uint32_t magic);
bool IsOatMagic(uint32_t magic);

// Wrapper on fork/execv to run a command in a subprocess.
bool Exec(std::vector<std::string>& arg_vector, std::string* error_msg);

class VoidFunctor {
 public:
  template <typename A>
  inline void operator() (A a) const {
    UNUSED(a);
  }

  template <typename A, typename B>
  inline void operator() (A a, B b) const {
    UNUSED(a);
    UNUSED(b);
  }

  template <typename A, typename B, typename C>
  inline void operator() (A a, B b, C c) const {
    UNUSED(a);
    UNUSED(b);
    UNUSED(c);
  }
};

// Deleter using free() for use with std::unique_ptr<>. See also UniqueCPtr<> below.
struct FreeDelete {
  // NOTE: Deleting a const object is valid but free() takes a non-const pointer.
  void operator()(const void* ptr) const {
    free(const_cast<void*>(ptr));
  }
};

// Alias for std::unique_ptr<> that uses the C function free() to delete objects.
template <typename T>
using UniqueCPtr = std::unique_ptr<T, FreeDelete>;

}  // namespace art

#endif  // ART_RUNTIME_UTILS_H_
