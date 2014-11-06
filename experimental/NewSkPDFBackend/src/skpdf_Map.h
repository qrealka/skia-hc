/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Map_DEFINED
#define skpdf_Map_DEFINED

#include "SkTDArray.h"

namespace skpdf {

// K, V are assignment-copyable and memcpy()-moveable plain-old-data.
// K has operator==(const K&) defined.
// K needs operator<(const K&) defined for sorting.
template <typename K, typename V>
class Map {
public:
    void add(const K& k, const V& v);
    void remove(const K& k);
    const V* find(const K& k) const;
    int count() const;
    class Pair {
    public:
        Pair(const K& key, const V& value);
        const K& key() const;
        const V& value() const;
        bool operator<(const Map<K, V>::Pair&) const;
    private:
        K fKey;
        V fValue;
    };
    const Pair& operator[](int i) const;
    Pair& operator[](int i);
    const Pair* begin() const;
    const Pair* end() const;
    void reset();
    void sortByKey();  // K must have operator<(const K&) const

private:
    SkTDArray<Pair> fArray;
    int findPair(const K& k) const;
};

}  // namespace skpdf

#include "skpdf_Map_impl.h"

#endif  // skpdf_Map_DEFINED
