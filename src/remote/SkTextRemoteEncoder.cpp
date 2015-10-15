/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "SkPath.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkTHash.h"
#include "SkTextRemoteEncoder.h"
#include "SkTextBlobRunIterator.h"

namespace {
#define COMMANDS(M)      \
    M(defineMatrix)      \
    M(defineMisc)        \
    M(definePath)        \
    M(defineStroke)      \
    M(defineTextBlob)    \
    M(defineAnnotation)  \
    M(defineColorFilter) \
    M(defineDrawLooper)  \
    M(defineImageFilter) \
    M(defineMaskFilter)  \
    M(definePathEffect)  \
    M(defineRasterizer)  \
    M(defineShader)      \
    M(defineXfermode)    \
    M(undefine)          \
    M(save)              \
    M(restore)           \
    M(setMatrix)         \
    M(clipPath)          \
    M(fillPath)          \
    M(strokePath)        \
    M(saveLayer)         \
    M(fillText)          \
    M(strokeText)

#define VERBS(M) \
    M(Move)      \
    M(Line)      \
    M(Quad)      \
    M(Conic)     \
    M(Cubic)     \
    M(Close)     \
    M(Done)

}  // namespace

#define RECT "Rect"
#define CCW_RECT "CCW_Rect"
#define OVAL "Oval"
#define RRECT "RoundRect"
#define PATH "Path"
#define TEXT_RUN "TextRun"
#define DONE "Done"
#define DEFAULT "Default"
#define HORIZ "Horiz"
#define FULL "Full"
#define MODE "Mode"
#define UNKNOWN "???"
#define NULLPTR "nullptr"

#define SK_REMOTE_TYPES(M) \
    M(Matrix)              \
    M(Misc)                \
    M(Path)                \
    M(Stroke)              \
    M(TextBlob)            \
    M(PathEffect)          \
    M(Shader)              \
    M(Xfermode)            \
    M(MaskFilter)          \
    M(ColorFilter)         \
    M(Rasterizer)          \
    M(DrawLooper)          \
    M(ImageFilter)         \
    M(Annotation)

static const char* remote_type(SkRemote::Type t) {
    switch (t) {
#define F(X) case SkRemote::Type::k##X: return #X;
        SK_REMOTE_TYPES(F);
#undef F
    }
    SkASSERT(false); return "";
}
#undef SK_REMOTE_TYPES

static const char* verb_to_string(SkPath::Verb v) {
    switch (v) {
#define CONV(X)               \
    case SkPath::k##X##_Verb: \
        return #X;
        VERBS(CONV)
#undef CONV
    }
    return "VERB_ERROR";
}

static SkXfermode* mode_name_to_mode(const char* name) {
    #define MODENAME(X)                                         \
        if (0 == strcmp(name, #X)) {                            \
            return SkXfermode::Create(SkXfermode::k##X##_Mode); \
        }
    MODENAME(Clear     )
    MODENAME(Src       )
    MODENAME(Dst       )
    MODENAME(SrcOver   )
    MODENAME(DstOver   )
    MODENAME(SrcIn     )
    MODENAME(DstIn     )
    MODENAME(SrcOut    )
    MODENAME(DstOut    )
    MODENAME(SrcATop   )
    MODENAME(DstATop   )
    MODENAME(Xor       )
    MODENAME(Plus      )
    MODENAME(Modulate  )
    MODENAME(Screen    )
    MODENAME(Overlay   )
    MODENAME(Darken    )
    MODENAME(Lighten   )
    MODENAME(ColorDodge)
    MODENAME(ColorBurn )
    MODENAME(HardLight )
    MODENAME(SoftLight )
    MODENAME(Difference)
    MODENAME(Exclusion )
    MODENAME(Multiply  )
    MODENAME(Hue       )
    MODENAME(Saturation)
    MODENAME(Color     )
    MODENAME(Luminosity)
    #undef MODENAME
    return nullptr;
}

static bool skwstream_printf(SkWStream* wstream, const char format[], ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    #ifdef SK_BUILD_FOR_WIN
        int written =
            _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    #else
        int written = vsnprintf(buffer, sizeof(buffer), format, args);
    #endif
    va_end(args);
    if (written >= 0 && written < (int)sizeof(buffer)) {
        return wstream->write(buffer, SkToSizeT(written));
    }
    SkASSERT(false);
    return false;
}

static void write(SkWStream* wstream, SkRemote::ID id) {
    SkAssertResult(skwstream_printf(wstream, "%" PRIX64 " ", id.val()));
}

static void write(SkWStream* wstream, SkRemote::Encoder::CommonIDs commons) {
    write(wstream, commons.misc);
    write(wstream, commons.patheffect);
    write(wstream, commons.shader);
    write(wstream, commons.xfermode);
    write(wstream, commons.maskfilter);
    write(wstream, commons.colorfilter);
    write(wstream, commons.rasterizer);
    write(wstream, commons.looper);
    write(wstream, commons.imagefilter);
    write(wstream, commons.annotation);
}

static void write(SkWStream* wstream, SkRemote::Type type) {
    SkAssertResult(skwstream_printf(wstream, "%" PRIX8 " ", (uint8_t)type));
}

static void write(SkWStream* wstream, char c) {
    SkAssertResult(wstream->write(&c, 1));
}

static void write(SkWStream* wstream, bool v) {
    SkAssertResult(wstream->writeText(v ? "1 " : "0 "));
}

static void write(SkWStream* wstream, int32_t v) {
    SkAssertResult(skwstream_printf(wstream, "%" PRId32 " ", v));
}

static void writeColor(SkWStream* wstream, SkColor c) {
    SkAssertResult(skwstream_printf(wstream, "%08" PRIX32 " ", c));
}

static void write(SkWStream* wstream, uint32_t c) {
    SkAssertResult(skwstream_printf(wstream, "%" PRIX32 " ", c));
}

static void write(SkWStream* wstream, uint16_t v) {
    SkAssertResult(skwstream_printf(wstream, "%" PRIX16 " ", v));
}

static void write(SkWStream* wstream, SkScalar v) {
    SkAssertResult(skwstream_printf(wstream, "%.9g ", v));
}

static void write(SkWStream* wstream, const SkPoint& pt) {
    write(wstream, pt.x());
    write(wstream, pt.y());
}

static void write(SkWStream* wstream, SkPath::Verb v) {
    SkAssertResult(skwstream_printf(wstream, "%s ", verb_to_string(v)));
}

static void write(SkWStream* wStream, const SkRect& rect) {
    write(wStream, rect.left());
    write(wStream, rect.top());
    write(wStream, rect.right());
    write(wStream, rect.bottom());
}

namespace SkRemote {

TextEncoder::TextEncoder(SkWStream* s) : fNext(0), fWStream(s) {}

TextEncoder::~TextEncoder() { fWStream->flush(); }

SkRemote::ID TextEncoder::define(const SkMatrix& mat) {
    SkRemote::ID id(SkRemote::Type::kMatrix, ++fNext);
    fWStream->writeText("defineMatrix ");
    write(fWStream, id);
    for (int i = 0; i < 9; ++i) {
        write(fWStream, mat[i]);
    }
    write(fWStream, '\n');
    return id;
}

SkRemote::ID TextEncoder::define(const SkRemote::Misc& misc) {
    SkRemote::ID id(SkRemote::Type::kMisc, ++fNext);
    fWStream->writeText("defineMisc ");
    write(fWStream, id);
    writeColor(fWStream, misc.fColor);
    write(fWStream, (int32_t)misc.fFilterQuality);
    write(fWStream, misc.fAntiAlias);
    write(fWStream, misc.fDither);
    write(fWStream, '\n');
    return id;
}

SkRemote::ID TextEncoder::define(const SkPath& path) {
    SkRemote::ID id(SkRemote::Type::kPath, ++fNext);
    fWStream->writeText("definePath ");
    write(fWStream, id);
    write(fWStream, (int32_t)path.getFillType());

    SkRect rect;
    bool isClosed;
    SkPath::Direction direction;
    if (path.isRect(&rect, &isClosed, &direction) && isClosed) {
        switch (direction) {
            case SkPath::kCW_Direction:  fWStream->writeText(RECT " ");     break;
            case SkPath::kCCW_Direction: fWStream->writeText(CCW_RECT " "); break;
            default: SkDEBUGFAIL("");
        }
        write(fWStream, rect);
        write(fWStream, '\n');
        return id;
    }
    if (path.isOval(&rect)) {
        fWStream->writeText(OVAL " ");
        write(fWStream, rect);
        write(fWStream, '\n');
        return id;
    }
    SkRRect rRect;
    if (path.isRRect(&rRect)) {
        fWStream->writeText(RRECT " ");
        write(fWStream, rRect.rect());
        for (int i = 0; i < 4; ++i) {
            write(fWStream, (const SkPoint&)rRect.radii((SkRRect::Corner)i));
        }
        write(fWStream, '\n');
        return id;
    }
    fWStream->writeText(PATH " ");
    SkPath::Iter pathIter(path, false);
    while (true) {
        SkPoint pts[4];
        SkPath::Verb verb = pathIter.next(pts, false);
        write(fWStream, verb);
        if (SkPath::kDone_Verb == verb) {
            break;
        }
        switch (verb) {
            case SkPath::kMove_Verb:
                write(fWStream, pts[0]);
                break;
            case SkPath::kLine_Verb:
                write(fWStream, pts[1]);
                break;
            case SkPath::kQuad_Verb:
                write(fWStream, pts[1]);
                write(fWStream, pts[2]);
                break;
            case SkPath::kConic_Verb:
                write(fWStream, pts[1]);
                write(fWStream, pts[2]);
                write(fWStream, pathIter.conicWeight());
                break;
            case SkPath::kCubic_Verb:
                write(fWStream, pts[1]);
                write(fWStream, pts[2]);
                write(fWStream, pts[3]);
                break;
            case SkPath::kClose_Verb:
            case SkPath::kDone_Verb:
                break;
        }
    }
    write(fWStream, '\n');
    return id;
}

SkRemote::ID TextEncoder::define(const SkRemote::Stroke& stroke) {
    SkRemote::ID id(SkRemote::Type::kStroke, ++fNext);
    fWStream->writeText("defineStroke ");
    write(fWStream, id);
    write(fWStream, stroke.fWidth);
    write(fWStream, stroke.fMiter);
    write(fWStream, (int32_t)stroke.fCap);
    write(fWStream, (int32_t)stroke.fJoin);
    write(fWStream, '\n');
    return id;
}

SkRemote::ID TextEncoder::define(const SkTextBlob* blob) {
    SkRemote::ID id(SkRemote::Type::kTextBlob, ++fNext);
    // TODO(https://bug.skia.org/4597): implement this.
    fWStream->writeText("defineTextBlob ");
    write(fWStream, id);
    for (SkTextBlobRunIterator it(blob); !it.done(); it.next()) {
        fWStream->writeText(TEXT_RUN " ");
        uint32_t glyphCount = it.glyphCount();
        SkPaint font;
        it.applyFontToPaint(&font);
        write(fWStream, font.getTextSize());
        write(fWStream, font.getTextScaleX());
        write(fWStream, font.getTextSkewX());
        write(fWStream, (uint32_t)font.getTextAlign());
        write(fWStream, (uint32_t)font.getHinting());
        write(fWStream, (uint32_t)font.getFlags());
        write(fWStream, SkToS32(glyphCount));
        switch (it.positioning()) {
            case SkTextBlob::kDefault_Positioning:
                fWStream->writeText(DEFAULT " ");
                write(fWStream, it.offset());
                break;
            case SkTextBlob::kHorizontal_Positioning:
                fWStream->writeText(HORIZ " ");
                write(fWStream, it.offset().y());
                for (uint32_t i = 0; i < glyphCount; ++i) {
                    write(fWStream, it.pos()[i]);
                }
                break;
            case SkTextBlob::kFull_Positioning:
                fWStream->writeText(FULL " ");
                for (uint32_t i = 0; i < (2 * glyphCount); ++i) {
                    write(fWStream, it.pos()[i]);
                }
                break;
            default:
                SkASSERT(false);
        }
        for (uint32_t i = 0; i < glyphCount; ++i) {
            write(fWStream, SkToU16(it.glyphs()[i]));

        }
    }
    fWStream->writeText(DONE "\n");
    return id;
}
SkRemote::ID TextEncoder::define(SkPathEffect* pathEffect) {
    SkRemote::ID id(SkRemote::Type::kPathEffect, ++fNext);
    fWStream->writeText("definePathEffect ");
    write(fWStream, id);
    // TODO(https://bug.skia.org/4597): implement this.
    //fWStream->writeText(pathEffect ? NULLPTR : UNKNOWN);
    write(fWStream, '\n');
    return id;
}
SkRemote::ID TextEncoder::define(SkShader* shader) {
    SkRemote::ID id(SkRemote::Type::kShader, ++fNext);
    fWStream->writeText("defineShader ");
    write(fWStream, id);
    // TODO(https://bug.skia.org/4597): implement this.
    //fWStream->writeText(shader ? NULLPTR : UNKNOWN);
    write(fWStream, '\n');
    return id;
}
SkRemote::ID TextEncoder::define(SkXfermode* xfermode) {
    SkRemote::ID id(SkRemote::Type::kXfermode, ++fNext);
    fWStream->writeText("defineXfermode ");
    write(fWStream, id);
    SkXfermode::Mode mode;
    if (SkXfermode::AsMode(xfermode, &mode)) {
        fWStream->writeText(MODE " ");
        fWStream->writeText(SkXfermode::ModeName(mode));
    } else {
        // TODO(https://bug.skia.org/4597): implement this.
        fWStream->writeText(UNKNOWN "\n");
    }
    write(fWStream, '\n');
    return id;
}
SkRemote::ID TextEncoder::define(SkMaskFilter* maskFilter) {
    SkRemote::ID id(SkRemote::Type::kMaskFilter, ++fNext);
    fWStream->writeText("defineMaskFilter ");
    write(fWStream, id);
    // TODO(https://bug.skia.org/4597): implement this.
    write(fWStream, '\n');
    return id;
}
SkRemote::ID TextEncoder::define(SkColorFilter* colorFilter) {
    SkRemote::ID id(SkRemote::Type::kColorFilter, ++fNext);
    fWStream->writeText("defineColorFilter ");
    write(fWStream, id);
    // TODO(https://bug.skia.org/4597): implement this.
    write(fWStream, '\n');
    return id;
}
SkRemote::ID TextEncoder::define(SkRasterizer* rasterizer) {
    SkRemote::ID id(SkRemote::Type::kRasterizer, ++fNext);
    fWStream->writeText("defineRasterizer ");
    write(fWStream, id);
    // TODO(https://bug.skia.org/4597): implement this.
    write(fWStream, '\n');
    return id;
}
SkRemote::ID TextEncoder::define(SkDrawLooper* drawLooper) {
    SkRemote::ID id(SkRemote::Type::kDrawLooper, ++fNext);
    fWStream->writeText("defineDrawLooper ");
    write(fWStream, id);
    // TODO(https://bug.skia.org/4597): implement this.
    write(fWStream, '\n');
    return id;
}
SkRemote::ID TextEncoder::define(SkImageFilter* imageFilter) {
    SkRemote::ID id(SkRemote::Type::kImageFilter, ++fNext);
    fWStream->writeText("defineImageFilter ");
    write(fWStream, id);
    // TODO(https://bug.skia.org/4597): implement this.
    write(fWStream, '\n');
    return id;
}
SkRemote::ID TextEncoder::define(SkAnnotation* annotation) {
    SkRemote::ID id(SkRemote::Type::kAnnotation, ++fNext);
    fWStream->writeText("defineAnnotation ");
    write(fWStream, id);
    // TODO(https://bug.skia.org/4597): implement this.
    write(fWStream, '\n');
    return id;
}

void TextEncoder::undefine(SkRemote::ID id) {
    fWStream->writeText("undefine ");
    write(fWStream, id.type());
    write(fWStream, id);
    write(fWStream, '\n');
}

void TextEncoder::save() { fWStream->writeText("save\n"); }

void TextEncoder::restore() { fWStream->writeText("restore\n"); }

void TextEncoder::setMatrix(SkRemote::ID matrix) {
    fWStream->writeText("setMatrix ");
    write(fWStream, matrix);
    write(fWStream, '\n');
}

void TextEncoder::clipPath(SkRemote::ID path, SkRegion::Op op, bool aa) {
    fWStream->writeText("clipPath ");
    write(fWStream, path);
    write(fWStream, (int32_t)op);
    write(fWStream, aa);
    write(fWStream, '\n');
}

void TextEncoder::fillPath(SkRemote::ID path,
                           SkRemote::Encoder::CommonIDs commons) {
    fWStream->writeText("fillPath ");
    write(fWStream, path);
    write(fWStream, commons);
    write(fWStream, '\n');
}

void TextEncoder::strokePath(SkRemote::ID path,
                             SkRemote::Encoder::CommonIDs commons,
                             SkRemote::ID stroke) {
    fWStream->writeText("strokePath ");
    write(fWStream, path);
    write(fWStream, commons);
    write(fWStream, stroke);
    write(fWStream, '\n');
}

void TextEncoder::saveLayer(SkRemote::ID bounds,
                            CommonIDs commons,
                            SkCanvas::SaveFlags flags) {
    fWStream->writeText("saveLayer ");
    write(fWStream, bounds);
    write(fWStream, commons);
    write(fWStream, (uint32_t)flags);
    write(fWStream, '\n');
}
void TextEncoder::fillText(SkRemote::ID text,
                           SkPoint point,
                           CommonIDs commons) {
    fWStream->writeText("fillText ");
    write(fWStream, text);
    write(fWStream, point);
    write(fWStream, commons);
    write(fWStream, '\n');
}
void TextEncoder::strokeText(SkRemote::ID text,
                             SkPoint point,
                             CommonIDs commons,
                             SkRemote::ID stroke) {
    fWStream->writeText("fillText ");
    write(fWStream, text);
    write(fWStream, point);
    write(fWStream, commons);
    write(fWStream, stroke);
    write(fWStream, '\n');
}
}  // namespace SkRemote


////////////////////////////////////////////////////////////////////////////////

// All tokens are space-separated.
// buffer will be nul-terminated.  if it overruns or failes, return 0.
static size_t next_token(SkStream* stream, char* buffer, size_t bufferSize) {
    SkASSERT(stream);
    SkASSERT(buffer);
    SkASSERT(bufferSize > 1);
    char* ptr = &buffer[0];
    const char* end = &buffer[bufferSize];
    do {
        if (stream->isAtEnd() || 1 != stream->read(ptr, 1)) {
            *ptr = '\0';
            return 0;
        }
    } while (isspace(*ptr));
    do {
        ++ptr;
        if (ptr == end ) {
            buffer[0] = '\0';
            SkDebugf("Token too long\n");
            return 0;
        }
        if (stream->isAtEnd() || 1 != stream->read(ptr, 1)) {
            break;  // end of stream is like a space;
        }
    } while (!isspace(*ptr));
    *ptr = '\0';  // replace space with nil.
    SkASSERT(ptr < buffer + bufferSize);
    SkASSERT(ptr > buffer);
    return SkToSizeT(ptr - buffer);
}

static bool scan(SkStream* stream, float* dst) {
    char buffer[80];
    return 0 != next_token(stream, buffer, sizeof(buffer))
        && 1 == sscanf(buffer, "%f", dst);
}
static bool scan_hex(SkStream* stream, uint32_t* dst) {
    char buffer[80];
    return 0 != next_token(stream, buffer, sizeof(buffer))
        && 1 == sscanf(buffer, "%" SCNx32, dst);
}
static bool scan_hex(SkStream* stream, uint16_t* dst) {
    char buffer[80];
    size_t len = next_token(stream, buffer, sizeof(buffer));
    return len != 0 && 1 == sscanf(buffer, "%" SCNx16, dst);
}
static bool scan_hex(SkStream* stream, uint64_t* dst) {
    char buffer[80];
    return 0 != next_token(stream, buffer, sizeof(buffer))
        && 1 == sscanf(buffer, "%" SCNx64, dst);
}
static bool scan_dec(SkStream* stream, int32_t* dst) {
    char buffer[80];
    return 0 != next_token(stream, buffer, sizeof(buffer))
        && 1 == sscanf(buffer, "%" SCNd32, dst);
}
static bool scan(SkStream* stream, bool* dst) {
    char buffer[80];
    int i;
    if (0 == next_token(stream, buffer, sizeof(buffer)) ||
        1 != sscanf(buffer, "%d", &i)) {
        return false;
    }
    *dst = i != 0;
    return true;
}

// scan successive like things into an array.
template <typename T, int Max>
bool scan(SkStream* stream, T (&values)[Max], int N) {
    SkASSERT(N <= Max);
    T* ptr = &values[0];
    const T* end = &values[N];
    while (ptr != end) {
        if (!scan(stream, ptr++)) { return false; }
    }
    return true;
}

// scan successive like things into an array.
template <typename T, int Max>
bool scan_hex(SkStream* stream, T (&values)[Max], int N) {
    SkASSERT(N <= Max);
    T* ptr = &values[0];
    const T* end = &values[N];
    while (ptr != end) {
        if (!scan_hex(stream, ptr++)) { return false; }
    }
    return true;
}


// scan successive like things into an array.
template <typename T, int Max>
bool scan_dec(SkStream* stream, T (&values)[Max], int N) {
    SkASSERT(N <= Max);
    T* ptr = &values[0];
    const T* end = &values[N];
    while (ptr != end) {
        if (!scan_dec(stream, ptr++)) { return false; }
    }
    return true;
}

static bool scan(SkStream* stream, SkRect* rect) {
    SkASSERT(rect);
    SkScalar values[4];
    if (!scan(stream, values, 4)) {
        return false;
    }
    rect->set(values[0], values[1], values[2], values[3]);
    return true;
}

static bool scan(SkStream* stream, SkPoint* point) {
    SkASSERT(point);
    SkScalar values[2];
    if (!scan(stream, values, 2)) {
        SkASSERT(false);
        return false;
    }
    point->set(values[0], values[1]);
    return true;
}

static bool read_verb(SkStream* stream, SkPath::Verb* v) {
    SkASSERT(v);
    char buffer[80];
    if (0 == next_token(stream, buffer, sizeof(buffer))) {
        return false;
    }
    #define CMP(X)                              \
    if (0 == strcmp(buffer, #X)) { \
        *v = SkPath::k##X##_Verb;               \
        return true;                            \
    }
    VERBS(CMP);
    #undef CMP
    return false;
}

static bool scan_id(SkRemote::Type type, SkStream* stream, SkRemote::ID* id) {
    SkASSERT(id);
    uint64_t value;
    if (!scan_hex(stream, &value)) {
        return false;
    } else {
        *id = SkRemote::ID(type, value);
        return true;
    }
}

namespace {
class Decoder {
public:
    SkTHashMap<SkRemote::ID, SkRemote::ID> fMap;
    SkRemote::Encoder* fDst;
    SkStream* fStream;
    Decoder(SkRemote::Encoder* dst, SkStream* s) : fDst(dst), fStream(s) {}
    bool convertID(SkRemote::Type, SkRemote::ID*);
    bool convertCommons(SkRemote::Encoder::CommonIDs*);
    #define HANDLE_COMMAND(X) bool X();
    COMMANDS(HANDLE_COMMAND)
    #undef HANDLE_COMMAND
};
}  // namespace

bool Decoder::defineMatrix() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kMatrix, fStream, &id)) {
        return false;
    }
    SkScalar buffer[9];
    if (!scan(fStream, buffer, 9)) {
        return false;
    }
    SkMatrix mat;
    mat.set9(buffer);
    fMap.set(id, fDst->define(mat));
    return true;
}

bool Decoder::defineMisc() {
    SkRemote::ID id;
    SkRemote::Misc misc;
    int32_t filterQuality;
    if (!scan_id(SkRemote::Type::kMisc, fStream, &id) ||
        !scan_hex(fStream, &misc.fColor) ||
        !scan_dec(fStream, &filterQuality) ||
        !scan(fStream, &misc.fAntiAlias) ||
        !scan(fStream, &misc.fDither)) {
        return false;
    }
    misc.fFilterQuality = (SkFilterQuality)filterQuality;
    fMap.set(id, fDst->define(misc));
    return true;
}

bool Decoder::definePath() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kPath, fStream, &id)) {
        return false;
    }
    SkPath path;
    int32_t fillType;
    if (!scan_dec(fStream, &fillType)) {
        return false;
    }
    path.setFillType((SkPath::FillType)fillType);
    char buffer[80];
    if (0 == next_token(fStream, buffer, sizeof(buffer))) {
        return false;
    }
    if (0 == strcmp(buffer, RECT)) {
        SkRect rect;
        if (!scan(fStream, &rect)) {
            return false;
        }
        path.addRect(rect, SkPath::kCW_Direction);
    } else if (0 == strcmp(buffer, CCW_RECT)) {
        SkRect rect;
        if (!scan(fStream, &rect)) {
            return false;
        }
        path.addRect(rect, SkPath::kCCW_Direction);
    } else if (0 == strcmp(buffer, OVAL)) {
        SkRect rect;
        if (!scan(fStream, &rect)) {
            return false;
        }
        path.addOval(rect);
    } else if (0 == strcmp(buffer, RRECT)) {
        SkRect rect;
        if (!scan(fStream, &rect)) {
            return false;
        }
        SkScalar radii[8];
        if (!scan(fStream, radii, 8)) {
            return false;
        }
        path.addRoundRect(rect, radii);
    } else if (0 == strcmp(buffer, PATH)) {
        while (true) {
            SkPath::Verb verb;
            if (!read_verb(fStream, &verb)) {
                return false;
            }
            if (SkPath::kDone_Verb == verb) {
                break;
            }
            SkScalar x[6];
            switch (verb) {
                case SkPath::kMove_Verb:
                    if (!scan(fStream, x, 2)) {
                        return false;
                    }
                    path.moveTo(x[0], x[1]);
                    break;
                case SkPath::kLine_Verb:
                    if (!scan(fStream, x, 2)) {
                        return false;
                    }
                    path.lineTo(x[0], x[1]);
                    break;
                case SkPath::kQuad_Verb:
                    if (!scan(fStream, x, 4)) {
                        return false;
                    }
                    path.quadTo(x[0], x[1], x[2], x[3]);
                    break;
                case SkPath::kConic_Verb:
                    if (!scan(fStream, x, 5)) {
                        return false;
                    }
                    path.conicTo(x[0], x[1], x[2], x[3], x[4]);
                    break;
                case SkPath::kCubic_Verb:
                    if (!scan(fStream, x, 6)) {
                        return false;
                    }
                    path.cubicTo(x[0], x[1], x[2], x[3], x[4], x[5]);
                    break;
                case SkPath::kClose_Verb:
                    path.close();
                    break;
                case SkPath::kDone_Verb:
                    break;
            }
        }
    } else {
        return false;
    }
    fMap.set(id, fDst->define(path));
    return true;
}

bool Decoder::defineTextBlob() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kTextBlob, fStream, &id)) {
        SkASSERT(false); return false;
    }
    SkTextBlobBuilder builder;
    while (true) {
        char buffer[1024];
        if (0 == next_token(fStream, buffer, sizeof(buffer))) {
            SkASSERT(false); return false;
        }
        if (0 == strcmp(buffer, DONE)) {
            break;
        }
        if (0 != strcmp(buffer, TEXT_RUN)) {
            SkASSERT(false); return false;
        }
        SkPaint font;
        font.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
        SkScalar scalars[3];
        if (!scan(fStream, scalars, 3)) {
            SkASSERT(false); return false;
        }
        font.setTextSize(scalars[0]);
        font.setTextScaleX(scalars[1]);
        font.setTextSkewX(scalars[2]);
        uint32_t values[3];
        if (!scan_hex(fStream, values, 3)) {
            SkASSERT(false); return false;
        }
        font.setTextAlign((SkPaint::Align)values[0]);
        font.setHinting((SkPaint::Hinting)values[0]);
        font.setFlags(values[0]);
        int32_t glyphCount;
        scan_dec(fStream, &glyphCount);
        if (0 == next_token(fStream, buffer, sizeof(buffer))) {
            SkASSERT(false); return false;
        }
        //token = scan(fStream);
        SkTextBlobBuilder::RunBuffer run;
        if (0 == strcmp(buffer, DEFAULT)) {
            SkScalar offset[2];
            if (!scan(fStream, offset, 2)) {
                SkASSERT(false); return false;
            }
            run = builder.allocRun(font, glyphCount, offset[0], offset[1]);
        } else if (0 == strcmp(buffer, HORIZ)) {
            SkScalar offset;
            if (!scan(fStream, &offset)) {
                SkASSERT(false); return false;
            }
            run = builder.allocRunPosH(font, glyphCount, offset);
            for (int32_t i = 0; i < glyphCount; ++i) {
                if (!scan(fStream, &run.pos[i])) { SkASSERT(false); return false; }
            }
        } else if (0 == strcmp(buffer, FULL)) {
            run = builder.allocRunPos(font, glyphCount);
            for (int32_t i = 0; i < (2 * glyphCount); ++i) {
                if (!scan(fStream, &run.pos[i])) { SkASSERT(false); return false; }
            }
        } else { SkASSERT(false); }
        for (int32_t i = 0; i < glyphCount; ++i) {
            if (!scan_hex(fStream, &run.glyphs[i])) { SkASSERT(false); SkASSERT(false); return false; }
        }
    }
    SkAutoTUnref<const SkTextBlob> blob(builder.build());
    fMap.set(id, fDst->define(blob));
    return true;
}

bool Decoder::defineAnnotation() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kAnnotation, fStream, &id)) {
        return false;
    }
    fMap.set(id, fDst->define((SkAnnotation*)nullptr));
    return true;
}
bool Decoder::defineColorFilter() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kColorFilter, fStream, &id)) {
        return false;
    }
    fMap.set(id, fDst->define((SkColorFilter*)nullptr));
    return true;
}
bool Decoder::defineDrawLooper() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kDrawLooper, fStream, &id)) {
        return false;
    }
    fMap.set(id, fDst->define((SkDrawLooper*)nullptr));
    return true;
}
bool Decoder::defineImageFilter() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kImageFilter, fStream, &id)) {
        return false;
    }
    fMap.set(id, fDst->define((SkImageFilter*)nullptr));
    return true;
}
bool Decoder::defineMaskFilter() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kMaskFilter, fStream, &id)) {
        return false;
    }
    fMap.set(id, fDst->define((SkMaskFilter*)nullptr));
    return true;
}
bool Decoder::defineRasterizer() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kRasterizer, fStream, &id)) {
        return false;
    }
    fMap.set(id, fDst->define((SkRasterizer*)nullptr));
    return true;
}
bool Decoder::defineShader() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kShader, fStream, &id)) {
        return false;
    }
    fMap.set(id, fDst->define((SkShader*)nullptr));
    return true;
}
bool Decoder::definePathEffect() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kPathEffect, fStream, &id)) {
        return false;
    }
    fMap.set(id, fDst->define((SkPathEffect*)nullptr));
    return true;
}
bool Decoder::defineXfermode() {
    SkRemote::ID id;
    char buffer[80];
    SkAutoTUnref<SkXfermode> xfermode;
    if (!scan_id(SkRemote::Type::kXfermode, fStream, &id) ||
        0 == next_token(fStream, buffer, sizeof(buffer))) {
        return false;
    }
    if (0 == strcmp(buffer, MODE)) {
        if (0 == next_token(fStream, buffer, sizeof(buffer))) {
            return false;
        }
        xfermode.reset(mode_name_to_mode(buffer));
    } else {
        SkASSERT(0 == strcmp(buffer, UNKNOWN));
    }
    fMap.set(id, fDst->define(xfermode.get()));
    return true;
}

bool Decoder::defineStroke() {
    SkRemote::ID id;
    SkRemote::Stroke stroke;
    int32_t x[2];
    if (!scan_id(SkRemote::Type::kStroke, fStream, &id) ||
        !scan(fStream, &stroke.fWidth) ||
        !scan(fStream, &stroke.fMiter) ||
        !scan_dec(fStream, x, 2)) {
        return false;
    }
    stroke.fCap = (SkPaint::Cap)x[0];
    stroke.fJoin = (SkPaint::Join)x[1];
    fMap.set(id, fDst->define(stroke));
    return true;
}

bool Decoder::undefine() {
    uint32_t type;
    SkRemote::ID id;
    if (!scan_hex(fStream, &type) ||
        !scan_id((SkRemote::Type)type, fStream, &id)) {
        return false;
    }
    if (SkRemote::ID* ptr = fMap.find(id)) {
        fDst->undefine(*ptr);
        return true;
    }
    SkDebugf("missing from map %s %x\n", remote_type(id.type()), (unsigned)id.val());
    return false;
    //return true;
}

bool Decoder::save() {
    fDst->save();
    return true;
}
bool Decoder::restore() {
    fDst->restore();
    return true;
}

bool Decoder::setMatrix() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kMatrix, fStream, &id)) {
        return false;
    }
    if (SkRemote::ID* ptr = fMap.find(id)) {
        fDst->setMatrix(*ptr);
        return true;
    }
    SkDebugf("Matrix %d missing from map", (int)id.val());
    return false;
}
bool Decoder::clipPath() {
    SkRemote::ID id;
    if (!scan_id(SkRemote::Type::kPath, fStream, &id)) {
        return false;
    }
    int32_t op;
    if (!scan_dec(fStream, &op)) {
        return false;
    }
    bool aa;
    if (!scan(fStream, &aa)) {
        return false;
    }
    if (SkRemote::ID* ptr = fMap.find(id)) {
        fDst->clipPath(*ptr, (SkRegion::Op)op, aa);
        return true;
    }
    SkDebugf("missing from map");
    return false;
}

bool Decoder::convertID(SkRemote::Type type, SkRemote::ID* dst) {
    SkRemote::ID id;
    if (!scan_id(type, fStream, &id)) {
        SkASSERT(false);
        return false;
    }
    if (SkRemote::ID* ptr = fMap.find(id)) {
        *dst = *ptr;
        return true;
    }
    SkDebugf("id missing.: %x %x\n", id.type(), id.val());
    return false;
}

bool Decoder::convertCommons(SkRemote::Encoder::CommonIDs* commons) {
    return this->convertID(SkRemote::Type::kMisc,        &commons->misc       )
        && this->convertID(SkRemote::Type::kPathEffect,  &commons->patheffect )
        && this->convertID(SkRemote::Type::kShader,      &commons->shader     )
        && this->convertID(SkRemote::Type::kXfermode,    &commons->xfermode   )
        && this->convertID(SkRemote::Type::kMaskFilter,  &commons->maskfilter )
        && this->convertID(SkRemote::Type::kColorFilter, &commons->colorfilter)
        && this->convertID(SkRemote::Type::kRasterizer,  &commons->rasterizer )
        && this->convertID(SkRemote::Type::kDrawLooper,  &commons->looper     )
        && this->convertID(SkRemote::Type::kImageFilter, &commons->imagefilter)
        && this->convertID(SkRemote::Type::kAnnotation,  &commons->annotation );
}

bool Decoder::fillPath() {
    SkRemote::ID path;
    SkRemote::Encoder::CommonIDs commons;
    if (!this->convertID(SkRemote::Type::kPath, &path) || false) {
        SkDebugf("convertID failed\n");
        return false;
    }
    if (
        !this->convertCommons(&commons)) {
        SkDebugf("convertCommons failed\n");
        return false;
    }
    fDst->fillPath(path, commons);
    return true;
}

bool Decoder::strokePath() {
    SkRemote::ID path, stroke;
    SkRemote::Encoder::CommonIDs commons;
    if (!this->convertID(SkRemote::Type::kPath, &path) ||
        !this->convertCommons(&commons) ||
        !this->convertID(SkRemote::Type::kStroke, &stroke)) {
        return false;
    }
    fDst->strokePath(path, commons, stroke);
    return true;
}

bool Decoder::saveLayer() {
    SkRemote::ID bounds;
    SkRemote::Encoder::CommonIDs commons;
    uint32_t flags;
    if (!this->convertID(SkRemote::Type::kPath, &bounds) ||
        !this->convertCommons(&commons) ||
        !scan_hex(fStream, &flags)) {
        return false;
    }
    fDst->saveLayer(bounds, commons, (SkCanvas::SaveFlags)flags);
    return true;
}
bool Decoder::fillText() {
    SkRemote::ID text;
    SkPoint offset;
    SkRemote::Encoder::CommonIDs commons;
    if (!this->convertID(SkRemote::Type::kTextBlob, &text) ||
        !scan(fStream, &offset) ||
        !this->convertCommons(&commons)) {
        return false;
    }
    fDst->fillText(text, offset, commons);
    return true;
}
bool Decoder::strokeText() {
    SkRemote::ID text, stroke;
    SkPoint offset;
    SkRemote::Encoder::CommonIDs commons;
    if (!this->convertID(SkRemote::Type::kTextBlob, &text) ||
        !scan(fStream, &offset) ||
        !this->convertCommons(&commons) ||
        !this->convertID(SkRemote::Type::kStroke, &stroke)) {
        return false;
    }
    fDst->strokeText(text, offset, commons, stroke);
    return true;
}

bool SkRemote::TextDecode(SkRemote::Encoder* dst, SkStream* stream) {
    Decoder dec(dst, stream);
    while (true) {
        char buffer[80];
        if (0 == next_token(stream, buffer, sizeof(buffer))) {
            return true;  // EOF is okay.
        }
        #define HANDLE_CASE(X)            \
        if (0 == strcmp(buffer, #X)) {    \
            if (!dec.X()) {               \
                SkDebugf("\n" #X " failed\n"); \
                SkASSERT(false);          \
                return false;             \
            }                             \
            continue;                     \
        }
        COMMANDS(HANDLE_CASE);
        #undef HANDLE_CASE
        SkDebugf("\n unhandled case: %s\n", buffer);
    }
    return true;
}

#undef COMMANDS
#undef VERBS
#undef RECT
#undef CCW_RECT
#undef OVAL
#undef RRECT
#undef PATH
#undef TEXT_RUN
#undef DONE
#undef DEFAULT
#undef HORIZ
#undef FULL
