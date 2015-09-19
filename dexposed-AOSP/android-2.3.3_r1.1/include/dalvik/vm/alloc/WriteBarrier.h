/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef DALVIK_ALLOC_WRITEBARRIER_H_
#define DALVIK_ALLOC_WRITEBARRIER_H_

/*
 * Note writes to the heap. These functions must be called if a field
 * of an Object in the heap changes, and before any GC safe-point. The
 * call is not needed if NULL is stored in the field.
 */

/*
 * The address within the Object has been written, and perhaps changed.
 */
INLINE void dvmWriteBarrierField(const Object *obj, void *addr)
{
    dvmMarkCard(obj);
}

/*
 * All of the Object may have changed.
 */
INLINE void dvmWriteBarrierObject(const Object *obj)
{
    dvmMarkCard(obj);
}

/*
 * Some or perhaps all of the array indexes in the Array, greater than
 * or equal to start and strictly less than end, have been written,
 * and perhaps changed.
 */
INLINE void dvmWriteBarrierArray(const ArrayObject *obj,
                                 size_t start, size_t end)
{
    dvmMarkCard((Object *)obj);
}

#endif  // DALVIK_ALLOC_WRITEBARRIER_H_
