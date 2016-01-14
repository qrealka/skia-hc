/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Resources.h"
#include "SkArithmeticMode.h"
#include "SkBitmapProcShader.h"
#include "SkColorFilter.h"
#include "SkColorFilterShader.h"
#include "SkComposeShader.h"
#include "SkData.h"
#include "SkGradientShader.h"
#include "SkLerpXfermode.h"
#include "SkLightingShader.h"
#include "SkPixelXorXfermode.h"
#include "SkStream.h"
#include "SkValue.h"
#include "SkXfermode.h"
#include "Test.h"
#include "sk_tool_utils.h"

#include "SkComposeShader.h"

////////////////////////////////////////////////////////////////////////////////

static void print_value(bool verbose, const SkValue&);

template <class T>
void test_value(skiatest::Reporter* r, T* object, SkValue::Type type) {
    const SkValue val = object->asValue();
    object->unref();
    REPORTER_ASSERT(r, type == val.type());
    print_value(r->verbose(), val);
}

DEF_TEST(Value_Xfermode, r) {
    // just test that these code paths work until we hook them up to a DM sink.
    test_value(r, SkXfermode::Create(SkXfermode::kDstOver_Mode),
               SkValue::ProcCoeffXfermode);
    test_value(r, SkArithmeticMode::Create(0.125f, 0.25f, 0.375f, 0.5f, true),
               SkValue::ArithmeticXfermode);
    test_value(r, SkLerpXfermode::Create(1.0f/3.0f), SkValue::LerpXfermode);
    test_value(r, SkPixelXorXfermode::Create(0xFF00FF00),
               SkValue::PixelXorXfermode);
}



#if 0
////////////////////////////////////////////////////////////////////////////////

#ifndef SkPaintEffects_DEFINED
#define SkPaintEffects_DEFINED

class SkShader;
class SkXfermode;
class SkBitmap;
class SkColorFilter;
namespace SkPaintEffects {
void Unref(SkColorFilter*);

void Unref(SkXfermode*);
SkXfermode* CreateXfermode(int);
SkXfermode* CreateArithmeticXfermode(
        SkShader, SkShader, SkShader, SkShader, bool enforcePMColor = true);
SkXfermode* CreateLerpXfermode(SkScalar);
SkXfermode* CreatePixelXorXfermode(SkColor);

void Unref(SkShader*);
SkShader* CreateEmptyShader();
SkShader* CreateColorShader(SkColor);
SkShader* CreateComposeShader(SkShader*, SkShader*, SkXfermode* = nullptr);
SkShader* CreateBitmapProcShader(const SkBitmap&, int, int);
SkShader* CreateColorFilterShader(SkShader*, SkColorFilter*);


////////////////////////////////////////////////////////////////////////////////
template <typename T> struct Unrefer {
    void operator()(T* t) { SkPaintEffects::Unref(t); }
};
template <typename T> class AutoUnref
    : public skstd::unique_ptr<T, SkPaintEffects::Unrefer<T>> {
public:
    explicit SkAutoTUnref(T* obj = nullptr)
        : skstd::unique_ptr<T, SkPaintEffects::Unrefer<T>>(obj) {}
    operator T*() const { return this->get(); }
};
}  // namespace
#endif  // SkPaintEffects_DEFINED
////////////////////////////////////////////////////////////////////////////////
#endif  // 0


SkShader* example_lighting_shader() {
    SkLightingShader::Lights::Builder builder;
    builder.add(SkLight(SkColor3f::Make(1.0f, 1.0f, 1.0f),
                        SkVector3::Make(1.0f, 0.0f, 0.0f)));
    builder.add(SkLight(SkColor3f::Make(0.2f, 0.2f, 0.2f)));
    SkAutoTUnref<const SkLightingShader::Lights> lights(builder.finish());
    SkBitmap hemi;
    hemi.allocN32Pixels(128, 128);
    sk_tool_utils::create_hemi_normal_map(&hemi, SkIRect::MakeWH(128, 128));
    SkBitmap diffuse = sk_tool_utils::create_checkerboard_bitmap(
            128, 128,0x0, 0xFF804020, 8);
    SkMatrix mat = SkMatrix::I();
    return SkLightingShader::Create(
            diffuse, hemi, lights, SkVector{1.0f, 0.0f}, &mat, &mat);
}

DEF_TEST(Value_Shader, r) {
    SkAutoTUnref<SkShader> emptyShader(SkShader::CreateEmptyShader());
    SkAutoTUnref<SkShader> colorShader(
            SkShader::CreateColorShader(SK_ColorCYAN));
    SkAutoTUnref<SkShader> composeShader(
            new SkComposeShader(emptyShader, colorShader));
    SkBitmap bm;
    SkAssertResult(GetResourceAsBitmap("color_wheel.png", &bm));
    SkAutoTUnref<SkShader> bitmapProcShader(
            new SkBitmapProcShader(bm, SkShader::kRepeat_TileMode,
                                   SkShader::kRepeat_TileMode));
 
    SkAutoTUnref<SkColorFilter> cf(
            SkColorFilter::CreateModeFilter(SK_ColorRED,
                                            SkXfermode::kDstOver_Mode));
    SkAutoTUnref<SkShader> colorFilterShader(
            new SkColorFilterShader(bitmapProcShader, cf));

    test_value(r, SkRef(example_lighting_shader()), SkValue::LightingShader);
    test_value(r, SkRef(emptyShader.get()), SkValue::EmptyShader);
    test_value(r, SkRef(colorShader.get()), SkValue::ColorShader);
    test_value(r, SkRef(composeShader.get()), SkValue::ComposeShader);
    test_value(r, SkRef(bitmapProcShader.get()), SkValue::BitmapProcShader);
    test_value(r, SkRef(colorFilterShader.get()), SkValue::ColorFilterShader);

    static const int kCount = 3;
    SkColor colors[kCount] = { SK_ColorCYAN, SK_ColorBLUE, SK_ColorMAGENTA };
    SkPoint pts[] = { SkPoint{2.25f, 2.25f}, SkPoint{12.0f, 12.0f} };

    test_value(r, SkGradientShader::CreateLinear(
                       pts, colors, nullptr, kCount, SkShader::kMirror_TileMode, 0, nullptr),
               SkValue::LinearGradientShader);


    test_value(r, SkGradientShader::CreateRadial( pts[0], 1.0f, colors, nullptr, kCount, SkShader::kMirror_TileMode, 0, nullptr),
               SkValue::RadialGradient);

    test_value(r, SkGradientShader::CreateTwoPointConical( pts[0], 1.0f, pts[1], 50.0f, colors, nullptr, kCount, SkShader::kMirror_TileMode, 0, nullptr),
               SkValue::TwoPointConicalGradientShader);

    test_value(r, SkGradientShader::CreateSweep( pts[0].x(), pts[1].y(), colors, nullptr, kCount, 0, nullptr),
               SkValue::SweepGradientShader);



}

// Conditional print.  Always evaluate the arguments.
static void do_nothing(const char*, ...) {}
#define CPRINT(DO_PRINT) ((DO_PRINT) ? SkDebugf : do_nothing)

static void dump(bool verbose, int indent, const SkValue&);
static void dump_object(bool verbose, int indent,
                        const char* name, const SkValue& val) {
    CPRINT(verbose)("[ \"%s\", {\n", name);
    // todo: sort keys
    val.foreach([=](SkValue::Key k, const SkValue& v){
            for (int i = 0; i < indent; ++i) {
                CPRINT(verbose)("    ");
            }
            CPRINT(verbose)("  \"key_%u\" : ", k);
            dump(verbose, indent + 1, v);
            CPRINT(verbose)("\n");
        });
    for (int i = 0; i < indent; ++i) {
        CPRINT(verbose)("    ");
    }
    CPRINT(verbose)("} ],");
}
static void dump_array(bool verbose, int indent, const SkValue& val) {
    CPRINT(verbose)("[ \"Array\", [\n");
    for (size_t i = 0; i < val.length(); ++i) {
        for (int i = 0; i < indent; ++i) {
            CPRINT(verbose)("    ");
        }
        dump(verbose, indent + 1, val.at(i));
        CPRINT(verbose)("\n");
    }
    for (int i = 0; i < indent; ++i) {
        CPRINT(verbose)("    ");
    }
    CPRINT(verbose)("] ],");
    return;
}

static void dump(bool verbose, int indent, const SkValue& val) {
    switch (val.type()) {
        case SkValue::Null:
            CPRINT(verbose)("[ \"Null\", null ],");
            return;
        case SkValue::S32:
            CPRINT(verbose)("[ \"S32\", %d ],", val.s32());
            return;
        case SkValue::U32:
            CPRINT(verbose)("[ \"U32\", 0x%X ],", val.u32());
            return;
        case SkValue::F32:
            CPRINT(verbose)("[ \"F32\", %.9g ],", val.f32());
            return;
        case SkValue::Bytes: {
            CPRINT(verbose)("[ \"Bytes\", 0x");
            SkData* data =  val.bytes();
            const size_t max = 40;
            for (size_t i = 0; i < SkTMin(max, data->size()); ++i) {
                CPRINT(verbose)("%02X", data->bytes()[i]);
            }
            if (data->size() > max) {
                CPRINT(verbose)("...(%u)", data->size());
            }
            CPRINT(verbose)(" ],");
            return;
        }
        case SkValue::U32s: {
            CPRINT(verbose)("[ \"U32s\", [ ");
            int count;
            const uint32_t* array = val.u32s(&count);
            for (int i = 0; i < count; ++i) {
                CPRINT(verbose)("0x%X", array[i]);
                if (i + 1 != count) {
                    CPRINT(verbose)(", ");
                }
            }
            CPRINT(verbose)(" ]");
            return;
        }
        case SkValue::F32s: {
            CPRINT(verbose)("[ \"F32s\", [ ");
            int count;
            const float* array = val.f32s(&count);
            for (int i = 0; i < count; ++i) {
                CPRINT(verbose)("%.9g", array[i]);
                if (i + 1 != count) {
                    CPRINT(verbose)(", ");
                }
            }
            CPRINT(verbose)(" ]");
            return;
        }
        case SkValue::Array:
            dump_array(verbose, indent, val);
            return;
        #define FN(OBJECT) \
            case SkValue::OBJECT: dump_object(verbose, indent, #OBJECT, val); return;
        FN(Image)
        FN(Light)
        FN(Matrix)

        FN(ArithmeticXfermode)
        FN(LerpXfermode)
        FN(OverdrawXfermode)
        FN(PixelXorXfermode)
        FN(ProcCoeffXfermode)

        FN(ThreeDeeShader)
        FN(BitmapProcShader)
        FN(ColorFilterShader)
        FN(ColorShader)
        FN(ComposeShader)
        FN(DCShader)
        FN(EmptyShader)
        FN(ImageShader)
        FN(LightingShader)
        FN(LinearGradientShader)
        FN(LocalMatrixShader)
        FN(PerlinNoiseShader)
        FN(PictureShader)
        FN(RadialGradient)
        FN(SweepGradientShader)
        FN(TriColorShader)
        FN(TwoPointConicalGradientShader)
        #undef FN
        default:
            if ((uint32_t)val.type() >= 0x80000000) {
                SkString object = SkStringPrintf("Test_0x%X", val.type());
                dump_object(verbose, indent, object.c_str(), val);
                return;
            }
            SkDebugf("UNKNOWN VALUE TYPE: %X\n", (uint32_t)val.type());
            SkASSERT(false);
    }
}

static void print_value(bool verbose, const SkValue& val) {
    CPRINT(verbose)("\n");
    dump(verbose, 0, val);
    CPRINT(verbose)("\n");
}

#if 0

#endif

