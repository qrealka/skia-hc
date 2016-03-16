/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkData.h"
#include "SkDocument.h"
#include "SkImage.h"
#include "SkPerlinNoiseShader.h"
#include "SkStream.h"
#include "ProcStats.h"

struct StdoutWStream : public SkWStream {
    size_t fBytesWritten;
    StdoutWStream() : fBytesWritten(0) {}
    bool write(const void* buffer, size_t size) final {
        fBytesWritten += size;
        if (buffer) {
            return size = fwrite(buffer, 1, size, stdout);
        }
        return true;
    }
    size_t bytesWritten() const final { return fBytesWritten; }
};

const int kImgSize = 2048;
const int N = 32;

static void drawtile(SkCanvas* canvas, int x, int y) {
    const int size = kImgSize / N;
    const float frequency = 1.0f / (1L << N);
    SkPaint paint;
    float fx = frequency * (1 << x);
    float fy = frequency * (1 << y);
    float xp = size * x;
    float yp = size * y;
    SkBitmap bitmap;
    SkDebugf("ALLOC\n");
    bitmap.allocN32Pixels(size, size, true);
    bitmap.eraseColor(SK_ColorWHITE);
    sk_sp<SkShader> noise(
            SkPerlinNoiseShader::CreateFractalNoise(fx, fy, 4, x + N * y));
    paint.setShader(std::move(noise));
    {
        SkCanvas tmpCanvas(bitmap);
        tmpCanvas.drawPaint(paint);
    }
    bitmap.setImmutable();
    // #if 1
    canvas->drawBitmap(bitmap, xp, yp);
    // #else
    // auto image = SkImage::MakeFromBitmap(bitmap);
    // sk_sp<SkData> jpg(image->encode(SkImageEncoder::kJPEG_Type, 75));
    // auto encodedImage = SkImage::MakeFromEncoded(std::move(jpg));
    // canvas->drawImage(encodedImage.get(), xp, yp);
    // #endif
}


int main(int argc, char** argv) {
    StdoutWStream wStream;
    sk_sp<SkDocument> doc(SkDocument::CreatePDF(&wStream));
    SkCanvas* canvas = doc->beginPage(kImgSize, kImgSize);
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            drawtile(canvas, x, y);
        }
    }
    doc->close();
    SkDebugf("%d\n", sk_tools::getMaxResidentSetSizeMB());
}
