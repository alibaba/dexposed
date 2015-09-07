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
 * Dalvik Debug Monitor
 */
#ifndef DALVIK_DDM_H_
#define DALVIK_DDM_H_

/*
 * Handle a packet full of DDM goodness.
 *
 * Returns "true" if we have anything to say in return; in which case,
 * "*pReplyBuf" and "*pReplyLen" will also be set.
 */
bool dvmDdmHandlePacket(const u1* buf, int dataLen, u1** pReplyBuf,
    int* pReplyLen);

/*
 * Deal with the DDM server connecting and disconnecting.
 */
void dvmDdmConnected(void);
void dvmDdmDisconnected(void);

/*
 * Turn thread notification on or off.
 */
void dvmDdmSetThreadNotification(bool enable);

/*
 * If thread start/stop notification is enabled, call this when threads
 * are created or die.
 */
void dvmDdmSendThreadNotification(Thread* thread, bool started);

/*
 * If thread start/stop notification is enabled, call this when the
 * thread name changes.
 */
void dvmDdmSendThreadNameChange(int threadId, StringObject* newName);

/*
 * Generate a byte[] full of thread stats for a THST packet.
 */
ArrayObject* dvmDdmGenerateThreadStats(void);

/*
 * Let the heap know that the HPIF when value has changed.
 *
 * @return true iff the when value is supported by the VM.
 */
bool dvmDdmHandleHpifChunk(int when);

/*
 * Let the heap know that the HPSG or NHSG what/when values have changed.
 *
 * @param native false for an HPSG chunk, true for an NHSG chunk
 *
 * @return true iff the what/when values are supported by the VM.
 */
bool dvmDdmHandleHpsgNhsgChunk(int when, int what, bool native);

/*
 * Get an array of StackTraceElement objects for the specified thread.
 */
ArrayObject* dvmDdmGetStackTraceById(u4 threadId);

/*
 * Gather up recent allocation data and return it in a byte[].
 *
 * Returns NULL on failure with an exception raised.
 */
ArrayObject* dvmDdmGetRecentAllocations(void);

#endif  // DALVIK_DDM_H_
