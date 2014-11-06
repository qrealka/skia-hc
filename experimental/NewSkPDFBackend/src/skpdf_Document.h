/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Document_DEFINED
#define skpdf_Document_DEFINED

#include "SkCanvas.h"
#include "SkDocument.h"
#include "SkMetaData.h"
#include "skpdf_BitmapKey.h"
#include "skpdf_CrossReferenceTable.h"
#include "skpdf_Device.h"
#include "skpdf_GlyphCollection.h"
#include "skpdf_Map.h"
#include "skpdf_Name.h"
#include "skpdf_Stream.h"
#include "skpdf_TokenWriter.h"
#include "skpdf_Types.h"

class SkStreamAsset;
class SkTypeFace;
class SkShader;
class SkMatrix;
class SkRect;

namespace skpdf {

// // TODO(halcanary):  move interface to SkDocument.
// class DocumentInfoProvider {
// public:
//     virtual ~DocumentInfoProvider() {}

//     /**
//      *  Set document information, for document formats, such as PDF,
//      *  that support it.  PDF Documents support the Title, Author,
//      *  Subject, Keywords, and Creator keys.
//      */
//     virtual SkMetaData& getMetaData() = 0;

//     /**
//      *  Add metadata in XMP (ISO 16684-1:2012) format, for document
//      *  formats, such as PDF, that support it.  This function will
//      *  take ownership of the asset.  If this document metadata is
//      *  given, it will disable use of metadata set with getMetaData().
//      */
//     virtual void setDocumentXMPMetadata(SkStreamAsset* asset) = 0;
// };

// class Document : public SkDocument, public DocumentInfoProvider {
class Document : public SkDocument {
public:
    Document(SkWStream* stream,
             void (*doneProc)(SkWStream*, bool),
             SkData* (*encoder)(size_t*, const SkBitmap&),
             SkScalar rasterDpi);

    // SkDocument interface:
    virtual ~Document();
    virtual SkCanvas* onBeginPage(SkScalar width,
                                  SkScalar height,
                                  const SkRect& trimBox) SK_OVERRIDE;
    virtual void onEndPage() SK_OVERRIDE;
    virtual bool onClose(SkWStream* stream) SK_OVERRIDE;
    virtual void onAbort() SK_OVERRIDE;

    // // Metadata methods.
    // virtual SkMetaData& getMetaData() SK_OVERRIDE;
    // virtual void setDocumentXMPMetadata(SkStreamAsset* asset) SK_OVERRIDE;

    // Methods for the skpdf::Device to call into.
    Name bitmap(const SkBitmap& bitmap);
    Name shader(const SkShader& shader,
                const SkMatrix& matrix,
                const SkIRect& surfaceBBox);
    Name paint(const SkPaint& paint);
    Name smask(/*FIXME*/);
    Name noSMaskGraphicState();
    Name font(SkTypeface* typeface, uint16_t glyphID);

private:
    SkAutoTUnref<Device> fDevice;
    SkAutoTUnref<SkCanvas> fCanvas;
    bool fDone;

    TokenWriter<SK_PDF_MAX_COLUMNS, SK_PDF_INDENT> fTok;
    size_t fOutStart;
    SkTDArray<Int> fPages;  // closeDocument uses to emit Page Tree Node.

    SkScalar fRasterDpi;
    Int fCurrentNameIndex;  // Assign unique names.

    Map<BitmapKey, Name> fBitmapMap;  // used by bitmap()
    Map<ShaderKey, Name> fShaderMap;  // used by shader()
    Map<PaintKey, Name> fPaintMap;    // used by paint()
    Map<SMaskKey, Name> fSMaskMap;    // used by smask()
    Map<FontKey, Name> fFontMap;      // used by font()

    Map<Name, GlyphCollection*> fGlyphCollectionMap;  // owns GlyphCollections

    // Resource Dictionaries
    Map<Name, Int> fFontDict;
    Map<Name, Int> fXObjectDict;
    Map<Name, Int> fExtGStateDict;
    Map<Name, Int> fPatternDict;

    CrossReferenceTable fXrefTable;
    Int fPageTreeNodeIndex;
    Int fCatalogIndex;
    Int fDocumentInformationDictionaryIndex;

    Name nextName(char);
    void writeResourceDictionary(const Map<Name, Int>&);
    Int startIndirectObject(Int index = -1);
    void endIndirectObject();
    Int writeStream(const Stream&);
    void  writeDocumentInformationDictionary();
    void writeAllFonts();
    void writeResources();
    void writePagesDictionary();
    Int writeMetadata(SkStreamAsset*);
    void writeCatalog(Int docMetadata);
    size_t writeXrefTable();
    void writeTrailer();

    Int serializeBitmap(const SkBitmap& bitmap);
};
}  // namespace skpdf

#include "skpdf_Document_impl.h"

#endif  // skpdf_Document_DEFINED
