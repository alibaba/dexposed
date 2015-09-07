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
 * Decode jar/apk/zip files.
 */
#ifndef DALVIK_JARFILE_H_
#define DALVIK_JARFILE_H_

/*
 * This represents an open, scanned Jar file.  (It's actually for any Zip
 * archive that happens to hold a Dex file.)
 */
struct JarFile {
    ZipArchive  archive;
    //MemMapping  map;
    char*       cacheFileName;
    DvmDex*     pDvmDex;
};

/*
 * Open the Zip archive and get a list of the classfile entries.
 *
 * On success, returns 0 and sets "*ppJarFile" to a newly-allocated JarFile.
 * On failure, returns a meaningful error code [currently just -1].
 */
int dvmJarFileOpen(const char* fileName, const char* odexOutputName,
    JarFile** ppJarFile, bool isBootstrap);

/*
 * Free a JarFile structure, along with any associated structures.
 */
void dvmJarFileFree(JarFile* pJarFile);

/* pry the DexFile out of a JarFile */
INLINE DvmDex* dvmGetJarFileDex(JarFile* pJarFile) {
    return pJarFile->pDvmDex;
}

/* get full path of optimized DEX file */
INLINE const char* dvmGetJarFileCacheFileName(JarFile* pJarFile) {
    return pJarFile->cacheFileName;
}

enum DexCacheStatus {
    DEX_CACHE_ERROR = -2,
    DEX_CACHE_BAD_ARCHIVE = -1,
    DEX_CACHE_OK = 0,
    DEX_CACHE_STALE,
    DEX_CACHE_STALE_ODEX,
};

/*
 * Checks the dependencies of the dex cache file corresponding
 * to the jar file at the absolute path "fileName".
 */
DexCacheStatus dvmDexCacheStatus(const char *fileName);

#endif  // DALVIK_JARFILE_H_
