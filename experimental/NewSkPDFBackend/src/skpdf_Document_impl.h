/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Document_impl_DEFINED
#define skpdf_Document_impl_DEFINED

namespace skpdf {

inline Document::Document(SkWStream* stream,
                          void (*doneProc)(SkWStream*, bool),
                          SkPicture::EncodeBitmap encoder,
                          SkScalar rasterDpi)
    : SkDocument(stream, doneProc)
    , fDone(false)
    , fTok(stream)
    , fOutStart(stream->bytesWritten())
    //, fEncoder(encoder)  // Not used.
    , fRasterDpi(rasterDpi)
    , fCurrentNameIndex(0) {
    // The PDF spec recommends including a comment with four bytes, all
    // with their high bits set.  This is "Skia" with the high bits set.
    fTok.write("%PDF-1.4\n%\xD3\xEB\xE9\xE1\n");
    #if (SK_PDF_SEVENBIT_OKAY)
    fTok.comment("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
    #endif
    SK_COMPILE_ASSERT(('S' | '\x80') == '\xD3', skia_high_bits);
    SK_COMPILE_ASSERT(('k' | '\x80') == '\xEB', skia_high_bits);
    SK_COMPILE_ASSERT(('i' | '\x80') == '\xE9', skia_high_bits);
    SK_COMPILE_ASSERT(('a' | '\x80') == '\xE1', skia_high_bits);

    // if (!fDocumentMetadata) {
    fDocumentInformationDictionaryIndex = fXrefTable.getNextUnusedIndex();
    // } else {
    //     fDocumentInformationDictionaryIndex = -1;
    // }
    fCatalogIndex = fXrefTable.getNextUnusedIndex();
    fPageTreeNodeIndex = fXrefTable.getNextUnusedIndex();
}

inline Name Document::bitmap(const SkBitmap& bitmap) {
    BitmapKey key(bitmap);
    const Name* namePtr = fBitmapMap.find(key);
    if (namePtr) {
        return *namePtr;
    }
    Int index = this->serializeBitmap(bitmap);
    Name name = this->nextName('X');
    fBitmapMap.add(key, name);
    fXObjectDict.add(name, index);
    return name;
}

inline Name Document::shader(const SkShader& shader,
                            const SkMatrix& matrix,
                            const SkIRect& surfaceBBox) {
    return /*FIXME*/ Name('X', 1);
}

inline Name Document::paint(const SkPaint& ) {
    PaintKey key;
    const Name* namePtr = fPaintMap.find(key);
    if (namePtr) {
        return *namePtr;
    }
    fTok.comment("External Graphic State");
    Int index = this->startIndirectObject();
    {
        fTok.beginDict("/ExtGState");
        fTok.write("/BM");
        fTok.write("/Normal");
        
        fTok.write("/CA");
        fTok.writeInt(1);
    
        fTok.write("/LC");
        fTok.writeInt(0);
        
        fTok.write("/LJ");
        fTok.writeInt(0);
        
        fTok.write("/LW");
        fTok.writeInt(0);
        
        fTok.write("/ML");
        fTok.writeInt(4);

        fTok.write("/SA");
        fTok.write("true");

        fTok.write("");
        fTok.writeInt(1);

        fTok.endDict();
    }
    this->endIndirectObject();

    Name name = this->nextName('G');
    fPaintMap.add(key, name);
    fExtGStateDict.add(name, index);
    return name;
}

inline Name Document::smask(/*FIXME*/) {
   return /*FIXME*/ Name('G', 1);
}

inline Name Document::noSMaskGraphicState() {
    return /*FIXME*/ Name('G', 1);
}

// Reverence is only valid as long as the SkPDF lives.
inline Name Document::font(SkTypeface* typeface, uint16_t glyphID) {
    return /*FIXME*/ Name('F', 1);
}

inline Name Document::nextName(char c) {
    return Name(c, ++fCurrentNameIndex);
}

inline void Document::writeResourceDictionary(const Map<Name, Int>& resources) {
    fTok.beginDict();
    {
        for (const Map<Name, Int>::Pair& nameAndIndex : resources) {
            fTok.writeName(nameAndIndex.key()); // Name
            fTok.writeIndirectReference(nameAndIndex.value());  // Index
        }
    }
    fTok.endDict();
}

inline size_t difference(size_t minuend, size_t subtrahend) {
    SkASSERT(minuend >= subtrahend);
    return minuend - subtrahend;
}

inline Int Document::startIndirectObject(Int index /* = -1 */) {
    if (index < 0) {
        index = fXrefTable.getNextUnusedIndex();
    } else {
        SkASSERT(fXrefTable.isAvailable(index));
    }
    size_t offset = difference(fTok.stream()->bytesWritten(), fOutStart);
    fXrefTable.setOffset(index, offset);
    fTok.writeInt(index);
    fTok.write("0 obj");
    return index;
}

inline void Document::endIndirectObject() {
    fTok.write("endobj");
    fTok.write("\n");
    #if (SK_PDF_SEVENBIT_OKAY)
    fTok.write("\n");
    #endif
}

inline Int Document::writeStream(const Stream& stream) {
    SkASSERT(stream.hasAsset());
    Int index = this->startIndirectObject();

    fTok.beginDict();
    {
        fTok.write("/Length");
        fTok.writeSize(stream.length());
        fTok.write(stream.filter());
    }
    fTok.endDict();

    fTok.write("stream\n");
    stream.write(fTok.stream());
    fTok.write("\nendstream");

    this->endIndirectObject();
    return index;
}

inline void Document::writeDocumentInformationDictionary() {
    fTok.comment("Document Information Dictionary");
    this->startIndirectObject(fDocumentInformationDictionaryIndex);
    fTok.beginDict();
    {
        // const char* const kKeys[] = {
        //     "Title",
        //     "Author",
        //     "Subject",
        //     "Keywords",
        //     "Creator"
        // };
        // for (size_t i = 0; i < SK_ARRAY_COUNT(kKeys); ++i) {
        //     const char* key = kKeys[i];
        //     const char* value = fMeta.findString(key);
        //     if (value) {
        //         fTok.writeAsName(key);
        //         fTok.writeAsPDFString(value, strlen(value));
        //     }
        // }
        fTok.write("/Producer");
        fTok.write("(SkiaPDF http://skia.org/)");
        
        SkString timestamp = CurrentTimeAsString();
        fTok.write("/CreationDate");
        fTok.writeAsPDFString(timestamp);
        
        fTok.write("/ModDate");
        fTok.writeAsPDFString(timestamp);
    }
    fTok.endDict();
    this->endIndirectObject();
}

inline void Document::writeAllFonts() {
    // FIXME
    // Clean up pointers.
    typedef Map<Name, GlyphCollection*>::Pair P;
    for (const P& nameAndGlyph : fGlyphCollectionMap) {
        delete nameAndGlyph.value();
    }
    fGlyphCollectionMap.reset();
}

inline void Document::writeResources() {
    fTok.beginDict();
    {
        if (fFontDict.count() > 0) {
            fTok.write("/Font");
            this->writeResourceDictionary(fFontDict);
        }

        if (fXObjectDict.count() > 0) {
            fTok.write("/XObject");
            this->writeResourceDictionary(fXObjectDict);
        }

        if (fExtGStateDict.count() > 0) {
            fTok.write("/ExtGState");
            this->writeResourceDictionary(fExtGStateDict);
        }

        if (fPatternDict.count() > 0) {
            fTok.write("/Pattern");
            this->writeResourceDictionary(fPatternDict);
        }
    }
    fTok.endDict();
}

inline void Document::writePagesDictionary() {
    fTok.comment("Page Tree Node");
    this->startIndirectObject(fPageTreeNodeIndex);
    fTok.beginDict("/Pages");
    {
        fTok.write("/Count");
        fTok.writeInt(fPages.count());

        fTok.write("/Resources");
        this->writeResources();

        fTok.write("/Kids");
        fTok.write("[");
        for (Int pageIndex : fPages) {
            fTok.writeIndirectReference(pageIndex);
        }
        fTok.write("]");
    }
    fTok.endDict();
    this->endIndirectObject();
}

inline Int Document::writeMetadata(SkStreamAsset* metadata) {
    fTok.comment("XMP Metadata");
    Int index = this->startIndirectObject();
    Stream stream(metadata);
    #if !(SK_PDF_SEVENBIT_OKAY)
    stream.deflate();
    #endif
    fTok.beginDict("/Metadata");
    {
        fTok.write("/Subtype");
        fTok.write("/XML");

        fTok.write("/Length");
        fTok.writeSize(stream.length());

        fTok.write(stream.filter());
    }
    fTok.endDict();
    fTok.write("stream\n");
    stream.write(fTok.stream());
    fTok.write("\nendstream");
    this->endIndirectObject();
    return index;
}

inline void Document::writeCatalog(Int docMetadata) {
    fTok.comment("Catalog");
    this->startIndirectObject(fCatalogIndex);
    fTok.beginDict("/Catalog");
    {
        fTok.write("/Pages");
        fTok.writeIndirectReference(fPageTreeNodeIndex);

        if (docMetadata > 0) {
            fTok.write("/Metadata");
            fTok.writeIndirectReference(docMetadata);
        }
    }
    fTok.endDict();
    this->endIndirectObject();
}

inline size_t Document::writeXrefTable() {
    fTok.comment("Cross-Reference Table");
    size_t xrefOffset = difference(fTok.stream()->bytesWritten(), fOutStart);
    fXrefTable.write(fTok.stream());
    return xrefOffset;
}

inline void Document::writeTrailer() {
    fTok.comment("Trailer");
    fTok.stream()->writeText("trailer");
    fTok.write("\n");
    fTok.beginDict();
    {
        fTok.write("/Root");
        fTok.writeIndirectReference(fCatalogIndex);

        fTok.write("/Size");
        fTok.writeSize(1 + fXrefTable.count());

        if (fDocumentInformationDictionaryIndex > 0) {
            fTok.write("/Info");
            fTok.writeIndirectReference(fDocumentInformationDictionaryIndex);
        }
    }
    fTok.endDict();
}
}  // namespace skpdf

#endif  // skpdf_Document_impl_DEFINED
