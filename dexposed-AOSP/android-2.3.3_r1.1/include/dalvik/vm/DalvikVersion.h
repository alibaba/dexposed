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
 * Dalvik VM version info.
 */
#ifndef DALVIK_VERSION_H_
#define DALVIK_VERSION_H_

/*
 * The version we show to tourists.
 */
#define DALVIK_MAJOR_VERSION    1
#define DALVIK_MINOR_VERSION    6
#define DALVIK_BUG_VERSION      0

/*
 * VM build number.  This must change whenever something that affects the
 * way classes load changes, e.g. field ordering or vtable layout.  Changing
 * this guarantees that the optimized form of the DEX file is regenerated.
 */
#define DALVIK_VM_BUILD         27

#endif  // DALVIK_VERSION_H_
