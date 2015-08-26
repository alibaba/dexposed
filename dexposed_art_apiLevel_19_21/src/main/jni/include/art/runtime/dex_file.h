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

#ifndef ART_RUNTIME_DEX_FILE_H_
#define ART_RUNTIME_DEX_FILE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/logging.h"
#include "base/mutex.h"  // For Locks::mutator_lock_.
#include "globals.h"
#include "invoke_type.h"
#include "jni.h"
#include "modifiers.h"
#include "utf.h"

namespace art {

// TODO: remove dependencies on mirror classes, primarily by moving
// EncodedStaticFieldValueIterator to its own file.
namespace mirror {
  class ArtField;
  class ArtMethod;
  class ClassLoader;
  class DexCache;
}  // namespace mirror
class ClassLinker;
class MemMap;
class Signature;
template<class T> class Handle;
class StringPiece;
class ZipArchive;

// TODO: move all of the macro functionality into the DexCache class.
class DexFile {
 public:
  static const byte kDexMagic[];
  static const byte kDexMagicVersion[];
  static const size_t kSha1DigestSize = 20;
  static const uint32_t kDexEndianConstant = 0x12345678;

  // name of the DexFile entry within a zip archive
  static const char* kClassesDex;

  // The value of an invalid index.
  static const uint32_t kDexNoIndex = 0xFFFFFFFF;

  // The value of an invalid index.
  static const uint16_t kDexNoIndex16 = 0xFFFF;

  // The separator charactor in MultiDex locations.
  static constexpr char kMultiDexSeparator = ':';

  // A string version of the previous. This is a define so that we can merge string literals in the
  // preprocessor.
  #define kMultiDexSeparatorString ":"

  // Raw header_item.
  struct Header {
    uint8_t magic_[8];
    uint32_t checksum_;  // See also location_checksum_
    uint8_t signature_[kSha1DigestSize];
    uint32_t file_size_;  // size of entire file
    uint32_t header_size_;  // offset to start of next section
    uint32_t endian_tag_;
    uint32_t link_size_;  // unused
    uint32_t link_off_;  // unused
    uint32_t map_off_;  // unused
    uint32_t string_ids_size_;  // number of StringIds
    uint32_t string_ids_off_;  // file offset of StringIds array
    uint32_t type_ids_size_;  // number of TypeIds, we don't support more than 65535
    uint32_t type_ids_off_;  // file offset of TypeIds array
    uint32_t proto_ids_size_;  // number of ProtoIds, we don't support more than 65535
    uint32_t proto_ids_off_;  // file offset of ProtoIds array
    uint32_t field_ids_size_;  // number of FieldIds
    uint32_t field_ids_off_;  // file offset of FieldIds array
    uint32_t method_ids_size_;  // number of MethodIds
    uint32_t method_ids_off_;  // file offset of MethodIds array
    uint32_t class_defs_size_;  // number of ClassDefs
    uint32_t class_defs_off_;  // file offset of ClassDef array
    uint32_t data_size_;  // unused
    uint32_t data_off_;  // unused

   private:
    DISALLOW_COPY_AND_ASSIGN(Header);
  };

  // Map item type codes.
  enum {
    kDexTypeHeaderItem               = 0x0000,
    kDexTypeStringIdItem             = 0x0001,
    kDexTypeTypeIdItem               = 0x0002,
    kDexTypeProtoIdItem              = 0x0003,
    kDexTypeFieldIdItem              = 0x0004,
    kDexTypeMethodIdItem             = 0x0005,
    kDexTypeClassDefItem             = 0x0006,
    kDexTypeMapList                  = 0x1000,
    kDexTypeTypeList                 = 0x1001,
    kDexTypeAnnotationSetRefList     = 0x1002,
    kDexTypeAnnotationSetItem        = 0x1003,
    kDexTypeClassDataItem            = 0x2000,
    kDexTypeCodeItem                 = 0x2001,
    kDexTypeStringDataItem           = 0x2002,
    kDexTypeDebugInfoItem            = 0x2003,
    kDexTypeAnnotationItem           = 0x2004,
    kDexTypeEncodedArrayItem         = 0x2005,
    kDexTypeAnnotationsDirectoryItem = 0x2006,
  };

  struct MapItem {
    uint16_t type_;
    uint16_t unused_;
    uint32_t size_;
    uint32_t offset_;

   private:
    DISALLOW_COPY_AND_ASSIGN(MapItem);
  };

  struct MapList {
    uint32_t size_;
    MapItem list_[1];

   private:
    DISALLOW_COPY_AND_ASSIGN(MapList);
  };

  // Raw string_id_item.
  struct StringId {
    uint32_t string_data_off_;  // offset in bytes from the base address

   private:
    DISALLOW_COPY_AND_ASSIGN(StringId);
  };

  // Raw type_id_item.
  struct TypeId {
    uint32_t descriptor_idx_;  // index into string_ids

   private:
    DISALLOW_COPY_AND_ASSIGN(TypeId);
  };

  // Raw field_id_item.
  struct FieldId {
    uint16_t class_idx_;  // index into type_ids_ array for defining class
    uint16_t type_idx_;  // index into type_ids_ array for field type
    uint32_t name_idx_;  // index into string_ids_ array for field name

   private:
    DISALLOW_COPY_AND_ASSIGN(FieldId);
  };

  // Raw method_id_item.
  struct MethodId {
    uint16_t class_idx_;  // index into type_ids_ array for defining class
    uint16_t proto_idx_;  // index into proto_ids_ array for method prototype
    uint32_t name_idx_;  // index into string_ids_ array for method name

   private:
    DISALLOW_COPY_AND_ASSIGN(MethodId);
  };

  // Raw proto_id_item.
  struct ProtoId {
    uint32_t shorty_idx_;  // index into string_ids array for shorty descriptor
    uint16_t return_type_idx_;  // index into type_ids array for return type
    uint16_t pad_;             // padding = 0
    uint32_t parameters_off_;  // file offset to type_list for parameter types

   private:
    DISALLOW_COPY_AND_ASSIGN(ProtoId);
  };

  // Raw class_def_item.
  struct ClassDef {
    uint16_t class_idx_;  // index into type_ids_ array for this class
    uint16_t pad1_;  // padding = 0
    uint32_t access_flags_;
    uint16_t superclass_idx_;  // index into type_ids_ array for superclass
    uint16_t pad2_;  // padding = 0
    uint32_t interfaces_off_;  // file offset to TypeList
    uint32_t source_file_idx_;  // index into string_ids_ for source file name
    uint32_t annotations_off_;  // file offset to annotations_directory_item
    uint32_t class_data_off_;  // file offset to class_data_item
    uint32_t static_values_off_;  // file offset to EncodedArray

    // Returns the valid access flags, that is, Java modifier bits relevant to the ClassDef type
    // (class or interface). These are all in the lower 16b and do not contain runtime flags.
    uint32_t GetJavaAccessFlags() const {
      // Make sure that none of our runtime-only flags are set.
      COMPILE_ASSERT((kAccValidClassFlags & kAccJavaFlagsMask) == kAccValidClassFlags,
                     valid_class_flags_not_subset_of_java_flags);
      COMPILE_ASSERT((kAccValidInterfaceFlags & kAccJavaFlagsMask) == kAccValidInterfaceFlags,
                     valid_interface_flags_not_subset_of_java_flags);

      if ((access_flags_ & kAccInterface) != 0) {
        // Interface.
        return access_flags_ & kAccValidInterfaceFlags;
      } else {
        // Class.
        return access_flags_ & kAccValidClassFlags;
      }
    }

   private:
    DISALLOW_COPY_AND_ASSIGN(ClassDef);
  };

  // Raw type_item.
  struct TypeItem {
    uint16_t type_idx_;  // index into type_ids section

   private:
    DISALLOW_COPY_AND_ASSIGN(TypeItem);
  };

  // Raw type_list.
  class TypeList {
   public:
    uint32_t Size() const {
      return size_;
    }

    const TypeItem& GetTypeItem(uint32_t idx) const {
      DCHECK_LT(idx, this->size_);
      return this->list_[idx];
    }

    // Size in bytes of the part of the list that is common.
    static constexpr size_t GetHeaderSize() {
      return 4U;
    }

    // Size in bytes of the whole type list including all the stored elements.
    static constexpr size_t GetListSize(size_t count) {
      return GetHeaderSize() + sizeof(TypeItem) * count;
    }

   private:
    uint32_t size_;  // size of the list, in entries
    TypeItem list_[1];  // elements of the list
    DISALLOW_COPY_AND_ASSIGN(TypeList);
  };

  // Raw code_item.
  struct CodeItem {
    uint16_t registers_size_;
    uint16_t ins_size_;
    uint16_t outs_size_;
    uint16_t tries_size_;
    uint32_t debug_info_off_;  // file offset to debug info stream
    uint32_t insns_size_in_code_units_;  // size of the insns array, in 2 byte code units
    uint16_t insns_[1];

   private:
    DISALLOW_COPY_AND_ASSIGN(CodeItem);
  };

  // Raw try_item.
  struct TryItem {
    uint32_t start_addr_;
    uint16_t insn_count_;
    uint16_t handler_off_;

   private:
    DISALLOW_COPY_AND_ASSIGN(TryItem);
  };

  // Annotation constants.
  enum {
    kDexVisibilityBuild         = 0x00,     /* annotation visibility */
    kDexVisibilityRuntime       = 0x01,
    kDexVisibilitySystem        = 0x02,

    kDexAnnotationByte          = 0x00,
    kDexAnnotationShort         = 0x02,
    kDexAnnotationChar          = 0x03,
    kDexAnnotationInt           = 0x04,
    kDexAnnotationLong          = 0x06,
    kDexAnnotationFloat         = 0x10,
    kDexAnnotationDouble        = 0x11,
    kDexAnnotationString        = 0x17,
    kDexAnnotationType          = 0x18,
    kDexAnnotationField         = 0x19,
    kDexAnnotationMethod        = 0x1a,
    kDexAnnotationEnum          = 0x1b,
    kDexAnnotationArray         = 0x1c,
    kDexAnnotationAnnotation    = 0x1d,
    kDexAnnotationNull          = 0x1e,
    kDexAnnotationBoolean       = 0x1f,

    kDexAnnotationValueTypeMask = 0x1f,     /* low 5 bits */
    kDexAnnotationValueArgShift = 5,
  };

  struct AnnotationsDirectoryItem {
    uint32_t class_annotations_off_;
    uint32_t fields_size_;
    uint32_t methods_size_;
    uint32_t parameters_size_;

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationsDirectoryItem);
  };

  struct FieldAnnotationsItem {
    uint32_t field_idx_;
    uint32_t annotations_off_;

   private:
    DISALLOW_COPY_AND_ASSIGN(FieldAnnotationsItem);
  };

  struct MethodAnnotationsItem {
    uint32_t method_idx_;
    uint32_t annotations_off_;

   private:
    DISALLOW_COPY_AND_ASSIGN(MethodAnnotationsItem);
  };

  struct ParameterAnnotationsItem {
    uint32_t method_idx_;
    uint32_t annotations_off_;

   private:
    DISALLOW_COPY_AND_ASSIGN(ParameterAnnotationsItem);
  };

  struct AnnotationSetRefItem {
    uint32_t annotations_off_;

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationSetRefItem);
  };

  struct AnnotationSetRefList {
    uint32_t size_;
    AnnotationSetRefItem list_[1];

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationSetRefList);
  };

  struct AnnotationSetItem {
    uint32_t size_;
    uint32_t entries_[1];

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationSetItem);
  };

  struct AnnotationItem {
    uint8_t visibility_;
    uint8_t annotation_[1];

   private:
    DISALLOW_COPY_AND_ASSIGN(AnnotationItem);
  };

  // Returns the checksum of a file for comparison with GetLocationChecksum().
  // For .dex files, this is the header checksum.
  // For zip files, this is the classes.dex zip entry CRC32 checksum.
  // Return true if the checksum could be found, false otherwise.
  static bool GetChecksum(const char* filename, uint32_t* checksum, std::string* error_msg);

  // Opens .dex files found in the container, guessing the container format based on file extension.
  static bool Open(const char* filename, const char* location, std::string* error_msg,
                   std::vector<const DexFile*>* dex_files);

  // Opens .dex file, backed by existing memory
  static const DexFile* Open(const uint8_t* base, size_t size,
                             const std::string& location,
                             uint32_t location_checksum,
                             std::string* error_msg) {
    return OpenMemory(base, size, location, location_checksum, NULL, error_msg);
  }

  // Open all classesXXX.dex files from a zip archive.
  static bool OpenFromZip(const ZipArchive& zip_archive, const std::string& location,
                          std::string* error_msg, std::vector<const DexFile*>* dex_files);

  // Closes a .dex file.
  virtual ~DexFile();

  const std::string& GetLocation() const {
    return location_;
  }

  // For normal dex files, location and base location coincide. If a dex file is part of a multidex
  // archive, the base location is the name of the originating jar/apk, stripped of any internal
  // classes*.dex path.
  static std::string GetBaseLocation(const char* location) {
    const char* pos = strrchr(location, kMultiDexSeparator);
    if (pos == nullptr) {
      return location;
    } else {
      return std::string(location, pos - location);
    }
  }

  std::string GetBaseLocation() const {
    size_t pos = location_.rfind(kMultiDexSeparator);
    if (pos == std::string::npos) {
      return location_;
    } else {
      return location_.substr(0, pos);
    }
  }

  // For DexFiles directly from .dex files, this is the checksum from the DexFile::Header.
  // For DexFiles opened from a zip files, this will be the ZipEntry CRC32 of classes.dex.
  uint32_t GetLocationChecksum() const {
    return location_checksum_;
  }

  const Header& GetHeader() const {
    DCHECK(header_ != NULL) << GetLocation();
    return *header_;
  }

  // Decode the dex magic version
  uint32_t GetVersion() const;

  // Returns true if the byte string points to the magic value.
  static bool IsMagicValid(const byte* magic);

  // Returns true if the byte string after the magic is the correct value.
  static bool IsVersionValid(const byte* magic);

  // Returns the number of string identifiers in the .dex file.
  size_t NumStringIds() const {
    DCHECK(header_ != NULL) << GetLocation();
    return header_->string_ids_size_;
  }

  // Returns the StringId at the specified index.
  const StringId& GetStringId(uint32_t idx) const {
    DCHECK_LT(idx, NumStringIds()) << GetLocation();
    return string_ids_[idx];
  }

  uint32_t GetIndexForStringId(const StringId& string_id) const {
    CHECK_GE(&string_id, string_ids_) << GetLocation();
    CHECK_LT(&string_id, string_ids_ + header_->string_ids_size_) << GetLocation();
    return &string_id - string_ids_;
  }

  int32_t GetStringLength(const StringId& string_id) const;

  // Returns a pointer to the UTF-8 string data referred to by the given string_id as well as the
  // length of the string when decoded as a UTF-16 string. Note the UTF-16 length is not the same
  // as the string length of the string data.
  const char* GetStringDataAndUtf16Length(const StringId& string_id, uint32_t* utf16_length) const;

  const char* GetStringData(const StringId& string_id) const {
    uint32_t ignored;
    return GetStringDataAndUtf16Length(string_id, &ignored);
  }

  // Index version of GetStringDataAndUtf16Length.
  const char* StringDataAndUtf16LengthByIdx(uint32_t idx, uint32_t* utf16_length) const {
    if (idx == kDexNoIndex) {
      *utf16_length = 0;
      return NULL;
    }
    const StringId& string_id = GetStringId(idx);
    return GetStringDataAndUtf16Length(string_id, utf16_length);
  }

  const char* StringDataByIdx(uint32_t idx) const {
    uint32_t unicode_length;
    return StringDataAndUtf16LengthByIdx(idx, &unicode_length);
  }

  // Looks up a string id for a given modified utf8 string.
  const StringId* FindStringId(const char* string) const;

  // Looks up a string id for a given utf16 string.
  const StringId* FindStringId(const uint16_t* string) const;

  // Returns the number of type identifiers in the .dex file.
  uint32_t NumTypeIds() const {
    DCHECK(header_ != NULL) << GetLocation();
    return header_->type_ids_size_;
  }

  // Returns the TypeId at the specified index.
  const TypeId& GetTypeId(uint32_t idx) const {
    DCHECK_LT(idx, NumTypeIds()) << GetLocation();
    return type_ids_[idx];
  }

  uint16_t GetIndexForTypeId(const TypeId& type_id) const {
    CHECK_GE(&type_id, type_ids_) << GetLocation();
    CHECK_LT(&type_id, type_ids_ + header_->type_ids_size_) << GetLocation();
    size_t result = &type_id - type_ids_;
    DCHECK_LT(result, 65536U) << GetLocation();
    return static_cast<uint16_t>(result);
  }

  // Get the descriptor string associated with a given type index.
  const char* StringByTypeIdx(uint32_t idx, uint32_t* unicode_length) const {
    const TypeId& type_id = GetTypeId(idx);
    return StringDataAndUtf16LengthByIdx(type_id.descriptor_idx_, unicode_length);
  }

  const char* StringByTypeIdx(uint32_t idx) const {
    const TypeId& type_id = GetTypeId(idx);
    return StringDataByIdx(type_id.descriptor_idx_);
  }

  // Returns the type descriptor string of a type id.
  const char* GetTypeDescriptor(const TypeId& type_id) const {
    return StringDataByIdx(type_id.descriptor_idx_);
  }

  // Looks up a type for the given string index
  const TypeId* FindTypeId(uint32_t string_idx) const;

  // Returns the number of field identifiers in the .dex file.
  size_t NumFieldIds() const {
    DCHECK(header_ != NULL) << GetLocation();
    return header_->field_ids_size_;
  }

  // Returns the FieldId at the specified index.
  const FieldId& GetFieldId(uint32_t idx) const {
    DCHECK_LT(idx, NumFieldIds()) << GetLocation();
    return field_ids_[idx];
  }

  uint32_t GetIndexForFieldId(const FieldId& field_id) const {
    CHECK_GE(&field_id, field_ids_) << GetLocation();
    CHECK_LT(&field_id, field_ids_ + header_->field_ids_size_) << GetLocation();
    return &field_id - field_ids_;
  }

  // Looks up a field by its declaring class, name and type
  const FieldId* FindFieldId(const DexFile::TypeId& declaring_klass,
                             const DexFile::StringId& name,
                             const DexFile::TypeId& type) const;

  // Returns the declaring class descriptor string of a field id.
  const char* GetFieldDeclaringClassDescriptor(const FieldId& field_id) const {
    const DexFile::TypeId& type_id = GetTypeId(field_id.class_idx_);
    return GetTypeDescriptor(type_id);
  }

  // Returns the class descriptor string of a field id.
  const char* GetFieldTypeDescriptor(const FieldId& field_id) const {
    const DexFile::TypeId& type_id = GetTypeId(field_id.type_idx_);
    return GetTypeDescriptor(type_id);
  }

  // Returns the name of a field id.
  const char* GetFieldName(const FieldId& field_id) const {
    return StringDataByIdx(field_id.name_idx_);
  }

  // Returns the number of method identifiers in the .dex file.
  size_t NumMethodIds() const {
    DCHECK(header_ != NULL) << GetLocation();
    return header_->method_ids_size_;
  }

  // Returns the MethodId at the specified index.
  const MethodId& GetMethodId(uint32_t idx) const {
    DCHECK_LT(idx, NumMethodIds()) << GetLocation();
    return method_ids_[idx];
  }

  uint32_t GetIndexForMethodId(const MethodId& method_id) const {
    CHECK_GE(&method_id, method_ids_) << GetLocation();
    CHECK_LT(&method_id, method_ids_ + header_->method_ids_size_) << GetLocation();
    return &method_id - method_ids_;
  }

  // Looks up a method by its declaring class, name and proto_id
  const MethodId* FindMethodId(const DexFile::TypeId& declaring_klass,
                               const DexFile::StringId& name,
                               const DexFile::ProtoId& signature) const;

  // Returns the declaring class descriptor string of a method id.
  const char* GetMethodDeclaringClassDescriptor(const MethodId& method_id) const {
    const DexFile::TypeId& type_id = GetTypeId(method_id.class_idx_);
    return GetTypeDescriptor(type_id);
  }

  // Returns the prototype of a method id.
  const ProtoId& GetMethodPrototype(const MethodId& method_id) const {
    return GetProtoId(method_id.proto_idx_);
  }

  // Returns a representation of the signature of a method id.
  const Signature GetMethodSignature(const MethodId& method_id) const;

  // Returns the name of a method id.
  const char* GetMethodName(const MethodId& method_id) const {
    return StringDataByIdx(method_id.name_idx_);
  }

  // Returns the shorty of a method id.
  const char* GetMethodShorty(const MethodId& method_id) const {
    return StringDataByIdx(GetProtoId(method_id.proto_idx_).shorty_idx_);
  }
  const char* GetMethodShorty(const MethodId& method_id, uint32_t* length) const {
    // Using the UTF16 length is safe here as shorties are guaranteed to be ASCII characters.
    return StringDataAndUtf16LengthByIdx(GetProtoId(method_id.proto_idx_).shorty_idx_, length);
  }
  // Returns the number of class definitions in the .dex file.
  uint32_t NumClassDefs() const {
    DCHECK(header_ != NULL) << GetLocation();
    return header_->class_defs_size_;
  }

  // Returns the ClassDef at the specified index.
  const ClassDef& GetClassDef(uint16_t idx) const {
    DCHECK_LT(idx, NumClassDefs()) << GetLocation();
    return class_defs_[idx];
  }

  uint16_t GetIndexForClassDef(const ClassDef& class_def) const {
    CHECK_GE(&class_def, class_defs_) << GetLocation();
    CHECK_LT(&class_def, class_defs_ + header_->class_defs_size_) << GetLocation();
    return &class_def - class_defs_;
  }

  // Returns the class descriptor string of a class definition.
  const char* GetClassDescriptor(const ClassDef& class_def) const {
    return StringByTypeIdx(class_def.class_idx_);
  }

  // Looks up a class definition by its class descriptor.
  const ClassDef* FindClassDef(const char* descriptor) const;

  // Looks up a class definition by its type index.
  const ClassDef* FindClassDef(uint16_t type_idx) const;

  const TypeList* GetInterfacesList(const ClassDef& class_def) const {
    if (class_def.interfaces_off_ == 0) {
        return NULL;
    } else {
      const byte* addr = begin_ + class_def.interfaces_off_;
      return reinterpret_cast<const TypeList*>(addr);
    }
  }

  // Returns a pointer to the raw memory mapped class_data_item
  const byte* GetClassData(const ClassDef& class_def) const {
    if (class_def.class_data_off_ == 0) {
      return NULL;
    } else {
      return begin_ + class_def.class_data_off_;
    }
  }

  //
  const CodeItem* GetCodeItem(const uint32_t code_off) const {
    if (code_off == 0) {
      return NULL;  // native or abstract method
    } else {
      const byte* addr = begin_ + code_off;
      return reinterpret_cast<const CodeItem*>(addr);
    }
  }

  const char* GetReturnTypeDescriptor(const ProtoId& proto_id) const {
    return StringByTypeIdx(proto_id.return_type_idx_);
  }

  // Returns the number of prototype identifiers in the .dex file.
  size_t NumProtoIds() const {
    DCHECK(header_ != NULL) << GetLocation();
    return header_->proto_ids_size_;
  }

  // Returns the ProtoId at the specified index.
  const ProtoId& GetProtoId(uint32_t idx) const {
    DCHECK_LT(idx, NumProtoIds()) << GetLocation();
    return proto_ids_[idx];
  }

  uint16_t GetIndexForProtoId(const ProtoId& proto_id) const {
    CHECK_GE(&proto_id, proto_ids_) << GetLocation();
    CHECK_LT(&proto_id, proto_ids_ + header_->proto_ids_size_) << GetLocation();
    return &proto_id - proto_ids_;
  }

  // Looks up a proto id for a given return type and signature type list
  const ProtoId* FindProtoId(uint16_t return_type_idx,
                             const uint16_t* signature_type_idxs, uint32_t signature_length) const;
  const ProtoId* FindProtoId(uint16_t return_type_idx,
                             const std::vector<uint16_t>& signature_type_idxs) const {
    return FindProtoId(return_type_idx, &signature_type_idxs[0], signature_type_idxs.size());
  }

  // Given a signature place the type ids into the given vector, returns true on success
  bool CreateTypeList(const StringPiece& signature, uint16_t* return_type_idx,
                      std::vector<uint16_t>* param_type_idxs) const;

  // Create a Signature from the given string signature or return Signature::NoSignature if not
  // possible.
  const Signature CreateSignature(const StringPiece& signature) const;

  // Returns the short form method descriptor for the given prototype.
  const char* GetShorty(uint32_t proto_idx) const {
    const ProtoId& proto_id = GetProtoId(proto_idx);
    return StringDataByIdx(proto_id.shorty_idx_);
  }

  const TypeList* GetProtoParameters(const ProtoId& proto_id) const {
    if (proto_id.parameters_off_ == 0) {
      return NULL;
    } else {
      const byte* addr = begin_ + proto_id.parameters_off_;
      return reinterpret_cast<const TypeList*>(addr);
    }
  }

  const byte* GetEncodedStaticFieldValuesArray(const ClassDef& class_def) const {
    if (class_def.static_values_off_ == 0) {
      return 0;
    } else {
      return begin_ + class_def.static_values_off_;
    }
  }

  static const TryItem* GetTryItems(const CodeItem& code_item, uint32_t offset);

  // Get the base of the encoded data for the given DexCode.
  static const byte* GetCatchHandlerData(const CodeItem& code_item, uint32_t offset) {
    const byte* handler_data =
        reinterpret_cast<const byte*>(GetTryItems(code_item, code_item.tries_size_));
    return handler_data + offset;
  }

  // Find which try region is associated with the given address (ie dex pc). Returns -1 if none.
  static int32_t FindTryItem(const CodeItem &code_item, uint32_t address);

  // Find the handler offset associated with the given address (ie dex pc). Returns -1 if none.
  static int32_t FindCatchHandlerOffset(const CodeItem &code_item, uint32_t address);

  // Get the pointer to the start of the debugging data
  const byte* GetDebugInfoStream(const CodeItem* code_item) const {
    if (code_item->debug_info_off_ == 0) {
      return NULL;
    } else {
      return begin_ + code_item->debug_info_off_;
    }
  }

  // Callback for "new position table entry".
  // Returning true causes the decoder to stop early.
  typedef bool (*DexDebugNewPositionCb)(void* context, uint32_t address, uint32_t line_num);

  // Callback for "new locals table entry". "signature" is an empty string
  // if no signature is available for an entry.
  typedef void (*DexDebugNewLocalCb)(void* context, uint16_t reg,
                                     uint32_t start_address,
                                     uint32_t end_address,
                                     const char* name,
                                     const char* descriptor,
                                     const char* signature);

  static bool LineNumForPcCb(void* context, uint32_t address, uint32_t line_num);

  // Debug info opcodes and constants
  enum {
    DBG_END_SEQUENCE         = 0x00,
    DBG_ADVANCE_PC           = 0x01,
    DBG_ADVANCE_LINE         = 0x02,
    DBG_START_LOCAL          = 0x03,
    DBG_START_LOCAL_EXTENDED = 0x04,
    DBG_END_LOCAL            = 0x05,
    DBG_RESTART_LOCAL        = 0x06,
    DBG_SET_PROLOGUE_END     = 0x07,
    DBG_SET_EPILOGUE_BEGIN   = 0x08,
    DBG_SET_FILE             = 0x09,
    DBG_FIRST_SPECIAL        = 0x0a,
    DBG_LINE_BASE            = -4,
    DBG_LINE_RANGE           = 15,
  };

  struct LocalInfo {
    LocalInfo()
        : name_(NULL), descriptor_(NULL), signature_(NULL), start_address_(0), is_live_(false) {}

    const char* name_;  // E.g., list
    const char* descriptor_;  // E.g., Ljava/util/LinkedList;
    const char* signature_;  // E.g., java.util.LinkedList<java.lang.Integer>
    uint16_t start_address_;  // PC location where the local is first defined.
    bool is_live_;  // Is the local defined and live.

   private:
    DISALLOW_COPY_AND_ASSIGN(LocalInfo);
  };

  struct LineNumFromPcContext {
    LineNumFromPcContext(uint32_t address, uint32_t line_num)
        : address_(address), line_num_(line_num) {}
    uint32_t address_;
    uint32_t line_num_;
   private:
    DISALLOW_COPY_AND_ASSIGN(LineNumFromPcContext);
  };

  void InvokeLocalCbIfLive(void* context, int reg, uint32_t end_address,
                           LocalInfo* local_in_reg, DexDebugNewLocalCb local_cb) const {
    if (local_cb != NULL && local_in_reg[reg].is_live_) {
      local_cb(context, reg, local_in_reg[reg].start_address_, end_address,
          local_in_reg[reg].name_, local_in_reg[reg].descriptor_,
          local_in_reg[reg].signature_ != NULL ? local_in_reg[reg].signature_ : "");
    }
  }

  // Determine the source file line number based on the program counter.
  // "pc" is an offset, in 16-bit units, from the start of the method's code.
  //
  // Returns -1 if no match was found (possibly because the source files were
  // compiled without "-g", so no line number information is present).
  // Returns -2 for native methods (as expected in exception traces).
  //
  // This is used by runtime; therefore use art::Method not art::DexFile::Method.
  int32_t GetLineNumFromPC(mirror::ArtMethod* method, uint32_t rel_pc) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  void DecodeDebugInfo(const CodeItem* code_item, bool is_static, uint32_t method_idx,
                       DexDebugNewPositionCb position_cb, DexDebugNewLocalCb local_cb,
                       void* context) const;

  const char* GetSourceFile(const ClassDef& class_def) const {
    if (class_def.source_file_idx_ == 0xffffffff) {
      return NULL;
    } else {
      return StringDataByIdx(class_def.source_file_idx_);
    }
  }

  int GetPermissions() const;

  bool IsReadOnly() const;

  bool EnableWrite() const;

  bool DisableWrite() const;

  const byte* Begin() const {
    return begin_;
  }

  size_t Size() const {
    return size_;
  }

  static std::string GetMultiDexClassesDexName(size_t number, const char* dex_location);

  // Returns the canonical form of the given dex location.
  //
  // There are different flavors of "dex locations" as follows:
  // the file name of a dex file:
  //     The actual file path that the dex file has on disk.
  // dex_location:
  //     This acts as a key for the class linker to know which dex file to load.
  //     It may correspond to either an old odex file or a particular dex file
  //     inside an oat file. In the first case it will also match the file name
  //     of the dex file. In the second case (oat) it will include the file name
  //     and possibly some multidex annotation to uniquely identify it.
  // canonical_dex_location:
  //     the dex_location where it's file name part has been made canonical.
  static std::string GetDexCanonicalLocation(const char* dex_location);

 private:
  // Opens a .dex file
  static const DexFile* OpenFile(int fd, const char* location, bool verify, std::string* error_msg);

  // Opens dex files from within a .jar, .zip, or .apk file
  static bool OpenZip(int fd, const std::string& location, std::string* error_msg,
                      std::vector<const DexFile*>* dex_files);

  enum class ZipOpenErrorCode {  // private
    kNoError,
    kEntryNotFound,
    kExtractToMemoryError,
    kDexFileError,
    kMakeReadOnlyError,
    kVerifyError
  };

  // Opens .dex file from the entry_name in a zip archive. error_code is undefined when non-nullptr
  // return.
  static const DexFile* Open(const ZipArchive& zip_archive, const char* entry_name,
                             const std::string& location, std::string* error_msg,
                             ZipOpenErrorCode* error_code);

  // Opens a .dex file at the given address backed by a MemMap
  static const DexFile* OpenMemory(const std::string& location,
                                   uint32_t location_checksum,
                                   MemMap* mem_map,
                                   std::string* error_msg);

  // Opens a .dex file at the given address, optionally backed by a MemMap
  static const DexFile* OpenMemory(const byte* dex_file,
                                   size_t size,
                                   const std::string& location,
                                   uint32_t location_checksum,
                                   MemMap* mem_map,
                                   std::string* error_msg);

  DexFile(const byte* base, size_t size,
          const std::string& location,
          uint32_t location_checksum,
          MemMap* mem_map);

  // Top-level initializer that calls other Init methods.
  bool Init(std::string* error_msg);

  // Returns true if the header magic and version numbers are of the expected values.
  bool CheckMagicAndVersion(std::string* error_msg) const;

  void DecodeDebugInfo0(const CodeItem* code_item, bool is_static, uint32_t method_idx,
      DexDebugNewPositionCb position_cb, DexDebugNewLocalCb local_cb,
      void* context, const byte* stream, LocalInfo* local_in_reg) const;

  // Check whether a location denotes a multidex dex file. This is a very simple check: returns
  // whether the string contains the separator character.
  static bool IsMultiDexLocation(const char* location);


  // The base address of the memory mapping.
  const byte* const begin_;

  // The size of the underlying memory allocation in bytes.
  const size_t size_;

  // Typically the dex file name when available, alternatively some identifying string.
  //
  // The ClassLinker will use this to match DexFiles the boot class
  // path to DexCache::GetLocation when loading from an image.
  const std::string location_;

  const uint32_t location_checksum_;

  // Manages the underlying memory allocation.
  std::unique_ptr<MemMap> mem_map_;

  // Points to the header section.
  const Header* const header_;

  // Points to the base of the string identifier list.
  const StringId* const string_ids_;

  // Points to the base of the type identifier list.
  const TypeId* const type_ids_;

  // Points to the base of the field identifier list.
  const FieldId* const field_ids_;

  // Points to the base of the method identifier list.
  const MethodId* const method_ids_;

  // Points to the base of the prototype identifier list.
  const ProtoId* const proto_ids_;

  // Points to the base of the class definition list.
  const ClassDef* const class_defs_;

  // Number of misses finding a class def from a descriptor.
  mutable Atomic<uint32_t> find_class_def_misses_;

  struct UTF16HashCmp {
    // Hash function.
    size_t operator()(const char* key) const {
      return ComputeUtf8Hash(key);
    }
    // std::equal function.
    bool operator()(const char* a, const char* b) const {
      return CompareModifiedUtf8ToModifiedUtf8AsUtf16CodePointValues(a, b) == 0;
    }
  };
  typedef std::unordered_map<const char*, const ClassDef*, UTF16HashCmp, UTF16HashCmp> Index;
  mutable Atomic<Index*> class_def_index_;
  mutable Mutex build_class_def_index_mutex_ DEFAULT_MUTEX_ACQUIRED_AFTER;
};
std::ostream& operator<<(std::ostream& os, const DexFile& dex_file);

// Iterate over a dex file's ProtoId's paramters
class DexFileParameterIterator {
 public:
  DexFileParameterIterator(const DexFile& dex_file, const DexFile::ProtoId& proto_id)
      : dex_file_(dex_file), size_(0), pos_(0) {
    type_list_ = dex_file_.GetProtoParameters(proto_id);
    if (type_list_ != NULL) {
      size_ = type_list_->Size();
    }
  }
  bool HasNext() const { return pos_ < size_; }
  void Next() { ++pos_; }
  uint16_t GetTypeIdx() {
    return type_list_->GetTypeItem(pos_).type_idx_;
  }
  const char* GetDescriptor() {
    return dex_file_.StringByTypeIdx(GetTypeIdx());
  }
 private:
  const DexFile& dex_file_;
  const DexFile::TypeList* type_list_;
  uint32_t size_;
  uint32_t pos_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(DexFileParameterIterator);
};

// Abstract the signature of a method.
class Signature {
 public:
  std::string ToString() const;

  static Signature NoSignature() {
    return Signature();
  }

  bool operator==(const Signature& rhs) const;
  bool operator!=(const Signature& rhs) const {
    return !(*this == rhs);
  }

  bool operator==(const StringPiece& rhs) const;

 private:
  Signature(const DexFile* dex, const DexFile::ProtoId& proto) : dex_file_(dex), proto_id_(&proto) {
  }

  Signature() : dex_file_(nullptr), proto_id_(nullptr) {
  }

  friend class DexFile;

  const DexFile* const dex_file_;
  const DexFile::ProtoId* const proto_id_;
};
std::ostream& operator<<(std::ostream& os, const Signature& sig);

// Iterate and decode class_data_item
class ClassDataItemIterator {
 public:
  ClassDataItemIterator(const DexFile& dex_file, const byte* raw_class_data_item)
      : dex_file_(dex_file), pos_(0), ptr_pos_(raw_class_data_item), last_idx_(0) {
    ReadClassDataHeader();
    if (EndOfInstanceFieldsPos() > 0) {
      ReadClassDataField();
    } else if (EndOfVirtualMethodsPos() > 0) {
      ReadClassDataMethod();
    }
  }
  uint32_t NumStaticFields() const {
    return header_.static_fields_size_;
  }
  uint32_t NumInstanceFields() const {
    return header_.instance_fields_size_;
  }
  uint32_t NumDirectMethods() const {
    return header_.direct_methods_size_;
  }
  uint32_t NumVirtualMethods() const {
    return header_.virtual_methods_size_;
  }
  bool HasNextStaticField() const {
    return pos_ < EndOfStaticFieldsPos();
  }
  bool HasNextInstanceField() const {
    return pos_ >= EndOfStaticFieldsPos() && pos_ < EndOfInstanceFieldsPos();
  }
  bool HasNextDirectMethod() const {
    return pos_ >= EndOfInstanceFieldsPos() && pos_ < EndOfDirectMethodsPos();
  }
  bool HasNextVirtualMethod() const {
    return pos_ >= EndOfDirectMethodsPos() && pos_ < EndOfVirtualMethodsPos();
  }
  bool HasNext() const {
    return pos_ < EndOfVirtualMethodsPos();
  }
  inline void Next() {
    pos_++;
    if (pos_ < EndOfStaticFieldsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataField();
    } else if (pos_ == EndOfStaticFieldsPos() && NumInstanceFields() > 0) {
      last_idx_ = 0;  // transition to next array, reset last index
      ReadClassDataField();
    } else if (pos_ < EndOfInstanceFieldsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataField();
    } else if (pos_ == EndOfInstanceFieldsPos() && NumDirectMethods() > 0) {
      last_idx_ = 0;  // transition to next array, reset last index
      ReadClassDataMethod();
    } else if (pos_ < EndOfDirectMethodsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataMethod();
    } else if (pos_ == EndOfDirectMethodsPos() && NumVirtualMethods() > 0) {
      last_idx_ = 0;  // transition to next array, reset last index
      ReadClassDataMethod();
    } else if (pos_ < EndOfVirtualMethodsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataMethod();
    } else {
      DCHECK(!HasNext());
    }
  }
  uint32_t GetMemberIndex() const {
    if (pos_ < EndOfInstanceFieldsPos()) {
      return last_idx_ + field_.field_idx_delta_;
    } else {
      DCHECK_LT(pos_, EndOfVirtualMethodsPos());
      return last_idx_ + method_.method_idx_delta_;
    }
  }
  uint32_t GetRawMemberAccessFlags() const {
    if (pos_ < EndOfInstanceFieldsPos()) {
      return field_.access_flags_;
    } else {
      DCHECK_LT(pos_, EndOfVirtualMethodsPos());
      return method_.access_flags_;
    }
  }
  uint32_t GetFieldAccessFlags() const {
    return GetRawMemberAccessFlags() & kAccValidFieldFlags;
  }
  uint32_t GetMethodAccessFlags() const {
    return GetRawMemberAccessFlags() & kAccValidMethodFlags;
  }
  bool MemberIsNative() const {
    return GetRawMemberAccessFlags() & kAccNative;
  }
  bool MemberIsFinal() const {
    return GetRawMemberAccessFlags() & kAccFinal;
  }
  InvokeType GetMethodInvokeType(const DexFile::ClassDef& class_def) const {
    if (HasNextDirectMethod()) {
      if ((GetRawMemberAccessFlags() & kAccStatic) != 0) {
        return kStatic;
      } else {
        return kDirect;
      }
    } else {
      DCHECK_EQ(GetRawMemberAccessFlags() & kAccStatic, 0U);
      if ((class_def.access_flags_ & kAccInterface) != 0) {
        return kInterface;
      } else if ((GetRawMemberAccessFlags() & kAccConstructor) != 0) {
        return kSuper;
      } else {
        return kVirtual;
      }
    }
  }
  const DexFile::CodeItem* GetMethodCodeItem() const {
    return dex_file_.GetCodeItem(method_.code_off_);
  }
  uint32_t GetMethodCodeItemOffset() const {
    return method_.code_off_;
  }
  const byte* EndDataPointer() const {
    CHECK(!HasNext());
    return ptr_pos_;
  }

 private:
  // A dex file's class_data_item is leb128 encoded, this structure holds a decoded form of the
  // header for a class_data_item
  struct ClassDataHeader {
    uint32_t static_fields_size_;  // the number of static fields
    uint32_t instance_fields_size_;  // the number of instance fields
    uint32_t direct_methods_size_;  // the number of direct methods
    uint32_t virtual_methods_size_;  // the number of virtual methods
  } header_;

  // Read and decode header from a class_data_item stream into header
  void ReadClassDataHeader();

  uint32_t EndOfStaticFieldsPos() const {
    return header_.static_fields_size_;
  }
  uint32_t EndOfInstanceFieldsPos() const {
    return EndOfStaticFieldsPos() + header_.instance_fields_size_;
  }
  uint32_t EndOfDirectMethodsPos() const {
    return EndOfInstanceFieldsPos() + header_.direct_methods_size_;
  }
  uint32_t EndOfVirtualMethodsPos() const {
    return EndOfDirectMethodsPos() + header_.virtual_methods_size_;
  }

  // A decoded version of the field of a class_data_item
  struct ClassDataField {
    uint32_t field_idx_delta_;  // delta of index into the field_ids array for FieldId
    uint32_t access_flags_;  // access flags for the field
    ClassDataField() :  field_idx_delta_(0), access_flags_(0) {}

   private:
    DISALLOW_COPY_AND_ASSIGN(ClassDataField);
  };
  ClassDataField field_;

  // Read and decode a field from a class_data_item stream into field
  void ReadClassDataField();

  // A decoded version of the method of a class_data_item
  struct ClassDataMethod {
    uint32_t method_idx_delta_;  // delta of index into the method_ids array for MethodId
    uint32_t access_flags_;
    uint32_t code_off_;
    ClassDataMethod() : method_idx_delta_(0), access_flags_(0), code_off_(0) {}

   private:
    DISALLOW_COPY_AND_ASSIGN(ClassDataMethod);
  };
  ClassDataMethod method_;

  // Read and decode a method from a class_data_item stream into method
  void ReadClassDataMethod();

  const DexFile& dex_file_;
  size_t pos_;  // integral number of items passed
  const byte* ptr_pos_;  // pointer into stream of class_data_item
  uint32_t last_idx_;  // last read field or method index to apply delta to
  DISALLOW_IMPLICIT_CONSTRUCTORS(ClassDataItemIterator);
};

class EncodedStaticFieldValueIterator {
 public:
  EncodedStaticFieldValueIterator(const DexFile& dex_file, Handle<mirror::DexCache>* dex_cache,
                                  Handle<mirror::ClassLoader>* class_loader,
                                  ClassLinker* linker, const DexFile::ClassDef& class_def)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  template<bool kTransactionActive>
  void ReadValueToField(mirror::ArtField* field) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  bool HasNext() { return pos_ < array_size_; }

  void Next();

  enum ValueType {
    kByte = 0x00,
    kShort = 0x02,
    kChar = 0x03,
    kInt = 0x04,
    kLong = 0x06,
    kFloat = 0x10,
    kDouble = 0x11,
    kString = 0x17,
    kType = 0x18,
    kField = 0x19,
    kMethod = 0x1a,
    kEnum = 0x1b,
    kArray = 0x1c,
    kAnnotation = 0x1d,
    kNull = 0x1e,
    kBoolean = 0x1f
  };

 private:
  static const byte kEncodedValueTypeMask = 0x1f;  // 0b11111
  static const byte kEncodedValueArgShift = 5;

  const DexFile& dex_file_;
  Handle<mirror::DexCache>* const dex_cache_;  // Dex cache to resolve literal objects.
  Handle<mirror::ClassLoader>* const class_loader_;  // ClassLoader to resolve types.
  ClassLinker* linker_;  // Linker to resolve literal objects.
  size_t array_size_;  // Size of array.
  size_t pos_;  // Current position.
  const byte* ptr_;  // Pointer into encoded data array.
  ValueType type_;  // Type of current encoded value.
  jvalue jval_;  // Value of current encoded value.
  DISALLOW_IMPLICIT_CONSTRUCTORS(EncodedStaticFieldValueIterator);
};
std::ostream& operator<<(std::ostream& os, const EncodedStaticFieldValueIterator::ValueType& code);

class CatchHandlerIterator {
  public:
    CatchHandlerIterator(const DexFile::CodeItem& code_item, uint32_t address);

    CatchHandlerIterator(const DexFile::CodeItem& code_item,
                         const DexFile::TryItem& try_item);

    explicit CatchHandlerIterator(const byte* handler_data) {
      Init(handler_data);
    }

    uint16_t GetHandlerTypeIndex() const {
      return handler_.type_idx_;
    }
    uint32_t GetHandlerAddress() const {
      return handler_.address_;
    }
    void Next();
    bool HasNext() const {
      return remaining_count_ != -1 || catch_all_;
    }
    // End of this set of catch blocks, convenience method to locate next set of catch blocks
    const byte* EndDataPointer() const {
      CHECK(!HasNext());
      return current_data_;
    }

  private:
    void Init(const DexFile::CodeItem& code_item, int32_t offset);
    void Init(const byte* handler_data);

    struct CatchHandlerItem {
      uint16_t type_idx_;  // type index of the caught exception type
      uint32_t address_;  // handler address
    } handler_;
    const byte *current_data_;  // the current handler in dex file.
    int32_t remaining_count_;   // number of handlers not read.
    bool catch_all_;            // is there a handler that will catch all exceptions in case
                                // that all typed handler does not match.
};

}  // namespace art

#endif  // ART_RUNTIME_DEX_FILE_H_
