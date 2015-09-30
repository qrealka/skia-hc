/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL
// DO NOT USE -- FOR INTERNAL TESTING ONLY

#include <stdlib.h>
#include <string.h>

#include "sk_canvas.h"
#include "sk_data.h"
#include "sk_image.h"
#include "sk_paint.h"
#include "sk_shader.h"
#include "sk_surface.h"
#include "sk_text.h"

extern void sk_test_c_api(sk_canvas_t*);

#define W   256
#define H   256

static sk_shader_t* make_shader() {
    sk_point_t pts[] = { { 0, 0 }, { W, H } };
    sk_color_t colors[] = { 0xFF00FF00, 0xFF0000FF };
    return sk_shader_new_linear_gradient(pts, colors, NULL, 2, CLAMP_SK_SHADER_TILEMODE, NULL);
}

static void do_draw(sk_canvas_t* canvas) {
    sk_paint_t* paint = sk_paint_new();
    sk_paint_set_antialias(paint, true);

    sk_paint_set_color(paint, 0xFFFFFFFF);
    sk_canvas_draw_paint(canvas, paint);

    sk_rect_t r = { 10, 10, W - 10, H - 10 };

    sk_paint_set_color(paint, 0xFFFF0000);
    sk_canvas_draw_rect(canvas, &r, paint);

    sk_shader_t* shader = make_shader();
    sk_paint_set_shader(paint, shader);
    sk_shader_unref(shader);

    sk_canvas_draw_oval(canvas, &r, paint);

    sk_paint_delete(paint);
}

void sk_test_c_api(sk_canvas_t* canvas) {
    do_draw(canvas);

    sk_imageinfo_t info = {
        W, H, sk_colortype_get_default_8888(), OPAQUE_SK_ALPHATYPE
    };
    sk_surfaceprops_t surfaceProps = { UNKNOWN_SK_PIXELGEOMETRY };
    sk_surface_t* surf = sk_surface_new_raster(&info, &surfaceProps);
    do_draw(sk_surface_get_canvas(surf));

    sk_image_t* img0 = sk_surface_new_image_snapshot(surf);
    sk_surface_unref(surf);

    sk_canvas_draw_image(canvas, img0, W + 10, 10, NULL);

    sk_data_t* data = sk_image_encode(img0);
    sk_image_unref(img0);

    sk_image_t* img1 = sk_image_new_from_encoded(data, NULL);
    sk_data_unref(data);

    if (img1) {
        sk_canvas_draw_image(canvas, img1, W/2, H/2, NULL);
        sk_image_unref(img1);
    }
}

#define color_set_rgb(r, g, b) sk_color_set_argb(255, r, g, b)

void sk_test_c_api_text(sk_canvas_t* canvas) {
    sk_typeface_t* typeface = sk_typeface_create_default();
    const char text[] = "Hello, Skia!";
    int count = sk_typeface_utf8text_to_glyph_count(typeface, text, strlen(text));
    sk_glyph_id_t* glyphArray = malloc(count * sizeof(sk_glyph_id_t));
    sk_typeface_utf8text_to_glyphs(typeface, text, strlen(text), glyphArray);
    sk_font_t* font = sk_font_new();
    sk_font_set_typeface(font, typeface);
    sk_typeface_unref(typeface);
    sk_font_set_scale_x(font, 1.75f);
    sk_font_set_size(font, 24.0f);
    sk_font_set_flags(font, ANTIALIAS_TEXT_SK_FONT_FLAG, true);
    sk_text_blob_builder_t* builder = sk_text_blob_new_builder();
    sk_glyph_id_t* run = sk_text_blob_allocate_run(builder, font, count, 0, 0, NULL);
    memcpy(run, glyphArray, count * sizeof(sk_glyph_id_t));
    sk_font_delete(font);
    free(glyphArray);
    const sk_text_blob_t* blob = sk_text_blob_build(builder);
    sk_text_blob_builder_delete(builder);
    sk_rect_t bounds;
    sk_text_blob_get_bounds(blob, &bounds);
    sk_paint_t* paint = sk_paint_new();
    sk_point_t center = {128.0f, -32.0f};
    sk_color_t colors[] = {
        color_set_rgb(156, 0, 0),
        color_set_rgb(0, 154, 0),
        color_set_rgb(0, 0, 156),
        color_set_rgb(156, 0, 0),
    };
    sk_shader_t* shader = sk_shader_new_radial_gradient(
            &center, 64.0f, colors, NULL, 4, REPEAT_SK_SHADER_TILEMODE, NULL);
    sk_paint_set_shader(paint, shader);
    sk_shader_unref(shader);
    float x = 0.5 * (256 - bounds.left - bounds.right);
    float y = 0.5 * (64 - bounds.top - bounds.bottom);
    sk_canvas_draw_text(canvas, blob, x, y, paint);
    sk_paint_delete(paint);
    sk_text_blob_unref(blob);
}
