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

#ifndef ART_RUNTIME_IMAGE_H_
#define ART_RUNTIME_IMAGE_H_

#include <string.h>

#include "globals.h"
#include "mirror/object.h"
#include "utils.h"

namespace art {

// header of image files written by ImageWriter, read and validated by Space.
class PACKED(4) ImageHeader {
 public:
  ImageHeader() {}

  ImageHeader(uint32_t image_begin,
              uint32_t image_size_,
              uint32_t image_bitmap_offset,
              uint32_t image_bitmap_size,
              uint32_t image_roots,
              uint32_t oat_checksum,
              uint32_t oat_file_begin,
              uint32_t oat_data_begin,
              uint32_t oat_data_end,
              uint32_t oat_file_end);

  bool IsValid() const;
  const char* GetMagic() const;

  byte* GetImageBegin() const {
    return reinterpret_cast<byte*>(image_begin_);
  }

  size_t GetImageSize() const {
    return static_cast<uint32_t>(image_size_);
  }

  size_t GetImageBitmapOffset() const {
    return image_bitmap_offset_;
  }

  size_t GetImageBitmapSize() const {
    return image_bitmap_size_;
  }

  uint32_t GetOatChecksum() const {
    return oat_checksum_;
  }

  void SetOatChecksum(uint32_t oat_checksum) {
    oat_checksum_ = oat_checksum;
  }

  byte* GetOatFileBegin() const {
    return reinterpret_cast<byte*>(oat_file_begin_);
  }

  byte* GetOatDataBegin() const {
    return reinterpret_cast<byte*>(oat_data_begin_);
  }

  byte* GetOatDataEnd() const {
    return reinterpret_cast<byte*>(oat_data_end_);
  }

  byte* GetOatFileEnd() const {
    return reinterpret_cast<byte*>(oat_file_end_);
  }

  off_t GetPatchDelta() const {
    return patch_delta_;
  }

  size_t GetBitmapOffset() const {
    return RoundUp(image_size_, kPageSize);
  }

  static std::string GetOatLocationFromImageLocation(const std::string& image) {
    std::string oat_filename = image;
    if (oat_filename.length() <= 3) {
      oat_filename += ".oat";
    } else {
      oat_filename.replace(oat_filename.length() - 3, 3, "oat");
    }
    return oat_filename;
  }

  enum ImageRoot {
    kResolutionMethod,
    kImtConflictMethod,
    kDefaultImt,
    kCalleeSaveMethod,
    kRefsOnlySaveMethod,
    kRefsAndArgsSaveMethod,
    kDexCaches,
    kClassRoots,
    kImageRootsMax,
  };

  mirror::Object* GetImageRoot(ImageRoot image_root) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  mirror::ObjectArray<mirror::Object>* GetImageRoots() const;

  void RelocateImage(off_t delta);

 private:
  static const byte kImageMagic[4];
  static const byte kImageVersion[4];

  byte magic_[4];
  byte version_[4];

  // Required base address for mapping the image.
  uint32_t image_begin_;

  // Image size, not page aligned.
  uint32_t image_size_;

  // Image bitmap offset in the file.
  uint32_t image_bitmap_offset_;

  // Size of the image bitmap.
  uint32_t image_bitmap_size_;

  // Checksum of the oat file we link to for load time sanity check.
  uint32_t oat_checksum_;

  // Start address for oat file. Will be before oat_data_begin_ for .so files.
  uint32_t oat_file_begin_;

  // Required oat address expected by image Method::GetCode() pointers.
  uint32_t oat_data_begin_;

  // End of oat data address range for this image file.
  uint32_t oat_data_end_;

  // End of oat file address range. will be after oat_data_end_ for
  // .so files. Used for positioning a following alloc spaces.
  uint32_t oat_file_end_;

  // The total delta that this image has been patched.
  int32_t patch_delta_;

  // Absolute address of an Object[] of objects needed to reinitialize from an image.
  uint32_t image_roots_;

  friend class ImageWriter;
};

}  // namespace art

#endif  // ART_RUNTIME_IMAGE_H_
