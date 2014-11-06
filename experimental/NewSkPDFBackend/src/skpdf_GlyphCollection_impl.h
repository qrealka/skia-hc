/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_GlyphCollection_impl_DEFINED
#define skpdf_GlyphCollection_impl_DEFINED

namespace skpdf {
inline GlyphCollection::GlyphCollection(SkTypeface* t)
    : fFinal(false), fTypeface(SkRef(t)), fBitSet(SK_MaxU16 + 1) {
}
inline bool GlyphCollection::addGlyph(uint16_t glyphID) {
    if (fBitSet.isBitSet(glyphID)) {
        return true;
    } else if (fFinal) {
        return false;
    }
    fBitSet.setBit(glyphID, true);
    return true;
}
inline void GlyphCollection::write(SkWStream* out) {
    // FIXME
    // subset the typeface and stream it out.
    fFinal = true;
}
}  // namespace skpdf

#endif  // skpdf_GlyphCollection_impl_DEFINED
