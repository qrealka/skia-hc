/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkMojo.h"
#ifdef SK_MOJO

#include "SkCanvas.h"
#include "SkImage.h"

namespace  {
class Canvas : public SkCanvas {
public:
    Canvas(SkMojo::Picture* dst) : SkCanvas(1,1), fDst(dst) {
        dst->commands.resize(0); // make it non-null.
    }

protected:
    void willSave() override;
    void didRestore() override;
    SaveLayerStrategy getSaveLayerStrategy(const SaveLayerRec&) override;
    void didConcat(const SkMatrix&) override;
    void didSetMatrix(const SkMatrix&) override;
    void onDrawOval(const SkRect&, const SkPaint&) override;
    void onDrawRect(const SkRect&, const SkPaint&) override;
    void onDrawRRect(const SkRRect&, const SkPaint&) override;
    void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) override;
    void onDrawPath(const SkPath&, const SkPaint&) override;
    void onDrawPaint(const SkPaint&) override;
    void onDrawPoints(PointMode, size_t, const SkPoint[], const SkPaint&) override;
    void onDrawDrawable(SkDrawable*, const SkMatrix*) override;
    void onDrawPicture(const SkPicture*, const SkMatrix*, const SkPaint*) override;
    void onDrawVertices(VertexMode,
                        int,
                        const SkPoint[],
                        const SkPoint[],
                        const SkColor[],
                        SkXfermode*,
                        const uint16_t[],
                        int,
                        const SkPaint&) override;
    void onDrawPatch(const SkPoint[12],
                     const SkColor[4],
                     const SkPoint[4],
                     SkXfermode*,
                     const SkPaint&) override;
    void onDrawAtlas(const SkImage*,
                     const SkRSXform[],
                     const SkRect[],
                     const SkColor[],
                     int,
                     SkXfermode::Mode,
                     const SkRect*,
                     const SkPaint*) override;
    void onDrawBitmap(const SkBitmap&, SkScalar, SkScalar, const SkPaint*) override;
    void onDrawBitmapRect(const SkBitmap&,
                          const SkRect*,
                          const SkRect&,
                          const SkPaint*,
                          SrcRectConstraint) override;
    void onDrawImage(const SkImage*, SkScalar, SkScalar, const SkPaint*) override;
    void onDrawImageRect(const SkImage*,
                         const SkRect*,
                         const SkRect&,
                         const SkPaint*,
                         SrcRectConstraint) override;
    void onDrawBitmapNine(const SkBitmap&, const SkIRect&, const SkRect&, const SkPaint*) override;
    void onDrawImageNine(const SkImage*, const SkIRect&, const SkRect&, const SkPaint*) override;
    void onDrawTextBlob(const SkTextBlob*, SkScalar, SkScalar, const SkPaint&) override;
    void onDrawText(const void*, size_t, SkScalar, SkScalar, const SkPaint&) override;
    void onDrawPosText(const void*, size_t, const SkPoint[], const SkPaint&) override;
    void onDrawPosTextH(const void*, size_t, const SkScalar[], SkScalar, const SkPaint&) override;
    void onClipRect(const SkRect&, SkRegion::Op, ClipEdgeStyle) override;
    void onClipRRect(const SkRRect&, SkRegion::Op, ClipEdgeStyle) override;
    void onClipPath(const SkPath&, SkRegion::Op, ClipEdgeStyle) override;
    void onClipRegion(const SkRegion&, SkRegion::Op) override;

private:
    SkMojo::Picture* fDst;
};

////////////////////////////////////////////////////////////////////////////////

using namespace SkMojo;

static RectPtr rect(const SkRect& r) {
    auto rectPtr = Rect::New();
    rectPtr->top = r.top();
    rectPtr->left = r.left();
    rectPtr->bottom = r.bottom();
    rectPtr->right = r.right();
    return rectPtr;
}

static PaintPtr paint(const SkPaint& p) {
    auto paintPtr = Paint::New();
    
    // PathEffect?   paintPtr->path_effect  = ;
    // Shader?       paintPtr->shader       = ;
    // Xfermode?     paintPtr->xfermode     = ;
    // MaskFilter?   paintPtr->mask_filter  = ;
    // ColorFilter?  paintPtr->color_filter = ;
    // Rasterizer?   paintPtr->rasterizer   = ;
    // DrawLooper?   paintPtr->looper       = ;
    // ImageFilter?  paintPtr->image_filter = ;

    // paintPtr->color = p.getColor();
    // float         paintPtr->stroke_width;
    // float         paintPtr->stroke_miter_limit;
    // uint32        paintPtr->flags;
    // Hinting       paintPtr->hinting; 
    // Style         paintPtr->style;
    // Cap           paintPtr->cap;
    // Join          paintPtr->join;
    // FilterQuality paintPtr->filter_quality;
    
    paintPtr->flags = p.getFlags();
    return paintPtr;
}


void Canvas::willSave() {
    auto command = CanvasCommand::New();
    command->set_save(SaveCommand::New());
    fDst->commands.push_back(std::move(command));
}
void Canvas::didRestore() {
    auto command = CanvasCommand::New();
    command->set_restore(RestoreCommand::New());
    fDst->commands.push_back(std::move(command));
}
SkCanvas::SaveLayerStrategy Canvas::getSaveLayerStrategy(const SkCanvas::SaveLayerRec& rec) {
        // const SkRect*           fBounds;    // optional
        // const SkPaint*          fPaint;     // optional
        // const SkImageFilter*    fBackdrop;  // optional
        // SaveLayerFlags          fSaveLayerFlags;
    auto cmd = SaveLayerCommand::New();
    if (rec.fBounds) {
        cmd->bounds = rect(*rec.fBounds);
    }
    if (rec.fPaint) {
        cmd->paint = 
        //PaintPtr paint;
        //ImageFilterPtr backdrop;
        //uint32_t save_layer_flags;


    auto command = CanvasCommand::New();
    command->set_save_layer(std::move(cmd));
    fDst->commands.push_back(std::move(command));
    // FIXME
    return kNoLayer_SaveLayerStrategy;
}

void Canvas::didConcat(const SkMatrix&) { this->didSetMatrix(this->getTotalMatrix()); }
void Canvas::didSetMatrix(const SkMatrix& matrix) {
    // FIXME
}

void Canvas::onDrawOval(const SkRect& oval, const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawRect(const SkRect& rect, const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawRRect(const SkRRect& rrect, const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawDRRect(const SkRRect& outside, const SkRRect& inside, const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawPath(const SkPath& path, const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawPaint(const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawPoints(PointMode mode, size_t count, const SkPoint pts[], const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawDrawable(SkDrawable* drawable, const SkMatrix* matrix) {
    // FIXME
    this->SkCanvas::onDrawDrawable(drawable, matrix);
}

void Canvas::onDrawPicture(const SkPicture* pic, const SkMatrix* matrix, const SkPaint* paint) {
    // FIXME
}

void Canvas::onDrawVertices(VertexMode vmode,
                            int vertexCount,
                            const SkPoint vertices[],
                            const SkPoint texs[],
                            const SkColor colors[],
                            SkXfermode* xmode,
                            const uint16_t indices[],
                            int indexCount,
                            const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawPatch(const SkPoint cubics[12],
                         const SkColor colors[4],
                         const SkPoint texCoords[4],
                         SkXfermode* xmode,
                         const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawAtlas(const SkImage* atlas,
                         const SkRSXform xform[],
                         const SkRect tex[],
                         const SkColor colors[],
                         int count,
                         SkXfermode::Mode mode,
                         const SkRect* cull,
                         const SkPaint* paint) {
    // FIXME
}

void Canvas::onDrawBitmap(const SkBitmap& bitmap,
                          SkScalar left,
                          SkScalar top,
                          const SkPaint* paint) {
    auto src = SkRect::MakeWH(bitmap.width(), bitmap.height()), dst = src.makeOffset(left, top);
    this->onDrawBitmapRect(bitmap, &src, dst, paint, kStrict_SrcRectConstraint);
}

void Canvas::onDrawBitmapRect(const SkBitmap& bitmap,
                              const SkRect* src,
                              const SkRect& dst,
                              const SkPaint* paint,
                              SrcRectConstraint constraint) {
    SkAutoTUnref<SkImage> image(SkImage::NewFromBitmap(bitmap));
    this->onDrawImageRect(image, src, dst, paint, constraint);
}

void Canvas::onDrawImage(const SkImage* image, SkScalar left, SkScalar top, const SkPaint* paint) {
    // FIXME
}

void Canvas::onDrawImageRect(const SkImage* image,
                             const SkRect* src,
                             const SkRect& dst,
                             const SkPaint* paint,
                             SrcRectConstraint constraint) {
    // FIXME
}

void Canvas::onDrawBitmapNine(const SkBitmap& bitmap,
                              const SkIRect& center,
                              const SkRect& dst,
                              const SkPaint* paint) {
    SkAutoTUnref<SkImage> image(SkImage::NewFromBitmap(bitmap));
    this->onDrawImageNine(image, center, dst, paint);
}

void Canvas::onDrawImageNine(const SkImage* image,
                             const SkIRect& center,
                             const SkRect& dst,
                             const SkPaint* paint) {
    // FIXME
}

void Canvas::onDrawTextBlob(const SkTextBlob* text, SkScalar x, SkScalar y, const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawText(
        const void* text, size_t byteLength, SkScalar x, SkScalar y, const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawPosText(const void* text,
                           size_t byteLength,
                           const SkPoint pos[],
                           const SkPaint& paint) {
    // FIXME
}

void Canvas::onDrawPosTextH(const void* text,
                            size_t byteLength,
                            const SkScalar xpos[],
                            SkScalar constY,
                            const SkPaint& paint) {
    // FIXME
}

// All clip calls need to call their parent method or we'll not get any quick rejects.
void Canvas::onClipRect(const SkRect& rect, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    this->SkCanvas::onClipRect(rect, op, edgeStyle);
    // FIXME
}

void Canvas::onClipRRect(const SkRRect& rrect, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    this->SkCanvas::onClipRRect(rrect, op, edgeStyle);
    // FIXME
}

void Canvas::onClipPath(const SkPath& path, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    this->SkCanvas::onClipPath(path, op, edgeStyle);
    // FIXME
}

void Canvas::onClipRegion(const SkRegion& region, SkRegion::Op op) {
    this->SkCanvas::onClipRegion(region, op);
    // FIXME
}

}  // namespace
////////////////////////////////////////////////////////////////////////////////

SkCanvas* SkMojo::CreateMojoSkCanvas(SkMojo::Picture* dst) {
    return new Canvas(dst);
}

bool SkMojo::PlaybackSkMojoPicture(const SkMojo::Picture& src, SkCanvas* dst) {
    if (!src.commands) { return false; }
    for (size_t i = 0; i < src.commands.size(); ++i) {
        if (!src.commands[i]) { continue; }
        const CanvasCommand& cmd = *src.commands[i];
        if (cmd.has_unknown_tag()) {
            continue;
        } else if (cmd.is_save()) {
            dst->save();
        } else if (cmd.is_restore()) {
            dst->restore();
        } else if (cmd.is_save_layer()) {
            const SaveLayerCommandPtr& save_layer = cmd.get_save_layer();
            (void)save_layer;
            // dst->saveLayer(); // FIXME
            dst->save();
        }
    }
    return true;
}

#endif  // SK_MOJO
