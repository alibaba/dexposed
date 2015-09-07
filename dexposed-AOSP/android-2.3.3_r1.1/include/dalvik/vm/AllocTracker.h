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
 * Allocation tracking and reporting.
 */
#ifndef DALVIK_ALLOCTRACKER_H_
#define DALVIK_ALLOCTRACKER_H_

/* initialization */
bool dvmAllocTrackerStartup(void);
void dvmAllocTrackerShutdown(void);

struct AllocRecord;

/*
 * Enable allocation tracking.  Does nothing if tracking is already enabled.
 */
bool dvmEnableAllocTracker(void);

/*
 * Disable allocation tracking.  Does nothing if tracking is not enabled.
 */
void dvmDisableAllocTracker(void);

/*
 * If allocation tracking is enabled, add a new entry to the set.
 */
#define dvmTrackAllocation(_clazz, _size)                                   \
    {                                                                       \
        if (gDvm.allocRecords != NULL)                                      \
            dvmDoTrackAllocation(_clazz, _size);                            \
    }
void dvmDoTrackAllocation(ClassObject* clazz, size_t size);

/*
 * Generate a DDM packet with all of the tracked allocation data.
 *
 * On success, returns "true" with "*pData" and "*pDataLen" set.  "*pData"
 * refers to newly-allocated storage that must be freed by the caller.
 */
bool dvmGenerateTrackedAllocationReport(u1** pData, size_t* pDataLen);

/*
 * Dump the tracked allocations to the log file.  If "enable" is set, this
 * will enable tracking if it's not already on.
 */
void dvmDumpTrackedAllocations(bool enable);

#endif  // DALVIK_ALLOCTRACKER_H_
