/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "skpdf_Device.h"
#include "skpdf_Document.h"
#include "skpdf_Name.h"

class SkDraw;
class SkPoint;
class SkPaint;
class SkRect;
class SkPath;
class SkMatrix;

namespace skpdf {

void Device::clear(SkColor color) {
    //SkDebugf("skpdf::Device::clear\n");
    // NOT IMPLEMENTED YET
}

void Device::drawPaint(const SkDraw&, const SkPaint& paint) {
    //SkDebugf("skpdf::Device::drawPaint\n");
    // NOT IMPLEMENTED YET
}

void Device::drawPoints(const SkDraw&,
                        SkCanvas::PointMode mode,
                        size_t count,
                        const SkPoint[],
                        const SkPaint& paint) {
    //SkDebugf("skpdf::Device::drawPoints\n");
    // NOT IMPLEMENTED YET
}

void Device::drawRect(const SkDraw&, const SkRect& r, const SkPaint& paint) {
    //SkDebugf("skpdf::Device::drawRect\n");
    // NOT IMPLEMENTED YET
}

void Device::drawOval(const SkDraw&, const SkRect& oval, const SkPaint& paint) {
    //SkDebugf("skpdf::Device::drawOval\n");
    // NOT IMPLEMENTED YET
}

void Device::drawRRect(const SkDraw&, const SkRRect& rr, const SkPaint& paint) {
    //SkDebugf("skpdf::Device::drawRRect\n");
    // NOT IMPLEMENTED YET
}

void Device::drawPath(const SkDraw&,
                      const SkPath& origpath,
                      const SkPaint& paint,
                      const SkMatrix* prePathMatrix,
                      bool pathIsMutable) {
    //SkDebugf("skpdf::Device::drawPath\n");
    // NOT IMPLEMENTED YET
}

void Device::drawBitmapRect(const SkDraw& draw,
                            const SkBitmap& bitmap,
                            const SkRect* src,
                            const SkRect& dst,
                            const SkPaint& paint,
                            SkCanvas::DrawBitmapRectFlags flags) {
    //SkDebugf("skpdf::Device::drawBitmapRect\n");
    // NOT IMPLEMENTED YET
}

void Device::drawBitmap(const SkDraw&,
                        const SkBitmap& bitmap,
                        const SkMatrix& matrix,
                        const SkPaint& paint) {
    // NOT IMPLEMENTED YET

    this->comment("skpdf::Device::drawBitmap()");
    Name bitmapName = fDocument->bitmap(bitmap);
    SkMatrix matrixCopy(matrix);
    // Flip upside down for PDF coordinate system.
    matrixCopy.preTranslate(0, bitmap.height());
    matrixCopy.preScale(bitmap.width(), -bitmap.height());
    SkScalar affineMatrix[6];
    if (!matrixCopy.asAffine(affineMatrix)) {
        return;
    }

#   if SK_PDF_SEVENBIT_OKAY
    fPageContent.writeText("\t");
#   endif
    fPageContent.writeText("q "); // save state

    for (int i = 0; i < 6; ++i)  {
        Write(&fPageContent, affineMatrix[i]);
        fPageContent.writeText(" ");
    }
    fPageContent.writeText("cm ");  // bitmap transform matrix

    bitmapName.write(&fPageContent);
    fPageContent.writeText(" Do ");  // draw bitmap

    fPageContent.writeText("Q");  // restore state
}

void Device::drawSprite(
    const SkDraw&, const SkBitmap& bitmap, int x, int y, const SkPaint& paint) {
    //SkDebugf("skpdf::Device::drawSprite\n");
    // NOT IMPLEMENTED YET
}

void Device::drawText(const SkDraw&,
                      const void* text,
                      size_t len,
                      SkScalar x,
                      SkScalar y,
                      const SkPaint&) {
    //SkDebugf("skpdf::Device::drawText\n");
    // NOT IMPLEMENTED YET
}

void Device::drawPosText(const SkDraw&,
                         const void* text,
                         size_t len,
                         const SkScalar pos[],
                         int scalarsPerPos,
                         const SkPoint& offset,
                         const SkPaint&) {
    //SkDebugf("skpdf::Device::drawPosText\n");
    // NOT IMPLEMENTED YET
}

void Device::drawTextOnPath(const SkDraw&,
                            const void* text,
                            size_t len,
                            const SkPath& path,
                            const SkMatrix* matrix,
                            const SkPaint& paint) {
    //SkDebugf("skpdf::Device::drawTextOnPath\n");
    // NOT IMPLEMENTED YET
}

void Device::drawVertices(const SkDraw&,
                          SkCanvas::VertexMode,
                          int vertexCount,
                          const SkPoint verts[],
                          const SkPoint texs[],
                          const SkColor colors[],
                          SkXfermode* xmode,
                          const uint16_t indices[],
                          int indexCount,
                          const SkPaint& paint) {
    //SkDebugf("skpdf::Device::drawVertices\n");
    // NOT IMPLEMENTED YET
}

void Device::drawDevice(
    const SkDraw&, SkBaseDevice*, int x, int y, const SkPaint&) {
    //SkDebugf("skpdf::Device::drawDevice\n");
    // NOT IMPLEMENTED YET
}

void Device::onAttachToCanvas(SkCanvas* canvas) {
    //SkDebugf("skpdf::Device::onAttachToCanvas\n");
    // NOT IMPLEMENTED YET
}

void Device::onDetachFromCanvas() {
    //SkDebugf("skpdf::Device::onDetachFromCanvas\n");
    // NOT IMPLEMENTED YET
}

SkImageInfo Device::imageInfo() const {
    return fLegacyBitmap.info();
}

const SkBitmap& Device::onAccessBitmap() {
    //SkDebugf("skpdf::Device::onAccessBitmap\n");
    // NOT IMPLEMENTED YET
    return fLegacyBitmap;
}

}  // namespace skpdf
