/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Name_DEFINED
#define skpdf_Name_DEFINED

#include "skpdf_Types.h"

class SkWStream;

namespace skpdf {

/**
 *  This name is used by the page content stream (pdf-flavored
 *  postscript) to refer to an indirect object.  The page dictionary
 *  will provide a mapping between Names and Indices.
 */
class Name {
public:
    Name(char c, Int i);
    static const size_t kWriteBuffer_MaxSize = SkStrAppendS32_MaxSize + 2;
    size_t writeToBuffer(char* buffer) const;
    void write(SkWStream* out) const;
    bool operator==(const Name& other) const;
    bool operator<(const Name& other) const;
private:
    char fType;  // 'G', 'P', 'X', or 'F'
    Int fIndex;
};

}  // namespace skpdf

#include "skpdf_Name_impl.h"

#endif  // skpdf_Name_DEFINED
