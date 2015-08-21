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

#ifndef ART_RUNTIME_BASE_LOGGING_H_
#define ART_RUNTIME_BASE_LOGGING_H_

#include <cerrno>
#include <cstring>
#include <iostream>  // NOLINT
#include <memory>
#include <sstream>
#include <signal.h>
#include <vector>

#include "base/macros.h"
#include "log_severity.h"

#define CHECK(x) \
  if (UNLIKELY(!(x))) \
    ::art::LogMessage(__FILE__, __LINE__, FATAL, -1).stream() \
        << "Check failed: " #x << " "

#define CHECK_OP(LHS, RHS, OP) \
  for (auto _values = ::art::MakeEagerEvaluator(LHS, RHS); \
       UNLIKELY(!(_values.lhs OP _values.rhs)); /* empty */) \
    ::art::LogMessage(__FILE__, __LINE__, FATAL, -1).stream() \
        << "Check failed: " << #LHS << " " << #OP << " " << #RHS \
        << " (" #LHS "=" << _values.lhs << ", " #RHS "=" << _values.rhs << ") "

#define CHECK_EQ(x, y) CHECK_OP(x, y, ==)
#define CHECK_NE(x, y) CHECK_OP(x, y, !=)
#define CHECK_LE(x, y) CHECK_OP(x, y, <=)
#define CHECK_LT(x, y) CHECK_OP(x, y, <)
#define CHECK_GE(x, y) CHECK_OP(x, y, >=)
#define CHECK_GT(x, y) CHECK_OP(x, y, >)

#define CHECK_STROP(s1, s2, sense) \
  if (UNLIKELY((strcmp(s1, s2) == 0) != sense)) \
    LOG(FATAL) << "Check failed: " \
               << "\"" << s1 << "\"" \
               << (sense ? " == " : " != ") \
               << "\"" << s2 << "\""

#define CHECK_STREQ(s1, s2) CHECK_STROP(s1, s2, true)
#define CHECK_STRNE(s1, s2) CHECK_STROP(s1, s2, false)

#define CHECK_PTHREAD_CALL(call, args, what) \
  do { \
    int rc = call args; \
    if (rc != 0) { \
      errno = rc; \
      PLOG(FATAL) << # call << " failed for " << what; \
    } \
  } while (false)

// CHECK that can be used in a constexpr function. For example,
//    constexpr int half(int n) {
//      return
//          DCHECK_CONSTEXPR(n >= 0, , 0)
//          CHECK_CONSTEXPR((n & 1) == 0), << "Extra debugging output: n = " << n, 0)
//          n / 2;
//    }
#define CHECK_CONSTEXPR(x, out, dummy) \
  (UNLIKELY(!(x))) ? (LOG(FATAL) << "Check failed: " << #x out, dummy) :

#ifndef NDEBUG

#define DCHECK(x) CHECK(x)
#define DCHECK_EQ(x, y) CHECK_EQ(x, y)
#define DCHECK_NE(x, y) CHECK_NE(x, y)
#define DCHECK_LE(x, y) CHECK_LE(x, y)
#define DCHECK_LT(x, y) CHECK_LT(x, y)
#define DCHECK_GE(x, y) CHECK_GE(x, y)
#define DCHECK_GT(x, y) CHECK_GT(x, y)
#define DCHECK_STREQ(s1, s2) CHECK_STREQ(s1, s2)
#define DCHECK_STRNE(s1, s2) CHECK_STRNE(s1, s2)
#define DCHECK_CONSTEXPR(x, out, dummy) CHECK_CONSTEXPR(x, out, dummy)

#else  // NDEBUG

#define DCHECK(condition) \
  while (false) \
    CHECK(condition)

#define DCHECK_EQ(val1, val2) \
  while (false) \
    CHECK_EQ(val1, val2)

#define DCHECK_NE(val1, val2) \
  while (false) \
    CHECK_NE(val1, val2)

#define DCHECK_LE(val1, val2) \
  while (false) \
    CHECK_LE(val1, val2)

#define DCHECK_LT(val1, val2) \
  while (false) \
    CHECK_LT(val1, val2)

#define DCHECK_GE(val1, val2) \
  while (false) \
    CHECK_GE(val1, val2)

#define DCHECK_GT(val1, val2) \
  while (false) \
    CHECK_GT(val1, val2)

#define DCHECK_STREQ(str1, str2) \
  while (false) \
    CHECK_STREQ(str1, str2)

#define DCHECK_STRNE(str1, str2) \
  while (false) \
    CHECK_STRNE(str1, str2)

#define DCHECK_CONSTEXPR(x, out, dummy) \
  (false && (x)) ? (dummy) :

#endif

#define LOG(severity) ::art::LogMessage(__FILE__, __LINE__, severity, -1).stream()
#define PLOG(severity) ::art::LogMessage(__FILE__, __LINE__, severity, errno).stream()

#define LG LOG(INFO)

#define UNIMPLEMENTED(level) LOG(level) << __PRETTY_FUNCTION__ << " unimplemented "

#define VLOG_IS_ON(module) UNLIKELY(::art::gLogVerbosity.module)
#define VLOG(module) if (VLOG_IS_ON(module)) ::art::LogMessage(__FILE__, __LINE__, INFO, -1).stream()
#define VLOG_STREAM(module) ::art::LogMessage(__FILE__, __LINE__, INFO, -1).stream()

//
// Implementation details beyond this point.
//

namespace art {

template <typename LHS, typename RHS>
struct EagerEvaluator {
  EagerEvaluator(LHS lhs, RHS rhs) : lhs(lhs), rhs(rhs) { }
  LHS lhs;
  RHS rhs;
};

// We want char*s to be treated as pointers, not strings. If you want them treated like strings,
// you'd need to use CHECK_STREQ and CHECK_STRNE anyway to compare the characters rather than their
// addresses. We could express this more succinctly with std::remove_const, but this is quick and
// easy to understand, and works before we have C++0x. We rely on signed/unsigned warnings to
// protect you against combinations not explicitly listed below.
#define EAGER_PTR_EVALUATOR(T1, T2) \
  template <> struct EagerEvaluator<T1, T2> { \
    EagerEvaluator(T1 lhs, T2 rhs) \
        : lhs(reinterpret_cast<const void*>(lhs)), \
          rhs(reinterpret_cast<const void*>(rhs)) { } \
    const void* lhs; \
    const void* rhs; \
  }
EAGER_PTR_EVALUATOR(const char*, const char*);
EAGER_PTR_EVALUATOR(const char*, char*);
EAGER_PTR_EVALUATOR(char*, const char*);
EAGER_PTR_EVALUATOR(char*, char*);
EAGER_PTR_EVALUATOR(const unsigned char*, const unsigned char*);
EAGER_PTR_EVALUATOR(const unsigned char*, unsigned char*);
EAGER_PTR_EVALUATOR(unsigned char*, const unsigned char*);
EAGER_PTR_EVALUATOR(unsigned char*, unsigned char*);
EAGER_PTR_EVALUATOR(const signed char*, const signed char*);
EAGER_PTR_EVALUATOR(const signed char*, signed char*);
EAGER_PTR_EVALUATOR(signed char*, const signed char*);
EAGER_PTR_EVALUATOR(signed char*, signed char*);

template <typename LHS, typename RHS>
EagerEvaluator<LHS, RHS> MakeEagerEvaluator(LHS lhs, RHS rhs) {
  return EagerEvaluator<LHS, RHS>(lhs, rhs);
}

// This indirection greatly reduces the stack impact of having
// lots of checks/logging in a function.
struct LogMessageData {
 public:
  LogMessageData(const char* file, int line, LogSeverity severity, int error);
  std::ostringstream buffer;
  const char* const file;
  const int line_number;
  const LogSeverity severity;
  const int error;

 private:
  DISALLOW_COPY_AND_ASSIGN(LogMessageData);
};

class LogMessage {
 public:
  LogMessage(const char* file, int line, LogSeverity severity, int error)
    : data_(new LogMessageData(file, line, severity, error)) {
  }

  ~LogMessage();  // TODO: enable LOCKS_EXCLUDED(Locks::logging_lock_).

  std::ostream& stream() {
    return data_->buffer;
  }

 private:
  static void LogLine(const LogMessageData& data, const char*);

  const std::unique_ptr<LogMessageData> data_;

  friend void HandleUnexpectedSignal(int signal_number, siginfo_t* info, void* raw_context);
  friend class Mutex;
  DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

// A convenience to allow any class with a "Dump(std::ostream& os)" member function
// but without an operator<< to be used as if it had an operator<<. Use like this:
//
//   os << Dumpable<MyType>(my_type_instance);
//
template<typename T>
class Dumpable {
 public:
  explicit Dumpable(T& value) : value_(value) {
  }

  void Dump(std::ostream& os) const {
    value_.Dump(os);
  }

 private:
  T& value_;

  DISALLOW_COPY_AND_ASSIGN(Dumpable);
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const Dumpable<T>& rhs) {
  rhs.Dump(os);
  return os;
}

template<typename T>
class ConstDumpable {
 public:
  explicit ConstDumpable(const T& value) : value_(value) {
  }

  void Dump(std::ostream& os) const {
    value_.Dump(os);
  }

 private:
  const T& value_;

  DISALLOW_COPY_AND_ASSIGN(ConstDumpable);
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const ConstDumpable<T>& rhs) {
  rhs.Dump(os);
  return os;
}

// Helps you use operator<< in a const char*-like context such as our various 'F' methods with
// format strings.
template<typename T>
class ToStr {
 public:
  explicit ToStr(const T& value) {
    std::ostringstream os;
    os << value;
    s_ = os.str();
  }

  const char* c_str() const {
    return s_.c_str();
  }

  const std::string& str() const {
    return s_;
  }

 private:
  std::string s_;
  DISALLOW_COPY_AND_ASSIGN(ToStr);
};

// The members of this struct are the valid arguments to VLOG and VLOG_IS_ON in code,
// and the "-verbose:" command line argument.
struct LogVerbosity {
  bool class_linker;  // Enabled with "-verbose:class".
  bool compiler;
  bool gc;
  bool heap;
  bool jdwp;
  bool jni;
  bool monitor;
  bool profiler;
  bool signals;
  bool startup;
  bool third_party_jni;  // Enabled with "-verbose:third-party-jni".
  bool threads;
  bool verifier;
};

extern LogVerbosity gLogVerbosity;

extern std::vector<std::string> gVerboseMethods;

// Used on fatal exit. Prevents recursive aborts. Allows us to disable
// some error checking to ensure fatal shutdown makes forward progress.
extern unsigned int gAborting;

extern void InitLogging(char* argv[]);

extern const char* GetCmdLine();
extern const char* ProgramInvocationName();
extern const char* ProgramInvocationShortName();

}  // namespace art

#endif  // ART_RUNTIME_BASE_LOGGING_H_
