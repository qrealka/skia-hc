/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Types_DEFINED
#define skpdf_Types_DEFINED

#include <stdint.h>

#if !defined(SK_PDF_SEVENBIT_OKAY)
#  ifdef SK_DEBUG
#    define SK_PDF_SEVENBIT_OKAY 1  // Produces 25% larger files, but readable.
#  else
#    define SK_PDF_SEVENBIT_OKAY 0  // smallest size, binary.
#  endif
#endif  // !defined(SK_PDF_SEVENBIT_OKAY)

#if SK_PDF_SEVENBIT_OKAY
#   define SK_PDF_MAX_COLUMNS 80  // make output more readable
#   define SK_PDF_INDENT true
#else
#   define SK_PDF_MAX_COLUMNS 255  // required by spec
#   define SK_PDF_INDENT false
#endif


namespace skpdf {
typedef int32_t Int;  // maps directly to PDF integer type.  PDF
                      // objects have a maximum number of indirect
                      // objects in a PDF file: (1<<23)-1, so this
                      // data type will be directly used to index
                      // indirect objects.

// TODO : write all of these.
struct ShaderKey {
    /*FIXME*/
};
struct PaintKey {
    bool operator==(const PaintKey&) const { return true; }
    /*FIXME*/
};
struct FontKey {
    /*FIXME*/
};
struct SMaskKey {
    /*FIXME*/
};



}  // namespace skpdf

#endif  // skpdf_Types_DEFINED
