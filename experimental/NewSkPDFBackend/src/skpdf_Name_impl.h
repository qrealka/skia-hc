/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Name_impl_DEFINED
#define skpdf_Name_impl_DEFINED

#include "skpdf_Utils.h"

namespace skpdf {
inline Name::Name(char c, Int i) : fType(c), fIndex(i) {
    SkASSERT(c == 'G' || c == 'P' || c == 'X' || c == 'F');
    SkASSERT(i > 0);
}

inline void Name::write(SkWStream* out) const {
    char buffer[kWriteBuffer_MaxSize];
    size_t len = this->writeToBuffer(buffer);
    out->write(buffer, len);
}

inline size_t Name::writeToBuffer(char* buffer) const {
    buffer[0] = '/';
    buffer[1] = fType;
    return SkStrAppendS32(&buffer[2], fIndex) - buffer;
}

inline bool Name::operator==(const Name& other) const {
    return (fType == other.fType) && (fIndex == other.fIndex);
}

inline bool Name::operator<(const Name& other) const {
    if (fIndex < other.fIndex) {
        return true;
    } else if (fIndex > other.fIndex) {
        return false;
    }
    return fType < other.fType;
}
}  // namespace skpdf

#endif  // skpdf_Name_impl_DEFINED
