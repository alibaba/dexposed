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
 * Resolve "constant pool" references into pointers to VM structs.
 */
#ifndef DALVIK_OO_RESOLVE_H_
#define DALVIK_OO_RESOLVE_H_

/*
 * "Direct" and "virtual" methods are stored independently.  The type of call
 * used to invoke the method determines which list we search, and whether
 * we travel up into superclasses.
 *
 * (<clinit>, <init>, and methods declared "private" or "static" are stored
 * in the "direct" list.  All others are stored in the "virtual" list.)
 */
enum MethodType {
    METHOD_UNKNOWN  = 0,
    METHOD_DIRECT,      // <init>, private
    METHOD_STATIC,      // static
    METHOD_VIRTUAL,     // virtual, super
    METHOD_INTERFACE    // interface
};

/*
 * Resolve a class, given the referring class and a constant pool index
 * for the DexTypeId.
 *
 * Does not initialize the class.
 *
 * Throws an exception and returns NULL on failure.
 */
extern "C" ClassObject* dvmResolveClass(const ClassObject* referrer,
                                        u4 classIdx,
                                        bool fromUnverifiedConstant);

/*
 * Resolve a direct, static, or virtual method.
 *
 * Can cause the method's class to be initialized if methodType is
 * METHOD_STATIC.
 *
 * Throws an exception and returns NULL on failure.
 */
extern "C" Method* dvmResolveMethod(const ClassObject* referrer, u4 methodIdx,
                                    MethodType methodType);

/*
 * Resolve an interface method.
 *
 * Throws an exception and returns NULL on failure.
 */
Method* dvmResolveInterfaceMethod(const ClassObject* referrer, u4 methodIdx);

/*
 * Resolve an instance field.
 *
 * Throws an exception and returns NULL on failure.
 */
extern "C" InstField* dvmResolveInstField(const ClassObject* referrer,
                                          u4 ifieldIdx);

/*
 * Resolve a static field.
 *
 * Causes the field's class to be initialized.
 *
 * Throws an exception and returns NULL on failure.
 */
extern "C" StaticField* dvmResolveStaticField(const ClassObject* referrer,
                                              u4 sfieldIdx);

/*
 * Resolve a "const-string" reference.
 *
 * Throws an exception and returns NULL on failure.
 */
extern "C" StringObject* dvmResolveString(const ClassObject* referrer, u4 stringIdx);

/*
 * Return debug string constant for enum.
 */
const char* dvmMethodTypeStr(MethodType methodType);

#endif  // DALVIK_OO_RESOLVE_H_
