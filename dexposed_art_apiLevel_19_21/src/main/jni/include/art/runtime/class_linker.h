/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ART_RUNTIME_CLASS_LINKER_H_
#define ART_RUNTIME_CLASS_LINKER_H_

#include <string>
#include <utility>
#include <vector>

#include "base/allocator.h"
#include "base/macros.h"
#include "base/mutex.h"
#include "dex_file.h"
#include "gc_root.h"
#include "gtest/gtest.h"
#include "jni.h"
#include "oat_file.h"
#include "object_callbacks.h"

namespace art {

namespace gc {
namespace space {
  class ImageSpace;
}  // namespace space
}  // namespace gc
namespace mirror {
  class ClassLoader;
  class DexCache;
  class DexCacheTest_Open_Test;
  class IfTable;
  template<class T> class ObjectArray;
  class StackTraceElement;
}  // namespace mirror

class InternTable;
template<class T> class ObjectLock;
class ScopedObjectAccessAlreadyRunnable;
template<class T> class Handle;

typedef bool (ClassVisitor)(mirror::Class* c, void* arg);

enum VisitRootFlags : uint8_t;

class ClassLinker {
 public:
  explicit ClassLinker(InternTable* intern_table);
  ~ClassLinker();

  // Initialize class linker by bootstraping from dex files.
  void InitWithoutImage(const std::vector<const DexFile*>& boot_class_path)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Initialize class linker from one or more images.
  void InitFromImage() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Finds a class by its descriptor, loading it if necessary.
  // If class_loader is null, searches boot_class_path_.
  mirror::Class* FindClass(Thread* self, const char* descriptor,
                           Handle<mirror::ClassLoader> class_loader)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Find a class in the path class loader, loading it if necessary.
  mirror::Class* FindClassInPathClassLoader(ScopedObjectAccessAlreadyRunnable& soa,
                                            Thread* self, const char* descriptor,
                                            Handle<mirror::ClassLoader> class_loader)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Finds a class by its descriptor using the "system" class loader, ie by searching the
  // boot_class_path_.
  mirror::Class* FindSystemClass(Thread* self, const char* descriptor)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Finds the array class given for the element class.
  mirror::Class* FindArrayClass(Thread* self, mirror::Class** element_class)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Returns true if the class linker is initialized.
  bool IsInitialized() const;

  // Define a new a class based on a ClassDef from a DexFile
  mirror::Class* DefineClass(const char* descriptor,
                             Handle<mirror::ClassLoader> class_loader,
                             const DexFile& dex_file, const DexFile::ClassDef& dex_class_def)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Finds a class by its descriptor, returning NULL if it isn't wasn't loaded
  // by the given 'class_loader'.
  mirror::Class* LookupClass(const char* descriptor, const mirror::ClassLoader* class_loader)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Finds all the classes with the given descriptor, regardless of ClassLoader.
  void LookupClasses(const char* descriptor, std::vector<mirror::Class*>& classes)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Class* FindPrimitiveClass(char type) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // General class unloading is not supported, this is used to prune
  // unwanted classes during image writing.
  bool RemoveClass(const char* descriptor, const mirror::ClassLoader* class_loader)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void DumpAllClasses(int flags)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void DumpForSigQuit(std::ostream& os)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  size_t NumLoadedClasses()
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Resolve a String with the given index from the DexFile, storing the
  // result in the DexCache. The referrer is used to identify the
  // target DexCache and ClassLoader to use for resolution.
  mirror::String* ResolveString(uint32_t string_idx, mirror::ArtMethod* referrer)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Resolve a String with the given index from the DexFile, storing the
  // result in the DexCache.
  mirror::String* ResolveString(const DexFile& dex_file, uint32_t string_idx,
                                Handle<mirror::DexCache> dex_cache)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Resolve a Type with the given index from the DexFile, storing the
  // result in the DexCache. The referrer is used to identity the
  // target DexCache and ClassLoader to use for resolution.
  mirror::Class* ResolveType(const DexFile& dex_file, uint16_t type_idx, mirror::Class* referrer)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Resolve a Type with the given index from the DexFile, storing the
  // result in the DexCache. The referrer is used to identify the
  // target DexCache and ClassLoader to use for resolution.
  mirror::Class* ResolveType(uint16_t type_idx, mirror::ArtMethod* referrer)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Class* ResolveType(uint16_t type_idx, mirror::ArtField* referrer)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Resolve a type with the given ID from the DexFile, storing the
  // result in DexCache. The ClassLoader is used to search for the
  // type, since it may be referenced from but not contained within
  // the given DexFile.
  mirror::Class* ResolveType(const DexFile& dex_file, uint16_t type_idx,
                             Handle<mirror::DexCache> dex_cache,
                             Handle<mirror::ClassLoader> class_loader)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Resolve a method with a given ID from the DexFile, storing the
  // result in DexCache. The ClassLinker and ClassLoader are used as
  // in ResolveType. What is unique is the method type argument which
  // is used to determine if this method is a direct, static, or
  // virtual method.
  mirror::ArtMethod* ResolveMethod(const DexFile& dex_file,
                                   uint32_t method_idx,
                                   Handle<mirror::DexCache> dex_cache,
                                   Handle<mirror::ClassLoader> class_loader,
                                   Handle<mirror::ArtMethod> referrer,
                                   InvokeType type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ArtMethod* GetResolvedMethod(uint32_t method_idx, mirror::ArtMethod* referrer,
                                       InvokeType type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::ArtMethod* ResolveMethod(Thread* self, uint32_t method_idx, mirror::ArtMethod** referrer,
                                   InvokeType type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ArtField* GetResolvedField(uint32_t field_idx, mirror::Class* field_declaring_class)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::ArtField* ResolveField(uint32_t field_idx, mirror::ArtMethod* referrer,
                                 bool is_static)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Resolve a field with a given ID from the DexFile, storing the
  // result in DexCache. The ClassLinker and ClassLoader are used as
  // in ResolveType. What is unique is the is_static argument which is
  // used to determine if we are resolving a static or non-static
  // field.
  mirror::ArtField* ResolveField(const DexFile& dex_file,
                                 uint32_t field_idx,
                                 Handle<mirror::DexCache> dex_cache,
                                 Handle<mirror::ClassLoader> class_loader,
                                 bool is_static)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Resolve a field with a given ID from the DexFile, storing the
  // result in DexCache. The ClassLinker and ClassLoader are used as
  // in ResolveType. No is_static argument is provided so that Java
  // field resolution semantics are followed.
  mirror::ArtField* ResolveFieldJLS(const DexFile& dex_file, uint32_t field_idx,
                                    Handle<mirror::DexCache> dex_cache,
                                    Handle<mirror::ClassLoader> class_loader)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Get shorty from method index without resolution. Used to do handlerization.
  const char* MethodShorty(uint32_t method_idx, mirror::ArtMethod* referrer, uint32_t* length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Returns true on success, false if there's an exception pending.
  // can_run_clinit=false allows the compiler to attempt to init a class,
  // given the restriction that no <clinit> execution is possible.
  bool EnsureInitialized(Handle<mirror::Class> c, bool can_init_fields, bool can_init_parents)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Initializes classes that have instances in the image but that have
  // <clinit> methods so they could not be initialized by the compiler.
  void RunRootClinits() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void RegisterDexFile(const DexFile& dex_file)
      LOCKS_EXCLUDED(dex_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void RegisterDexFile(const DexFile& dex_file, Handle<mirror::DexCache> dex_cache)
      LOCKS_EXCLUDED(dex_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  const OatFile* RegisterOatFile(const OatFile* oat_file)
      LOCKS_EXCLUDED(dex_lock_);

  const std::vector<const DexFile*>& GetBootClassPath() {
    return boot_class_path_;
  }

  void VisitClasses(ClassVisitor* visitor, void* arg)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Less efficient variant of VisitClasses that copies the class_table_ into secondary storage
  // so that it can visit individual classes without holding the doesn't hold the
  // Locks::classlinker_classes_lock_. As the Locks::classlinker_classes_lock_ isn't held this code
  // can race with insertion and deletion of classes while the visitor is being called.
  void VisitClassesWithoutClassesLock(ClassVisitor* visitor, void* arg)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void VisitClassRoots(RootCallback* callback, void* arg, VisitRootFlags flags)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void VisitRoots(RootCallback* callback, void* arg, VisitRootFlags flags)
      LOCKS_EXCLUDED(dex_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::DexCache* FindDexCache(const DexFile& dex_file)
      LOCKS_EXCLUDED(dex_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool IsDexFileRegistered(const DexFile& dex_file)
      LOCKS_EXCLUDED(dex_lock_) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void FixupDexCaches(mirror::ArtMethod* resolution_method)
      LOCKS_EXCLUDED(dex_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Generate an oat file from a dex file
  bool GenerateOatFile(const char* dex_filename,
                       int oat_fd,
                       const char* oat_cache_filename,
                       std::string* error_msg)
      LOCKS_EXCLUDED(Locks::mutator_lock_);

  // Find or create the oat file holding dex_location. Then load all corresponding dex files
  // (if multidex) into the given vector.
  bool OpenDexFilesFromOat(const char* dex_location, const char* oat_location,
                           std::vector<std::string>* error_msgs,
                           std::vector<const DexFile*>* dex_files)
      LOCKS_EXCLUDED(dex_lock_, Locks::mutator_lock_);

  // Returns true if the given oat file has the same image checksum as the image it is paired with.
  static bool VerifyOatImageChecksum(const OatFile* oat_file, const InstructionSet instruction_set);
  // Returns true if the oat file checksums match with the image and the offsets are such that it
  // could be loaded with it.
  static bool VerifyOatChecksums(const OatFile* oat_file, const InstructionSet instruction_set,
                                 std::string* error_msg);
  // Returns true if oat file contains the dex file with the given location and checksum.
  static bool VerifyOatAndDexFileChecksums(const OatFile* oat_file,
                                           const char* dex_location,
                                           uint32_t dex_location_checksum,
                                           InstructionSet instruction_set,
                                           std::string* error_msg);

  // TODO: replace this with multiple methods that allocate the correct managed type.
  template <class T>
  mirror::ObjectArray<T>* AllocObjectArray(Thread* self, size_t length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ObjectArray<mirror::Class>* AllocClassArray(Thread* self, size_t length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ObjectArray<mirror::String>* AllocStringArray(Thread* self, size_t length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ObjectArray<mirror::ArtMethod>* AllocArtMethodArray(Thread* self, size_t length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::IfTable* AllocIfTable(Thread* self, size_t ifcount)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ObjectArray<mirror::ArtField>* AllocArtFieldArray(Thread* self, size_t length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ObjectArray<mirror::StackTraceElement>* AllocStackTraceElementArray(Thread* self,
                                                                              size_t length)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void VerifyClass(Handle<mirror::Class> klass) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool VerifyClassUsingOatFile(const DexFile& dex_file, mirror::Class* klass,
                               mirror::Class::Status& oat_file_class_status)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void ResolveClassExceptionHandlerTypes(const DexFile& dex_file,
                                         Handle<mirror::Class> klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void ResolveMethodExceptionHandlerTypes(const DexFile& dex_file, mirror::ArtMethod* klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Class* CreateProxyClass(ScopedObjectAccessAlreadyRunnable& soa, jstring name,
                                  jobjectArray interfaces, jobject loader, jobjectArray methods,
                                  jobjectArray throws)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  std::string GetDescriptorForProxy(mirror::Class* proxy_class)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::ArtMethod* FindMethodForProxy(mirror::Class* proxy_class,
                                        mirror::ArtMethod* proxy_method)
      LOCKS_EXCLUDED(dex_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Get the oat code for a method when its class isn't yet initialized
  const void* GetQuickOatCodeFor(mirror::ArtMethod* method)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
#if defined(ART_USE_PORTABLE_COMPILER)
  const void* GetPortableOatCodeFor(mirror::ArtMethod* method, bool* have_portable_code)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
#endif

  // Get the oat code for a method from a method index.
  const void* GetQuickOatCodeFor(const DexFile& dex_file, uint16_t class_def_idx, uint32_t method_idx)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
#if defined(ART_USE_PORTABLE_COMPILER)
  const void* GetPortableOatCodeFor(const DexFile& dex_file, uint16_t class_def_idx, uint32_t method_idx)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
#endif

  pid_t GetClassesLockOwner();  // For SignalCatcher.
  pid_t GetDexLockOwner();  // For SignalCatcher.

  const void* GetPortableResolutionTrampoline() const {
    return portable_resolution_trampoline_;
  }

  const void* GetQuickGenericJniTrampoline() const {
    return quick_generic_jni_trampoline_;
  }

  const void* GetQuickResolutionTrampoline() const {
    return quick_resolution_trampoline_;
  }

  const void* GetPortableImtConflictTrampoline() const {
    return portable_imt_conflict_trampoline_;
  }

  const void* GetQuickImtConflictTrampoline() const {
    return quick_imt_conflict_trampoline_;
  }

  const void* GetQuickToInterpreterBridgeTrampoline() const {
    return quick_to_interpreter_bridge_trampoline_;
  }

  InternTable* GetInternTable() const {
    return intern_table_;
  }

  // Attempts to insert a class into a class table.  Returns NULL if
  // the class was inserted, otherwise returns an existing class with
  // the same descriptor and ClassLoader.
  mirror::Class* InsertClass(const char* descriptor, mirror::Class* klass, size_t hash)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Special code to allocate an art method, use this instead of class->AllocObject.
  mirror::ArtMethod* AllocArtMethod(Thread* self) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ObjectArray<mirror::Class>* GetClassRoots() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) {
    mirror::ObjectArray<mirror::Class>* class_roots = class_roots_.Read();
    DCHECK(class_roots != NULL);
    return class_roots;
  }

 private:
  bool FindOatMethodFor(mirror::ArtMethod* method, OatFile::OatMethod* oat_method)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  OatFile& GetImageOatFile(gc::space::ImageSpace* space)
      LOCKS_EXCLUDED(dex_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void FinishInit(Thread* self) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // For early bootstrapping by Init
  mirror::Class* AllocClass(Thread* self, mirror::Class* java_lang_Class, uint32_t class_size)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Alloc* convenience functions to avoid needing to pass in mirror::Class*
  // values that are known to the ClassLinker such as
  // kObjectArrayClass and kJavaLangString etc.
  mirror::Class* AllocClass(Thread* self, uint32_t class_size)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::DexCache* AllocDexCache(Thread* self, const DexFile& dex_file)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::ArtField* AllocArtField(Thread* self) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Class* CreatePrimitiveClass(Thread* self, Primitive::Type type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::Class* InitializePrimitiveClass(mirror::Class* primitive_class, Primitive::Type type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);


  mirror::Class* CreateArrayClass(Thread* self, const char* descriptor,
                                  Handle<mirror::ClassLoader> class_loader)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void AppendToBootClassPath(const DexFile& dex_file)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void AppendToBootClassPath(const DexFile& dex_file, Handle<mirror::DexCache> dex_cache)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void ConstructFieldMap(const DexFile& dex_file, const DexFile::ClassDef& dex_class_def,
                         mirror::Class* c, SafeMap<uint32_t, mirror::ArtField*>& field_map)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Precomputes size needed for Class, in the case of a non-temporary class this size must be
  // sufficient to hold all static fields.
  uint32_t SizeOfClassWithoutEmbeddedTables(const DexFile& dex_file,
                                            const DexFile::ClassDef& dex_class_def);

  void LoadClass(const DexFile& dex_file,
                 const DexFile::ClassDef& dex_class_def,
                 Handle<mirror::Class> klass,
                 mirror::ClassLoader* class_loader)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void LoadClassMembers(const DexFile& dex_file,
                        const byte* class_data,
                        Handle<mirror::Class> klass,
                        mirror::ClassLoader* class_loader,
                        const OatFile::OatClass* oat_class)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void LoadField(const DexFile& dex_file, const ClassDataItemIterator& it,
                 Handle<mirror::Class> klass, Handle<mirror::ArtField> dst)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::ArtMethod* LoadMethod(Thread* self, const DexFile& dex_file,
                                const ClassDataItemIterator& dex_method,
                                Handle<mirror::Class> klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void FixupStaticTrampolines(mirror::Class* klass) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Finds the associated oat class for a dex_file and descriptor. Returns whether the class
  // was found, and sets the data in oat_class.
  bool FindOatClass(const DexFile& dex_file, uint16_t class_def_idx, OatFile::OatClass* oat_class)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void RegisterDexFileLocked(const DexFile& dex_file, Handle<mirror::DexCache> dex_cache)
      EXCLUSIVE_LOCKS_REQUIRED(dex_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool IsDexFileRegisteredLocked(const DexFile& dex_file)
      SHARED_LOCKS_REQUIRED(dex_lock_, Locks::mutator_lock_);

  bool InitializeClass(Handle<mirror::Class> klass, bool can_run_clinit,
                       bool can_init_parents)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool WaitForInitializeClass(Handle<mirror::Class> klass, Thread* self,
                              ObjectLock<mirror::Class>& lock);
  bool ValidateSuperClassDescriptors(Handle<mirror::Class> klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool IsSameDescriptorInDifferentClassContexts(Thread* self, const char* descriptor,
                                                Handle<mirror::ClassLoader> class_loader1,
                                                Handle<mirror::ClassLoader> class_loader2)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool IsSameMethodSignatureInDifferentClassContexts(Thread* self, mirror::ArtMethod* method,
                                                     mirror::Class* klass1,
                                                     mirror::Class* klass2)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool LinkClass(Thread* self, const char* descriptor, Handle<mirror::Class> klass,
                 Handle<mirror::ObjectArray<mirror::Class>> interfaces,
                 mirror::Class** new_class)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool LinkSuperClass(Handle<mirror::Class> klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool LoadSuperAndInterfaces(Handle<mirror::Class> klass, const DexFile& dex_file)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool LinkMethods(Thread* self, Handle<mirror::Class> klass,
                   Handle<mirror::ObjectArray<mirror::Class>> interfaces)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool LinkVirtualMethods(Thread* self, Handle<mirror::Class> klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool LinkInterfaceMethods(Handle<mirror::Class> klass,
                            Handle<mirror::ObjectArray<mirror::Class>> interfaces)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool LinkStaticFields(Handle<mirror::Class> klass, size_t* class_size)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool LinkInstanceFields(Handle<mirror::Class> klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  bool LinkFields(Handle<mirror::Class> klass, bool is_static, size_t* class_size)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void LinkCode(Handle<mirror::ArtMethod> method, const OatFile::OatClass* oat_class,
                const DexFile& dex_file, uint32_t dex_method_index, uint32_t method_index)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void CreateReferenceInstanceOffsets(Handle<mirror::Class> klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void CreateReferenceStaticOffsets(Handle<mirror::Class> klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  void CreateReferenceOffsets(Handle<mirror::Class> klass, bool is_static,
                              uint32_t reference_offsets)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // For use by ImageWriter to find DexCaches for its roots
  ReaderWriterMutex* DexLock()
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_) LOCK_RETURNED(dex_lock_) {
    return &dex_lock_;
  }
  size_t GetDexCacheCount() SHARED_LOCKS_REQUIRED(Locks::mutator_lock_, dex_lock_) {
    return dex_caches_.size();
  }
  mirror::DexCache* GetDexCache(size_t idx) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_, dex_lock_);

  const OatFile::OatDexFile* FindOpenedOatDexFileForDexFile(const DexFile& dex_file)
      LOCKS_EXCLUDED(dex_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Find an opened oat dex file that contains dex_location. If oat_location is not nullptr,
  // the file must have that location, else any oat location is accepted.
  const OatFile::OatDexFile* FindOpenedOatDexFile(const char* oat_location,
                                                  const char* dex_location,
                                                  const uint32_t* dex_location_checksum)
      LOCKS_EXCLUDED(dex_lock_);

  // Will open the oat file directly without relocating, even if we could/should do relocation.
  const OatFile* FindOatFileFromOatLocation(const std::string& oat_location,
                                            std::string* error_msg)
      LOCKS_EXCLUDED(dex_lock_);

  const OatFile* FindOpenedOatFileFromOatLocation(const std::string& oat_location)
      LOCKS_EXCLUDED(dex_lock_);

  const OatFile* OpenOatFileFromDexLocation(const std::string& dex_location,
                                            InstructionSet isa,
                                            bool* already_opened,
                                            bool* obsolete_file_cleanup_failed,
                                            std::vector<std::string>* error_msg)
      LOCKS_EXCLUDED(dex_lock_, Locks::mutator_lock_);

  const OatFile* GetInterpretedOnlyOat(const std::string& oat_path,
                                       InstructionSet isa,
                                       std::string* error_msg);

  const OatFile* PatchAndRetrieveOat(const std::string& input, const std::string& output,
                                     const std::string& image_location, InstructionSet isa,
                                     std::string* error_msg)
      LOCKS_EXCLUDED(Locks::mutator_lock_);

  bool CheckOatFile(const OatFile* oat_file, InstructionSet isa,
                    bool* checksum_verified, std::string* error_msg);
  int32_t GetRequiredDelta(const OatFile* oat_file, InstructionSet isa);

  // Note: will not register the oat file.
  const OatFile* FindOatFileInOatLocationForDexFile(const char* dex_location,
                                                    uint32_t dex_location_checksum,
                                                    const char* oat_location,
                                                    std::string* error_msg)
      LOCKS_EXCLUDED(dex_lock_);

  // Creates the oat file from the dex_location to the oat_location. Needs a file descriptor for
  // the file to be written, which is assumed to be under a lock.
  const OatFile* CreateOatFileForDexLocation(const char* dex_location,
                                             int fd, const char* oat_location,
                                             std::vector<std::string>* error_msgs)
      LOCKS_EXCLUDED(dex_lock_, Locks::mutator_lock_);

  // Finds an OatFile that contains a DexFile for the given a DexFile location.
  //
  // Note 1: this will not check open oat files, which are assumed to be stale when this is run.
  // Note 2: Does not register the oat file. It is the caller's job to register if the file is to
  //         be kept.
  const OatFile* FindOatFileContainingDexFileFromDexLocation(const char* dex_location,
                                                             const uint32_t* dex_location_checksum,
                                                             InstructionSet isa,
                                                             std::vector<std::string>* error_msgs,
                                                             bool* obsolete_file_cleanup_failed)
      LOCKS_EXCLUDED(dex_lock_, Locks::mutator_lock_);

  // Verifies:
  //  - that the oat file contains the dex file (with a matching checksum, which may be null if the
  // file was pre-opted)
  //  - the checksums of the oat file (against the image space)
  //  - the checksum of the dex file against dex_location_checksum
  //  - that the dex file can be opened
  // Returns true iff all verification succeed.
  //
  // The dex_location is the dex location as stored in the oat file header.
  // (see DexFile::GetDexCanonicalLocation for a description of location conventions)
  bool VerifyOatWithDexFile(const OatFile* oat_file, const char* dex_location,
                            const uint32_t* dex_location_checksum,
                            std::string* error_msg);

  mirror::ArtMethod* CreateProxyConstructor(Thread* self, Handle<mirror::Class> klass,
                                            mirror::Class* proxy_class)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::ArtMethod* CreateProxyMethod(Thread* self, Handle<mirror::Class> klass,
                                       Handle<mirror::ArtMethod> prototype)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // Ensures that methods have the kAccPreverified bit set. We use the kAccPreverfied bit on the
  // class access flags to determine whether this has been done before.
  void EnsurePreverifiedMethods(Handle<mirror::Class> c)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  mirror::Class* LookupClassFromTableLocked(const char* descriptor,
                                            const mirror::ClassLoader* class_loader,
                                            size_t hash)
      SHARED_LOCKS_REQUIRED(Locks::classlinker_classes_lock_, Locks::mutator_lock_);

  mirror::Class* UpdateClass(const char* descriptor, mirror::Class* klass, size_t hash)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void MoveImageClassesToClassTable() LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      LOCKS_EXCLUDED(Locks::classlinker_classes_lock_)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::Class* LookupClassFromImage(const char* descriptor)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  // EnsureResolved is called to make sure that a class in the class_table_ has been resolved
  // before returning it to the caller. Its the responsibility of the thread that placed the class
  // in the table to make it resolved. The thread doing resolution must notify on the class' lock
  // when resolution has occurred. This happens in mirror::Class::SetStatus. As resolution may
  // retire a class, the version of the class in the table is returned and this may differ from
  // the class passed in.
  mirror::Class* EnsureResolved(Thread* self, const char* descriptor, mirror::Class* klass)
      WARN_UNUSED SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void FixupTemporaryDeclaringClass(mirror::Class* temp_class, mirror::Class* new_class)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  std::vector<const DexFile*> boot_class_path_;

  mutable ReaderWriterMutex dex_lock_ DEFAULT_MUTEX_ACQUIRED_AFTER;
  std::vector<size_t> new_dex_cache_roots_ GUARDED_BY(dex_lock_);;
  std::vector<GcRoot<mirror::DexCache>> dex_caches_ GUARDED_BY(dex_lock_);
  std::vector<const OatFile*> oat_files_ GUARDED_BY(dex_lock_);


  // multimap from a string hash code of a class descriptor to
  // mirror::Class* instances. Results should be compared for a matching
  // Class::descriptor_ and Class::class_loader_.
  typedef AllocationTrackingMultiMap<size_t, GcRoot<mirror::Class>, kAllocatorTagClassTable> Table;
  // This contains strong roots. To enable concurrent root scanning of
  // the class table, be careful to use a read barrier when accessing this.
  Table class_table_ GUARDED_BY(Locks::classlinker_classes_lock_);
  std::vector<std::pair<size_t, GcRoot<mirror::Class>>> new_class_roots_;

  // Do we need to search dex caches to find image classes?
  bool dex_cache_image_class_lookup_required_;
  // Number of times we've searched dex caches for a class. After a certain number of misses we move
  // the classes into the class_table_ to avoid dex cache based searches.
  Atomic<uint32_t> failed_dex_cache_class_lookups_;

  // indexes into class_roots_.
  // needs to be kept in sync with class_roots_descriptors_.
  enum ClassRoot {
    kJavaLangClass,
    kJavaLangObject,
    kClassArrayClass,
    kObjectArrayClass,
    kJavaLangString,
    kJavaLangDexCache,
    kJavaLangRefReference,
    kJavaLangReflectArtField,
    kJavaLangReflectArtMethod,
    kJavaLangReflectProxy,
    kJavaLangStringArrayClass,
    kJavaLangReflectArtFieldArrayClass,
    kJavaLangReflectArtMethodArrayClass,
    kJavaLangClassLoader,
    kJavaLangThrowable,
    kJavaLangClassNotFoundException,
    kJavaLangStackTraceElement,
    kPrimitiveBoolean,
    kPrimitiveByte,
    kPrimitiveChar,
    kPrimitiveDouble,
    kPrimitiveFloat,
    kPrimitiveInt,
    kPrimitiveLong,
    kPrimitiveShort,
    kPrimitiveVoid,
    kBooleanArrayClass,
    kByteArrayClass,
    kCharArrayClass,
    kDoubleArrayClass,
    kFloatArrayClass,
    kIntArrayClass,
    kLongArrayClass,
    kShortArrayClass,
    kJavaLangStackTraceElementArrayClass,
    kClassRootsMax,
  };
  GcRoot<mirror::ObjectArray<mirror::Class>> class_roots_;

  mirror::Class* GetClassRoot(ClassRoot class_root) SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void SetClassRoot(ClassRoot class_root, mirror::Class* klass)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  static const char* class_roots_descriptors_[];

  const char* GetClassRootDescriptor(ClassRoot class_root) {
    const char* descriptor = class_roots_descriptors_[class_root];
    CHECK(descriptor != NULL);
    return descriptor;
  }

  // The interface table used by all arrays.
  GcRoot<mirror::IfTable> array_iftable_;

  // A cache of the last FindArrayClass results. The cache serves to avoid creating array class
  // descriptors for the sake of performing FindClass.
  static constexpr size_t kFindArrayCacheSize = 16;
  GcRoot<mirror::Class> find_array_class_cache_[kFindArrayCacheSize];
  size_t find_array_class_cache_next_victim_;

  bool init_done_;
  bool log_new_dex_caches_roots_ GUARDED_BY(dex_lock_);
  bool log_new_class_table_roots_ GUARDED_BY(Locks::classlinker_classes_lock_);

  InternTable* intern_table_;

  // Trampolines within the image the bounce to runtime entrypoints. Done so that there is a single
  // patch point within the image. TODO: make these proper relocations.
  const void* portable_resolution_trampoline_;
  const void* quick_resolution_trampoline_;
  const void* portable_imt_conflict_trampoline_;
  const void* quick_imt_conflict_trampoline_;
  const void* quick_generic_jni_trampoline_;
  const void* quick_to_interpreter_bridge_trampoline_;

  friend class ImageWriter;  // for GetClassRoots
  friend class ImageDumper;  // for FindOpenedOatFileFromOatLocation
  friend class ElfPatcher;  // for FindOpenedOatFileForDexFile & FindOpenedOatFileFromOatLocation
  friend class NoDex2OatTest;  // for FindOpenedOatFileForDexFile
  friend class NoPatchoatTest;  // for FindOpenedOatFileForDexFile
  FRIEND_TEST(ClassLinkerTest, ClassRootDescriptors);
  FRIEND_TEST(mirror::DexCacheTest, Open);
  FRIEND_TEST(ExceptionTest, FindExceptionHandler);
  FRIEND_TEST(ObjectTest, AllocObjectArray);
  DISALLOW_COPY_AND_ASSIGN(ClassLinker);
};

}  // namespace art

#endif  // ART_RUNTIME_CLASS_LINKER_H_
