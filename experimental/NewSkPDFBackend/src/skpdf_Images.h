/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Images_DEFINED
#define skpdf_Images_DEFINED

// A set of useful functions for transformaing

#include "SkString.h"
#include "skpdf_Types.h"

class SkStreamAsset;
class SkWStream;
class SkBitmap;

namespace skpdf {

class Stream;

/**
 *  If the bitmap is not subsetted, return its encoded data, if
 *  availible and it represents a JFIF JPEG.
 */

void WriteImage(
        const skpdf::Stream& stream,
        int32_t width,
        int32_t height,
        const SkString& colorSpace,
        int BitsPerComponent,
        const char* otherKeys,
        Int AlphaMask,
        Int stencilMaskIndex,
        SkWStream* out);

SkStreamAsset* ExtractJpeg(const SkBitmap& bitmap);

SkStreamAsset* ExtractColorTable(const SkBitmap& bitmap);

SkStreamAsset* ExtractBitmapMask(const SkBitmap& bitmap);

SkStreamAsset* ExtractBitmapAlpha(const SkBitmap& bitmap);

SkStreamAsset* ExtractBitmapRGB(const SkBitmap& bitmap);

}  // namespace skpdf

#endif  // skpdf_Images_DEFINED
