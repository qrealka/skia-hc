/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkTextRemoteEncoder_DEFINED
#define SkTextRemoteEncoder_DEFINED

#include "SkCanvas.h"
#include "SkRemote.h"

class SkStream;
class SkWStream;

namespace SkRemote {
class TextEncoder final : public SkRemote::Encoder {
public:
    TextEncoder(SkWStream*);
    ~TextEncoder();

    SkRemote::ID define(const SkMatrix&)        override;
    SkRemote::ID define(const SkRemote::Misc&)  override;
    SkRemote::ID define(const SkPath&)          override;
    SkRemote::ID define(const SkRemote::Stroke&)override;
    SkRemote::ID define(const SkTextBlob*)      override;
    SkRemote::ID define(SkPathEffect*)          override;
    SkRemote::ID define(SkShader*)              override;
    SkRemote::ID define(SkXfermode*)            override;
    SkRemote::ID define(SkMaskFilter*)          override;
    SkRemote::ID define(SkColorFilter*)         override;
    SkRemote::ID define(SkRasterizer*)          override;
    SkRemote::ID define(SkDrawLooper*)          override;
    SkRemote::ID define(SkImageFilter*)         override;
    SkRemote::ID define(SkAnnotation*)          override;

    void undefine(SkRemote::ID) override;
    void saveLayer(ID bounds, CommonIDs, SkCanvas::SaveFlags) override;
    void save() override;
    void restore() override;
    void setMatrix(SkRemote::ID matrix) override;
    void clipPath(SkRemote::ID path, SkRegion::Op, bool aa) override;
    void fillPath(SkRemote::ID path, SkRemote::Encoder::CommonIDs) override;
    void strokePath(SkRemote::ID path,
                    SkRemote::Encoder::CommonIDs,
                    SkRemote::ID stroke) override;
    void   fillText(ID text, SkPoint, CommonIDs)            override;
    void strokeText(ID text, SkPoint, CommonIDs, ID stroke) override;

private:
    uint64_t fNext;
    SkWStream* fWStream;
};

bool TextDecode(SkRemote::Encoder* dst, SkStream* src);
}

#endif  // SkTextRemoteEncoder_DEFINED
