/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef SkPDFDocument_DEFINED
#define SkPDFDocument_DEFINED

#include "SkDocument.h"
#include "SkPDFMetadata.h"

class SkPDFDevice;

sk_sp<SkDocument> SkPDFMakeDocument(
        SkWStream* stream,
        void (*doneProc)(SkWStream*, bool),
        SkScalar rasterDpi,
        SkPixelSerializer* jpegEncoder);

// Logically part of SkPDFDocument (like SkPDFCanon), but
struct SkPDFObjectSerializer : SkNoncopyable {
    SkPDFObjNumMap fObjNumMap;
    SkPDFSubstituteMap fSubstituteMap;
    SkTDArray<int32_t> fOffsets;
    sk_sp<SkPDFObject> fInfoDict;
    size_t fBaseOffset;
    int32_t fNextToBeSerialized;  // index in fObjNumMap

    SkPDFObjectSerializer();
    void addObjectRecursively(const sk_sp<SkPDFObject>&);
    void serializeHeader(SkWStream*, const SkPDFMetadata&);
    void serializeObjects(SkWStream*);
    void serializeFooter(SkWStream*, const sk_sp<SkPDFObject>, sk_sp<SkPDFObject>);
    int32_t offset(SkWStream*);
};

class SkPDFDocument : public SkDocument {
public:
    SkPDFDocument(SkWStream*,
                  void (*)(SkWStream*, bool),
                  SkScalar,
                  SkPixelSerializer*);
    virtual ~SkPDFDocument();
    SkCanvas* onBeginPage(SkScalar, SkScalar, const SkRect&) override;
    void onEndPage() override;
    bool onClose(SkWStream*) override;
    void onAbort() override;
    void setMetadata(const SkDocument::Attribute[],
                     int,
                     const SkTime::DateTime*,
                     const SkTime::DateTime*) override;
    void serialize(const sk_sp<SkPDFObject>&);
    SkPDFCanon* canon() { return &fCanon; }

private:
    SkPDFObjectSerializer fObjectSerializer;
    SkPDFCanon fCanon;
    SkTArray<sk_sp<const SkPDFDevice>> fPageDevices;
    sk_sp<SkCanvas> fCanvas;
    SkScalar fRasterDpi;
    SkPDFMetadata fMetadata;
};

#endif  // SkPDFDocument_DEFINED
