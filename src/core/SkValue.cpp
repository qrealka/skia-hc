/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <unordered_map>
#include <vector>

#include "SkBitmap.h"
#include "SkData.h"
#include "SkImage.h"
#include "SkLight.h"
#include "SkMatrix.h"
#include "SkValue.h"

class SkValue::Obj {
public:
    std::unordered_map<SkValue::Key, SkValue> fMap;
};

class SkValue::Arr {
public:
    std::vector<SkValue> fVec;
};

SkValue::SkValue() : fType(Null) {}

SkValue::SkValue(const SkValue& o) {
    memcpy(this, &o, sizeof(o));
    if (this->isData()) {
        fBytes->ref();
    } else if (this->isObject()) {
        fObject = new Obj(*fObject);
    } else if (Array == fType) {
        fArray = new Arr(*fArray);
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
    if (this->isData()) {
        fBytes->unref();
    } else if (this->isObject()) {
        delete fObject;
    } else if (Array == fType) {
        delete fArray;
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
SkValue  SkValue::ValueArray(size_t size) {
    SkValue v;
    v.fType = Array;
    v.fArray = new Arr;
    v.fArray->fVec.resize(size);
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

size_t SkValue::length() const {
    SkASSERT(Array == fType);
    return fArray->fVec.size();
}
const SkValue& SkValue::at(size_t idx) const {
    SkASSERT(Array == fType);
    SkASSERT(idx < fArray->fVec.size());
    return fArray->fVec.at(idx);
}
void SkValue::setAt(size_t idx, SkValue val) {
    SkASSERT(Array == fType);
    SkASSERT(idx < fArray->fVec.size());
    fArray->fVec[idx] = std::move(val);
}

SkValue SkValue::FromF32s(const float* src, int count) {
    SkValue val;
    val.fType = F32s;
    SkASSERT(val.isData());
    val.fBytes = SkData::NewWithCopy(src, SkToSizeT(count * sizeof(float)));
    SkASSERT(SkIsAlign4(uintptr_t(val.fBytes->bytes())));
    return val;
}
SkValue SkValue::FromU32s(const uint32_t* src, int count) {
    SkValue val;
    val.fType = U32s;
    SkASSERT(val.isData());
    val.fBytes = SkData::NewWithCopy(src, SkToSizeT(count * sizeof(uint32_t)));
    SkASSERT(SkIsAlign4(uintptr_t(val.fBytes->bytes())));
    return val;
}

const float* SkValue::f32s(int* count) const {
    SkASSERT(F32s == fType);
    SkASSERT(count);
    *count = fBytes->size() / sizeof(float);
    return static_cast<const float*>(fBytes->data());
}

const uint32_t* SkValue::u32s(int* count) const {
    SkASSERT(U32s == fType);
    SkASSERT(count);
    *count = fBytes->size() / sizeof(uint32_t);
    return static_cast<const uint32_t*>(fBytes->data());
}


////////////////////////////////////////////////////////////////////////////////

SkValue SkValue::Encode(const SkBitmap& bm) {
    SkAutoTUnref<SkImage> image(SkImage::NewFromBitmap(bm));
    SkASSERT(image && image->dimensions() == bm.dimensions());
    return SkValue::Encode(image);
}

enum { kEncoded };

SkValue SkValue::Encode(const SkImage* image) {
    if (!image) { return SkValue(); }
    SkAutoTUnref<SkData> encoded(image->encode());
    if (!encoded || 0 == encoded->size()) { return SkValue(); }
    auto val = SkValue::Object(SkValue::Image);
    val.set(kEncoded, SkValue::FromBytes(encoded));
    return val;
}

bool SkValue::Decode(const SkValue& val, SkBitmap* dst) {
    if (SkImage* image = DecodeImage(val)) {
        return image->asLegacyBitmap(dst, SkImage::kRO_LegacyBitmapMode);
    }
    return false;
}

SkImage* SkValue::DecodeImage(const SkValue& val) {
    if (val.type() != SkValue::Image) {
        return nullptr;
    }
    auto encoded = val.get(kEncoded);
    if (!encoded || encoded->type() != SkValue::Bytes) {
        return nullptr;
    }
    return SkImage::NewFromEncoded(encoded->bytes());
}

SkValue from(int32_t  v) { return SkValue::FromS32(v); }
SkValue from(uint32_t v) { return SkValue::FromU32(v); }
SkValue from(float    v) { return SkValue::FromF32(v); }
template<typename T> void setv(SkValue* dst, SkValue::Key key, T t) {
    if (t != T{}) {
        dst->set(key, from(t));
    }
}

SkValue SkValue::Encode(const SkMatrix& mat) {
    auto val = SkValue::Object(SkValue::Matrix);
    setv(&val, SkMatrix::kMScaleX, (float)mat[SkMatrix::kMScaleX]);
    setv(&val, SkMatrix::kMSkewX , (float)mat[SkMatrix::kMSkewX ]);
    setv(&val, SkMatrix::kMTransX, (float)mat[SkMatrix::kMTransX]);
    setv(&val, SkMatrix::kMSkewY , (float)mat[SkMatrix::kMSkewY ]);
    setv(&val, SkMatrix::kMScaleY, (float)mat[SkMatrix::kMScaleY]);
    setv(&val, SkMatrix::kMTransY, (float)mat[SkMatrix::kMTransY]);
    setv(&val, SkMatrix::kMPersp0, (float)mat[SkMatrix::kMPersp0]);
    setv(&val, SkMatrix::kMPersp1, (float)mat[SkMatrix::kMPersp1]);
    setv(&val, SkMatrix::kMPersp2, (float)mat[SkMatrix::kMPersp2]);
    return std::move(val);
}

template <typename T> T as(const SkValue&);
template<> int32_t  as<int32_t >(const SkValue& v) { return v.s32(); }
template<> uint32_t as<uint32_t>(const SkValue& v) { return v.u32(); }
template<> float    as<float   >(const SkValue& v) { return v.f32(); }
template<typename T> SkValue::Type toType();
template<> SkValue::Type toType<int32_t >() { return SkValue::S32; }
template<> SkValue::Type toType<uint32_t>() { return SkValue::U32; }
template<> SkValue::Type toType<float   >() { return SkValue::F32; }

// return false on key/type mismatch.
template <typename T> bool getv(const SkValue& val, SkValue::Key key, T* dst) {
    const SkValue* v = val.get(key);
    if (!v) {
        *dst = T{};
        return true;
    }
    if (v->type() != toType<T>()) {
        return false;
    }
    *dst = as<T>(*v);
    return true;
}

bool SkValue::Decode(const SkValue& val, SkMatrix* dst) {
    float mat[9];
    if (val.type() != SkValue::Matrix
        || !getv<float>(val, SkMatrix::kMScaleX, &mat[SkMatrix::kMScaleX])
        || !getv<float>(val, SkMatrix::kMSkewX , &mat[SkMatrix::kMSkewX ])
        || !getv<float>(val, SkMatrix::kMTransX, &mat[SkMatrix::kMTransX])
        || !getv<float>(val, SkMatrix::kMSkewY , &mat[SkMatrix::kMSkewY ])
        || !getv<float>(val, SkMatrix::kMScaleY, &mat[SkMatrix::kMScaleY])
        || !getv<float>(val, SkMatrix::kMTransY, &mat[SkMatrix::kMTransY])
        || !getv<float>(val, SkMatrix::kMPersp0, &mat[SkMatrix::kMPersp0])
        || !getv<float>(val, SkMatrix::kMPersp1, &mat[SkMatrix::kMPersp1])
        || !getv<float>(val, SkMatrix::kMPersp2, &mat[SkMatrix::kMPersp2])) {
        return false;
    }
    dst->set9(mat);
    return true;
}

enum { kLightType, kColorX, kColorY, kColorZ, kDirX, kDirY, kDirZ };

SkValue SkValue::Encode(const SkLight& light) {
    auto val = SkValue::Object(SkValue::Light);
    val.set(kLightType, SkValue::FromS32(SkToS32(light.type())));
    val.set(kColorX, SkValue::FromF32(light.color().x()));
    val.set(kColorY, SkValue::FromF32(light.color().y()));
    val.set(kColorZ, SkValue::FromF32(light.color().z()));
    if (SkLight::kAmbient_LightType != light.type()) {
        val.set(kDirX, SkValue::FromF32(light.dir().x()));
        val.set(kDirY, SkValue::FromF32(light.dir().y()));
        val.set(kDirZ, SkValue::FromF32(light.dir().z()));
    }
    return val;
}
#include "SkLight.h"
bool SkValue::Decode(const SkValue& val, SkLight* light) {
    int32_t lightTypeInt;
    float color[3];
    if (val.type() != SkValue::Light
        || !getv<int32_t>(val, kLightType, &lightTypeInt)
        || !getv<float>(val, kColorX, &color[0])
        || !getv<float>(val, kColorY, &color[1])
        || !getv<float>(val, kColorZ, &color[2])) {
        return false;
    }
    SkLight::LightType lt = static_cast<SkLight::LightType>(lightTypeInt);
    SkColor3f color3{color[0], color[1], color[2]};
    if (SkLight::kAmbient_LightType == lt) {
        *light = SkLight(color3);
        return true;
    } else if (SkLight::kDirectional_LightType == lt) {
        float dir[3];
        if (    !getv<float>(val, kDirX, &dir[0])
             || !getv<float>(val, kDirY, &dir[1])
             || !getv<float>(val, kDirZ, &dir[2])) {
            return false;
        }
        *light = SkLight(color3, SkVector3{dir[0], dir[1], dir[2]});
        return true;
    } else {
        return false;
    }
}
