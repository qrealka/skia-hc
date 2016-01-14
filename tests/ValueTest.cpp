/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkValue.h"
#include "Test.h"

static const SkValue::Type example_type = SkValue::Type(0x80000001);

enum { kExampleS32, kExampleF32, kExampleU32, kExampleObject};

static SkValue make_example(skiatest::Reporter* r, int level = 4) {
    auto value = SkValue::Object(example_type);
    value.set(kExampleU32, SkValue::FromU32(1000));
    value.set(kExampleS32, SkValue::FromS32(-123));
    value.set(kExampleF32, SkValue::FromF32(0.5f));
    value.set(kExampleU32, SkValue::FromU32(1234));
    if (level > 0) {
        value.set(kExampleObject, make_example(r, 0));
        value.set(kExampleObject, make_example(r, level - 1)); // replace
    }
    return value;
}

DEF_TEST(Value, r) {
    SkValue val = make_example(r);
    REPORTER_ASSERT(r, example_type == val.type());
    val.set(4321, SkValue());
    SkValue valCopy = val;
    REPORTER_ASSERT(r, example_type == valCopy.type());
    const SkValue* v;
    v = val.get(kExampleS32);
    REPORTER_ASSERT(r, v && -123 == v->s32());
    v = val.get(kExampleF32);
    REPORTER_ASSERT(r, v && 0.5f == v->f32());
    v = val.get(kExampleU32);
    REPORTER_ASSERT(r, v && 1234 == v->u32());
}
