/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <unordered_map>

#include "SkData.h"
#include "SkValue.h"

class SkValue::Obj {
public:
    std::unordered_map<SkValue::Key, SkValue> fMap;
};

SkValue::SkValue() : fType(Null) {}

SkValue::SkValue(const SkValue& o) {
    memcpy(this, &o, sizeof(o));
    if (Bytes == fType) {
        fBytes->ref();
    } else if (this->isObject()) {
        fObject = new Obj(*fObject);
    }
}

SkValue::SkValue(SkValue&& o) {
    memcpy(this, &o, sizeof(o));
    new (&o) SkValue();
}

SkValue& SkValue::operator=(const SkValue& o) {
    if (this != &o) {
        this->~SkValue();
        new (this) SkValue(o);
    }
    return *this;
}

SkValue& SkValue::operator=(SkValue&& o) {
    if (this != &o) {
        this->~SkValue();
        new (this) SkValue(std::move(o));
    }
    return *this;
}

SkValue::~SkValue() {
    if (Bytes == fType) {
        fBytes->unref();
    } else if (this->isObject()) {
        delete fObject;
    }
}

#define FN(NAME, LNAME, TYPE)             \
    SkValue SkValue::From##NAME(TYPE x) { \
        SkValue v;                        \
        v.fType = NAME;                   \
        v.f##NAME = x;                    \
        return v;                         \
    }                                     \
    TYPE SkValue::LNAME() const {         \
        SkASSERT(NAME == fType);          \
        return f##NAME;                   \
    }
FN(S32, s32, int32_t)
FN(U32, u32, uint32_t)
FN(F32, f32, float)
#undef FN

SkValue SkValue::FromBytes(SkData* data) {
    if (!data) {
        return SkValue();
    }
    SkValue v;
    v.fType = Bytes;
    v.fBytes = SkRef(data);
    return v;
}

SkValue SkValue::Object(SkValue::Type t) {
    SkValue v;
    v.fType = t;
    SkASSERT(v.isObject());
    v.fObject = new Obj;
    return v;
}

SkValue::Type SkValue::type() const { return fType; }

SkData* SkValue::bytes() const {
    SkASSERT(Bytes == fType);
    return fBytes;
}

void SkValue::set(SkValue::Key k, SkValue v) {
    SkASSERT(this->isObject());
    fObject->fMap[k] = std::move(v);
}

const SkValue* SkValue::get(SkValue::Key k) const {
    SkASSERT(this->isObject());
    auto it = fObject->fMap.find(k);
    return it != fObject->fMap.end() ? &it->second : nullptr;
}

void SkValue::foreach(std::function<void(Key, const SkValue&)> fn) const {
    SkASSERT(this->isObject());
    for (const auto& pair : fObject->fMap) {
        fn(pair.first, pair.second);
    }
}
