/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_CrossReferenceTable_impl_DEFINED
#define skpdf_CrossReferenceTable_impl_DEFINED

#include "SkStream.h"
#include "skpdf_Utils.h"

namespace skpdf {
inline CrossReferenceTable::CrossReferenceTable() : fLast(0) {}

inline Int CrossReferenceTable::getNextUnusedIndex() {
    do {
        ++fLast;
    } while (!this->isAvailable(fLast));
    return fLast;
}
inline bool CrossReferenceTable::isAvailable(Int index) const {
    return !fMap.find(index);
}
inline void CrossReferenceTable::setOffset(Int index, size_t offset) {
    SkASSERT(this->isAvailable(index));
    fMap.add(index, offset);
}

inline void CrossReferenceTable::write(SkWStream* out) {
    fMap.sortByKey();  // make the iterators produce sorted output

    Write(out, "xref\n");
    Write(out, "0 1\n");
    Write(out, "0000000000 65535 f \n");

    typedef Map<Int, size_t>::Pair Pair;
    const Pair* const end = fMap.end();
    const Pair* iter = fMap.begin();
    while (iter < end) {
        const Pair* firstIndexIter = iter;
        do {
            ++iter;
        } while (iter < end && (iter->key() == (iter[-1].key() + 1)));
        Write(out, firstIndexIter->key());
        Write(out, " ");
        Int indexCount = iter - firstIndexIter;
        Write(out, indexCount);
        Write(out, "\n");
        for (const Pair* ptr = firstIndexIter; ptr < iter; ++ptr) {
            Write(out, ptr->value(), 10);
            Write(out, " 00000 n \n");
        }
    }
}
inline void CrossReferenceTable::reset() {
    fMap.reset();
}

inline size_t CrossReferenceTable::count() {
    return fMap.count();
}
}  // namespace skpdf
#endif  // skpdf_CrossReferenceTable_impl_DEFINED
