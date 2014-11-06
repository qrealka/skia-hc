/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Stream_DEFINED
#define skpdf_Stream_DEFINED

#include "skpdf_Utils.h"

namespace skpdf {
class Stream {
public:
    enum Filters {
        kNoFilter_Filters = 0,
        kDCTDecode_Filters = 1 << 0,
        kFlateDecode_Filters = 1 << 1,
        kASCII85Decode_Filters = 1 << 2,
        kDCTDecode_ASCII85Decode_Filters =
           kDCTDecode_Filters | kASCII85Decode_Filters,
        kFlateDecode_ASCII85Decode_Filters =
           kFlateDecode_Filters | kASCII85Decode_Filters,
    };
    // takes sole owership of asset.
    explicit Stream(SkStreamAsset* asset, Filters filters = kNoFilter_Filters);
    size_t length() const;
    bool hasAsset() const;
    bool write(SkWStream*) const;
    void deflate();
    void base85();
    const char* filter() const;
    void free();
private:
    SkAutoTDelete<SkStreamAsset> fAsset;
    Filters fFilters;
};
}  // namespace skpdf

#include "skpdf_Stream_impl.h"

#endif  // skpdf_Stream_DEFINED
