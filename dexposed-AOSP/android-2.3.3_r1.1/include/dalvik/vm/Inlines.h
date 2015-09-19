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
 * In gcc, "extern inline" ensures that the copy in the header is never
 * turned into a separate function.  This prevents us from having multiple
 * non-inline copies.  However, we still need to provide a non-inline
 * version in the library for the benefit of applications that include our
 * headers and are built with optimizations disabled.  Either that, or use
 * the "always_inline" gcc attribute to ensure that the non-inline version
 * is never needed.
 *
 * (Note C99 has different notions about what the keyword combos mean.)
 */
#ifndef _DALVIK_GEN_INLINES             /* only defined by Inlines.c */
# define INLINE extern __inline__
#else
# define INLINE
#endif
