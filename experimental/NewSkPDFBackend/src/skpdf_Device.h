/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Device_DEFINED
#define skpdf_Device_DEFINED

#include "SkDevice.h"
#include "SkStream.h"

namespace skpdf {

class Document;

class Device : public SkBaseDevice {
public:
    Device(Document* doc, const SkSize& pageSize);
    virtual ~Device();
    /**
     *  After finalize is called, no more draw commands should be made.
     */

    SkSize pageSize() const;
    SkStreamAsset* finalize();
    bool isFinalized();

    SkDEBUGCODE(bool fVerbose;)
    void comment(const char* s);

    // virtuals from SkBaseDevice.
    virtual void clear(SkColor color) SK_OVERRIDE;
    virtual void drawPaint(const SkDraw&, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(const SkDraw&,
                            SkCanvas::PointMode mode,
                            size_t count,
                            const SkPoint[],
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkDraw&, const SkRect& r, const SkPaint& paint);
    virtual void drawOval(const SkDraw&,
                          const SkRect& oval,
                          const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRRect(const SkDraw&,
                           const SkRRect& rr,
                           const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPath(const SkDraw&,
                          const SkPath& origpath,
                          const SkPaint& paint,
                          const SkMatrix* prePathMatrix,
                          bool pathIsMutable) SK_OVERRIDE;
    virtual void drawBitmapRect(const SkDraw& draw,
                                const SkBitmap& bitmap,
                                const SkRect* src,
                                const SkRect& dst,
                                const SkPaint& paint,
                                SkCanvas::DrawBitmapRectFlags flags)
        SK_OVERRIDE;
    virtual void drawBitmap(const SkDraw&,
                            const SkBitmap& bitmap,
                            const SkMatrix& matrix,
                            const SkPaint&) SK_OVERRIDE;
    virtual void drawSprite(const SkDraw&,
                            const SkBitmap& bitmap,
                            int x,
                            int y,
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawText(const SkDraw&,
                          const void* text,
                          size_t len,
                          SkScalar x,
                          SkScalar y,
                          const SkPaint&) SK_OVERRIDE;
    virtual void drawPosText(const SkDraw&,
                             const void* text,
                             size_t len,
                             const SkScalar pos[],
                             int scalarsPerPos,
                             const SkPoint& offset,
                             const SkPaint&) SK_OVERRIDE;
    virtual void drawTextOnPath(const SkDraw&,
                                const void* text,
                                size_t len,
                                const SkPath& path,
                                const SkMatrix* matrix,
                                const SkPaint& paint) SK_OVERRIDE;
    virtual void drawVertices(const SkDraw&,
                              SkCanvas::VertexMode,
                              int vertexCount,
                              const SkPoint verts[],
                              const SkPoint texs[],
                              const SkColor colors[],
                              SkXfermode* xmode,
                              const uint16_t indices[],
                              int indexCount,
                              const SkPaint& paint) SK_OVERRIDE;
    virtual void drawDevice(
        const SkDraw&, SkBaseDevice*, int x, int y, const SkPaint&) SK_OVERRIDE;
    virtual void onAttachToCanvas(SkCanvas* canvas) SK_OVERRIDE;
    virtual void onDetachFromCanvas() SK_OVERRIDE;
    virtual SkImageInfo imageInfo() const SK_OVERRIDE;
    virtual const SkBitmap& onAccessBitmap() SK_OVERRIDE;

    // All draw calls:
    //     write into fPageContent and
    //     get IRefs from fCatalog
    //     add IRefs to Page's dictionaries

private:
    Document* fDocument;
    const SkSize fPageSize;
    SkDynamicMemoryWStream fPageContent;
    SkBitmap fLegacyBitmap;
};
}  // namespace skpdf

#include "skpdf_Device_impl.h"

#endif  // skpdf_Device_DEFINED
