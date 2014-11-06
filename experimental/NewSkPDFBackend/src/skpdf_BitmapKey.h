/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_BitmapKey_DEFINED
#define skpdf_BitmapKey_DEFINED

#include "SkBitmap.h"

namespace skpdf {
class BitmapKey {
public:
    explicit BitmapKey(const SkBitmap&);
    bool operator==(const BitmapKey& other) const;

private:
    uint32_t fGenerationID;
    SkIPoint fPixelRefOrigin;
    SkISize fDimensions;
};
}  // namespace skpdf

#include "skpdf_BitmapKey_impl.h"

#endif  // skpdf_BitmapKey_DEFINED
