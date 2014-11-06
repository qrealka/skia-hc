/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Utils_DEFINED
#define skpdf_Utils_DEFINED

#include "SkScalar.h"
#include "SkStream.h"
#include "skpdf_Types.h"

namespace skpdf {

void Write(SkWStream* stream, char c);

void Write(SkWStream* stream, const char* s);

void Write(SkWStream* stream, SkScalar value);

void Write(SkWStream* stream, Int value);

void Write(SkWStream* stream, size_t value, int minDigits = 0);

#define kWriteScalarMaxSize 19

size_t WriteScalar(char buffer[kWriteScalarMaxSize], SkScalar value);

SkString CurrentTimeAsString();

SkString StringToPDFString(const SkString&);

SkString StringToPDFString(const char*, size_t);
}  // namespace skpdf

#include "skpdf_Utils_impl.h"

#endif  // skpdf_Utils_DEFINED

