/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "SkCanvas.h"
#include "SkDocument.h"
#include "SkForceLinking.h"
#include "SkGraphics.h"
#include "SkNullCanvas.h"
#include "SkPicture.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "ProcStats.h"

#ifdef SK_ENABLE_NEW_SKPDF_BACKEND
#include "skpdf.h"
#endif

__SK_FORCE_IMAGE_DECODER_LINKING;

namespace {
class NullWStream : public SkWStream {
public:
    NullWStream() : fBytesWritten(0) {
    }
    virtual bool write(const void*, size_t size) SK_OVERRIDE {
        fBytesWritten += size;
        return true;
    }
    virtual size_t bytesWritten() const SK_OVERRIDE {
        return fBytesWritten;
    }
    size_t fBytesWritten;
};

SkDocument* CreatePDFDocument(SkWStream* out) {
#ifdef SK_ENABLE_NEW_SKPDF_BACKEND
    if (getenv("SKIA_USE_NEW_SKPDF_BACKEND")) {
        return skpdf::CreatePDFDocument(out);
    }
#endif
    return SkDocument::CreatePDF(out);
}
}  // namespace

const char kUseage[] =
    "USAGE:\n"
    "  multipage_pdf_profiler RENDER PATH\n"
    "RENDER is either 0 or 1\n"
    "PATH is the location of a SKP\n";

int main(int argc, char** argv) {
    if (argc < 3) {
        SkDebugf(kUseage);
        return 1;
    }
    bool render = (atoi(argv[1]) != 0);
    const char* path = argv[2];

    SkAutoGraphics ag;

    SkFILEStream inputStream(path);
    if (!inputStream.isValid()) {
        SkDebugf("Could not open file %s\n", path);
        SkDebugf(kUseage);
        return 2;
    }
    SkAutoTUnref<SkPicture> picture(SkPicture::CreateFromStream(&inputStream));
    if (NULL == picture.get()) {
        SkDebugf("Could not read an SkPicture from %s\n", path);
        SkDebugf(kUseage);
        return 3;
    }

    int width = picture->cullRect().width();
    int height = picture->cullRect().height();

    const int kLetterWidth = 612;
    const int kLetterHeight = 792;
    SkRect letterRect = SkRect::MakeWH(SkIntToScalar(kLetterWidth),
                                       SkIntToScalar(kLetterHeight));

    int xPages = ((width - 1) / kLetterWidth) + 1;
    int yPages = ((height - 1) / kLetterHeight) + 1;

    (void)letterRect; (void)xPages; (void)yPages;

    SkAutoTDelete<SkWStream> out(SkNEW(NullWStream));

    const char* opath = getenv("SKIA_OUTPUT_PDF_PATH");
    if (render && opath && *opath) {
        out.reset(SkNEW_ARGS(SkFILEWStream, (opath)));
    }

    SkAutoTUnref<SkDocument> pdfDocument;
    if (render) {
        pdfDocument.reset(CreatePDFDocument(out.get()));
    }
    SkCanvas* nullCanvas = SkCreateNullCanvas();

    for (int y = 0; y < yPages; ++y) {
        for (int x = 0; x < xPages; ++x) {
            SkCanvas* canvas;
            if (render) {
                canvas = pdfDocument->beginPage(kLetterWidth, kLetterHeight);
            } else {
                canvas = nullCanvas;
            }
            {
                SkAutoCanvasRestore autoCanvasRestore(canvas, true);
                canvas->clipRect(letterRect);
                canvas->translate(SkIntToScalar(-kLetterWidth * x),
                                  SkIntToScalar(-kLetterHeight * y));
                canvas->drawPicture(picture);
            }
            canvas->flush();
            if (render) {
                pdfDocument->endPage();
            }
        }
    }
    if (render) {
        pdfDocument->close();
        pdfDocument.reset(NULL);
    }
    printf(SK_SIZE_T_SPECIFIER "\t%4d\n", inputStream.getLength(),
           sk_tools::getMaxResidentSetSizeMB());
    return 0;
}
