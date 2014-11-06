/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_CrossReferenceTable_DEFINED
#define skpdf_CrossReferenceTable_DEFINED

#include "skpdf_Map.h"
#include "skpdf_Types.h"

class SkWStream;

namespace skpdf {
class CrossReferenceTable {
public:
    CrossReferenceTable();
    /**
     *  Always returns a different positive integer, can be used to
     *  reserve a index.
     */
    Int getNextUnusedIndex();
    bool isAvailable(Int index) const;
    void setOffset(Int index, size_t offset);
    void write(SkWStream*);
    void reset();
    size_t count();
private:
    Int fLast;
    Map<Int, size_t> fMap;
};
}  // namespace

#include "skpdf_CrossReferenceTable_impl.h"

#endif  // skpdf_CrossReferenceTable_DEFINED
