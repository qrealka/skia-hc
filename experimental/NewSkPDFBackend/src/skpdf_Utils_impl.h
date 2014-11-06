/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Utils_impl_DEFINED
#define skpdf_Utils_impl_DEFINED

#include "SkString.h"
#include "SkStream.h"

namespace skpdf {

inline void Write(SkWStream* stream, char c) {
    stream->write(&c, sizeof(c));
}

inline void Write(SkWStream* stream, const char* s) {
    stream->write(s, strlen(s));
}

// Use low-level SkString functions to avoid unnecessary mallocs.
inline void Write(SkWStream* stream, Int value) {
    char buffer[SkStrAppendS32_MaxSize]; // + 1];
    //    buffer[0] = ' ';  // space the buffer, so we don't hit the previous token.
    char* stop = SkStrAppendS32(buffer, value);
    stream->write(buffer, stop - buffer);
}

inline void Write(SkWStream* stream, size_t value, int minDigits) {
    SkASSERT(minDigits <= SkStrAppendU64_MaxSize);
    char buffer[SkStrAppendU64_MaxSize];
    char* stop = SkStrAppendU64(buffer, value, minDigits);
    stream->write(buffer, stop - buffer);
}

inline void Write(SkWStream* stream, SkScalar value) {
    char buffer[kWriteScalarMaxSize];
    size_t len = WriteScalar(buffer, value);
    stream->write(buffer, len);
}

inline SkString StringToPDFString(const SkString& s) {
    return StringToPDFString(s.c_str(), s.size());
}
}  // namespace skpdf

#endif  // skpdf_Utils_impl_DEFINED
