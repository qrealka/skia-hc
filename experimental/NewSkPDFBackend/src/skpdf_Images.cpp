/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkColorPriv.h"
#include "SkData.h"
#include "SkBitmap.h"
#include "SkPixelRef.h"
#include "SkUnPreMultiply.h"
#include "SkString.h"

#include "skpdf_Types.h"
#include "skpdf_Images.h"
#include "skpdf_Stream.h"
#include "skpdf_Utils.h"
#include "skpdf_TokenWriter.h"

void skpdf::WriteImage(const skpdf::Stream& stream, int32_t width,
                       int32_t height, const SkString& colorSpace,
                       int bitsPerComponent, const char* otherKeys,
                       skpdf::Int alphaMask, Int stencilMaskIndex,
                       SkWStream* out) {
    TokenWriter<SK_PDF_MAX_COLUMNS, SK_PDF_INDENT> tok;
    tok.write("<<");
    tok.write("/Type/XObject");
    tok.write("/Subtype/Image");
    tok.write("/Width ");
    tok.writeInt(width);
    tok.write("/Height ");
    tok.writeInt(height);
    tok.write("/ColorSpace");
    tok.write(colorSpace.c_str(), colorSpace.size());
    tok.write("/BitsPerComponent");
    tok.writeInt(bitsPerComponent);
    if (otherKeys) {
        tok.write(otherKeys);
    }
    if (alphaMask > 0) {
        tok.write("/SMask");
        tok.writeInt(alphaMask);
        tok.write("0 R");
    }
    if (stencilMaskIndex > 0) {
        tok.write("/Mask");
        tok.writeInt(stencilMaskIndex);
        tok.write("0 R");
    }
    tok.write(stream.filter());
    tok.write("/Length");
    tok.writeSize(stream.length());
    tok.write(">>");
    tok.write("stream\n");
    stream.write(out);
    tok.write("\nendstream");
}

SkStreamAsset* skpdf::ExtractJpeg(const SkBitmap& bm) {
    if ((NULL == bm.pixelRef())
        || !bm.pixelRefOrigin().isZero()
        || (bm.info().dimensions() != bm.pixelRef()->info().dimensions())) {
        return NULL;
    }
    SkAutoTUnref<SkData> data(bm.pixelRef()->refEncodedData());
    if (!data || (data->size() < 11)) {
        return NULL;
    }
    // 0   1   2   3   4   5   6   7   8   9   10
    // FF  D8  FF  E0  ??  ??  'J' 'F' 'I' 'F' 00 ...
    const uint8_t bytesZeroToThree[] = {0xFF, 0xD8, 0xFF, 0xE0};
    if (0 != memcmp(data->bytes(), bytesZeroToThree,
                    sizeof(bytesZeroToThree))) {
        return NULL;
    }
    const uint8_t bytesSixToTen[] = {'J', 'F', 'I', 'F', 0};
    if (0 != memcmp(data->bytes() + 6, bytesSixToTen,
                    sizeof(bytesSixToTen))) {
        return NULL;
    }
    return SkNEW_ARGS(SkMemoryStream, (data));
}

inline uint8_t unpremul(uint32_t scale, uint32_t component) {
    return static_cast<uint8_t>(SkUnPreMultiply::ApplyScale(scale, component));
}

template <typename T>
T* get_writable_memory(SkMemoryStream* mem) {
    return const_cast<T*>(static_cast<const T*>(mem->getMemoryBase()));
}

inline SkMemoryStream* new_memory(size_t size) {
    return SkNEW_ARGS(SkMemoryStream, (size));
}

inline size_t pixel_count(const SkBitmap& bitmap) {
    return bitmap.width() * bitmap.height();
}

SkStreamAsset* skpdf::ExtractColorTable(const SkBitmap& bitmap) {
    if (bitmap.colorType() != kIndex_8_SkColorType) {
        return NULL;
    }
    const SkColorTable& table = *(bitmap.getColorTable());
    SkAutoTUnref<SkMemoryStream> buffer(new_memory(3 * table.count()));
    uint8_t* dst = get_writable_memory<uint8_t>(buffer);
    for (int i = 0; i < table.count(); i++) {
        uint32_t pmcolor = table[i];
        uint32_t scale = SkUnPreMultiply::GetScale(SkGetPackedA32(pmcolor));
        *(dst++) = unpremul(scale, SkGetPackedR32(pmcolor));
        *(dst++) = unpremul(scale, SkGetPackedG32(pmcolor));
        *(dst++) = unpremul(scale, SkGetPackedB32(pmcolor));
    }
    return buffer.detach();
}

////////////////////////////////////////////////////////////////////////////////

static size_t div_by_eight_and_round_up(size_t x) {
    return x > 0 ? ((x - 1) >> 3) + 1 : 0;
}

static SkStreamAsset* extract_indexed_alpha(const SkBitmap& bitmap) {
    SkAutoTUnref<SkMemoryStream> buffer(new_memory(pixel_count(bitmap)));
    uint8_t* dst = get_writable_memory<uint8_t>(buffer);
    const uint8_t* src = static_cast<const uint8_t*>(bitmap.getPixels());
    const SkColorTable& colorTable = *(bitmap.getColorTable());
    size_t rowBytes = bitmap.rowBytes();
    for (int y = 0; y < bitmap.height(); ++y) {
        for (int x = 0; x < bitmap.width(); ++x) {
            *(dst++) =
                static_cast<uint8_t>(SkGetPackedA32(colorTable[src[x]]));
        }
        src += rowBytes;
    }
    return buffer.detach();
}

SkStreamAsset* extract_n32_alpha(const SkBitmap& bitmap) {
    SkAutoTUnref<SkMemoryStream> buffer(new_memory(pixel_count(bitmap)));
    uint8_t* dst = get_writable_memory<uint8_t>(buffer);
    const uint8_t* src = static_cast<const uint8_t*>(bitmap.getPixels());
    size_t rowBytes = bitmap.rowBytes();
    for (int y = 0; y < bitmap.height(); ++y) {
        const uint32_t* row = reinterpret_cast<const uint32_t*>(src);
        for (int x = 0; x < bitmap.width(); ++x) {
            *(dst++) = static_cast<uint8_t>(SkGetPackedA32(row[x]));
        }
        src += rowBytes;
    }
    return buffer.detach();
}

SkStreamAsset* extract_default_alpha(const SkBitmap& bitmap) {
    SkAutoTUnref<SkMemoryStream> buffer(new_memory(pixel_count(bitmap)));
    uint8_t* dst = get_writable_memory<uint8_t>(buffer);
    for (int y = 0; y < bitmap.height(); ++y) {
        for (int x = 0; x < bitmap.width(); ++x) {
            *(dst++) = static_cast<uint8_t>(SkColorGetA(bitmap.getColor(x, y)));
        }
    }
    return buffer.detach();
}

SkStreamAsset* skpdf::ExtractBitmapAlpha(const SkBitmap& bitmap) {
    switch (bitmap.colorType()) {
        case kN32_SkColorType:
            return extract_n32_alpha(bitmap);
        case kIndex_8_SkColorType:
            return extract_indexed_alpha(bitmap);
        default:
            return extract_default_alpha(bitmap);
    }
    SkASSERT(false); return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static SkStreamAsset* extract_indexed_mask(const SkBitmap& bitmap) {
    SkAutoTUnref<SkMemoryStream> buffer(
            new_memory(div_by_eight_and_round_up(bitmap.width())
                       * bitmap.height()));
    uint8_t* dst = get_writable_memory<uint8_t>(buffer);
    const uint8_t* src = static_cast<const uint8_t*>(bitmap.getPixels());
    const SkColorTable& colorTable = *(bitmap.getColorTable());
    size_t rowBytes = bitmap.rowBytes();
    for (int y = 0; y < bitmap.height(); ++y) {
        int bitIndex = 7;  // start at high-order bit.
        uint8_t value = 0;
        for (int x = 0; x < bitmap.width(); ++x) {
            uint8_t alpha =
                static_cast<uint8_t>(SkGetPackedA32(colorTable[src[x]]));
            if (alpha > 0 && alpha < 255) {
                return NULL;
            }
            value |= (alpha ? 0 : 1) << bitIndex;
            if (0 == bitIndex) {  // have eight bits.
                *(dst++) = value;
                value = 0;
                bitIndex = 7;
            } else {
                --bitIndex;
            }
        }
        if (bitIndex < 7) {
            *(dst++) = value;
        }
        src += rowBytes;
    }
    SkASSERT(dst == get_writable_memory<uint8_t>(buffer) + buffer->getLength());
    return buffer.detach();
}

SkStreamAsset* skpdf::ExtractBitmapMask(const SkBitmap& bitmap) {
    switch (bitmap.colorType()) {
        case kIndex_8_SkColorType:
            return extract_indexed_mask(bitmap);
        default:
            // Not implemented yet.  unlikely to work anyway.
            return NULL;
    }
    SkASSERT(false); return NULL;
}

////////////////////////////////////////////////////////////////////////////////

static SkStreamAsset* extract_n32_rgb(const SkBitmap& bitmap) {
    SkAutoTUnref<SkMemoryStream> buffer(new_memory(3 * pixel_count(bitmap)));
    uint8_t* dst = get_writable_memory<uint8_t>(buffer);
    for (int y = 0; y < bitmap.height(); ++y) {
        uint32_t* src = bitmap.getAddr32(0, y);
        uint32_t* end = src + bitmap.width();
        while (src != end) {
            uint32_t pmcolor = *(src++);
            uint32_t scale = SkUnPreMultiply::GetScale(SkGetPackedA32(pmcolor));
            *(dst++) = unpremul(scale, SkGetPackedR32(pmcolor));
            *(dst++) = unpremul(scale, SkGetPackedG32(pmcolor));
            *(dst++) = unpremul(scale, SkGetPackedB32(pmcolor));
        }
    }
    return buffer.detach();
}

static SkStreamAsset* extract_indexed_colors(const SkBitmap& bitmap) {
    SkASSERT(bitmap.colorType() == kIndex_8_SkColorType);
    SkAutoTUnref<SkMemoryStream> buffer(new_memory(pixel_count(bitmap)));
    uint8_t* dst = get_writable_memory<uint8_t>(buffer);
    const uint8_t* src = static_cast<const uint8_t*>(bitmap.getPixels());
    size_t rowBytes = bitmap.rowBytes();
    for (int y = 0; y < bitmap.height(); ++y) {
        memcpy(dst, src, bitmap.width());
        src += rowBytes;
        dst += bitmap.width();
    }
    return buffer.detach();
}

static SkStreamAsset* extract_bitmap_default(const SkBitmap& bitmap) {
    SkAutoTUnref<SkMemoryStream> buffer(new_memory(3 * pixel_count(bitmap)));
    uint8_t* dst = get_writable_memory<uint8_t>(buffer);
    for (int y = 0; y < bitmap.height(); ++y) {
        for (int x = 0; x < bitmap.width(); ++x) {
            SkColor c = bitmap.getColor(x, y);
            *(dst++) = static_cast<uint8_t>(SkColorGetR(c));
            *(dst++) = static_cast<uint8_t>(SkColorGetG(c));
            *(dst++) = static_cast<uint8_t>(SkColorGetB(c));
        }
    }
    return buffer.detach();
}

SkStreamAsset* skpdf::ExtractBitmapRGB(const SkBitmap& bitmap) {
    switch (bitmap.colorType()) {
        case kN32_SkColorType:
            return extract_n32_rgb(bitmap);
        case kIndex_8_SkColorType:
            return extract_indexed_colors(bitmap);
        default:
            return extract_bitmap_default(bitmap);
    }
    SkASSERT(false); return NULL;
}
