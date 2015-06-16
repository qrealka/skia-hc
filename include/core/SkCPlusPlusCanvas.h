/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkCPlusPlusCanvas_DEFINED
#define SkCPlusPlusCanvas_DEFINED

#include "SkSize.h"
class SkCanvas;
class SkWStream;

/**
 *  Return a new canvas.  On destruction, will finish.  Does not take
 *  ownership of stream.
 */
SkCanvas* SkCreateCPlusPlusCanvas(SkWStream*, SkISize);

#endif  // SkCPlusPlusCanvas_DEFINED
