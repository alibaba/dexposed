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
 * instanceof, checkcast, etc.
 */
#ifndef DALVIK_OO_TYPECHECK_H_
#define DALVIK_OO_TYPECHECK_H_

/* VM startup/shutdown */
bool dvmInstanceofStartup(void);
void dvmInstanceofShutdown(void);


/* used by dvmInstanceof; don't call */
extern "C" int dvmInstanceofNonTrivial(const ClassObject* instance,
                                       const ClassObject* clazz);

/*
 * Determine whether "instance" is an instance of "clazz".
 *
 * Returns 0 (false) if not, 1 (true) if so.
 */
INLINE int dvmInstanceof(const ClassObject* instance, const ClassObject* clazz)
{
    if (instance == clazz) {
        if (CALC_CACHE_STATS)
            gDvm.instanceofCache->trivial++;
        return 1;
    } else
        return dvmInstanceofNonTrivial(instance, clazz);
}

/*
 * Determine whether a class implements an interface.
 *
 * Returns 0 (false) if not, 1 (true) if so.
 */
int dvmImplements(const ClassObject* clazz, const ClassObject* interface);

/*
 * Determine whether "sub" is a sub-class of "clazz".
 *
 * Returns 0 (false) if not, 1 (true) if so.
 */
INLINE int dvmIsSubClass(const ClassObject* sub, const ClassObject* clazz) {
    do {
        /*printf("###### sub='%s' clazz='%s'\n", sub->name, clazz->name);*/
        if (sub == clazz)
            return 1;
        sub = sub->super;
    } while (sub != NULL);

    return 0;
}

/*
 * Determine whether or not we can store an object into an array, based
 * on the classes of the two.
 *
 * Returns 0 (false) if not, 1 (true) if so.
 */
extern "C" bool dvmCanPutArrayElement(const ClassObject* elemClass,
    const ClassObject* arrayClass);

#endif  // DALVIK_OO_TYPECHECK_H_
