/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkTime.h"
#include "skpdf_Utils.h"

namespace skpdf {
SK_COMPILE_ASSERT(kWriteScalarMaxSize >= SkStrAppendS32_MaxSize, size);
SK_COMPILE_ASSERT(kWriteScalarMaxSize >= SkStrAppendScalar_MaxSize, size);
size_t WriteScalar(char buffer[kWriteScalarMaxSize], SkScalar value) {
// The range of reals in PDF/A is the same as SkFixed: +/- 32,767 and
// +/- 1/65,536 (though integers can range from 2^31 - 1 to -2^31).
// When using floats that are outside the whole value range, we can use
// integers instead.
#if !defined(SK_ALLOW_LARGE_PDF_SCALARS)
    if (value > 32767 || value < -32767) {
        char* end = SkStrAppendS32(buffer, value);
        return end - buffer;
    } else {
        char* end = SkStrAppendFixed(buffer, SkScalarToFixed(value));
        return end - buffer;
    }
#else   // SK_ALLOW_LARGE_PDF_SCALARS)
    // Floats have 24bits of significance, so anything outside that range is
    // no more precise than an int. (Plus PDF doesn't support scientific
    // notation, so this clamps to SK_Max/MinS32).
    if (value > (1 << 24) || value < -(1 << 24)) {
        char* end = SkStrAppendS32(buffer, value);
        return end - buffer;
    }
    // Continue to enforce the PDF limits for small floats.
    if (value < 1.0f / 65536 && value > -1.0f / 65536) {
        char* end = SkStrAppendS32(buffer, 0);
        return end - buffer;
    }
    static const int kFloat_MaxSize = 19;
    SK_COMPILE_ASSERT(kWriteScalarMaxSize >= kFloat_MaxSize, size);
    // SkStrAppendFloat might still use scientific notation, so use snprintf
    // directly..
    int len = SNPRINTF(buffer, kFloat_MaxSize, "%#.8f", value);
    // %f always prints trailing 0s, so strip them.
    for (; buffer[len - 1] == '0' && len > 0; len--) {
        buffer[len - 1] = '\0';
    }
    if (buffer[len - 1] == '.') {
        buffer[len - 1] = '\0';
        --len;
    }
    return len;
#endif  // SK_ALLOW_LARGE_PDF_SCALARS
}

SkString CurrentTimeAsString() {
    SkTime::DateTime dateTime;
    SkTime::GetDateTime(&dateTime);
    return SkStringPrintf(
            "D:%04u%02u%02u%02u%02u%02u",
            static_cast<unsigned>(dateTime.fYear),
            static_cast<unsigned>(dateTime.fMonth),
            static_cast<unsigned>(dateTime.fDay),
            static_cast<unsigned>(dateTime.fHour),
            static_cast<unsigned>(dateTime.fMinute),
            static_cast<unsigned>(dateTime.fSecond));
}

static char to_hex(unsigned char c) {
    return (c > 9) ? (('A' - 10) + c)  : ('0' + c);
}

SkString StringToPDFString(const char* s, size_t len) {
    SkString escapedString("(");
    for (size_t i = 0; i < len; ++i) {
        char c = s[i];
        switch (c) {
            case '\n':
                escapedString.append("\\n");
                break;
            case '\r':
                escapedString.append("\\r");
                break;
            case '\t':
                escapedString.append("\\t");
                break;
            case '(':
                escapedString.append("\\(");
                break;
            case ')':
                escapedString.append("\\)");
                break;
            case '\\':
                escapedString.append("\\\\");
                break;
            default:
                if (c <= 31 || c >= 127) {
                    escapedString.appendf("\\%03hho", c);
                } else {
                    escapedString.append(&c, 1);
                }
        }
    }
    escapedString.append(")");

    SkString hexEncodedString((len * 2) + 2);
    hexEncodedString[0] = '<';
    for (size_t i = 0; i < len; ++i) {
        unsigned char highBits = static_cast<unsigned char>(s[i]) >> 4;
        unsigned char lowBits = s[i] & 0x0F;
        hexEncodedString[(i * 2) + 1] = to_hex(highBits);
        hexEncodedString[(i * 2) + 2] = to_hex(lowBits);
    }
    hexEncodedString[hexEncodedString.size() - 1] = '>';
    if (hexEncodedString.size() < escapedString.size()) {
        return hexEncodedString;
    }

    return escapedString;
}

}  // namespace skpdf
