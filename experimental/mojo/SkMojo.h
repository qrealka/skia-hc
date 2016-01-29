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

namespace SkMojo {

SkCanvas* CreateMojoSkCanvas(SkMojo::Picture* dst);

bool PlaybackSkMojoPicture(const SkMojo::Picture& src, SkCanvas* dst);

}  // namespace SkMojo

#endif  // SK_MOJO
#endif  // SkMojo_DEFINED
