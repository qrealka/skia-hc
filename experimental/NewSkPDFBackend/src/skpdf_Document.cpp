/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkData.h"
#include "SkPixelRef.h"
#include "skpdf.h"
#include "skpdf_Device.h"
#include "skpdf_Document.h"
#include "skpdf_Images.h"
#include "skpdf_Name.h"
#include "skpdf_Stream.h"

namespace skpdf {

// Entrypoint to the skpdf backend (the successor to the SkPDF
// backend).  When testing is complete, this will replace
// SkDocument::CreatePDF().  Exposed in skpdf.h.
SkDocument* CreatePDFDocument(SkWStream* out,
                              void (*done)(SkWStream*, bool),
                              SkData* (*enc)(size_t*, const SkBitmap&),
                              SkScalar dpi) {
    return out ? SkNEW_ARGS(Document, (out, done, enc, dpi)) : NULL;
}

Document::~Document() {
    this->close();
}

SkCanvas* Document::onBeginPage(SkScalar width,
                                SkScalar height,
                                const SkRect& trimBox) {
    if (fDone) {
        SkASSERT(!fDone);
        return NULL;
    }
    fDevice.reset(SkNEW_ARGS(Device, (this, SkSize::Make(width, height))));
    fCanvas.reset(SkNEW_ARGS(SkCanvas, (fDevice.get())));

    fCanvas->clipRect(trimBox);
    fCanvas->translate(trimBox.x(), trimBox.y());

    return fCanvas;
}

void Document::onEndPage() {
    SkASSERT(!fDone);

    fCanvas->flush();
    SkASSERT(fCanvas->unique());  // It is an error to keep a reference
    fCanvas.reset(NULL);          // to this canvas when the device is
                                  // unrefed.

    SkSize pageSize = fDevice->pageSize();
    Stream pageContentStream(fDevice->finalize());
    SkASSERT(pageContentStream.hasAsset());
    fDevice.reset(NULL);  // we are now done with the device.

    #if !(SK_PDF_SEVENBIT_OKAY)
    pageContentStream.deflate();
    #endif
    fTok.comment("Content Stream");
    Int contentStreamIndex = this->writeStream(pageContentStream);
    pageContentStream.free();

    fTok.comment("Page Object");
    Int pageIndex = this->startIndirectObject();
    fTok.beginDict("/Page");
    {
        fTok.write("/Parent");
        fTok.writeIndirectReference(fPageTreeNodeIndex);
        fTok.write("/MediaBox[0 0");
        fTok.writeScalar(pageSize.width());
        fTok.writeScalar(pageSize.height());
        fTok.write("]");
        fTok.write("/Contents");
        fTok.writeIndirectReference(contentStreamIndex);
    }
    fTok.endDict();
    this->endIndirectObject();

    fTok.comment("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");

    fPages.push(pageIndex);  // Save page index.
}

bool Document::onClose(SkWStream*) {
    if (fDone) {
        return false;
    }

    Int documentMetadata = -1;
    // if (fDocumentMetadata) {
    //     documentMetadata = this->writeMetadata(fDocumentMetadata.detach());
    // // 
    // } else {
    //     // TODO(halcanary): instead of writing a document information
    //     // dictionary, create a XMP stream with the given data for
    //     // PDF/A compliance.
    //     this->writeDocumentInformationDictionary();
    // }

    this->writeDocumentInformationDictionary();

    this->writeCatalog(documentMetadata);

    this->writePagesDictionary();

    fTok.comment("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");

    size_t xrefOffset = this->writeXrefTable();

    this->writeTrailer();

    fTok.stream()->writeText("\nstartxref\n");
    fTok.writeSize(xrefOffset);
    fTok.stream()->writeText("\n%%EOF\n");

    fTok.stream()->flush();

    // TODO: Run through Valgrind.
    fFontDict.reset();
    fXObjectDict.reset();
    fExtGStateDict.reset();
    fPatternDict.reset();

    fBitmapMap.reset();
    fShaderMap.reset();
    fPaintMap.reset();
    fSMaskMap.reset();
    fFontMap.reset();

    fDone = true;
    return true;
}

void Document::onAbort() {
}

// SkMetaData& Document::getMetaData() {
//     return fMeta;
// }

// void Document::setDocumentXMPMetadata(SkStreamAsset* data) {
//     fDocumentMetadata.reset(data);
// }

skpdf::Int skpdf::Document::serializeBitmap(const SkBitmap& bitmap) {
    if (bitmap.drawsNothing()) {
        return -1;  // error signal
    }
    SkAutoTUnref<SkStreamAsset> jpeg(ExtractJpeg(bitmap));
    if (jpeg) {
        skpdf::Stream jpegStream(jpeg.detach(),
                                 skpdf::Stream::kDCTDecode_Filters);
        // Assume jpegStream.deflate() won't produce smaller file
        #if SK_PDF_SEVENBIT_OKAY
        jpegStream.base85();
        #endif
        fTok.comment("JPEG");
        Int index = this->startIndirectObject();
        WriteImage(jpegStream, bitmap.width(), bitmap.height(),
                   SkString("/DeviceRGB"), 8, "/ColorTransform 0",
                   -1, -1, this->getStream());
        this->endIndirectObject();
        return index;
    }
    if (bitmap.colorType() == kUnknown_SkColorType) {
        return -1;
    }
    Int alphaMaskIndex = -1;
    Int stencilMaskIndex = -1;
    SkAutoTDelete<SkStreamAsset> colors;
    SkString colorSpace("/DeviceRGB");
    {
        SkAutoLockPixels autoLockPixels(bitmap);
        if (!bitmap.isOpaque() && !SkBitmap::ComputeIsOpaque(bitmap)) {
            SkStreamAsset* stencilMask = ExtractBitmapMask(bitmap);
            if (stencilMask) {
                skpdf::Stream maskOut(stencilMask);
                maskOut.deflate();
                fTok.comment("stencil mask (1 bit-per-pixel");
                stencilMaskIndex = this->startIndirectObject();
                WriteImage(maskOut, bitmap.width(), bitmap.height(),
                           SkString("/DeviceGray"), 1, NULL, -1, -1,
                           this->getStream());
                this->endIndirectObject();
                
            } else {
                // need to serialize alpha channel separately
                skpdf::Stream alphaOut(ExtractBitmapAlpha(bitmap));
                alphaOut.deflate();
                fTok.comment("alpha mask");
                alphaMaskIndex = this->startIndirectObject();
                WriteImage(alphaOut, bitmap.width(), bitmap.height(),
                           SkString("/DeviceGray"), 8, NULL, -1, -1,
                           this->getStream());
                this->endIndirectObject();
            }
        }

        colors.reset(ExtractBitmapRGB(bitmap));

        if (bitmap.colorType() == kIndex_8_SkColorType) {
            skpdf::Stream colorTable(ExtractColorTable(bitmap));
            size_t count = bitmap.getColorTable()->count();
            SkASSERT(colorTable.length() == 3 * count);
            colorTable.deflate();
            fTok.comment("color table");
            Int ctIndex = this->writeStream(colorTable);
            colorSpace.printf("[/Indexed/DeviceRGB %lu %ld 0 R]",
                              (unsigned long)(count - 1),
                              (long)ctIndex);
        }
    }
    skpdf::Stream output(colors.detach());
    output.deflate();

    fTok.comment("bitmap");
    Int index = this->startIndirectObject();
    WriteImage(output, bitmap.width(), bitmap.height(),
               colorSpace, 8, NULL, alphaMaskIndex,
               stencilMaskIndex, this->getStream());
    this->endIndirectObject();
    return index;
}

}  // namespace skpdf
