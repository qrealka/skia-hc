/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "fiddle_main.h"
void draw(SkCanvas* canvas) {
    canvas->clear(SK_ColorWHITE);
    SkMatrix matrix;
    matrix.setScale(0.75f, 0.75f);
    matrix.preRotate(30.0f);
    SkShader* shader =
            SkShader::CreateBitmapShader(source, SkShader::kRepeat_TileMode,
                                         SkShader::kRepeat_TileMode, &matrix);
    SkPaint paint;
    paint.setShader(shader);
    shader->unref();
    canvas->drawPaint(paint);
}
