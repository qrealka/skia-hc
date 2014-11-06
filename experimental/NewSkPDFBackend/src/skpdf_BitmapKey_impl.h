/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_BitmapKey_impl_DEFINED
#define skpdf_BitmapKey_impl_DEFINED

namespace skpdf {
inline BitmapKey::BitmapKey(const SkBitmap& bm)
    : fGenerationID(bm.getGenerationID())
    , fPixelRefOrigin(bm.pixelRefOrigin())
    , fDimensions(bm.dimensions()) {
}

inline bool BitmapKey::operator==(const BitmapKey& other) const {
    return (fGenerationID == other.fGenerationID) &&
           (fPixelRefOrigin == other.fPixelRefOrigin) &&
           (fDimensions == other.fDimensions);
}
}  // namespace skpdf

#endif  // skpdf_BitmapKey_impl_DEFINED
