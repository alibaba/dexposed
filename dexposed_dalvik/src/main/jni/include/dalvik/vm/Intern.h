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
 * Interned strings.
 */
#ifndef DALVIK_INTERN_H_
#define DALVIK_INTERN_H_

bool dvmStringInternStartup(void);
void dvmStringInternShutdown(void);
StringObject* dvmLookupInternedString(StringObject* strObj);
StringObject* dvmLookupImmortalInternedString(StringObject* strObj);
bool dvmIsWeakInternedString(StringObject* strObj);
void dvmGcDetachDeadInternedStrings(int (*isUnmarkedObject)(void *));

#endif  // DALVIK_INTERN_H_
