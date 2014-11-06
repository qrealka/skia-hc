/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef skpdf_Map_impl_DEFINED
#define skpdf_Map_impl_DEFINED

#include "SkTSort.h"

namespace skpdf {
template <typename K, typename V>
Map<K,V>::Pair::Pair(const K& key, const V& value) : fKey(key), fValue(value) {
}

template <typename K, typename V>
const K& Map<K,V>::Pair::key() const {
    return fKey;
}

template <typename K, typename V>
const V& Map<K,V>::Pair::value() const {
    return fValue;
}

template <typename K, typename V>
inline void Map<K, V>::add(const K& key, const V& value) {
    SkASSERT(-1 == this->findPair(key));
    fArray.push(Pair(key, value));
}

template <typename K, typename V>
inline void Map<K, V>::remove(const K& key) {
    int index = findPair(key);
    SkASSERT(index > 0);
    fArray.removeShuffle(index);
}

template <typename K, typename V>
inline const V* Map<K, V>::find(const K& key) const {
    int index = this->findPair(key);
    return (index >= 0) ? &(fArray[index].value()) : NULL;
}

template <typename K, typename V>
inline int Map<K, V>::count() const {
    return fArray.count();
}

template <typename K, typename V>
inline const typename Map<K, V>::Pair& Map<K, V>::operator[](int i) const {
    return fArray[i];
}

template <typename K, typename V>
inline typename Map<K, V>::Pair& Map<K, V>::operator[](int i) {
    return fArray[i];
}

template <typename K, typename V>
inline const typename Map<K, V>::Pair* Map<K, V>::begin() const {
    return fArray.begin();
}

template <typename K, typename V>
inline const typename Map<K, V>::Pair* Map<K, V>::end() const {
    return fArray.end();
}

template <typename K, typename V>
inline void Map<K, V>::reset() {
    fArray.reset();
}

template <typename K, typename V>
inline bool Map<K, V>::Pair::operator<(const Map<K, V>::Pair& rhs) const {
    return this->key() < rhs.key();
}

template <typename K, typename V>
inline void Map<K, V>::sortByKey() {
    // K must have operator<(const K&)
    SkTQSort(fArray.begin(), fArray.end() - 1);
}

template <typename K, typename V>
inline int Map<K, V>::findPair(const K& key) const {
    for (int i = 0; i < fArray.count(); ++i) {
        if (key == fArray[i].key()) {
            return i;
        }
    }
    return -1;
}

}  // namespace skpdf

#endif  // skpdf_Map_impl_DEFINED
