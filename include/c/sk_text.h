/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef sk_text_DEFINED
#define sk_text_DEFINED

#include "sk_types.h"

SK_C_PLUS_PLUS_BEGIN_GUARD

// move to sk_types.h
typedef struct sk_text_blob_t sk_text_blob_t;

/******************************************************************************/

// move to sk_data.h
SK_API sk_data_t* sk_data_new_from_file(const char* path);

/******************************************************************************/

// move to sk_surface.h
SK_API void sk_canvas_draw_text(sk_canvas_t*,
                                const sk_text_blob_t*,
                                float x,
                                float y,
                                const sk_paint_t*);

/******************************************************************************/

typedef uint16_t sk_glyph_id_t;

typedef struct sk_typeface_t sk_typeface_t;

typedef struct sk_font_t sk_font_t;

SK_API sk_typeface_t* sk_typeface_create_default();
SK_API sk_typeface_t* sk_typeface_create_from_sk_data(sk_data_t*,
                                                      int index);
SK_API void sk_typeface_unref(sk_typeface_t*);

/* Note: you probably want to use harfbuzz <http://harfbuzz.org/> or
   another text shaping engine to convert unicode to glyphs. These two
   functions are provided for testing only. They will not do text
   shaping correctly: this is outside of Skia's domain. */
SK_API int sk_typeface_utf8text_to_glyph_count(sk_typeface_t*,
                                               const char* utf8text,
                                               size_t byteLength);
SK_API int sk_typeface_utf8text_to_glyphs(sk_typeface_t*,
                                          const char* utf8text,
                                          size_t byteLength,
                                          sk_glyph_id_t* glyphArray);


/******************************************************************************/

/**
    Create a new font with default settings:
        typeface : default typeface
        size : 12.0f
        scale_x : 1.0f
        skew : 0.0f
        underline : false
        strikethru : false
*/
SK_API sk_font_t* sk_font_new();
SK_API void sk_font_delete(sk_font_t*);
SK_API void sk_font_set_typeface(sk_font_t*, sk_typeface_t*);
SK_API void sk_font_set_size(sk_font_t*, float);
SK_API void sk_font_set_scale_x(sk_font_t*, float);
SK_API void sk_font_set_skew(sk_font_t*, float);
typedef enum {
    ANTIALIAS_TEXT_SK_FONT_FLAG       = 0x0001,
    UNDERLINE_TEXT_SK_FONT_FLAG       = 0x0008,
    STRIKETHRU_TEXT_SK_FONT_FLAG      = 0x0010,
    VERTICAL_TEXT_SK_FONT_FLAG        = 0x1000,
} sk_text_flags;
SK_API void sk_font_set_flags(sk_font_t*, sk_text_flags flags, bool);

/******************************************************************************/

SK_API void sk_text_blob_unref(const sk_text_blob_t*);
SK_API void sk_text_blob_get_bounds(const sk_text_blob_t*, sk_rect_t* dst);

/******************************************************************************/

typedef struct sk_text_blob_builder_t sk_text_blob_builder_t;
SK_API sk_text_blob_builder_t* sk_text_blob_new_builder();
SK_API void sk_text_blob_builder_delete(sk_text_blob_builder_t*);
SK_API const sk_text_blob_t* sk_text_blob_build(sk_text_blob_builder_t*);

/**
   bounds may be NULL.
 */
/* return pointer to array of size count.*/
/* This function assumes that you want to use the glyphs' default advace. */
SK_API sk_glyph_id_t* sk_text_blob_allocate_run(sk_text_blob_builder_t*,
                                                const sk_font_t* font,
                                                int count,
                                                float x, float y,
                                                const sk_rect_t* bounds);

typedef struct {
    sk_glyph_id_t* glyphs;  // points at array of size count.
    float*         x_pos;  // points at array of size count.
} sk_text_blob_run_buffer_pos_h_t;

/* This function assumes that you do not want to use the glyphs'
   default advances, but that you do want them to line up horizontaly */
SK_API const sk_text_blob_run_buffer_pos_h_t* sk_text_blob_allocate_run_pos_h(
        sk_text_blob_builder_t*,
        const sk_font_t* font,
        int count,
        float y,
        const sk_rect_t* bounds);

typedef struct {
    sk_glyph_id_t* glyphs;  // points at array of size count.
    sk_point_t*    xy_pos;  // points at array of size count.
} sk_text_blob_run_buffer_pos_t;
/* This function indenpendant positioning of each glyph */
SK_API const sk_text_blob_run_buffer_pos_t* sk_text_blob_allocate_run_pos(
        sk_text_blob_builder_t*,
        const sk_font_t* font,
        int count,
        const sk_rect_t* bounds);

SK_C_PLUS_PLUS_END_GUARD

#endif  // sk_text_DEFINED
