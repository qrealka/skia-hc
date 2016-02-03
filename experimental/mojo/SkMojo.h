/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkMojo_DEFINED
#define SkMojo_DEFINED
#ifdef SK_MOJO

#include "SkMojo.mojom.h"

class SkCanvas;
struct SkRect;

// Utility funtion to create an empty picture ready to write to.
SkMojo::PicturePtr SkMojoCreatePicture(const SkRect& cullRect);

class SkMojoContext {
public:
    static std::unique_ptr<SkMojoContext> New();
    virtual ~SkMojoContext() {}
    virtual SkCanvas* newCanvas(SkMojo::Picture* dst) = 0;
};

bool SkMojoPlaybackPicture(const SkMojo::Picture& src, SkCanvas* dst);

#endif  // SK_MOJO
#endif  // SkMojo_DEFINED
