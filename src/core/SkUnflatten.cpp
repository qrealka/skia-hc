/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkFlattenable.h"
#include "SkUnflatten.h"
#include "SkValidatingReadBuffer.h"

static SkFlattenable* unflatten(const char* name,
                                const void* data,
                                size_t size,
                                SkFlattenable::Type expected) {
    SkFlattenable::Type type;
    SkFlattenable::Factory factory = SkFlattenable::NameToFactory(name);
    if (!factory || !SkFlattenable::NameToType(name, &type) || expected != type) {
        return nullptr;
    }
    SkValidatingReadBuffer readBuffer(data, size);
    SkFlattenable* flattenable = factory(readBuffer);
    return readBuffer.isValid() ? flattenable : nullptr;
}

template <typename T>
static T* unflatten(const char* name, const void* data, size_t size) {
    return reinterpret_cast<T*>(unflatten(name, data, size, T::GetFlattenableType()));
}

SkPathEffect* SkUnflattenPathEffect(const char* n, const void* d, size_t s) {
    return unflatten<SkPathEffect>(n, d, s);
}
SkShader* SkUnflattenShader(const char* n, const void* d, size_t s) {
    return unflatten<SkShader>(n, d, s);
}
SkXfermode* SkUnflattenXfermode(const char* n, const void* d, size_t s) {
    return unflatten<SkXfermode>(n, d, s);
}
SkMaskFilter* SkUnflattenMaskFilter(const char* n, const void* d, size_t s) {
    return unflatten<SkMaskFilter>(n, d, s);
}
SkColorFilter* SkUnflattenColorFilter(const char* n, const void* d, size_t s) {
    return unflatten<SkColorFilter>(n, d, s);
}
SkRasterizer* SkUnflattenRasterizer(const char* n, const void* d, size_t s) {
    return unflatten<SkRasterizer>(n, d, s);
}
SkDrawLooper* SkUnflattenDrawLooper(const char* n, const void* d, size_t s) {
    return unflatten<SkDrawLooper>(n, d, s);
}
SkImageFilter* SkUnflattenImageFilter(const char* n, const void* d, size_t s) {
    return unflatten<SkImageFilter>(n, d, s);
}
