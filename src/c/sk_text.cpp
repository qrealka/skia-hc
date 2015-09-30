/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkTypes.h"

#include "sk_text.h"

#include "SkData.h"
#include "SkTypeface.h"
#include "SkStream.h"
#include "SkCanvas.h"
#include "SkTextBlob.h"

sk_data_t* sk_data_new_from_file(const char* path) {
    return reinterpret_cast<sk_data_t*>(SkData::NewFromFileName(path));
}

/******************************************************************************/

sk_typeface_t* sk_typeface_create_default() {
    return reinterpret_cast<sk_typeface_t*>(SkTypeface::RefDefault());
}

sk_typeface_t* sk_typeface_create_from_sk_data(sk_data_t* data, int idx) {
    return reinterpret_cast<sk_typeface_t*>(
            SkTypeface::CreateFromStream(
                    new SkMemoryStream(reinterpret_cast<SkData*>(data)),
                    idx));
}

void sk_typeface_unref(sk_typeface_t* ptr) {
    reinterpret_cast<SkTypeface*>(ptr)->unref();
}

int sk_typeface_utf8text_to_glyph_count(sk_typeface_t* typeface,
                                        const char* utf8text,
                                        size_t byteLength) {
    SkPaint paint;
    paint.setTextEncoding(SkPaint::kUTF8_TextEncoding);
    paint.setTypeface(reinterpret_cast<SkTypeface*>(typeface));
    return paint.countText(utf8text, byteLength);
}

int sk_typeface_utf8text_to_glyphs(sk_typeface_t* typeface,
                                   const char* utf8text,
                                   size_t byteLength,
                                   sk_glyph_id_t* glyphArray) {
    SkPaint paint;
    paint.setTextEncoding(SkPaint::kUTF8_TextEncoding);
    paint.setTypeface(reinterpret_cast<SkTypeface*>(typeface));
    return paint.textToGlyphs(utf8text, byteLength, glyphArray);
}


static const uint32_t kFlagMask =
    ANTIALIAS_TEXT_SK_FONT_FLAG |
    UNDERLINE_TEXT_SK_FONT_FLAG |
    STRIKETHRU_TEXT_SK_FONT_FLAG |
    VERTICAL_TEXT_SK_FONT_FLAG;

struct sk_font_t {
    SkAutoTUnref<SkTypeface> fTypeface;
    SkScalar                 fSize;
    SkScalar                 fScaleX;
    SkScalar                 fSkew;
    uint32_t                 fFlags;
    sk_font_t() : fSize(12.0f), fScaleX(1.0f), fSkew(0.0f), fFlags(0) {}
    void populate(SkPaint* p) const {
        p->reset();
        p->setTypeface(fTypeface);
        p->setTextSize(fSize);
        p->setTextScaleX(fScaleX);
        p->setTextSkewX(fSkew);
        p->setTextEncoding(SkPaint::kGlyphID_TextEncoding);
        p->setFlags((p->getFlags() & ~kFlagMask) | (fFlags & kFlagMask));
    }
};

sk_font_t* sk_font_new() { return new sk_font_t(); }

void sk_font_delete(sk_font_t* font) { delete font; }

void sk_font_set_typeface(sk_font_t* font, sk_typeface_t* typeface) {
    font->fTypeface.reset(SkRef(reinterpret_cast<SkTypeface*>(typeface)));
}

void sk_font_set_size(sk_font_t* font, float v) { font->fSize = v; }

void sk_font_set_scale_x(sk_font_t* font, float v) { font->fScaleX = v; }

void sk_font_set_skew(sk_font_t* font, float v) { font->fSkew = v; }

void sk_font_set_flags(sk_font_t* font, sk_text_flags flags, bool set) {
    if (set) {
        font->fFlags |= (flags & kFlagMask);
    } else {  // unset
        font->fFlags &= (~flags & ~kFlagMask);
    }
}

const sk_text_blob_t* sk_text_blob_build(sk_text_blob_builder_t* builder) {
    return reinterpret_cast<const sk_text_blob_t*>(
            reinterpret_cast<SkTextBlobBuilder*>(builder)->build());
}

void sk_text_blob_unref(const sk_text_blob_t* blob) {
    reinterpret_cast<const SkTextBlob*>(blob)->unref();
}

void sk_text_blob_get_bounds(const sk_text_blob_t* blob, sk_rect_t* dst) {
    *dst = reinterpret_cast<const sk_rect_t&>(
            reinterpret_cast<const SkTextBlob*>(blob)->bounds());
}

sk_text_blob_builder_t* sk_text_blob_new_builder() {
    return reinterpret_cast<sk_text_blob_builder_t*>(new SkTextBlobBuilder);
}

void sk_text_blob_builder_delete(sk_text_blob_builder_t* builder) {
    delete reinterpret_cast<SkTextBlobBuilder*>(builder);
}

sk_glyph_id_t* sk_text_blob_allocate_run(sk_text_blob_builder_t* builder,
                                         const sk_font_t* font,
                                         int count,
                                         float x, float y,
                                         const sk_rect_t* bounds) {
    SkPaint paint;
    font->populate(&paint);
    return reinterpret_cast<SkTextBlobBuilder*>(builder)->allocRun(
            paint, count, x, y, reinterpret_cast<const SkRect*>(bounds)).glyphs;
}

/* This function assumes that you do not want to use the glyphs'
   default advances, but that you do want them to line up horizontaly */
const sk_text_blob_run_buffer_pos_h_t* sk_text_blob_allocate_run_pos_h(
        sk_text_blob_builder_t* builder,
        const sk_font_t* font,
        int count,
        float y,
        const sk_rect_t* bounds) {
    SkPaint paint;
    font->populate(&paint);
    return reinterpret_cast<const sk_text_blob_run_buffer_pos_h_t*>(
        &reinterpret_cast<SkTextBlobBuilder*>(builder)->allocRunPosH(
                paint, count, y, reinterpret_cast<const SkRect*>(bounds)));
}

/* This function indenpendant positioning of each glyph */
const sk_text_blob_run_buffer_pos_t* sk_text_blob_allocate_run_pos(
        sk_text_blob_builder_t* builder,
        const sk_font_t* font,
        int count,
        const sk_rect_t* bounds) {
    SkPaint paint;
    font->populate(&paint);
    return reinterpret_cast<const sk_text_blob_run_buffer_pos_t*>(
        &reinterpret_cast<SkTextBlobBuilder*>(builder)->allocRunPos(
                paint, count, reinterpret_cast<const SkRect*>(bounds)));
}

// move to sk_surface.cpp
void sk_canvas_draw_text(sk_canvas_t* c,
                         const sk_text_blob_t* b,
                         float x,
                         float y,
                         const sk_paint_t* p) {
    reinterpret_cast<SkCanvas*>(c)->drawTextBlob(
            reinterpret_cast<const SkTextBlob*>(b), x,y,
            *reinterpret_cast<const SkPaint*>(p));
}
