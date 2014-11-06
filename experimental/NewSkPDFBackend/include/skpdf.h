/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_DEFINED
#define skpdf_DEFINED

#include "SkScalar.h"

class SkBitmap;
class SkData;
class SkDocument;
class SkWStream;

namespace skpdf {

/**
 *  Entrypoint to the skpdf backend (the successor to the SkPDF
 *  backend).  When testing is complete, this will replace
 *  SkDocument::CreatePDF().
 */
SkDocument* CreatePDFDocument(SkWStream* stream,
                              void (*doneProc)(SkWStream*, bool) = NULL,
                              SkData* (*encoder)(size_t*,
                                                 const SkBitmap&) = NULL,
                              SkScalar rasterDpi = 72.0f);

}  // namespace skpdf

#endif  // skpdf_DEFINED
