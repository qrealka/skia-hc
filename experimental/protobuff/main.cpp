/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
//-*-c++-*-

#include <cstdio>

#include "SkCanvas.h"
#include "SkStream.h"
#include "SkPicture.h"
#include "ProtocolRemoteEncoder.h"

int main(int argc, char** argv) {
    SkAutoTDelete<SkStream> stream(SkStream::NewFromFile("/dev/stdin"));
    SkASSERT(stream);
    SkAutoTUnref<SkPicture> pic(SkPicture::CreateFromStream(stream));
    SkASSERT(pic);
    stream.reset((SkStream*)nullptr);
    SkFILEWStream wstream("/dev/stdout");
    SkAutoTDelete<SkRemote::Encoder> encoder(
            ProtocolRemoteEncoder::CreateEncoder(&wstream));
    SkAutoTDelete<SkRemote::Encoder> cache(
            SkRemote::NewCachingEncoder(encoder));
    SkAutoTDelete<SkCanvas> canvas(SkRemote::NewCanvas(cache));
    canvas->drawPicture(pic);
    return 0;
}
