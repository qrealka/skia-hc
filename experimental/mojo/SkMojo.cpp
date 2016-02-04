/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkMojo.h"
#ifdef SK_MOJO

#include "SkAnnotation.h"
#include "SkCanvas.h"
#include "SkColorFilter.h"
#include "SkDrawLooper.h"
#include "SkImage.h"
#include "SkImageFilter.h"
#include "SkMaskFilter.h"
#include "SkPathEffect.h"
#include "SkPictureRecorder.h"
#include "SkRSXform.h"
#include "SkRasterizer.h"
#include "SkShader.h"
#include "SkTypeface.h"
#include "SkWriteBuffer.h"
#include "SkXfermode.h"

// Should this be doable with only public APIs?
#include "../../src/core/SkUnflatten.h"
#include "../../src/core/SkTextBlobRunIterator.h"  // Made public?
#include "../private/SkTHash.h"  // Use std::unordered_{set, map}?

namespace {
template <typename K, typename T> 
static bool has(const mojo::Map<K, T>& map, K key) {
    return map.find(key) != map.cend();
}

class Canvas : public SkCanvas {
public:
    Canvas(SkMojo::Picture* dst, SkMojo::Context* context)
        : SkCanvas(1,1), fDst(dst), fContext(context) {
        dst->commands.resize(0); // make it non-null.
    }

protected:
    void willSave() final;
    void didRestore() final;
    SaveLayerStrategy getSaveLayerStrategy(const SaveLayerRec&) final;
    void didConcat(const SkMatrix&) final;
    void didSetMatrix(const SkMatrix&) final;
    void onDrawOval(const SkRect&, const SkPaint&) final;
    void onDrawRect(const SkRect&, const SkPaint&) final;
    void onDrawRRect(const SkRRect&, const SkPaint&) final;
    void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) final;
    void onDrawPath(const SkPath&, const SkPaint&) final;
    void onDrawPaint(const SkPaint&) final;
    void onDrawPoints(PointMode, size_t, const SkPoint[], const SkPaint&) final;
    void onDrawDrawable(SkDrawable*, const SkMatrix*) final;
    void onDrawPicture(const SkPicture*, const SkMatrix*, const SkPaint*) final;
    void onDrawVertices(VertexMode,
                        int,
                        const SkPoint[],
                        const SkPoint[],
                        const SkColor[],
                        SkXfermode*,
                        const uint16_t[],
                        int,
                        const SkPaint&) final;
    void onDrawPatch(const SkPoint[12],
                     const SkColor[4],
                     const SkPoint[4],
                     SkXfermode*,
                     const SkPaint&) final;
    void onDrawAtlas(const SkImage*,
                     const SkRSXform[],
                     const SkRect[],
                     const SkColor[],
                     int,
                     SkXfermode::Mode,
                     const SkRect*,
                     const SkPaint*) final;
    void onDrawBitmap(const SkBitmap&, SkScalar, SkScalar, const SkPaint*) final;
    void onDrawBitmapRect(const SkBitmap&,
                          const SkRect*,
                          const SkRect&,
                          const SkPaint*,
                          SrcRectConstraint) final;
    void onDrawImage(const SkImage*, SkScalar, SkScalar, const SkPaint*) final;
    void onDrawImageRect(const SkImage*,
                         const SkRect*,
                         const SkRect&,
                         const SkPaint*,
                         SrcRectConstraint) final;
    void onDrawBitmapNine(const SkBitmap&, const SkIRect&, const SkRect&, const SkPaint*) final;
    void onDrawImageNine(const SkImage*, const SkIRect&, const SkRect&, const SkPaint*) final;
    void onDrawTextBlob(const SkTextBlob*, SkScalar, SkScalar, const SkPaint&) final;
    void onDrawText(const void*, size_t, SkScalar, SkScalar, const SkPaint&) final;
    void onDrawPosText(const void*, size_t, const SkPoint[], const SkPaint&) final;
    void onDrawPosTextH(const void*, size_t, const SkScalar[], SkScalar, const SkPaint&) final;
    void onClipRect(const SkRect&, SkRegion::Op, ClipEdgeStyle) final;
    void onClipRRect(const SkRRect&, SkRegion::Op, ClipEdgeStyle) final;
    void onClipPath(const SkPath&, SkRegion::Op, ClipEdgeStyle) final;
    void onClipRegion(const SkRegion&, SkRegion::Op) final;

private:
    SkMojo::Picture* fDst;
    SkMojo::Context* fContext;

    static_assert((int)kHard_ClipEdgeStyle == (int)SkMojo::ClipEdgeStyle::HARD, "");
    static_assert((int)kSoft_ClipEdgeStyle == (int)SkMojo::ClipEdgeStyle::SOFT, "");
};


////////////////////////////////////////////////////////////////////////////////


using namespace SkMojo;

static RectPtr convert(const SkRect& r) {
    auto rectPtr = Rect::New();
    rectPtr->top    = r.top();
    rectPtr->left   = r.left();
    rectPtr->bottom = r.bottom();
    rectPtr->right  = r.right();
    return rectPtr;
}

static_assert((int)SkPaint::kFill_Style     == (int)Style::FILL,           "");
static_assert((int)SkPaint::kStroke_Style   == (int)Style::STROKE,         "");
static_assert((int)SkPaint::kStrokeAndFill_Style == (int)Style::STROKE_AND_FILL, "");
static_assert((int)SkPaint::kButt_Cap       == (int)Cap::BUTT,             "");
static_assert((int)SkPaint::kRound_Cap      == (int)Cap::ROUND,            "");
static_assert((int)SkPaint::kSquare_Cap     == (int)Cap::SQUARE,           "");
static_assert((int)SkPaint::kMiter_Join     == (int)Join::MITER,           "");
static_assert((int)SkPaint::kRound_Join     == (int)Join::ROUND,           "");
static_assert((int)SkPaint::kBevel_Join     == (int)Join::BEVEL,           "");
static_assert((int)kNone_SkFilterQuality    == (int)FilterQuality::NONE,   "");
static_assert((int)kLow_SkFilterQuality     == (int)FilterQuality::LOW,    "");
static_assert((int)kMedium_SkFilterQuality  == (int)FilterQuality::MEDIUM, "");
static_assert((int)kHigh_SkFilterQuality    == (int)FilterQuality::HIGH,   "");
static_assert((int)SkPaint::kNo_Hinting     == (int)Hinting::NO,           "");
static_assert((int)SkPaint::kSlight_Hinting == (int)Hinting::SLIGHT,       "");
static_assert((int)SkPaint::kNormal_Hinting == (int)Hinting::NORMAL,       "");
static_assert((int)SkPaint::kFull_Hinting   == (int)Hinting::FULL,         "");
static_assert((int)SkPaint::kLeft_Align     == (int)Align::LEFT,           "");
static_assert((int)SkPaint::kCenter_Align   == (int)Align::CENTER,         "");
static_assert((int)SkPaint::kRight_Align    == (int)Align::RIGHT,          "");

template <class T, class U> static mojo::StructPtr<T> flatten(const U* v) {
    if (!v) { return nullptr; }
    auto r = T::New();
    if (const char* typeName = v->getTypeName()) {
        r->name = typeName;
        SkWriteBuffer writeBuffer;
        SkAutoTUnref<SkPixelSerializer> ps(
                SkImageEncoder::CreatePixelSerializer());
        writeBuffer.setPixelSerializer(ps);
        ((const SkFlattenable*)v)->flatten(writeBuffer);
        r->data.resize(writeBuffer.bytesWritten());
        writeBuffer.writeToMemory(r->data.data());
    } else {
        r->name = "UNKNOWN";
        r->data.resize(0);
    }
    return r;
}

static PaintPtr paint(const SkPaint& p) {
    auto paintPtr = Paint::New();
    paintPtr->path_effect        = flatten<PathEffect >(p.getPathEffect());
    paintPtr->shader             = flatten<Shader     >(p.getShader());
    paintPtr->xfermode           = flatten<Xfermode   >(p.getXfermode());
    paintPtr->mask_filter        = flatten<MaskFilter >(p.getMaskFilter());
    paintPtr->color_filter       = flatten<ColorFilter>(p.getColorFilter() );
    paintPtr->rasterizer         = flatten<Rasterizer >(p.getRasterizer());
    paintPtr->looper             = flatten<DrawLooper >(p.getLooper());
    paintPtr->image_filter       = flatten<ImageFilter>(p.getImageFilter());
    paintPtr->color              = p.getColor();
    paintPtr->stroke_width       = p.getStrokeWidth();
    paintPtr->stroke_miter_limit = p.getStrokeMiter();
    paintPtr->flags              = p.getFlags();
    paintPtr->hinting            = (Hinting)      p.getHinting();
    paintPtr->style              = (Style)        p.getStyle();
    paintPtr->cap                = (Cap)          p.getStrokeCap();
    paintPtr->join               = (Join)         p.getStrokeJoin();
    paintPtr->filter_quality     = (FilterQuality)p.getFilterQuality();
    return paintPtr;
}

template<typename T>
void add_command(SkMojo::Picture* dst, void (CanvasCommand::*m)(T), T cmd) {
    auto command = CanvasCommand::New();
    (command.get()->*m)(std::move(cmd));
    dst->commands.push_back(std::move(command));
}

void Canvas::willSave() {
    add_command(fDst, &CanvasCommand::set_save, SaveCommand::New());
}
void Canvas::didRestore() {
    add_command(fDst, &CanvasCommand::set_restore, RestoreCommand::New());
}
SkCanvas::SaveLayerStrategy Canvas::getSaveLayerStrategy(
        const SkCanvas::SaveLayerRec& rec) {
    auto cmd = SaveLayerCommand::New();
    cmd->bounds = rec.fBounds ? convert(*rec.fBounds) : nullptr;
    cmd->paint = rec.fPaint ? paint(*rec.fPaint) : nullptr;
    cmd->backdrop = flatten<ImageFilter>(rec.fBackdrop);
    cmd->save_layer_flags = rec.fSaveLayerFlags;
    add_command(fDst, &CanvasCommand::set_save_layer, std::move(cmd));
    return kNoLayer_SaveLayerStrategy;
}

static MatrixPtr to_matrix(const SkMatrix& skm) {
    auto m = Matrix::New();
    m->scale_x  = skm[SkMatrix::kMScaleX];
    m->skew_x   = skm[SkMatrix::kMSkewX ];
    m->trans_x  = skm[SkMatrix::kMTransX];
    m->skew_y   = skm[SkMatrix::kMSkewY ];
    m->scale_y  = skm[SkMatrix::kMScaleY];
    m->trans_y  = skm[SkMatrix::kMTransY];
    m->persp_0  = skm[SkMatrix::kMPersp0];
    m->persp_1  = skm[SkMatrix::kMPersp1];
    m->persp_2  = skm[SkMatrix::kMPersp2];
    return m;
};

void Canvas::didConcat(const SkMatrix&) { this->didSetMatrix(this->getTotalMatrix()); }
void Canvas::didSetMatrix(const SkMatrix& m) {
    auto cmd = SetMatrixCommand::New();
    cmd->matrix = to_matrix(m);
    add_command(fDst, &CanvasCommand::set_set_matrix, std::move(cmd));
}

void Canvas::onDrawOval(const SkRect& oval, const SkPaint& p) {
    auto cmd = DrawOvalCommand::New();
    cmd->oval = convert(oval);
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_oval, std::move(cmd));
}

void Canvas::onDrawRect(const SkRect& r, const SkPaint& p) {
    auto cmd = DrawRectCommand::New();
    cmd->rect = convert(r);
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_rect, std::move(cmd));
}

static RoundRectPtr round_rect(const SkRRect& skrr) {
    auto rr = RoundRect::New();
    rr->left          = skrr.rect().left();
    rr->top           = skrr.rect().top();
    rr->right         = skrr.rect().right();
    rr->bottom        = skrr.rect().bottom();
    rr->upper_left_x  = skrr.radii(SkRRect::kUpperLeft_Corner).x();
    rr->upper_left_y  = skrr.radii(SkRRect::kUpperLeft_Corner).y();
    rr->upper_right_x = skrr.radii(SkRRect::kUpperRight_Corner).x();
    rr->upper_right_y = skrr.radii(SkRRect::kUpperRight_Corner).y();
    rr->lower_right_x = skrr.radii(SkRRect::kLowerRight_Corner).x();
    rr->lower_right_y = skrr.radii(SkRRect::kLowerRight_Corner).y();
    rr->lower_left_x  = skrr.radii(SkRRect::kLowerLeft_Corner).x();
    rr->lower_left_y  = skrr.radii(SkRRect::kLowerLeft_Corner).y();
    return rr;
}

void Canvas::onDrawRRect(const SkRRect& rrect, const SkPaint& p) {
    auto cmd = DrawRoundRectCommand::New();
    cmd->round_rect = round_rect(rrect);
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_round_rect, std::move(cmd));
}

void Canvas::onDrawDRRect(const SkRRect& outside,
                          const SkRRect& inside,
                          const SkPaint& p) {
    auto cmd = DrawDoubleRoundRectCommand::New();
    cmd->outside = round_rect(outside);
    cmd->inside = round_rect(inside);
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_double_round_rect, std::move(cmd));
}

static_assert((int)SkPath::kWinding_FillType        == (int)FillType::WINDING,          "");
static_assert((int)SkPath::kEvenOdd_FillType        == (int)FillType::EVEN_ODD,         "");
static_assert((int)SkPath::kInverseWinding_FillType == (int)FillType::INVERSE_WINDING,  "");
static_assert((int)SkPath::kInverseEvenOdd_FillType == (int)FillType::INVERSE_EVEN_ODD, "");

template<typename T>
void add_verb(SkMojo::Path* dst, void (PathVerb::*m)(T), T verb) {
    auto verbUnion = PathVerb::New();
    (verbUnion.get()->*m)(std::move(verb));
    dst->verbs.push_back(std::move(verbUnion));
}

static PathPtr convert(const SkPath& skpath) {
    auto p = Path::New();
    p->fill_type = (FillType)skpath.getFillType();
    p->verbs.resize(0);
    SkPath::Iter pathIter(skpath, false);
    while (true) {
        SkPoint pts[4];
        SkPath::Verb verb = pathIter.next(pts, false);
        switch (verb) {
            case SkPath::kDone_Verb:
                return p;
            case SkPath::kMove_Verb: {
                auto v = MovePathVerb::New();
                v->end_x = pts[0].x();
                v->end_y = pts[0].y();
                add_verb(p.get(), &PathVerb::set_move, std::move(v));
                break;
            }
            case SkPath::kLine_Verb: {
                auto v = LinePathVerb::New();
                v->end_x = pts[1].x();
                v->end_y = pts[1].y();
                add_verb(p.get(), &PathVerb::set_line, std::move(v));
                break;
            }
            case SkPath::kQuad_Verb: {
                auto v = QuadPathVerb::New();
                v->control_x = pts[1].x();
                v->control_y = pts[1].y();
                v->end_x = pts[2].x();
                v->end_y = pts[2].y();
                add_verb(p.get(), &PathVerb::set_quad, std::move(v));
                break;
            }
            case SkPath::kConic_Verb: {
                auto v = ConicPathVerb::New();
                v->control_x = pts[1].x();
                v->control_y = pts[1].y();
                v->end_x     = pts[2].x();
                v->end_y     = pts[2].y();
                v->weight    = pathIter.conicWeight();
                add_verb(p.get(), &PathVerb::set_conic, std::move(v));
                break;
            }
            case SkPath::kCubic_Verb: {
                auto v = CubicPathVerb::New();
                v->control_1_x = pts[1].x();
                v->control_1_y = pts[1].y();
                v->control_2_x = pts[2].x();
                v->control_2_y = pts[2].y();
                v->end_x       = pts[3].x();
                v->end_y       = pts[4].y();
                add_verb(p.get(), &PathVerb::set_cubic, std::move(v));
                break;
            }
            case SkPath::kClose_Verb: {
                auto v = ClosePathVerb::New();
                add_verb(p.get(), &PathVerb::set_close, std::move(v));
                break;
            }
        }
    }
}

static uint32_t convert(const SkPath& skpath, Context* context) {
    uint32_t id = skpath.getGenerationID();
    if (!has(context->paths, id)) {
        context->paths.insert(id, convert(skpath));
    }
    return id;
}

void Canvas::onDrawPath(const SkPath& skpath, const SkPaint& p) {
    auto cmd = DrawPathCommand::New();
    cmd->path = convert(skpath, fContext);
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_path, std::move(cmd));
}

void Canvas::onDrawPaint(const SkPaint& p) {
    auto cmd = DrawPaintCommand::New();
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_paint, std::move(cmd));
}

static_assert((int)SkCanvas::kPoints_PointMode  == (int)PointMode::POINTS,  "");
static_assert((int)SkCanvas::kLines_PointMode   == (int)PointMode::LINES,   "");
static_assert((int)SkCanvas::kPolygon_PointMode == (int)PointMode::POLYGON, "");

void Canvas::onDrawPoints(SkCanvas::PointMode mode, size_t count,
                          const SkPoint pts[], const SkPaint& p) {
    auto cmd = DrawPointsCommand::New();
    cmd->point_mode = (SkMojo::PointMode)mode;
    cmd->points.resize(2 * count);
    for (size_t i = 0; i < count; ++i) {
        cmd->points[2 * i    ] = pts[i].x();
        cmd->points[2 * i + 1] = pts[i].y();
    }
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_points, std::move(cmd));
}

void Canvas::onDrawDrawable(SkDrawable* drawable, const SkMatrix* matrix) {
    this->SkCanvas::onDrawDrawable(drawable, matrix);  // seems good to me;
}

PicturePtr convert(const SkPicture* pic, Context* context) {
    SkASSERT(pic);
    auto picture = Picture::New();
    picture->cull_rect = convert(pic->cullRect());
    Canvas pictureCanvas(picture.get(), context);
    pic->playback(&pictureCanvas);
    return picture;
}

void Canvas::onDrawPicture(const SkPicture* pic, const SkMatrix* m, const SkPaint* p) {
    uint32_t id = pic->uniqueID();
    if (!has(fContext->pictures, id)) {
        fContext->pictures.insert(id, convert(pic, fContext));
    }
    auto cmd = DrawPictureCommand::New();
    cmd->picture = id;
    cmd->matrix = m ? to_matrix(*m) : nullptr;
    cmd->paint = p ? paint(*p) : nullptr;
    add_command(fDst, &CanvasCommand::set_draw_picture, std::move(cmd));
}

static_assert((int)SkCanvas::kTriangles_VertexMode     ==
              (int)VertexMode::TRIANGLES     ,  "");
static_assert((int)SkCanvas::kTriangleStrip_VertexMode ==
              (int)VertexMode::TRIANGLE_STRIP,  "");
static_assert((int)SkCanvas::kTriangleFan_VertexMode   ==
              (int)VertexMode::TRIANGLE_FAN  ,  "");

template<typename T>
static void convert(int count, const T* src, mojo::Array<T>* dst) {
    if (src) {
        dst->resize(count);
        static_assert(std::is_trivial<T>::value, "can memcpy array");
        memcpy(dst->data(), src, sizeof(T) * count);
    } else {
        dst->resize(0);
    }
}

static void convert(int count, const SkPoint* src, mojo::Array<float>* dst) {
    static_assert(std::is_trivial<SkPoint>::value, "can memcpy array");
    static_assert(sizeof(SkPoint) == 2 * sizeof(float), "point:=x,y");
    convert(count * 2, reinterpret_cast<const float*>(src), dst);
}

void Canvas::onDrawVertices(VertexMode vmode,
                            int vertexCount,
                            const SkPoint vertices[],
                            const SkPoint texs[],
                            const SkColor colors[],
                            SkXfermode* xmode,
                            const uint16_t indices[],
                            int indexCount,
                            const SkPaint& p) {
    auto cmd = DrawVerticesCommand::New();
    cmd->vertex_mode = (SkMojo::VertexMode)vmode;
    convert(vertexCount, vertices, &cmd->vertices);
    convert(vertexCount, texs, &cmd->texs);
    convert(vertexCount, colors, &cmd->colors);
    cmd->xfermode = flatten<Xfermode>(xmode);
    convert(indexCount, indices, &cmd->indices);
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_vertices, std::move(cmd));
}

void Canvas::onDrawPatch(const SkPoint cubics[12],
                         const SkColor colors[4],
                         const SkPoint texCoords[4],
                         SkXfermode* xmode,
                         const SkPaint& p) {
    auto cmd = DrawPatchCommand::New();
    cmd->cubic_0_x = cubics[0].x();
    cmd->cubic_0_y = cubics[0].y();
    cmd->cubic_1_x = cubics[1].x();
    cmd->cubic_1_y = cubics[1].y();
    cmd->cubic_2_x = cubics[2].x();
    cmd->cubic_2_y = cubics[2].y();
    cmd->cubic_3_x = cubics[3].x();
    cmd->cubic_3_y = cubics[3].y();
    cmd->cubic_4_x = cubics[4].x();
    cmd->cubic_4_y = cubics[4].y();
    cmd->cubic_5_x = cubics[5].x();
    cmd->cubic_5_y = cubics[5].y();
    cmd->cubic_6_x = cubics[6].x();
    cmd->cubic_6_y = cubics[6].y();
    cmd->cubic_7_x = cubics[7].x();
    cmd->cubic_7_y = cubics[7].y();
    cmd->cubic_8_x = cubics[8].x();
    cmd->cubic_8_y = cubics[8].y();
    cmd->cubic_9_x = cubics[9].x();
    cmd->cubic_9_y = cubics[9].y();
    cmd->cubic_10_x = cubics[10].x();
    cmd->cubic_10_y = cubics[10].y();
    cmd->cubic_11_x = cubics[11].x();
    cmd->cubic_11_y = cubics[11].y();
    cmd->color_0 = colors[0];
    cmd->color_1 = colors[1];
    cmd->color_2 = colors[2];
    cmd->color_3 = colors[3];
    cmd->tex_coords_0_x = texCoords[0].x();
    cmd->tex_coords_0_y = texCoords[0].y();
    cmd->tex_coords_1_x = texCoords[1].x();
    cmd->tex_coords_1_y = texCoords[1].y();
    cmd->tex_coords_2_x = texCoords[2].x();
    cmd->tex_coords_2_y = texCoords[2].y();
    cmd->tex_coords_3_x = texCoords[3].x();
    cmd->tex_coords_3_y = texCoords[3].y();
    cmd->xfermode = flatten<Xfermode>(xmode);
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_patch, std::move(cmd));
}

static uint32_t define_image(Context* context,
                             const SkImage* img,
                             SkMojo::Picture* dst) {
    SkASSERT(img);
    uint32_t id = img->uniqueID();
    if (!has(context->images, id)) {
        // TODO(halcanary): Find some mojoish way to share SkData cross-process.
        // TODO(halcanary): Add custom SkPixelSerializer to this class.
        SkAutoTUnref<SkData> encoded(img->encode());
        if (encoded) {  // TODO:assert on nullptr.
            auto bytes = mojo::Array<uint8_t>::New(encoded->size());
            memcpy(bytes.data(), encoded->data(), encoded->size());
            context->images.insert(id, std::move(bytes));
        } else {
            context->images.insert(id, mojo::Array<uint8_t>());
        }
    }
    return id;
}

static_assert((int)SkXfermode::kClear_Mode      == (int)XfermodeMode::CLEAR,       "");
static_assert((int)SkXfermode::kSrc_Mode        == (int)XfermodeMode::SRC,         "");
static_assert((int)SkXfermode::kDst_Mode        == (int)XfermodeMode::DST,         "");
static_assert((int)SkXfermode::kSrcOver_Mode    == (int)XfermodeMode::SRC_OVER,    "");
static_assert((int)SkXfermode::kDstOver_Mode    == (int)XfermodeMode::DST_OVER,    "");
static_assert((int)SkXfermode::kSrcIn_Mode      == (int)XfermodeMode::SRC_IN,      "");
static_assert((int)SkXfermode::kDstIn_Mode      == (int)XfermodeMode::DST_IN,      "");
static_assert((int)SkXfermode::kSrcOut_Mode     == (int)XfermodeMode::SRC_OUT,     "");
static_assert((int)SkXfermode::kDstOut_Mode     == (int)XfermodeMode::DST_OUT,     "");
static_assert((int)SkXfermode::kSrcATop_Mode    == (int)XfermodeMode::SRC_ATOP,    "");
static_assert((int)SkXfermode::kDstATop_Mode    == (int)XfermodeMode::DST_ATOP,    "");
static_assert((int)SkXfermode::kXor_Mode        == (int)XfermodeMode::XOR,         "");
static_assert((int)SkXfermode::kPlus_Mode       == (int)XfermodeMode::PLUS,        "");
static_assert((int)SkXfermode::kModulate_Mode   == (int)XfermodeMode::MODULATE,    "");
static_assert((int)SkXfermode::kScreen_Mode     == (int)XfermodeMode::SCREEN,      "");
static_assert((int)SkXfermode::kOverlay_Mode    == (int)XfermodeMode::OVERLAY,     "");
static_assert((int)SkXfermode::kDarken_Mode     == (int)XfermodeMode::DARKEN,      "");
static_assert((int)SkXfermode::kLighten_Mode    == (int)XfermodeMode::LIGHTEN,     "");
static_assert((int)SkXfermode::kColorDodge_Mode == (int)XfermodeMode::COLOR_DODGE, "");
static_assert((int)SkXfermode::kColorBurn_Mode  == (int)XfermodeMode::COLOR_BURN,  "");
static_assert((int)SkXfermode::kHardLight_Mode  == (int)XfermodeMode::HARD_LIGHT,  "");
static_assert((int)SkXfermode::kSoftLight_Mode  == (int)XfermodeMode::SOFT_LIGHT,  "");
static_assert((int)SkXfermode::kDifference_Mode == (int)XfermodeMode::DIFFERENCE,  "");
static_assert((int)SkXfermode::kExclusion_Mode  == (int)XfermodeMode::EXCLUSION,   "");
static_assert((int)SkXfermode::kMultiply_Mode   == (int)XfermodeMode::MULTIPLY,    "");
static_assert((int)SkXfermode::kHue_Mode        == (int)XfermodeMode::HUE,         "");
static_assert((int)SkXfermode::kSaturation_Mode == (int)XfermodeMode::SATURATION,  "");
static_assert((int)SkXfermode::kColor_Mode      == (int)XfermodeMode::COLOR,       "");
static_assert((int)SkXfermode::kLuminosity_Mode == (int)XfermodeMode::LUMINOSITY,  "");

void Canvas::onDrawAtlas(const SkImage* atlas,
                         const SkRSXform xform[],
                         const SkRect tex[],
                         const SkColor colors[],
                         int count,
                         SkXfermode::Mode mode,
                         const SkRect* cull,
                         const SkPaint* p) {
    if (!atlas) { return; }
    auto cmd = DrawAtlasCommand::New();
    cmd->image = define_image(fContext, atlas, fDst);
    SkASSERT(xform);
    cmd->rotation_and_scale_transforms.resize(4 * count);
    for (int i = 0; i < count; ++i) {
        cmd->rotation_and_scale_transforms[4 * i    ] = xform[i].fSCos;
        cmd->rotation_and_scale_transforms[4 * i + 1] = xform[i].fSSin;
        cmd->rotation_and_scale_transforms[4 * i + 2] = xform[i].fTx;
        cmd->rotation_and_scale_transforms[4 * i + 3] = xform[i].fTy;
    }
    SkASSERT(tex);
    cmd->tex.resize(count * 4);
    for (int i = 0; i < count; ++i) {
        cmd->tex[4 * i    ] = tex[i].left();
        cmd->tex[4 * i + 1] = tex[i].top();
        cmd->tex[4 * i + 2] = tex[i].right();
        cmd->tex[4 * i + 3] = tex[i].bottom();
    }
    convert(count, colors, &cmd->colors);
    cmd->mode = (XfermodeMode)mode;
    cmd->cull_rect = cull ? convert(*cull) : nullptr;
    cmd->paint = p ? paint(*p) : nullptr;
    add_command(fDst, &CanvasCommand::set_draw_atlas, std::move(cmd));
}

void Canvas::onDrawBitmap(const SkBitmap& bitmap,
                          SkScalar left,
                          SkScalar top,
                          const SkPaint* paint) {
    SkAutoTUnref<SkImage> img(SkImage::NewFromBitmap(bitmap));
    this->onDrawImage(img, left, top, paint);
}

void Canvas::onDrawBitmapRect(const SkBitmap& bitmap,
                              const SkRect* src,
                              const SkRect& dst,
                              const SkPaint* paint,
                              SrcRectConstraint constraint) {
    SkAutoTUnref<SkImage> image(SkImage::NewFromBitmap(bitmap));
    this->onDrawImageRect(image, src, dst, paint, constraint);
}

void Canvas::onDrawBitmapNine(const SkBitmap& bitmap,
                              const SkIRect& center,
                              const SkRect& dst,
                              const SkPaint* paint) {
    SkAutoTUnref<SkImage> image(SkImage::NewFromBitmap(bitmap));
    this->onDrawImageNine(image, center, dst, paint);
}

void Canvas::onDrawImage(const SkImage* image, SkScalar left, SkScalar top, const SkPaint* p) {
    if (!image) { return; }
    auto cmd = DrawImageCommand::New();
    cmd->image = define_image(fContext, image, fDst);
    cmd->left = left;
    cmd->top = top;
    cmd->paint = p ? paint(*p) : nullptr;
    add_command(fDst, &CanvasCommand::set_draw_image, std::move(cmd));
}

static_assert((int)SkCanvas::kStrict_SrcRectConstraint == (int)SrcRectConstraint::STRICT, "");
static_assert((int)SkCanvas::kFast_SrcRectConstraint   == (int)SrcRectConstraint::FAST,   "");

void Canvas::onDrawImageRect(const SkImage* image,
                             const SkRect* src,
                             const SkRect& dst,
                             const SkPaint* p,
                             SrcRectConstraint constraint) {
    if (!image) { return; }
    auto cmd = DrawImageRectCommand::New();
    cmd->image = define_image(fContext, image, fDst);
    cmd->src = src ? convert(*src) : nullptr;
    cmd->dst = convert(dst);
    cmd->paint = p ? paint(*p) : nullptr;
    cmd->constraint = (SkMojo::SrcRectConstraint)constraint;
    add_command(fDst, &CanvasCommand::set_draw_image_rect, std::move(cmd));
}

void Canvas::onDrawImageNine(const SkImage* image,
                             const SkIRect& center,
                             const SkRect& dst,
                             const SkPaint* p) {
    if (!image) { return; }
    auto cmd = DrawImageNineCommand::New();
    cmd->image = define_image(fContext, image, fDst);
    cmd->center_rect_left   = center.left();
    cmd->center_rect_top    = center.top();
    cmd->center_rect_right  = center.right();
    cmd->center_rect_bottom = center.bottom();
    cmd->dst = convert(dst);
    cmd->paint = p ? paint(*p) : nullptr;
    add_command(fDst, &CanvasCommand::set_draw_image_nine, std::move(cmd));
}


static uint32_t define_typeface(Context* context,
                                const SkTypeface* t,
                                SkMojo::Picture* dst) {
    SkAutoTUnref<const SkTypeface> face(t ? SkRef(t) : SkTypeface::RefDefault());
    uint32_t id = face->uniqueID();
    if (!has(context->typefaces, id)) {
        // TODO(halcanary): More mojoish way to share typefaces cross-process.
        SkDynamicMemoryWStream buffer;
        face->serialize(&buffer);
        auto bytes = mojo::Array<uint8_t>::New(buffer.bytesWritten());
        buffer.copyTo(bytes.data());
        context->typefaces.insert(id, std::move(bytes));
    }
    return id;
}

static TextRunPtr text_run(SkTextBlob::GlyphPositioning glyphPos,
                           uint32_t glyphCount,
                           const uint16_t* glyphs,
                           const SkScalar* pos,
                           const SkPoint& offset,
                           const SkPaint& p,
                           Context* context,
                           SkMojo::Picture* dst) {
    auto run = TextRun::New();
    run->text_size = p.getTextSize();
    run->text_scale_x = p.getTextScaleX();
    run->text_skew_x = p.getTextSkewX();
    run->text_align = (Align)p.getTextAlign();
    run->hinting = (Hinting)p.getHinting();
    run->typeface = define_typeface(context, p.getTypeface(), dst);
    run->flags = p.getFlags();
    run->offset_x = offset.x();
    run->offset_y = offset.y();
    convert(glyphCount, glyphs, &run->glyphs);
    switch(glyphPos) {
        case SkTextBlob::kDefault_Positioning:
            convert(0, nullptr, &run->positions);
            break;
        case SkTextBlob::kHorizontal_Positioning:
            convert(glyphCount, pos, &run->positions);
            break;
        case SkTextBlob::kFull_Positioning:
            convert(glyphCount * 2, pos, &run->positions);
            break;
        default: SkASSERT(false);
    }
    return run;
}

static uint32_t define_text_blob(Context* context,
                                 const SkTextBlob* blob,
                                 SkMojo::Picture* dst) {
    uint32_t id = blob->uniqueID();
    if (!has(context->text_blobs, id)) {
        mojo::Array<TextRunPtr> runs;
        for (SkTextBlobRunIterator it(blob); !it.done(); it.next()) {
            SkPaint font;
            it.applyFontToPaint(&font);
            runs.push_back(text_run(it.positioning(),
                                    it.glyphCount(), it.glyphs(),
                                    it.pos(), it.offset(), font,
                                    context, dst));
        }
        context->text_blobs.insert(id, std::move(runs));
    }
    return id;
}


void Canvas::onDrawTextBlob(const SkTextBlob* text, SkScalar x, SkScalar y, const SkPaint& p) {
    auto cmd = DrawTextBlobCommand::New();
    cmd->text_blob = define_text_blob(fContext, text, fDst);
    cmd->x = x;
    cmd->y = y;
    cmd->paint = paint(p);
    add_command(fDst, &CanvasCommand::set_draw_text_blob, std::move(cmd));
}

static void draw_text(const void* text, size_t byteLength,
                      const SkPoint& offset, const SkScalar* pos,
                      const SkPaint& originalPaint,
                      SkTextBlob::GlyphPositioning glyphPos,
                      Context* context, SkMojo::Picture* dst) {
    SkPaint p(originalPaint);
    uint32_t glyphCount = 0;
    SkAutoMalloc storage;
    const uint16_t* glyphs = nullptr;
    switch (p.getTextEncoding()) {
        case SkPaint::kUTF8_TextEncoding:
        case SkPaint::kUTF16_TextEncoding:
        case SkPaint::kUTF32_TextEncoding:
            glyphCount = SkToU32(p.textToGlyphs(text, byteLength, nullptr));
            storage.reset(glyphCount * sizeof(uint16_t));
            p.textToGlyphs(text, byteLength,
                           reinterpret_cast<uint16_t*>(storage.get()));
            glyphs = reinterpret_cast<const uint16_t*>(storage.get());
            break;
        case SkPaint::kGlyphID_TextEncoding:
            glyphCount = byteLength / sizeof(uint16_t);
            glyphs = reinterpret_cast<const uint16_t*>(text);
    }
    p.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    auto cmd = DrawTextCommand::New();
    cmd->text = text_run(glyphPos, glyphCount, glyphs, pos, offset, p, context, dst);
    cmd->x = offset.x();
    cmd->y = offset.y();
    cmd->paint = paint(p);
    add_command(dst, &CanvasCommand::set_draw_text, std::move(cmd));
}

void Canvas::onDrawText(const void* text, size_t byteLength,
                        SkScalar x, SkScalar y, const SkPaint& paint) {
    draw_text(text, byteLength, SkPoint{x, y}, nullptr,
              paint, SkTextBlob::kDefault_Positioning, fContext, fDst);
}

void Canvas::onDrawPosText(const void* text,
                           size_t byteLength,
                           const SkPoint pos[],
                           const SkPaint& paint) {
    draw_text(text, byteLength, SkPoint{0, 0},
              reinterpret_cast<const SkScalar*>(pos),
              paint, SkTextBlob::kFull_Positioning, fContext, fDst);
}

void Canvas::onDrawPosTextH(const void* text,
                            size_t byteLength,
                            const SkScalar xpos[],
                            SkScalar constY,
                            const SkPaint& paint) {
    draw_text(text, byteLength, SkPoint{0, constY}, xpos,
              paint, SkTextBlob::kHorizontal_Positioning, fContext, fDst);
}

static_assert((int)SkRegion::kDifference_Op        == (int)RegionOp::DIFFERENCE,         "");
static_assert((int)SkRegion::kIntersect_Op         == (int)RegionOp::INTERSECT,          "");
static_assert((int)SkRegion::kUnion_Op             == (int)RegionOp::UNION,              "");
static_assert((int)SkRegion::kXOR_Op               == (int)RegionOp::XOR,                "");
static_assert((int)SkRegion::kReverseDifference_Op == (int)RegionOp::REVERSE_DIFFERENCE, "");
static_assert((int)SkRegion::kReplace_Op           == (int)RegionOp::REPLACE,            "");

// All clip calls need to call their parent method or we'll not get any quick rejects.
void Canvas::onClipRect(const SkRect& r, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    this->SkCanvas::onClipRect(r, op, edgeStyle);
    auto cmd = ClipRectCommand::New();
    cmd->clip_rect = convert(r);
    cmd->op = (RegionOp)op;
    cmd->edge_style = (SkMojo::ClipEdgeStyle)edgeStyle;
    add_command(fDst, &CanvasCommand::set_clip_rect, std::move(cmd));
}

void Canvas::onClipRRect(const SkRRect& rrect, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    this->SkCanvas::onClipRRect(rrect, op, edgeStyle);
    auto cmd = ClipRoundRectCommand::New();
    cmd->round_rect = round_rect(rrect);
    cmd->op = (RegionOp)op;
    cmd->edge_style = (SkMojo::ClipEdgeStyle)edgeStyle;
    add_command(fDst, &CanvasCommand::set_clip_round_rect, std::move(cmd));
}

void Canvas::onClipPath(const SkPath& skpath, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    this->SkCanvas::onClipPath(skpath, op, edgeStyle);
    auto cmd = ClipPathCommand::New();
    cmd->path = convert(skpath, fContext);
    cmd->op = (RegionOp)op;
    cmd->edge_style = (SkMojo::ClipEdgeStyle)edgeStyle;
    add_command(fDst, &CanvasCommand::set_clip_path, std::move(cmd));
}

void Canvas::onClipRegion(const SkRegion& region, SkRegion::Op op) {
    this->SkCanvas::onClipRegion(region, op);
    auto cmd = ClipRegionCommand::New();
    for (SkRegion::Iterator iter(region); !iter.done(); iter.next()) {
        cmd->region.push_back(iter.rect().left());
        cmd->region.push_back(iter.rect().top());
        cmd->region.push_back(iter.rect().right());
        cmd->region.push_back(iter.rect().bottom());
    }
    cmd->op = (RegionOp)op;
    add_command(fDst, &CanvasCommand::set_clip_region, std::move(cmd));
}

}  // namespace
////////////////////////////////////////////////////////////////////////////////

SkMojo::PicturePtr SkMojoCreatePicture(const SkRect& r) {
    SkMojo::PicturePtr mojoPicture = SkMojo::Picture::New();
    mojoPicture->cull_rect = convert(r);
    return mojoPicture;
}

SkAutoTUnref<SkCanvas> SkMojoCreateCanvas(SkMojo::Picture* p, SkMojo::Context* c) {
    return SkAutoTUnref<SkCanvas>(new Canvas(p, c));
}


////////////////////////////////////////////////////////////////////////////////
#if 0
namespace {

#define REQUIRE(COND) do { if (!(COND)) { SkDEBUGFAIL(#COND); return false; } } while (false)

struct Playbacker {
    SkTHashMap<uint32_t, SkTypeface*>       fTypefaces;
    SkTHashMap<uint32_t, SkImage*>          fImages;
    SkTHashMap<uint32_t, const SkTextBlob*> fTextBlobs;
    SkTHashMap<uint32_t, SkPicture*>        fPictures;
    SkTHashMap<uint32_t, SkPath>            fPaths;
    ~Playbacker() {
        fTypefaces.foreach([](uint32_t, SkTypeface**       t) { SkSafeUnref(*t); });
        fImages   .foreach([](uint32_t, SkImage**          t) { SkSafeUnref(*t); });
        fTextBlobs.foreach([](uint32_t, const SkTextBlob** t) { SkSafeUnref(*t); });
        fPictures .foreach([](uint32_t, SkPicture**        t) { SkSafeUnref(*t); });
    }
    bool playbackPicture(const SkMojo::Picture& src, SkCanvas* dst);
    bool operator()(SkCanvas*, const DefineImagePtr&);
    bool operator()(SkCanvas*, const DefinePathPtr&);
    bool operator()(SkCanvas*, const DefinePicturePtr&);
    bool operator()(SkCanvas*, const DefineTextBlobPtr&);
    bool operator()(SkCanvas*, const DefineTypefacePtr&);
    bool operator()(SkCanvas*, const SaveCommandPtr&);
    bool operator()(SkCanvas*, const RestoreCommandPtr&);
    bool operator()(SkCanvas*, const SaveLayerCommandPtr&);
    bool operator()(SkCanvas*, const SetMatrixCommandPtr&);
    bool operator()(SkCanvas*, const ClipRectCommandPtr&);
    bool operator()(SkCanvas*, const ClipRoundRectCommandPtr&);
    bool operator()(SkCanvas*, const ClipPathCommandPtr&);
    bool operator()(SkCanvas*, const ClipRegionCommandPtr&);
    bool operator()(SkCanvas*, const DrawOvalCommandPtr&);
    bool operator()(SkCanvas*, const DrawRectCommandPtr&);
    bool operator()(SkCanvas*, const DrawRoundRectCommandPtr&);
    bool operator()(SkCanvas*, const DrawDoubleRoundRectCommandPtr&);
    bool operator()(SkCanvas*, const DrawPathCommandPtr&);
    bool operator()(SkCanvas*, const DrawPaintCommandPtr&);
    bool operator()(SkCanvas*, const DrawPointsCommandPtr&);
    bool operator()(SkCanvas*, const DrawDrawableCommandPtr&);
    bool operator()(SkCanvas*, const DrawPictureCommandPtr&);
    bool operator()(SkCanvas*, const DrawVerticesCommandPtr&);
    bool operator()(SkCanvas*, const DrawPatchCommandPtr&);
    bool operator()(SkCanvas*, const DrawAtlasCommandPtr&);
    bool operator()(SkCanvas*, const DrawImageCommandPtr&);
    bool operator()(SkCanvas*, const DrawImageRectCommandPtr&);
    bool operator()(SkCanvas*, const DrawImageNineCommandPtr&);
    bool operator()(SkCanvas*, const DrawTextBlobCommandPtr&);
    bool operator()(SkCanvas*, const DrawTextCommandPtr&);
};

SkRect convert(const Rect& r) {
    return SkRect::MakeLTRB(r.left, r.top, r.right, r.bottom);
}

static SkPathEffect* unflatten(const PathEffectPtr& u) {
    return (u && u->name && u->data)
        ? SkUnflattenPathEffect(u->name.data(), u->data.data(), u->data.size())
        : nullptr;
}
static SkShader* unflatten(const ShaderPtr& u) {
    return (u && u->name && u->data)
        ? SkUnflattenShader(u->name.data(), u->data.data(), u->data.size())
        : nullptr;
}
static SkXfermode* unflatten(const XfermodePtr& u) {
    return (u && u->name && u->data)
        ? SkUnflattenXfermode(u->name.data(), u->data.data(), u->data.size())
        : nullptr;
}
static SkMaskFilter* unflatten(const MaskFilterPtr& u) {
    return (u && u->name && u->data)
        ? SkUnflattenMaskFilter(u->name.data(), u->data.data(), u->data.size())
        : nullptr;
}
static SkColorFilter* unflatten(const ColorFilterPtr& u) {
    return (u && u->name && u->data)
        ? SkUnflattenColorFilter(u->name.data(), u->data.data(), u->data.size())
        : nullptr;
}
static SkRasterizer* unflatten(const RasterizerPtr& u) {
    return (u && u->name && u->data)
        ? SkUnflattenRasterizer(u->name.data(), u->data.data(), u->data.size())
        : nullptr;
}
static SkDrawLooper* unflatten(const DrawLooperPtr& u) {
    return (u && u->name && u->data)
        ? SkUnflattenDrawLooper(u->name.data(), u->data.data(), u->data.size())
        : nullptr;
}
static SkImageFilter* unflatten(const ImageFilterPtr& u) {
    return (u && u->name && u->data)
        ? SkUnflattenImageFilter(u->name.data(), u->data.data(), u->data.size())
        : nullptr;
}

static SkMatrix convert(const Matrix& m) {
     SkMatrix matrix;
     matrix[SkMatrix::kMScaleX] = m.scale_x;
     matrix[SkMatrix::kMSkewX ] = m.skew_x;
     matrix[SkMatrix::kMTransX] = m.trans_x;
     matrix[SkMatrix::kMSkewY ] = m.skew_y;
     matrix[SkMatrix::kMScaleY] = m.scale_y;
     matrix[SkMatrix::kMTransY] = m.trans_y;
     matrix[SkMatrix::kMPersp0] = m.persp_0;
     matrix[SkMatrix::kMPersp1] = m.persp_1;
     matrix[SkMatrix::kMPersp2] = m.persp_2;
     return matrix;
}

static SkPaint convert(const Paint& p) {
    SkPaint paint;
    paint.setPathEffect(   unflatten(p.path_effect ));
    paint.setShader(       unflatten(p.shader      ));
    paint.setXfermode(     unflatten(p.xfermode    ));
    paint.setMaskFilter(   unflatten(p.mask_filter ));
    paint.setColorFilter(  unflatten(p.color_filter));
    paint.setRasterizer(   unflatten(p.rasterizer  ));
    paint.setLooper(       unflatten(p.looper      ));
    paint.setImageFilter(  unflatten(p.image_filter));
    paint.setColor(        p.color                  );
    paint.setStrokeWidth(  p.stroke_width           );
    paint.setStrokeMiter(  p.stroke_miter_limit     );
    paint.setFlags(        p.flags                  );
    paint.setHinting(      (SkPaint::Hinting)p.hinting       );
    paint.setStyle(        (SkPaint::Style)  p.style         );
    paint.setStrokeCap(    (SkPaint::Cap)    p.cap           );
    paint.setStrokeJoin(   (SkPaint::Join)   p.join          );
    paint.setFilterQuality((SkFilterQuality) p.filter_quality);
    return paint;
}

SkAutoTUnref<SkData> convert(const mojo::Array<uint8_t>& d) {
    return SkAutoTUnref<SkData>(
            d ? SkData::NewWithCopy(d.data(), d.size()) : SkData::NewEmpty());
}

template<typename T, typename U>
T* convert(T* storage, const U& u) {
    if (u && u.get()) {
        *storage = convert(*u.get());
        return storage;
    }
    return nullptr;
}

bool Playbacker::operator()(SkCanvas*, const DefineImagePtr& def) {
    REQUIRE(def);
    auto encoded = convert(def->image_data);
    SkAutoTUnref<SkImage> img(SkImage::NewFromEncoded(encoded));
    fImages.set(def->id, img.detach());
    return true;
}
bool Playbacker::operator()(SkCanvas*, const DefinePathPtr& def) {
    REQUIRE(def);
    SkPath path;
    path.setFillType((SkPath::FillType)(def->path->fill_type));
    for (const PathVerbPtr& verb : def->path->verbs) {
        REQUIRE(verb);
        if (verb->is_move()) {
            MovePathVerbPtr& v = verb->get_move();
            REQUIRE(v);
            path.moveTo(v->end_x, v->end_y);
        } else if (verb->is_line()) {
            LinePathVerbPtr& v = verb->get_line();
            REQUIRE(v);
            path.lineTo(v->end_x, v->end_y);
        } else if (verb->is_quad()) {
            QuadPathVerbPtr& v = verb->get_quad();
            REQUIRE(v);
            path.quadTo(v->control_x, v->control_y, v->end_x, v->end_y);
        } else if (verb->is_conic()) {
            ConicPathVerbPtr& v = verb->get_conic();
            REQUIRE(v);
            path.conicTo(v->control_x, v->control_y, v->end_x, v->end_y,
                         v->weight);
        } else if (verb->is_cubic()) {
            CubicPathVerbPtr& v = verb->get_cubic();
            REQUIRE(v);
            path.cubicTo(v->control_1_x, v->control_1_y,
                         v->control_2_x, v->control_2_y,
                         v->end_x, v->end_y);
        } else if (verb->is_close()) {
            ClosePathVerbPtr& v = verb->get_close();
            REQUIRE(v);
            path.close();
        } else {
            REQUIRE(false);
        }
    }
    fPaths.set(def->id, path);
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DefinePicturePtr& def) {
    REQUIRE(def);
    const PicturePtr& pp = def->picture;
    REQUIRE(pp);
    REQUIRE(pp->cull_rect);
    SkRect cullRect = convert(*pp->cull_rect);
    SkPictureRecorder pictureRecorder;
    REQUIRE(this->playbackPicture(*pp, pictureRecorder.beginRecording(cullRect)));
    fPictures.set(def->id, pictureRecorder.endRecordingAsPicture());
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DefineTextBlobPtr& def) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DefineTypefacePtr& def) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const SaveCommandPtr& cmd) {
    dst->save();
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const RestoreCommandPtr& cmd) {
    dst->restore();
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const SaveLayerCommandPtr& cmd) {
    REQUIRE(cmd);
    SkRect bounds;
    SkPaint paint;
    dst->saveLayer(SkCanvas::SaveLayerRec(convert(&bounds, cmd->bounds),
                                          convert(&paint, cmd->paint),
                                          unflatten(cmd->backdrop),
                                          cmd->save_layer_flags));
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const SetMatrixCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const ClipRectCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const ClipRoundRectCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const ClipPathCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const ClipRegionCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawOvalCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawRectCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawRoundRectCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawDoubleRoundRectCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawPathCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawPaintCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawPointsCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawDrawableCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawPictureCommandPtr& cmd) {
    REQUIRE(cmd);
    SkPicture** ptr = fPictures.find(cmd->picture);
    REQUIRE(ptr);
    REQUIRE(*ptr);
    SkMatrix matrix;
    SkPaint paint;
    dst->drawPicture(*ptr, convert(&matrix, cmd->matrix), convert(&paint, cmd->paint));
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawVerticesCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawPatchCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawAtlasCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawImageCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawImageRectCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawImageNineCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawTextBlobCommandPtr& cmd) {
    // FIXME
    return true;
}
bool Playbacker::operator()(SkCanvas* dst, const DrawTextCommandPtr& cmd) {
    // FIXME
    return true;
}

bool Playbacker::playbackPicture(const SkMojo::Picture& src, SkCanvas* dst) {
    for (size_t i = 0; i < src.commands.size(); ++i) {
        REQUIRE(src.commands[i]);
        const CanvasCommand& c = *src.commands[i];
        if        (c.is_define_image()) {     REQUIRE((*this)(dst, c.get_define_image()));
        } else if (c.is_define_path()) {      REQUIRE((*this)(dst, c.get_define_path()));
        } else if (c.is_define_picture()) {   REQUIRE((*this)(dst, c.get_define_picture()));
        } else if (c.is_define_text_blob()) { REQUIRE((*this)(dst, c.get_define_text_blob()));
        } else if (c.is_define_typeface()) {  REQUIRE((*this)(dst, c.get_define_typeface()));
        } else if (c.is_save()) {             REQUIRE((*this)(dst, c.get_save()));
        } else if (c.is_restore()) {          REQUIRE((*this)(dst, c.get_restore()));
        } else if (c.is_save_layer()) {       REQUIRE((*this)(dst, c.get_save_layer()));
        } else if (c.is_set_matrix()) {       REQUIRE((*this)(dst, c.get_set_matrix()));
        } else if (c.is_clip_rect()) {        REQUIRE((*this)(dst, c.get_clip_rect()));
        } else if (c.is_clip_round_rect()) {  REQUIRE((*this)(dst, c.get_clip_round_rect()));
        } else if (c.is_clip_path()) {        REQUIRE((*this)(dst, c.get_clip_path()));
        } else if (c.is_clip_region()) {      REQUIRE((*this)(dst, c.get_clip_region()));
        } else if (c.is_draw_oval()) {        REQUIRE((*this)(dst, c.get_draw_oval()));
        } else if (c.is_draw_rect()) {        REQUIRE((*this)(dst, c.get_draw_rect()));
        } else if (c.is_draw_round_rect()) {  REQUIRE((*this)(dst, c.get_draw_round_rect()));
        } else if (c.is_draw_double_round_rect()) {
            REQUIRE((*this)(dst, c.get_draw_double_round_rect()));
        } else if (c.is_draw_path()) {        REQUIRE((*this)(dst, c.get_draw_path()));
        } else if (c.is_draw_paint()) {       REQUIRE((*this)(dst, c.get_draw_paint()));
        } else if (c.is_draw_points()) {      REQUIRE((*this)(dst, c.get_draw_points()));
        } else if (c.is_draw_drawable()) {    REQUIRE((*this)(dst, c.get_draw_drawable()));
        } else if (c.is_draw_picture()) {     REQUIRE((*this)(dst, c.get_draw_picture()));
        } else if (c.is_draw_vertices()) {    REQUIRE((*this)(dst, c.get_draw_vertices()));
        } else if (c.is_draw_patch()) {       REQUIRE((*this)(dst, c.get_draw_patch()));
        } else if (c.is_draw_atlas()) {       REQUIRE((*this)(dst, c.get_draw_atlas()));
        } else if (c.is_draw_image()) {       REQUIRE((*this)(dst, c.get_draw_image()));
        } else if (c.is_draw_image_rect()) {  REQUIRE((*this)(dst, c.get_draw_image_rect()));
        } else if (c.is_draw_image_nine()) {  REQUIRE((*this)(dst, c.get_draw_image_nine()));
        } else if (c.is_draw_text_blob()) {   REQUIRE((*this)(dst, c.get_draw_text_blob()));
        } else if (c.is_draw_text()) {        REQUIRE((*this)(dst, c.get_draw_text()));
        } else {
            REQUIRE(false);
        }
    }
    return true;
}
#undef REQUIRE
}  // namespace

#endif // 0

bool SkMojoPlaybackPicture(const SkMojo::Picture& src,
                           SkMojo::Context& context,
                           SkCanvas* dst) { return true; }
// bool SkMojoPlaybackPicture(const SkMojo::Picture& src, SkCanvas* dst) {
//     if (!src.commands) { return false; }
//     Playbacker playbacker;
//     return playbacker.playbackPicture(src, dst);
// }
#endif  // SK_MOJO
