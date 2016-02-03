/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkUnflatten_DEFINED
#define SkUnflatten_DEFINED

#include "SkTypes.h"

class SkColorFilter;
class SkDrawLooper;
class SkImageFilter;
class SkMaskFilter;
class SkPathEffect;
class SkRasterizer;
class SkShader;
class SkXfermode;

// Does this belong in the public API?

SkColorFilter* SkUnflattenColorFilter(const char* type, const void*, size_t);
SkDrawLooper*  SkUnflattenDrawLooper (const char* type, const void*, size_t);
SkImageFilter* SkUnflattenImageFilter(const char* type, const void*, size_t);
SkMaskFilter*  SkUnflattenMaskFilter (const char* type, const void*, size_t);
SkPathEffect*  SkUnflattenPathEffect (const char* type, const void*, size_t);
SkRasterizer*  SkUnflattenRasterizer (const char* type, const void*, size_t);
SkShader*      SkUnflattenShader     (const char* type, const void*, size_t);
SkXfermode*    SkUnflattenXfermode   (const char* type, const void*, size_t);

#endif  // SkUnflatten_DEFINED
