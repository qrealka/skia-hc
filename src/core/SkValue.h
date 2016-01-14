/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkValue_DEFINED
#define SkValue_DEFINED

#include "SkTypes.h"
#include <functional>

class SkBitmap;
class SkData;
class SkImage;
class SkLight;
class SkMatrix;

class SkValue {
public:
    enum Type : uint32_t {
        // 0-255 are reserved for built-in SkValue types.
        Null,
        Byte , S16 , U16 , S32 , U32 , S64 , U64 , F32 , F64 ,
        Bytes, S16s, U16s, S32s, U32s, S64s, U64s, F32s, F64s,
        Array,

        kMaxBuiltin = 0xFF,

        // 256-2147483647 may be used by Skia for public Object types.

        Image,
        Light,
        Matrix,

        ArithmeticXfermode,
        LerpXfermode,
        OverdrawXfermode,
        PixelXorXfermode,
        ProcCoeffXfermode,

        ThreeDeeShader,
        BitmapProcShader,
        ColorFilterShader,
        ColorShader,
        ComposeShader,
        DCShader,
        EmptyShader,
        ImageShader,
        LightingShader,
        LinearGradientShader,
        LocalMatrixShader,
        PerlinNoiseShader,
        PictureShader,
        RadialGradient,
        SweepGradientShader,
        TriColorShader,
        TwoPointConicalGradientShader,

        // 2147483648+ won't be used by Skia.  They're open for client-specific use, testing, etc.
    };

    // Each Object type may define its own namespace of Key values,
    // so there are no pre-defined Keys here.
    //
    // This is just a reminder that they must fit in a uint32_t,
    // and that their namespace is distinct from other uint32_ts (e.g. Type).
    typedef uint32_t Key;

    SkValue();
    SkValue(const SkValue&);
    SkValue(SkValue&&);

    SkValue& operator=(const SkValue&);
    SkValue& operator=(SkValue&&);

    ~SkValue();

    static SkValue FromS32(int32_t);
    static SkValue FromU32(uint32_t);
    static SkValue FromF32(float);
    static SkValue FromBytes(SkData*);
    static SkValue Object(Type);
    static SkValue ValueArray(size_t);
    static SkValue FromU32s(const uint32_t*, int);
    static SkValue FromF32s(const float*, int);

    Type type() const;

    // These remaining methods may assert they're called on a value of the appropriate type.

    int32_t  s32() const;
    uint32_t u32() const;
    float    f32() const;

    SkData* bytes() const;

    void set(Key, SkValue);
    const SkValue* get(Key) const;
    void foreach(std::function<void(Key, const SkValue&)>) const;

    size_t length() const;
    const SkValue& at(size_t) const;
    void setAt(size_t, SkValue);

    const uint32_t* u32s(int* count) const;
    const float*    f32s(int* count) const;

    static SkValue Encode(const SkImage*);
    static SkValue Encode(const SkBitmap&);
    static SkValue Encode(const SkMatrix&);
    static SkValue Encode(const SkLight&);

    static SkImage* DecodeImage(const SkValue&);
    static bool Decode(const SkValue&, SkBitmap*);
    static bool Decode(const SkValue&, SkMatrix*);
    static bool Decode(const SkValue&, SkLight*);

private:
    class Obj;
    class Arr;

    Type fType;
    union {
        int32_t  fS32;
        uint32_t fU32;
        float    fF32;
        SkData*  fBytes;
        Obj*     fObject;
        Arr*     fArray;
    };

    bool isObject() const { return fType > kMaxBuiltin; }
    bool isData() const {
        return Bytes == fType || F32s == fType || U32s == fType;
    }
};

#endif//SkValue_DEFINED
