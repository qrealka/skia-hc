/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdarg.h>

#include "SkCPlusPlusCanvas.h"
#include "SkCanvas.h"
#include "SkPaintDefaults.h"
#include "SkPaintDefaults.h"
#include "SkShader.h"
#include "SkStream.h"

namespace {
class CppCanvas : public SkCanvas {
public:
    CppCanvas(SkWStream*, SkISize);
    ~CppCanvas();

    void willSave() override;
    SkCanvas::SaveLayerStrategy willSaveLayer(const SkRect*,
                                    const SkPaint*,
                                    SkCanvas::SaveFlags) override;
    void willRestore() override;
    void didRestore() override;

    void didConcat(const SkMatrix&) override;
    void didSetMatrix(const SkMatrix&) override;

    void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) override;
    void onDrawDrawable(SkDrawable*) override;
    void onDrawText(const void* text,
                    size_t byteLength,
                    SkScalar x,
                    SkScalar y,
                    const SkPaint& paint) override;
    void onDrawPosText(const void* text,
                       size_t byteLength,
                       const SkPoint pos[],
                       const SkPaint& paint) override;
    void onDrawPosTextH(const void* text,
                        size_t byteLength,
                        const SkScalar xpos[],
                        SkScalar constY,
                        const SkPaint& paint) override;
    void onDrawTextOnPath(const void* text,
                          size_t byteLength,
                          const SkPath& path,
                          const SkMatrix* matrix,
                          const SkPaint& paint) override;
    void onDrawTextBlob(const SkTextBlob* blob,
                        SkScalar x,
                        SkScalar y,
                        const SkPaint& paint) override;
    void onDrawPatch(const SkPoint cubics[12],
                     const SkColor colors[4],
                     const SkPoint texCoords[4],
                     SkXfermode* xmode,
                     const SkPaint& paint) override;

    void onDrawPaint(const SkPaint&) override;
    void onDrawPoints(PointMode,
                      size_t count,
                      const SkPoint pts[],
                      const SkPaint&) override;
    void onDrawRect(const SkRect&, const SkPaint&) override;
    void onDrawOval(const SkRect&, const SkPaint&) override;
    void onDrawRRect(const SkRRect&, const SkPaint&) override;
    void onDrawPath(const SkPath&, const SkPaint&) override;
    void onDrawBitmap(const SkBitmap&,
                      SkScalar left,
                      SkScalar top,
                      const SkPaint*) override;
    void onDrawBitmapRect(const SkBitmap&,
                          const SkRect* src,
                          const SkRect& dst,
                          const SkPaint*,
                          DrawBitmapRectFlags flags) override;
    void onDrawImage(const SkImage*,
                     SkScalar left,
                     SkScalar top,
                     const SkPaint*) override;
    void onDrawImageRect(const SkImage*,
                         const SkRect* src,
                         const SkRect& dst,
                         const SkPaint*) override;
    void onDrawBitmapNine(const SkBitmap&,
                          const SkIRect& center,
                          const SkRect& dst,
                          const SkPaint*) override;
    void onDrawSprite(const SkBitmap&,
                      int left,
                      int top,
                      const SkPaint*) override;
    void onDrawVertices(VertexMode vmode,
                        int vertexCount,
                        const SkPoint vertices[],
                        const SkPoint texs[],
                        const SkColor colors[],
                        SkXfermode* xmode,
                        const uint16_t indices[],
                        int indexCount,
                        const SkPaint&) override;

    void onClipRect(const SkRect& rect,
                    SkRegion::Op op,
                    ClipEdgeStyle edgeStyle) override;
    void onClipRRect(const SkRRect& rrect,
                     SkRegion::Op op,
                     ClipEdgeStyle edgeStyle) override;
    void onClipPath(const SkPath& path,
                    SkRegion::Op op,
                    ClipEdgeStyle edgeStyle) override;
    void onClipRegion(const SkRegion& deviceRgn, SkRegion::Op op) override;

    void onDrawPicture(const SkPicture*,
                       const SkMatrix*,
                       const SkPaint*) override;

    void beginCommentGroup(const char*) override;
    void addComment(const char*, const char*) override;
    void endCommentGroup() override;

    SkSurface* onNewSurface(const SkImageInfo&,
                            const SkSurfaceProps&) override {
        return NULL;
    }

private:
    int fTmpIndex;
    SkWStream* fOut;
    SkTDArray<SkPaint> fPaints;

    SkString serializePaint(const SkPaint&);
};
}  // namespace


////////////////////////////////////////////////////////////////////////////////

static void sk_fprintf(SkWStream* o, const char format[], ...) {
    static const size_t kBufferSize = 1024;
    char buffer[kBufferSize];
    va_list args;
    va_start(args, format);
#ifdef SK_BUILD_FOR_WIN
    int written = _vsnprintf_s(buffer, kBufferSize, _TRUNCATE, format, args);
#else
    int written = vsnprintf(buffer, kBufferSize, format, args);
#endif
    va_end(args);
    o->write(buffer, written);
}

static SkString serialize_rect(const SkRect& r) {
    return SkStringPrintf("SkRect::MakeLTRB(f(%.7g), f(%.7g), f(%.7g), f(%.7g))",
                          r.left(), r.top(), r.right(), r.bottom());
}

static void printMatrix(SkWStream* o,
                        const SkString& name,
                        const SkMatrix& m) {
    if (m.isIdentity()) {
        sk_fprintf(o, "    SkMatrix %s = SkMatrix::I();\n", name.c_str());
        return;
    } else if (m.isScaleTranslate()) {
        SkScalar dx = m.getTranslateX();
        SkScalar dy= m.getTranslateY();
        if (dx != 0.0f || dy != 0.0f) {
            sk_fprintf(o, "    SkMatrix %s = SkMatrix::MakeTrans(f(%.7g), f(%.7g));\n",
                       name.c_str(), dx, dy);
        } else {
            sk_fprintf(o, "    SkMatrix %s = SkMatrix::I();\n", name.c_str());
        }
        SkScalar sx = m.getScaleX();
        SkScalar sy = m.getScaleY();
        if (sx != 1.0f || sy != 1.0f) {
            sk_fprintf( o, "    %s.preScale(f(%.7g), f(%.7g));\n", name.c_str(), sx, sy);
        }
        return;
    }
    sk_fprintf(o, "    SkMatrix %s;\n", name.c_str());
    sk_fprintf(o, "    %s.setAll(\n", name.c_str());
    sk_fprintf(o, "            f(%.7g), f(%.7g), f(%.7g),\n",
                    m.getScaleX(), m.getSkewX(), m.getTranslateX());
    sk_fprintf(o, "            f(%.7g), f(%.7g), f(%.7g),\n",
                    m.getSkewY(), m.getScaleY(), m.getTranslateY());
    sk_fprintf(o, "            f(%.7g), f(%.7g), f(%.7g));\n",
                    m.getPerspX(), m.getPerspY(), m[SkMatrix::kMPersp2]);
}
#define CASE_RETURN(VALUE) case VALUE: return #VALUE 
static const char* stroke_cap(SkPaint::Cap cap) {
    switch (cap) {
        CASE_RETURN(SkPaint::kButt_Cap);
        CASE_RETURN(SkPaint::kRound_Cap);
        CASE_RETURN(SkPaint::kSquare_Cap);
        default: SkASSERT(false); return "???";
    }
}
static const char* stroke_join(SkPaint::Join join) {
    switch (join) {
        CASE_RETURN(SkPaint::kMiter_Join);
        CASE_RETURN(SkPaint::kRound_Join);
        CASE_RETURN(SkPaint::kBevel_Join);
        default: SkASSERT(false); return "???";
    }
}
static const char* text_align(SkPaint::Align align) {
    switch (align) {
        CASE_RETURN(SkPaint::kLeft_Align);
        CASE_RETURN(SkPaint::kCenter_Align);
        CASE_RETURN(SkPaint::kRight_Align);
        default: SkASSERT(false); return "???";
    }
}
static const char* style(SkPaint::Style style) {
    switch (style) {
        CASE_RETURN(SkPaint::kFill_Style);
        CASE_RETURN(SkPaint::kStroke_Style);
        CASE_RETURN(SkPaint::kStrokeAndFill_Style);
        default: SkASSERT(false); return "???";
    }
}
static const char* text_encoding(SkPaint::TextEncoding encoding) {
    switch (encoding) {
        CASE_RETURN(SkPaint::kUTF8_TextEncoding);
        CASE_RETURN(SkPaint::kUTF16_TextEncoding);
        CASE_RETURN(SkPaint::kUTF32_TextEncoding);
        CASE_RETURN(SkPaint::kGlyphID_TextEncoding);
        default: SkASSERT(false); return "???";
    }
}
static const char* hinting(SkPaint::Hinting hinting) {
    switch (hinting) {
        CASE_RETURN(SkPaint::kNo_Hinting);
        CASE_RETURN(SkPaint::kSlight_Hinting);
        CASE_RETURN(SkPaint::kNormal_Hinting);
        CASE_RETURN(SkPaint::kFull_Hinting);
        default: SkASSERT(false); return "???";
    }
}
static const char* filter_quality(SkFilterQuality quality) {
    switch (quality) {
        CASE_RETURN(kNone_SkFilterQuality);
        CASE_RETURN(kLow_SkFilterQuality);
        CASE_RETURN(kMedium_SkFilterQuality);
        CASE_RETURN(kHigh_SkFilterQuality);
        default: SkASSERT(false); return "???";
    }
}

static const char* color_type(SkColorType colorType) {
    switch (colorType) {
        CASE_RETURN(kUnknown_SkColorType);
        CASE_RETURN(kAlpha_8_SkColorType);
        CASE_RETURN(kRGB_565_SkColorType);
        CASE_RETURN(kARGB_4444_SkColorType);
        CASE_RETURN(kRGBA_8888_SkColorType);
        CASE_RETURN(kBGRA_8888_SkColorType);
        CASE_RETURN(kIndex_8_SkColorType);
        CASE_RETURN(kGray_8_SkColorType);
        default: SkASSERT(false); return "???";
    }
}
static const char* alpha_type(SkAlphaType alphaType) {
    switch (alphaType) {
        CASE_RETURN(kUnknown_SkAlphaType);
        CASE_RETURN(kOpaque_SkAlphaType);
        CASE_RETURN(kPremul_SkAlphaType);
        CASE_RETURN(kUnpremul_SkAlphaType);
        default: SkASSERT(false); return "???";
    }
}
static const char* color_profile_type(SkColorProfileType colorProfile) {
    switch (colorProfile) {
        CASE_RETURN(kLinear_SkColorProfileType);
        CASE_RETURN(kSRGB_SkColorProfileType);
        default: SkASSERT(false); return "???";
    }
}
#undef CASE_RETURN

#define CASE_RETURN_SKSTRING(VALUE) case VALUE: return SkString(#VALUE)
static SkString get_color(SkColor color) {
    switch (color) {
        CASE_RETURN_SKSTRING(SK_ColorTRANSPARENT);
        CASE_RETURN_SKSTRING(SK_ColorBLACK);
        CASE_RETURN_SKSTRING(SK_ColorDKGRAY);
        CASE_RETURN_SKSTRING(SK_ColorGRAY);
        CASE_RETURN_SKSTRING(SK_ColorLTGRAY);
        CASE_RETURN_SKSTRING(SK_ColorWHITE);
        CASE_RETURN_SKSTRING(SK_ColorRED);
        CASE_RETURN_SKSTRING(SK_ColorGREEN);
        CASE_RETURN_SKSTRING(SK_ColorBLUE);
        CASE_RETURN_SKSTRING(SK_ColorYELLOW);
        CASE_RETURN_SKSTRING(SK_ColorCYAN);
        CASE_RETURN_SKSTRING(SK_ColorMAGENTA);
        default: return SkStringPrintf("0x%08X", color);
    }
}
#undef CASE_RETURN_SKSTRING

static SkString serialize_bitmap(SkWStream* o, int i, const SkBitmap& bm) {
    SkAutoLockPixels alp(bm);
    if (!bm.getPixels()) { return SkString("???"); }
    switch (bm.bytesPerPixel()) {
        case 4: {
            sk_fprintf(o, "    static const uint32_t bmitmap%dPixels[] = {", i);
            for (int y = 0; y < bm.height(); ++y) {
                int count = 0;
                sk_fprintf(o, "\n    ");
                for (int x = 0; x < bm.width(); ++x) {
                    sk_fprintf(o, "0x%08X, ",
                                    (unsigned)*bm.getAddr32(x, y));
                    if (++count >= (76 / 12)) {
                        sk_fprintf(o, "\n    ");
                        count = 0;
                    }
                }
            }
            break;
        }
        case 2: {
            sk_fprintf(o, "    static const uint16_t bmitmap%dPixels[] = {", i);
            for (int y = 0; y < bm.height(); ++y) {
                int count = 0;
                sk_fprintf(o, "\n    ");
                for (int x = 0; x < bm.width(); ++x) {
                    sk_fprintf(o, "0x%04X, ",
                                    (unsigned)*bm.getAddr16(x, y));
                    if (++count >= (76 / 8)) {
                        sk_fprintf(o, "\n    ");
                        count = 0;
                    }
                }
            }
            break;
        }
        case 1: {
            sk_fprintf(o, "    static const uint8_t bmitmap%dPixels[] = {", i);
            for (int y = 0; y < bm.height(); ++y) {
                int count = 0;
                sk_fprintf(o, "\n    ");
                for (int x = 0; x < bm.width(); ++x) {
                    sk_fprintf(o, "0x%02X, ", (unsigned)*bm.getAddr8(x, y));
                    if (++count >= (76 / 6)) {
                        sk_fprintf(o, "\n    ");
                        count = 0;
                    }
                }
            }
            break;
        }
        default:
            SkASSERT(false);
    }
    sk_fprintf(o, "\n    };\n");
    sk_fprintf(o, "    SkBitmap bitmap%d;\n", i);
    sk_fprintf(o, "    SkImageInfo bmitmap%dInfo = SkImageInfo::Make(\n", i);
    sk_fprintf(o, "            %d, %d, %s, %s, %s);\n",
                    bm.width(), bm.height(), color_type(bm.colorType()),
                    alpha_type(bm.alphaType()),
                    color_profile_type(bm.profileType()));
    if (SkColorTable* ct = bm.getColorTable()) {
        size_t count = ct->count();
        sk_fprintf(o, "    const SkPMColor bitmap%dcolorArray[%u] = {\n",
                   i, (unsigned)count);
        for (size_t j = 0; j < count; ++j) {
            sk_fprintf(o, "    0x%08X\n", (*ct)[j]);
        }
        sk_fprintf(o, "    };\n");
        sk_fprintf(o, "    SkAutoTUnref<SkColorTable> bitmap%dcolorTable(new "
                   "SkColorTable(bitmap%dcolorArray, count));\n", i, i);
        sk_fprintf(o, "    (void)bitmap%d.installPixels(\n", i);
        sk_fprintf(o, "            bmitmap%dInfo, const_cast<void*>(bmitmap%dPixels), "
                   "bmitmap%dInfo.minRowBytes(), bitmap%dcolorTable, NULL, NULL);\n",
                   i, i, i, i);
    } else {
        sk_fprintf(o, "    (void)bitmap%d.installPixels(\n", i);
        sk_fprintf(o, "            bmitmap%dInfo, const_cast<void*>(bmitmap%dPixels), "
                   "bmitmap%dInfo.minRowBytes());\n", i, i, i);
    }
    sk_fprintf(o, "    bitmap%d.setImmutable();\n", i);
    return SkStringPrintf("bitmap%d", i);
}

static void set_flag(SkWStream* o, const char* n, uint32_t flags) {
    uint32_t nonDefaultFlags = flags ^ SkPaintDefaults_Flags;
    struct {
        uint32_t flag;
        const char* command;
    } flagList[] = {
        { SkPaint::kAntiAlias_Flag            , "setAntiAlias"            },
        { SkPaint::kDither_Flag               , "setDither"               },
        { SkPaint::kUnderlineText_Flag        , "setUnderlineText"        },
        { SkPaint::kStrikeThruText_Flag       , "setStrikeThruText"       },
        { SkPaint::kFakeBoldText_Flag         , "setFakeBoldText"         },
        { SkPaint::kLinearText_Flag           , "setLinearText"           },
        { SkPaint::kSubpixelText_Flag         , "setSubpixelText"         },
        { SkPaint::kDevKernText_Flag          , "setDevKernText"          },
        { SkPaint::kLCDRenderText_Flag        , "setLCDRenderText"        },
        { SkPaint::kEmbeddedBitmapText_Flag   , "setEmbeddedBitmapText"   },
        { SkPaint::kAutoHinting_Flag          , "setAutohinted"           },
        { SkPaint::kVerticalText_Flag         , "setVerticalText"         },
        { SkPaint::kDistanceFieldTextTEMP_Flag, "setDistanceFieldTextTEMP"},
    };
    for (size_t i = 0; i < SK_ARRAY_COUNT(flagList); ++i) {
        if (SkToBool(flagList[i].flag & nonDefaultFlags)) {
            const char* value =
                SkToBool(flags & flagList[i].flag) ? "true" : "false";
            sk_fprintf(o, "    %s.%s(%s);\n", n, flagList[i].command, value);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////

SkCanvas* SkCreateCPlusPlusCanvas(SkWStream* o, SkISize s) {
    return SkNEW_ARGS(CppCanvas, (o, s));
}

CppCanvas::CppCanvas(SkWStream* out, SkISize size)
    : SkCanvas(size.width(), size.height())
    , fTmpIndex(0)
    , fOut(out) {
    fOut->writeText("#define f(X) (static_cast<SkScalar>(X))\n");
    fOut->writeText("void draw(SkCanvas* canvas) {\n");
}

CppCanvas::~CppCanvas() {
    fOut->writeText("}\n");
}

void CppCanvas::willSave() {
    fOut->writeText("    canvas->save();\n");
}

SkCanvas::SaveLayerStrategy CppCanvas::willSaveLayer(const SkRect* r,
                                                     const SkPaint* p,
                                                     SkCanvas::SaveFlags) {
    SkString paint, rect;
    if (p) {
        paint = this->serializePaint(*p);
        paint.prepend("&");
    } else {
        paint = "NULL";
    }
    if (r) {
        SkString rval = serialize_rect(*r);
        rect = SkStringPrintf("rect%d", ++fTmpIndex);
        sk_fprintf(fOut, "    SkRect %s = %s;\n", rect.c_str(), rval.c_str());
        rect.prepend("&");
    } else {
        rect = "NULL";
    }
    sk_fprintf(fOut, "    canvas->saveLayer(%s, %s);\n",
                    rect.c_str(), paint.c_str());
    return SkCanvas::kNoLayer_SaveLayerStrategy;
}

void CppCanvas::willRestore() {}

void CppCanvas::didRestore() {
    fOut->writeText("    canvas->restore();\n");
}

void CppCanvas::didConcat(const SkMatrix& matrix) {
    if (matrix.isIdentity()) {
        sk_fprintf(fOut, "    canvas->concat(SkMatrix::I());\n");
        return;
    } else if (matrix.isScaleTranslate()) {
        SkScalar dx = matrix.getTranslateX();
        SkScalar dy = matrix.getTranslateY();
        if (dx != 0.0f || dy != 0.0f) {
            sk_fprintf(fOut, "    canvas->translate(f(%.7g), f(%.7g));\n", dx, dy);
        }
        SkScalar sx = matrix.getScaleX();
        SkScalar sy = matrix.getScaleY();
        if (sx != 1.0f || sy != 1.0f) {
            sk_fprintf(fOut, "    canvas->scale(f(%.7g), f(%.7g));\n", sx, sy);
        }
        return;
    }
    SkString matrixName = SkStringPrintf("matrix%d", ++fTmpIndex);
    printMatrix(fOut, matrixName, matrix);
    sk_fprintf(fOut, "    canvas->concat(%s);\n", matrixName.c_str());
}

void CppCanvas::didSetMatrix(const SkMatrix& matrix) {
    if (matrix.isIdentity()) {
        sk_fprintf(fOut, "    canvas->resetMatrix();\n");
        return;
    }
    SkString matrixName = SkStringPrintf("matrix%d", ++fTmpIndex);
    printMatrix(fOut, matrixName, matrix);
    sk_fprintf(fOut, "    canvas->setMatrix(%s);\n", matrixName.c_str());
}

void CppCanvas::onDrawDRRect(const SkRRect& outer,
                             const SkRRect& inner,
                             const SkPaint& p) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
    // SkString outerRect = serialize_rect(outer);
    // SkString innerRect = serialize_rect(inner);
    // SkString paint = this->serializePaint(p);
    // sk_fprintf(fOut, "    canvas->drawDRRect(%s, %s, %s);\n",
    //                 outerRect, innerRect, paint);
}

void CppCanvas::onDrawDrawable(SkDrawable* drawable) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawText(const void* text,
                           size_t byteLength,
                           SkScalar x,
                           SkScalar y,
                           const SkPaint& paint) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawPosText(const void* text,
                              size_t byteLength,
                              const SkPoint pos[],
                              const SkPaint& paint) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawPosTextH(const void* text,
                               size_t byteLength,
                               const SkScalar xpos[],
                               SkScalar constY,
                               const SkPaint& paint) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawTextOnPath(const void* text,
                                 size_t byteLength,
                                 const SkPath& path,
                                 const SkMatrix* matrix,
                                 const SkPaint& paint) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawTextBlob(const SkTextBlob* blob,
                               SkScalar x,
                               SkScalar y,
                               const SkPaint& paint) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawPatch(const SkPoint cubics[12],
                            const SkColor colors[4],
                            const SkPoint texCoords[4],
                            SkXfermode* xmode,
                            const SkPaint& paint) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawPaint(const SkPaint& p) {
    SkXfermode::Mode mode;
    if (SkXfermode::AsMode(p.getXfermode(), &mode)) {
        SkPaint tmp;
        tmp.setColor(p.getColor());
        tmp.setXfermodeMode(mode);
        if (tmp == p) {
            SkString color = get_color(p.getColor());
            if (SkXfermode::kSrc_Mode == mode) {
                sk_fprintf(fOut, "    canvas->clear(%s);\n", color.c_str());
            } else {
                sk_fprintf(fOut, "    canvas->drawColor(%s, "
                           "SkXfermode::k%s_Mode);\n",
                           color.c_str(), SkXfermode::ModeName(mode));
            }
            return;
        }
    }
    SkString paint = this->serializePaint(p);
    sk_fprintf(fOut, "    canvas->drawPaint(%s);\n", paint.c_str());
}
void CppCanvas::onDrawPoints(PointMode,
                             size_t count,
                             const SkPoint pts[],
                             const SkPaint&) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawRect(const SkRect& r, const SkPaint& p) {
    SkString paint = this->serializePaint(p);
    SkString rect = serialize_rect(r);
    sk_fprintf(fOut, "    canvas->drawRect(%s, %s);\n",
               rect.c_str(), paint.c_str());
}

void CppCanvas::onDrawOval(const SkRect& r, const SkPaint& p) {
    SkString paint = this->serializePaint(p);
    SkString rect = serialize_rect(r);
    sk_fprintf(fOut, "    canvas->drawOval(%s, %s);\n",
               rect.c_str(), paint.c_str());
}

void CppCanvas::onDrawRRect(const SkRRect&, const SkPaint&) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawPath(const SkPath&, const SkPaint&) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawBitmap(const SkBitmap& bm,
                             SkScalar x,
                             SkScalar y,
                             const SkPaint* p) {
    SkString paint;
    if (p) {
        paint = this->serializePaint(*p);
        paint.prepend("&");
    } else {
        paint = "NULL";
    }
    SkString bitmap = serialize_bitmap(fOut, ++fTmpIndex,  bm);
    sk_fprintf(fOut, "    canvas->drawBitmap(%s, f(%.7g), f(%.7g), %s);'\n",
               bitmap.c_str(), x, y, paint.c_str());
}

void CppCanvas::onDrawBitmapRect(const SkBitmap&,
                                 const SkRect* src,
                                 const SkRect& dst,
                                 const SkPaint*,
                                 DrawBitmapRectFlags flags) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawImage(const SkImage*,
                            SkScalar left,
                            SkScalar top,
                            const SkPaint*) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawImageRect(const SkImage*,
                                const SkRect* src,
                                const SkRect& dst,
                                const SkPaint*) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawBitmapNine(const SkBitmap&,
                                 const SkIRect& center,
                                 const SkRect& dst,
                                 const SkPaint*) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawSprite(const SkBitmap&,
                             int left,
                             int top,
                             const SkPaint*) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawVertices(VertexMode vmode,
                               int vertexCount,
                               const SkPoint vertices[],
                               const SkPoint texs[],
                               const SkColor colors[],
                               SkXfermode* xmode,
                               const uint16_t indices[],
                               int indexCount,
                               const SkPaint&) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onClipRect(const SkRect& rect,
                           SkRegion::Op op,
                           ClipEdgeStyle edgeStyle) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onClipRRect(const SkRRect& rrect,
                            SkRegion::Op op,
                            ClipEdgeStyle edgeStyle) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onClipPath(const SkPath& path,
                           SkRegion::Op op,
                           ClipEdgeStyle edgeStyle) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onClipRegion(const SkRegion& deviceRgn, SkRegion::Op op) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::onDrawPicture(const SkPicture*,
                              const SkMatrix*,
                              const SkPaint*) {
    sk_fprintf(fOut, "    SkASSERT(false);  // NOT YET\n");
}

void CppCanvas::beginCommentGroup(const char* g) {
    sk_fprintf(fOut, "    canvas->beginCommentGroup(\"%s\");\n", g);
}

void CppCanvas::addComment(const char* x, const char* y) {
    sk_fprintf(fOut, "    canvas->addComment(\"%s\", \"%s\");\n", x, y);
}

void CppCanvas::endCommentGroup() {
    sk_fprintf(fOut, "    canvas->endCommentGroup();\n");
}

SkString CppCanvas::serializePaint(const SkPaint& p) {
    int index = fPaints.find(p);
    if (index >= 0) {
        return SkStringPrintf("paint%u", index);
    }
    SkString name = SkStringPrintf("paint%u", fPaints.count());
    const char* n = name.c_str();
    (void)SkNEW_PLACEMENT_ARGS(fPaints.append(), SkPaint, (p));
    sk_fprintf(fOut, "    SkPaint %s;\n", n);
    if (SkPaintDefaults_TextSize != p.getTextSize()) {
        sk_fprintf(fOut, "    %s.setTextSize(f(%.7g));\n", n, p.getTextSize());
    }
    if (SK_Scalar1 != p.getTextScaleX()) {
        sk_fprintf(fOut, "    %s.setTextScaleX(f(%.7g));\n", n, p.getTextScaleX());
    }
    if (0 != p.getTextSkewX()) {
        sk_fprintf(fOut, "    %s.setTextSkewX(f(%.7g));\n", n, p.getTextSkewX());
    }
    if (SK_ColorBLACK != p.getColor()) {
        SkString color = get_color(p.getColor());
        sk_fprintf(fOut, "    %s.setColor(%s);\n", n, color.c_str());
    }
    if (0 != p.getStrokeWidth()) {
        sk_fprintf(fOut, "    %s.setStrokeWidth(f(%.7g));\n", n, p.getStrokeWidth());
    }
    if (SkPaintDefaults_MiterLimit != p.getStrokeMiter()) {
        sk_fprintf(fOut, "    %s.setStrokeMiter(f(%.7g));\n", n, p.getStrokeMiter());
    }
    if (SkPaintDefaults_Flags != p.getFlags()) {
        //sk_fprintf(fOut, "    %s.setFlags(0x%X);\n", n, p.getFlags());
        set_flag(fOut, n, p.getFlags());
        // todo: make readable so that each flag is readable.
    }
    if (SkPaint::kDefault_Cap != p.getStrokeCap()) {
        sk_fprintf(fOut, "    %s.setStrokeCap(%s);\n", n,
                   stroke_cap(p.getStrokeCap()));
    }
    if (SkPaint::kDefault_Join != p.getStrokeJoin()) {
        sk_fprintf(fOut, "    %s.setStrokeJoin(%s);\n", n,
                   stroke_join(p.getStrokeJoin()));
    }
    if (SkPaint::kLeft_Align != p.getTextAlign()) {
        sk_fprintf(fOut, "    %s.settTextAlign(%s);\n", n,
                   text_align(p.getTextAlign()));
    }
    if (SkPaint::kFill_Style != p.getStyle()) {
        sk_fprintf(fOut, "    %s.setStyle(%s);\n", n,
                   style(p.getStyle()));
    }
    if (SkPaint::kUTF8_TextEncoding != p.getTextEncoding()) {
        sk_fprintf(fOut, "    %s.setTextEncoding(%s);\n", n,
                   text_encoding(p.getTextEncoding()));
    }
    if (SkPaintDefaults_Hinting != p.getHinting()) {
        sk_fprintf(fOut, "    %s.setHinting(%s);\n", n,
                   hinting(p.getHinting()));
    }
    if (0 != p.getFilterQuality()) {
        sk_fprintf(fOut, "    %s.setFilterQuality(%s);\n", n,
                   filter_quality(p.getFilterQuality()));
    }
    if (SkXfermode* xfermode = p.getXfermode()) {
        SkXfermode::Mode mode;
        if (xfermode->asMode(&mode)) {
            sk_fprintf(fOut, "    %s.setXfermodeMode(SkXfermode::k%s_Mode);\n", n, SkXfermode::ModeName(mode));
        } else 
            sk_fprintf(fOut, "    %s.setXfermode(%p);  // FIXME\n", n, p.getXfermode());
    }
    if (SkShader* shader = p.getShader()) {
        SkString s;
        #ifndef SK_IGNORE_TO_STRING
        shader->toString(&s);
        #endif
        int idx = ++fTmpIndex;
        sk_fprintf(fOut, "    SkAutoTUnref<SkShader> shader%d(\n        /*%s*/\n    );  // FIXME\n", idx, s.c_str());
        sk_fprintf(fOut, "    %s.setShader(shader%d);\n", n, idx);
    }
    if (p.getTypeface()   ) { sk_fprintf(fOut, "    %s.setTypeface(   %p);  // FIXME\n", n, p.getTypeface()   ); }
    if (p.getPathEffect() ) { sk_fprintf(fOut, "    %s.setPathEffect( %p);  // FIXME\n", n, p.getPathEffect() ); }
    if (p.getMaskFilter() ) { sk_fprintf(fOut, "    %s.setMaskFilter( %p);  // FIXME\n", n, p.getMaskFilter() ); }
    if (p.getColorFilter()) { sk_fprintf(fOut, "    %s.setColorFilter(%p);  // FIXME\n", n, p.getColorFilter()); }
    if (p.getRasterizer() ) { sk_fprintf(fOut, "    %s.setRasterizer( %p);  // FIXME\n", n, p.getRasterizer() ); }
    if (p.getLooper()     ) { sk_fprintf(fOut, "    %s.setLooper(     %p);  // FIXME\n", n, p.getLooper()     ); }
    if (p.getImageFilter()) { sk_fprintf(fOut, "    %s.setImageFilter(%p);  // FIXME\n", n, p.getImageFilter()); }
    return name;
}
