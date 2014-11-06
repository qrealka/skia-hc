/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_TokenWriter_DEFINED
#define skpdf_TokenWriter_DEFINED

#include "SkScalar.h"
#include "SkString.h"
#include "skpdf_Types.h"
#include "skpdf_Name.h"

namespace skpdf {
/**
 *  Write one token at a time to the PDF stream.  If necessary, insert
 *  a space before a token if it may be necessary to differenciate it
 *  from the previous token.  If WIDTH > 0, wrap the lines at WIDTH
 *  columns; The PDF standard requires that "lines that are not part
 *  of stream object data are limited to no more than 255 characters"
 *  Smaller length lines may be preferred for makeing the PDF output
 *  human-readable.
 */
template<int WIDTH, bool INDENT>
class TokenWriter {
public:
    TokenWriter(SkWStream*);
    /**
     *  Write a single token to the output stream.  
     */
    bool write(const char* token, size_t length);
    void write(const SkString&);
    void write(const char*);  // uses strlen()

    // write the given values to the string
    void writeInt(Int);
    void writeSize(size_t);
    void writeScalar(SkScalar);
    void writeName(const Name&);
    // Prepends a '/' to a string to make a name token.
    void writeAsName(const char*);
    // Encodes the string as a PDF string object.
    void writeAsPDFString(const char*, size_t);
    void writeAsPDFString(const SkString&);

    void writeIndirectReference(Int index);
    void beginDict(const char* type = NULL);
    void endDict();
    // if SK_PDF_SEVENBIT_OKAY, write a comment ("% COMMENT\n"),
    // otherwise this is a no-op.
    void comment(const char*);
    SkWStream* stream() const;

private:
    SkWStream* const fOut;
    int fCount;
    int fLast;
};
}  // namespace skpdf

#include "skpdf_TokenWriter_impl.h"

#endif  // skpdf_TokenWriter_DEFINED

