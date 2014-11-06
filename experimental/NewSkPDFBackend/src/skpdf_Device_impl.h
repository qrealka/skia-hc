/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef skpdf_Device_impl_DEFINED
#define skpdf_Device_impl_DEFINED

#include "skpdf_Stream.h"
#include "skpdf_Utils.h"

namespace skpdf {
inline Device::Device(Document* doc, const SkSize& pageSize)
    : fDocument(doc), fPageSize(pageSize) {
    fLegacyBitmap.setInfo(
            SkImageInfo::MakeUnknown(fPageSize.width(), fPageSize.height()));

    // Flip the page upside down.  (BEWARE)
#   if SK_PDF_SEVENBIT_OKAY
    fPageContent.writeText("\t% initial transform to flip coordinates\n");
    fPageContent.writeText("\t");
#   endif
    fPageContent.writeText("1 0 0 -1 0 ");
    Write(&fPageContent, fPageSize.height());
    fPageContent.writeText(" cm ");
}

inline Device::~Device() {
}

inline SkSize Device::pageSize() const {
    return fPageSize;
}

inline SkStreamAsset* Device::finalize() {
    SkASSERT(!this->isFinalized());
    fDocument = NULL;  // mark as finalized

    // This is filler.  Remove and replace with some real implementation.
    // fPageContent.writeText("BT /F1 18 Tf 18 35 Td (HTTP/404!) Tj ET");

    // fDocument takes ownership of this stream
    return fPageContent.detachAsStream();
}

inline bool Device::isFinalized() {
    return NULL == fDocument;
}

inline void Device::comment(const char* s) {
    #if SK_PDF_SEVENBIT_OKAY
    fPageContent.writeText("\n\t% ");
    fPageContent.writeText(s);
    fPageContent.writeText("\n");
    #else
    (void)s;
    #endif
}

}  // namespace skpdf

#endif  // skpdf_Device_impl_DEFINED
