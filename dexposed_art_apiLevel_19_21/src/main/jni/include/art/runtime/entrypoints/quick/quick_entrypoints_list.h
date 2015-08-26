/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_LIST_H_
#define ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_LIST_H_

// All quick entrypoints. Format is name, return type, argument types.

#define QUICK_ENTRYPOINT_LIST(V) \
  V(AllocArray, void*, uint32_t, void*, int32_t) \
  V(AllocArrayResolved, void*, void*, void*, int32_t) \
  V(AllocArrayWithAccessCheck, void*, uint32_t, void*, int32_t) \
  V(AllocObject, void*, uint32_t, void*) \
  V(AllocObjectResolved, void*, void*, void*) \
  V(AllocObjectInitialized, void*, void*, void*) \
  V(AllocObjectWithAccessCheck, void*, uint32_t, void*) \
  V(CheckAndAllocArray, void*, uint32_t, void*, int32_t) \
  V(CheckAndAllocArrayWithAccessCheck, void*, uint32_t, void*, int32_t) \
\
  V(InstanceofNonTrivial, uint32_t, const mirror::Class*, const mirror::Class*) \
  V(CheckCast, void , void*, void*) \
\
  V(InitializeStaticStorage, void*, uint32_t, void*) \
  V(InitializeTypeAndVerifyAccess, void*, uint32_t, void*) \
  V(InitializeType, void*, uint32_t, void*) \
  V(ResolveString, void*, void*, uint32_t) \
\
  V(Set32Instance, int, uint32_t, void*, int32_t) \
  V(Set32Static, int, uint32_t, int32_t) \
  V(Set64Instance, int, uint32_t, void*, int64_t) \
  V(Set64Static, int, uint32_t, int64_t) \
  V(SetObjInstance, int, uint32_t, void*, void*) \
  V(SetObjStatic, int, uint32_t, void*) \
  V(Get32Instance, int32_t, uint32_t, void*) \
  V(Get32Static, int32_t, uint32_t) \
  V(Get64Instance, int64_t, uint32_t, void*) \
  V(Get64Static, int64_t, uint32_t) \
  V(GetObjInstance, void*, uint32_t, void*) \
  V(GetObjStatic, void*, uint32_t) \
\
  V(AputObjectWithNullAndBoundCheck, void, void*, uint32_t, void*) \
  V(AputObjectWithBoundCheck, void, void*, uint32_t, void*) \
  V(AputObject, void, void*, uint32_t, void*) \
  V(HandleFillArrayData, void, void*, void*) \
\
  V(JniMethodStart, uint32_t, Thread*) \
  V(JniMethodStartSynchronized, uint32_t, jobject to_lock, Thread* self) \
  V(JniMethodEnd, void, uint32_t cookie, Thread* self) \
  V(JniMethodEndSynchronized, void, uint32_t cookie, jobject locked, Thread* self) \
  V(JniMethodEndWithReference, mirror::Object*, jobject result, uint32_t cookie, Thread* self) \
  V(JniMethodEndWithReferenceSynchronized, mirror::Object*, jobject result, uint32_t cookie, jobject locked, Thread* self) \
  V(QuickGenericJniTrampoline, void, mirror::ArtMethod*) \
\
  V(LockObject, void, void*) \
  V(UnlockObject, void, void*) \
\
  V(CmpgDouble, int32_t, double, double) \
  V(CmpgFloat, int32_t, float, float) \
  V(CmplDouble, int32_t, double, double) \
  V(CmplFloat, int32_t, float, float) \
  V(Fmod, double, double, double) \
  V(L2d, double, int64_t) \
  V(Fmodf, float, float, float) \
  V(L2f, float, int64_t) \
  V(D2iz, int32_t, double) \
  V(F2iz, int32_t, float) \
  V(Idivmod, int32_t, int32_t, int32_t) \
  V(D2l, int64_t, double) \
  V(F2l, int64_t, float) \
  V(Ldiv, int64_t, int64_t, int64_t) \
  V(Lmod, int64_t, int64_t, int64_t) \
  V(Lmul, int64_t, int64_t, int64_t) \
  V(ShlLong, uint64_t, uint64_t, uint32_t) \
  V(ShrLong, uint64_t, uint64_t, uint32_t) \
  V(UshrLong, uint64_t, uint64_t, uint32_t) \
\
  V(IndexOf, int32_t, void*, uint32_t, uint32_t, uint32_t) \
  V(StringCompareTo, int32_t, void*, void*) \
  V(Memcpy, void*, void*, const void*, size_t) \
\
  V(QuickImtConflictTrampoline, void, mirror::ArtMethod*) \
  V(QuickResolutionTrampoline, void, mirror::ArtMethod*) \
  V(QuickToInterpreterBridge, void, mirror::ArtMethod*) \
  V(InvokeDirectTrampolineWithAccessCheck, void, uint32_t, void*) \
  V(InvokeInterfaceTrampolineWithAccessCheck, void, uint32_t, void*) \
  V(InvokeStaticTrampolineWithAccessCheck, void, uint32_t, void*) \
  V(InvokeSuperTrampolineWithAccessCheck, void, uint32_t, void*) \
  V(InvokeVirtualTrampolineWithAccessCheck, void, uint32_t, void*) \
\
  V(TestSuspend, void, void) \
\
  V(DeliverException, void, void*) \
  V(ThrowArrayBounds, void, int32_t, int32_t) \
  V(ThrowDivZero, void, void) \
  V(ThrowNoSuchMethod, void, int32_t) \
  V(ThrowNullPointer, void, void) \
  V(ThrowStackOverflow, void, void*) \
\
  V(A64Load, int64_t, volatile const int64_t *) \
  V(A64Store, void, volatile int64_t *, int64_t)


#endif  // ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_LIST_H_
#undef ART_RUNTIME_ENTRYPOINTS_QUICK_QUICK_ENTRYPOINTS_LIST_H_   // #define is only for lint.
