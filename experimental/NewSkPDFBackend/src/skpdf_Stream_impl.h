/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Stream_impl_DEFINED
#define skpdf_Stream_impl_DEFINED

#include "SkFlate.h"
#include "SkMath.h"
#include "skpdf_Types.h"

namespace skpdf {
inline Stream::Stream(SkStreamAsset* asset, Filters filters)
    : fAsset(asset), fFilters(filters) {
}

inline size_t Stream::length() const {
    return fAsset.get() ? fAsset->getLength() : 0;
}


inline bool Stream::hasAsset() const {
    return fAsset.get();
}

inline bool Stream::write(SkWStream* out) const {
    if (fAsset.get()) {
        SkStreamAsset* asset = const_cast<SkStreamAsset*>(fAsset.get());
        if (!out->writeStream(asset, asset->getLength())) {
            return false;
        }
        asset->rewind();
    }
    return true;
}

inline void Stream::deflate() {
    if (fFilters & kFlateDecode_Filters) {
        return;
    }
    SkASSERT(!(fFilters & kDCTDecode_Filters));
    if (SkFlate::HaveFlate()) {
        SkDynamicMemoryWStream deflateBuffer;
        if (SkFlate::Deflate(fAsset.get(), &deflateBuffer)) {
            const size_t kMinimum = strlen("/FlateDecode");
            if ((deflateBuffer.getOffset() + kMinimum) < fAsset->getLength()) {
                fAsset.reset(deflateBuffer.detachAsStream());
                fFilters = Filters(fFilters | kFlateDecode_Filters);
                #if SK_PDF_SEVENBIT_OKAY
                this->base85();
                #endif
                return;
            }
        }
        fAsset->rewind();
    }
    #if SK_PDF_SEVENBIT_OKAY
    this->base85();
    #endif
}

inline void Stream::base85() {
    if (fFilters & kASCII85Decode_Filters) {
        return;
    }
    SkDynamicMemoryWStream ascii85Buffer;
    int column = 0;
    const int kMaxColumn = 64;
    ascii85Buffer.write("\t", 1);
    while (!fAsset->isAtEnd()) {
        uint8_t inBuffer[4] = {0, 0, 0, 0};
        size_t read = fAsset->read(inBuffer, sizeof(inBuffer));
        uint32_t value =
            (inBuffer[0] << 24) |
            (inBuffer[1] << 16) |
            (inBuffer[2] << 8) |
            (inBuffer[3] << 0);
        uint8_t outBuffer[5];
        uint32_t quotient, remainder;
        const uint32_t base = 85;
        SkTDivMod(value, base, &quotient, &remainder);
        value = quotient;
        outBuffer[4] = remainder;
        SkTDivMod(value, base, &quotient, &remainder);
        value = quotient;
        outBuffer[3] = remainder;
        SkTDivMod(value, base, &quotient, &remainder);
        value = quotient;
        outBuffer[2] = remainder;
        SkTDivMod(value, base, &quotient, &remainder);
        outBuffer[0] = quotient;
        outBuffer[1] = remainder;

        if (read < sizeof(inBuffer)) {
            SkASSERT(read + 1 <= sizeof(outBuffer));
            SkASSERT(fAsset->isAtEnd());
            for (size_t i = 0; i < read + 1; ++i) {
                outBuffer[i] += '!';
            }
            ascii85Buffer.write(outBuffer, read + 1);
            break;
        }

        if ((outBuffer[0] | outBuffer[1] | outBuffer[2] |
             outBuffer[3] | outBuffer[4]) == 0) {
            ascii85Buffer.write("z", 1);
            ++column;
            if (column >= kMaxColumn) {
                ascii85Buffer.writeText("\n\t");
                column = 0;
            }
            continue;
        }
        for (size_t i = 0; i < sizeof(outBuffer); ++i) {
            outBuffer[i] += '!';
        }

        ascii85Buffer.write(outBuffer, sizeof(outBuffer));
        column += sizeof(outBuffer);
        if (column >= kMaxColumn) {
            ascii85Buffer.writeText("\n\t");
            column = 0;
        }
    }
    ascii85Buffer.writeText("~>");
    fAsset.reset(ascii85Buffer.detachAsStream());
    fFilters = Filters(fFilters | kASCII85Decode_Filters);
}

inline const char* Stream::filter() const {
    switch (fFilters) {
        case kNoFilter_Filters:
            return "";
        case kDCTDecode_Filters:
            return "/Filter/DCTDecode";
        case kFlateDecode_Filters:
            return "/Filter/FlateDecode";
        case kASCII85Decode_Filters:
            return "/Filter/ASCII85Decode";
        case kDCTDecode_ASCII85Decode_Filters:
            return "/Filter[/ASCII85Decode/DCTDecode]";
        case kFlateDecode_ASCII85Decode_Filters:
            return "/Filter[/ASCII85Decode/FlateDecode]";
        default:
            SkASSERT(false);
            return "";
    }
}

inline void Stream::free() {
    fAsset.free(); 
    fFilters = kNoFilter_Filters;
}

}  // namespace skpdf

#endif  // skpdf_Stream_impl_DEFINED
