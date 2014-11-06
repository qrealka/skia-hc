/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_GlyphCollection_DEFINED
#define skpdf_GlyphCollection_DEFINED

#include "SkTypes.h"
#include "SkTypeface.h"
#include "SkBitSet.h"

namespace skpdf {
class GlyphCollection : SkNoncopyable {
public:
    GlyphCollection(SkTypeface* t);
    bool addGlyph(uint16_t glyphid);
    void write(SkWStream*);

private:
    bool fFinal;
    SkAutoTUnref<SkTypeface> fTypeface;
    SkBitSet fBitSet;
};  // mutable font subset

}  // namespace skpdf

#include "skpdf_GlyphCollection_impl.h"

#endif  // skpdf_GlyphCollection_DEFINED
