/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "sk_types.h"

extern "C" void sk_test_c_api(sk_canvas_t*);
extern "C" void sk_test_c_api_text(sk_canvas_t*);

DEF_SIMPLE_GM(c_gms, canvas, 640, 480) {
    sk_test_c_api((sk_canvas_t*)canvas);
}
DEF_SIMPLE_GM(c_gms_text, canvas, 256, 64) {
    sk_test_c_api_text((sk_canvas_t*)canvas);
}
