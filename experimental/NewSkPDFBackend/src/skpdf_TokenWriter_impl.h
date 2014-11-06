/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_TokenWriter_impl_DEFINED
#define skpdf_TokenWriter_impl_DEFINED

#include "string.h"

namespace skpdf {
template<int WIDTH, bool INDENT>
inline TokenWriter<WIDTH, INDENT>::TokenWriter(SkWStream* out)
    : fOut(out), fCount(0), fLast(-1) {
    SkASSERT(out);
}

template<int WIDTH, bool INDENT>
inline bool TokenWriter<WIDTH, INDENT>::write(const char* ptr, size_t len) {
    if (len == 0) {
        return true;
    }
    bool spaceNeeded
        = (fLast != -1)
        && !strchr("(<>[] \t\n", fLast)
        && !strchr("(<>[]/% \t\n", ptr[0]);

    if (WIDTH > 0 && fCount + len + (spaceNeeded ? 1 : 0) > WIDTH) {
        const char* newline = INDENT ? "\n\t" : "\n";
        fCount = INDENT ? 8 : 0;
        if (!fOut->write(newline, strlen(newline))) {
            return false;
        }
        spaceNeeded = false;  // newline was a space
    }
    if (spaceNeeded) {
        fCount += 1;
        if (!fOut->write(" ", 1)) {
            return false;
        }
    }
    fCount += len;
    fLast = ptr[len - 1];
    if (fLast == '\n') {
        fCount = 0;
    }
    return fOut->write(ptr, len);
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::write(const SkString& value) {
    this->write(value.c_str(), value.size());
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::write(const char* value) {
    this->write(value, strlen(value));
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::writeInt(Int value) {
    char buffer[SkStrAppendS32_MaxSize];
    char* stop = SkStrAppendS32(buffer, value);
    this->write(buffer, stop - buffer);
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::writeScalar(SkScalar value) {
    char buffer[kWriteScalarMaxSize];
    size_t len = WriteScalar(buffer, value);
    this->write(buffer, len);
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::writeSize(size_t value) {
    char buffer[SkStrAppendU64_MaxSize];
    char* stop = SkStrAppendU64(buffer, value, 0);
    this->write(buffer, stop - buffer);
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::writeName(const Name& name) {
    char buffer[Name::kWriteBuffer_MaxSize];
    size_t len = name.writeToBuffer(buffer);
    this->write(buffer, len);
}

inline void validate_name(const SkString& name) {
#   ifdef SK_DEBUG
    SkASSERT(name[0] == '/');
    for (size_t i = 1; i < name.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if (strchr("\t\n\f\r ", c) || c == 0) {
            SkDebugf("Name \"%s\" contains a whitespace character.\n",
                     name.c_str());
            SkDEBUGFAIL("bad pdf name");
        }
        if (strchr("()<>[]{}/%", c)) {
            SkDebugf("Name \"%s\" contains a delimiter character.\n",
                     name.c_str());
            SkDEBUGFAIL("bad pdf name");
        }
        if (c < 32 || c == 127) {
            SkDebugf("Name \"%s\" contains a control character.\n",
                     name.c_str());
            SkDEBUGFAIL("bad pdf name");
        }
    }
#   endif
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::writeAsName(const char* string) {
    SkString buffer = SkStringPrintf("/%s", string);
    SkDEBUGCODE(validate_name(buffer);)
    this->write(buffer.c_str(), buffer.size());
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::writeAsPDFString(const char* s,
                                                         size_t length) {
    SkString pdfString = StringToPDFString(s, length);
    this->write(pdfString.c_str(), pdfString.size());
}


template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::writeAsPDFString(const SkString& s) {
    SkString pdfString = StringToPDFString(s);
    this->write(pdfString.c_str(), pdfString.size());
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::writeIndirectReference(Int index) {
    this->writeInt(index);
    this->write("0 R");
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::beginDict(const char* type) {
    this->write("<<");
    if (type) {
        this->write("/Type");
        this->write(type);
        SkASSERT(type[0] == '/');
    }
}

template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::endDict() {
    this->write(">>");
}


template<int WIDTH, bool INDENT>
inline void TokenWriter<WIDTH, INDENT>::comment(const char* s) {
    #if SK_PDF_SEVENBIT_OKAY
    SkString buffer = SkStringPrintf("%% %s\n", s);
    this->write(buffer.c_str(), buffer.size());
    #else
    (void)s;
    #endif

}

template<int WIDTH, bool INDENT>
SkWStream* TokenWriter<WIDTH, INDENT>::stream() const {
    return fOut;
}

}  // namespace skpdf

#endif  // skpdf_TokenWriter_impl_DEFINED
