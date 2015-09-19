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
 * VM initialization and shutdown.
 */
#ifndef DALVIK_INIT_H_
#define DALVIK_INIT_H_

/*
 * Standard VM initialization, usually invoked through JNI.
 */
std::string dvmStartup(int argc, const char* const argv[],
        bool ignoreUnrecognized, JNIEnv* pEnv);
void dvmShutdown(void);
bool dvmInitAfterZygote(void);

/*
 * Enable Java programming language assert statements after the Zygote fork.
 */
void dvmLateEnableAssertions(void);

/*
 * Partial VM initialization; only used as part of "dexopt", which may be
 * asked to optimize a DEX file holding fundamental classes.
 */
int dvmPrepForDexOpt(const char* bootClassPath, DexOptimizerMode dexOptMode,
    DexClassVerifyMode verifyMode, int dexoptFlags);

/*
 * Look up the set of classes and members used directly by the VM,
 * storing references to them into the globals instance. See
 * Globals.h. This function is exposed so that dex optimization may
 * call it (while avoiding doing other unnecessary VM initialization).
 *
 * The function returns a success flag (true == success).
 */
bool dvmFindRequiredClassesAndMembers(void);

/*
 * Look up required members of the class Reference, and set the global
 * reference to Reference itself too. This needs to be done separately
 * from dvmFindRequiredClassesAndMembers(), during the course of
 * linking the class Reference (which is done specially).
 */
bool dvmFindReferenceMembers(ClassObject* classReference);

/*
 * Replacement for fprintf() when we want to send a message to the console.
 * This defaults to fprintf(), but will use the JNI fprintf callback if
 * one was provided.
 */
int dvmFprintf(FILE* fp, const char* format, ...)
#if defined(__GNUC__)
    __attribute__ ((format(printf, 2, 3)))
#endif
    ;

#endif  // DALVIK_INIT_H_
