/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkMojo_DEFINED
#define SkMojo_DEFINED
#ifdef SK_MOJO

#include "SkCanvas.h"
#include "SkMojo.mojom.h"

// Utility funtion to create an empty picture ready to write to.
SkMojo::PicturePtr SkMojoCreatePicture(const SkRect& cullRect);

SkAutoTUnref<SkCanvas> SkMojoCreateCanvas(SkMojo::Picture*, SkMojo::Context*);

bool SkMojoPlaybackPicture(const SkMojo::Picture& src,
                           SkMojo::Context& context,
                           SkCanvas* dst);

#endif  // SK_MOJO
#endif  // SkMojo_DEFINED
