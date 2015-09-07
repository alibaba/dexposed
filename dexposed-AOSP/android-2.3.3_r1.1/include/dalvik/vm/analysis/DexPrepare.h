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

/*
 * DEX preparation declarations.
 */
#ifndef DALVIK_DEXPREPARE_H_
#define DALVIK_DEXPREPARE_H_

/*
 * Global DEX optimizer control.  Determines the circumstances in which we
 * try to rewrite instructions in the DEX file.
 *
 * Optimizing is performed ahead-of-time by dexopt and, in some cases, at
 * load time by the VM.
 */
enum DexOptimizerMode {
    OPTIMIZE_MODE_UNKNOWN = 0,
    OPTIMIZE_MODE_NONE,         /* never optimize (except "essential") */
    OPTIMIZE_MODE_VERIFIED,     /* only optimize verified classes (default) */
    OPTIMIZE_MODE_ALL,          /* optimize verified & unverified (risky) */
    OPTIMIZE_MODE_FULL          /* fully opt verified classes at load time */
};

/* some additional bit flags for dexopt */
enum DexoptFlags {
    DEXOPT_OPT_ENABLED       = 1,       /* optimizations enabled? */
    DEXOPT_OPT_ALL           = 1 << 1,  /* optimize when verify fails? */
    DEXOPT_VERIFY_ENABLED    = 1 << 2,  /* verification enabled? */
    DEXOPT_VERIFY_ALL        = 1 << 3,  /* verify bootstrap classes? */
    DEXOPT_IS_BOOTSTRAP      = 1 << 4,  /* is dex in bootstrap class path? */
    DEXOPT_GEN_REGISTER_MAPS = 1 << 5,  /* generate register maps during vfy */
    DEXOPT_UNIPROCESSOR      = 1 << 6,  /* specify uniprocessor target */
    DEXOPT_SMP               = 1 << 7   /* specify SMP target */
};

/*
 * An enumeration of problems that can turn up during verification.
 */
enum VerifyError {
    VERIFY_ERROR_NONE = 0,      /* no error; must be zero */
    VERIFY_ERROR_GENERIC,       /* VerifyError */

    VERIFY_ERROR_NO_CLASS,      /* NoClassDefFoundError */
    VERIFY_ERROR_NO_FIELD,      /* NoSuchFieldError */
    VERIFY_ERROR_NO_METHOD,     /* NoSuchMethodError */
    VERIFY_ERROR_ACCESS_CLASS,  /* IllegalAccessError */
    VERIFY_ERROR_ACCESS_FIELD,  /* IllegalAccessError */
    VERIFY_ERROR_ACCESS_METHOD, /* IllegalAccessError */
    VERIFY_ERROR_CLASS_CHANGE,  /* IncompatibleClassChangeError */
    VERIFY_ERROR_INSTANTIATION, /* InstantiationError */
};

/*
 * Identifies the type of reference in the instruction that generated the
 * verify error (e.g. VERIFY_ERROR_ACCESS_CLASS could come from a method,
 * field, or class reference).
 *
 * This must fit in two bits.
 */
enum VerifyErrorRefType {
    VERIFY_ERROR_REF_CLASS  = 0,
    VERIFY_ERROR_REF_FIELD  = 1,
    VERIFY_ERROR_REF_METHOD = 2,
};

#define kVerifyErrorRefTypeShift 6

#define VERIFY_OK(_failure) ((_failure) == VERIFY_ERROR_NONE)

/*
 * Given the full path to a DEX or Jar file, and (if appropriate) the name
 * within the Jar, open the optimized version from the cache.
 *
 * If "*pNewFile" is set, a new file has been created with only a stub
 * "opt" header, and the caller is expected to fill in the blanks.
 *
 * Returns the file descriptor, locked and seeked past the "opt" header.
 */
int dvmOpenCachedDexFile(const char* fileName, const char* cachedFile,
    u4 modWhen, u4 crc, bool isBootstrap, bool* pNewFile, bool createIfMissing);

/*
 * Unlock the specified file descriptor.  Use in conjunction with
 * dvmOpenCachedDexFile().
 *
 * Returns true on success.
 */
bool dvmUnlockCachedDexFile(int fd);

/*
 * Verify the contents of the "opt" header, and check the DEX file's
 * dependencies on its source zip (if available).
 */
bool dvmCheckOptHeaderAndDependencies(int fd, bool sourceAvail, u4 modWhen,
    u4 crc, bool expectVerify, bool expectOpt);

/*
 * Optimize a DEX file.  The file must start with the "opt" header, followed
 * by the plain DEX data.  It must be mmap()able.
 *
 * "fileName" is only used for debug output.
 */
bool dvmOptimizeDexFile(int fd, off_t dexOffset, long dexLen,
    const char* fileName, u4 modWhen, u4 crc, bool isBootstrap);

/*
 * Continue the optimization process on the other side of a fork/exec.
 */
bool dvmContinueOptimization(int fd, off_t dexOffset, long dexLength,
    const char* fileName, u4 modWhen, u4 crc, bool isBootstrap);

/*
 * Prepare DEX data that is only available to the VM as in-memory data.
 */
bool dvmPrepareDexInMemory(u1* addr, size_t len, DvmDex** ppDvmDex);

/*
 * Prep data structures.
 */
bool dvmCreateInlineSubsTable(void);
void dvmFreeInlineSubsTable(void);

#endif  // DALVIK_DEXPREPARE_H_
