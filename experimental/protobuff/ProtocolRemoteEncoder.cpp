/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkAnnotation.h"
#include "SkCanvas.h"
#include "SkImageEncoder.h"
#include "SkPath.h"
#include "SkPathEffect.h"
#include "SkReadBuffer.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkTHash.h"
#include "SkTHash.h"
#include "SkTextBlobRunIterator.h"
#include "SkTypeface.h"
#include "SkWriteBuffer.h"

#include "ProtocolRemoteEncoder.h"

#include "skia.pb.h"

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

using namespace SkProto;

namespace {

class IDFactory {
private:
    uint64_t fNext[(unsigned)SkRemote::Type::kAnnotation + 1];
public:
    IDFactory() : fNext{0} {}
    SkRemote::ID next(SkRemote::Type t) {
        SkASSERT((unsigned)t < SK_ARRAY_COUNT(fNext));
        return SkRemote::ID(t, (fNext[(unsigned)t])++);
    }
};
class WStreamInterface : public google::protobuf::io::CopyingOutputStream {
public:
    WStreamInterface(SkWStream* s): fWStream(s) {}
    ~WStreamInterface() { fWStream->flush(); }
    bool Write(const void* buffer, int size) override {
        return fWStream->write(buffer, SkToSizeT(size));
    }
private:
    SkWStream* fWStream;
};

class ProtoEncoder final : public SkRemote::Encoder {
public:
    ProtoEncoder(SkWStream*);
    ~ProtoEncoder();
    SkRemote::ID define(const SkMatrix&)        override;
    SkRemote::ID define(const SkRemote::Misc&)  override;
    SkRemote::ID define(const SkPath&)          override;
    SkRemote::ID define(const SkRemote::Stroke&)override;
    SkRemote::ID define(const SkTextBlob*)      override;
    SkRemote::ID define(SkPathEffect*)          override;
    SkRemote::ID define(SkShader*)              override;
    SkRemote::ID define(SkXfermode*)            override;
    SkRemote::ID define(SkMaskFilter*)          override;
    SkRemote::ID define(SkColorFilter*)         override;
    SkRemote::ID define(SkRasterizer*)          override;
    SkRemote::ID define(SkDrawLooper*)          override;
    SkRemote::ID define(SkImageFilter*)         override;
    SkRemote::ID define(SkAnnotation*)          override;
    void undefine(SkRemote::ID) override;

    void saveLayer(SkRemote::ID bounds,
                   SkRemote::Encoder::CommonIDs,
                   uint32_t saveLayerFlags) override;
    void save() override;
    void restore() override;
    void setMatrix(SkRemote::ID matrix) override;
    void clipPath(SkRemote::ID path, SkRegion::Op, bool aa) override;
    void fillPath(SkRemote::ID path, SkRemote::Encoder::CommonIDs) override;
    void strokePath(SkRemote::ID path,
                    SkRemote::Encoder::CommonIDs,
                    SkRemote::ID stroke) override;
    void   fillText(SkRemote::ID text,
                    SkPoint,
                    SkRemote::Encoder::CommonIDs) override;
    void strokeText(SkRemote::ID text,
                    SkPoint,
                    SkRemote::Encoder::CommonIDs,
                    SkRemote::ID stroke) override;

private:
    IDFactory fIDs;
    SkWStream* fWStream;
    SkTHashSet<SkFontID> fDefinedTypefaces;
    void defineTypeface(SkTypeface*);
    RemoteCommands fCommands;
};

ProtoEncoder::ProtoEncoder(SkWStream* s) : fWStream(s) {}
ProtoEncoder::~ProtoEncoder() {
    WStreamInterface iface(fWStream);
    google::protobuf::io::CopyingOutputStreamAdaptor adaptor(&iface);
    google::protobuf::TextFormat::Print(fCommands, &adaptor);
}
SkRemote::ID ProtoEncoder::define(const SkMatrix& matrix)  {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kMatrix));
    Matrix* m = new Matrix;
    m->set_scale_x(matrix[SkMatrix::kMScaleX]);
    m->set_skew_x (matrix[SkMatrix::kMSkewX ]);
    m->set_trans_x(matrix[SkMatrix::kMTransX]);
    m->set_skew_y (matrix[SkMatrix::kMSkewY ]);
    m->set_scale_y(matrix[SkMatrix::kMScaleY]);
    m->set_trans_y(matrix[SkMatrix::kMTransY]);
    m->set_persp_0(matrix[SkMatrix::kMPersp0]);
    m->set_persp_1(matrix[SkMatrix::kMPersp1]);
    m->set_persp_2(matrix[SkMatrix::kMPersp2]);
    RemoteCommands::DefineMatrix* define_matrix(
            new RemoteCommands::DefineMatrix);
    define_matrix->set_id(id.val());
    define_matrix->set_allocated_matrix(m);
    fCommands.add_remote_commands()->set_allocated_define_matrix(define_matrix);
    return id;
}
SkRemote::ID ProtoEncoder::define(const SkRemote::Misc& misc) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kMisc));
    Misc* pmisc = new Misc;
    pmisc->set_color(misc.fColor);
    FilterQuality q(FILTER_QUALITY_NONE);
    switch (misc.fFilterQuality) {
        case kNone_SkFilterQuality:   q = FILTER_QUALITY_NONE  ; break;
        case kLow_SkFilterQuality:    q = FILTER_QUALITY_LOW   ; break;
        case kMedium_SkFilterQuality: q = FILTER_QUALITY_MEDIUM; break;
        case kHigh_SkFilterQuality:   q = FILTER_QUALITY_HIGH  ; break;
        default: SkDEBUGFAIL("");
    }
    pmisc->set_filter_quality(q);
    pmisc->set_anti_alias(misc.fAntiAlias ? ANTIALIAS_ON : ANTIALIAS_OFF);
    pmisc->set_dither(misc.fDither ? DITHER_ON : DITHER_OFF);
    RemoteCommands::DefineMisc* define_misc(new RemoteCommands::DefineMisc);
    define_misc->set_id(id.val());
    define_misc->set_allocated_misc(pmisc);
    fCommands.add_remote_commands()->set_allocated_define_misc(define_misc);
    return id;
}

static Path::Direction to_direction(SkPath::Direction direction) {
    switch (direction) {
        case SkPath::kCW_Direction:  return Path::DIRECTION_CW;
        case SkPath::kCCW_Direction: return Path::DIRECTION_CCW;
        default: SkDEBUGFAIL("");
    }
    return Path::DIRECTION_CW;
}
static Rect* to_rect(const SkRect& rect) {
    Rect* r = new Rect;
    r->set_left(rect.left());
    r->set_top(rect.top()) ;
    r->set_right(rect.right());
    r->set_bottom(rect.bottom());
    return r;
}
static Path::FillType to_fill_type(SkPath::FillType fillType) {
    switch (fillType) {
        case SkPath::kWinding_FillType:        return Path::FILL_TYPE_WINDING;
        case SkPath::kEvenOdd_FillType:        return Path::FILL_TYPE_EVEN_ODD;
        case SkPath::kInverseWinding_FillType: return Path::FILL_TYPE_INVERSE_WINDING;
        case SkPath::kInverseEvenOdd_FillType: return Path::FILL_TYPE_INVERSE_EVEN_ODD;
        default: SkDEBUGFAIL("");              return Path::FILL_TYPE_WINDING;
    }
}

// static Vector* to_vector(const SkVector& vect) {
//     Vector* v = new Vector;
//     v->set_x(vect.x());
//     v->set_y(vect.y()) ;
//     return v;
// }

static RoundRect* to_round_rect(const SkRRect& rrect) {
    RoundRect* roundRect = new RoundRect;
    roundRect->set_left         (rrect.rect().left  ());
    roundRect->set_top          (rrect.rect().top   ());
    roundRect->set_right        (rrect.rect().right ());
    roundRect->set_bottom       (rrect.rect().bottom());
    roundRect->set_upper_left_x (rrect.radii(SkRRect::kUpperLeft_Corner ).x());
    roundRect->set_upper_left_y (rrect.radii(SkRRect::kUpperLeft_Corner ).y());
    roundRect->set_upper_right_x(rrect.radii(SkRRect::kUpperRight_Corner).x());
    roundRect->set_upper_right_y(rrect.radii(SkRRect::kUpperRight_Corner).y());
    roundRect->set_lower_right_x(rrect.radii(SkRRect::kLowerRight_Corner).x());
    roundRect->set_lower_right_y(rrect.radii(SkRRect::kLowerRight_Corner).y());
    roundRect->set_lower_left_x (rrect.radii(SkRRect::kLowerLeft_Corner ).x());
    roundRect->set_lower_left_y (rrect.radii(SkRRect::kLowerLeft_Corner ).y());
    return roundRect;
}

// static Point* to_point(const SkPoint& point) {
//     Point* p = new Point;
//     p->set_x(point.x());
//     p->set_y(point.y()) ;
//     return p;
// }

SkRemote::ID ProtoEncoder::define(const SkPath& path) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kPath));
    Path* protoPath = new Path;
    protoPath->set_fill_type(to_fill_type(path.getFillType()));
    SkRect rect;
    SkRRect rRect;
    bool isClosed;
    SkPath::Direction direction = (SkPath::Direction)0xDEADBEEF;
    if (path.isRect(&rect, &isClosed, &direction) && isClosed) {
        Path::RectPath* rectPath = new Path::RectPath;
        rectPath->set_direction(to_direction(direction));
        rectPath->set_allocated_rect(to_rect(rect));
        protoPath->set_allocated_rect_path(rectPath);
    } else if (path.isOval(&rect)) {
        Path::OvalPath* ovalPath = new Path::OvalPath;
        ovalPath->set_direction(Path::DIRECTION_CW);
        ovalPath->set_allocated_rect(to_rect(rect));
        protoPath->set_allocated_oval_path(ovalPath);
    } else if (path.isRRect(&rRect)) {
        Path::RoundRectPath* roundRectPath = new Path::RoundRectPath;
        roundRectPath->set_allocated_round_rect(to_round_rect(rRect));
        roundRectPath->set_direction(Path::DIRECTION_CW);
        protoPath->set_allocated_round_rect_path(roundRectPath);
    } else {
        Path::GenericPath* genericPath = new Path::GenericPath;
        SkPoint pts[4];
        SkPath::Iter pathIter(path, false);
        for (SkPath::Verb verb(pathIter.next(pts, false));
             verb != SkPath::kDone_Verb; verb = pathIter.next(pts, false)) {
            switch (verb) {
                case SkPath::kMove_Verb: {
                    Path::Move* move = new Path::Move;
                    move->set_end_x(pts[0].x());
                    move->set_end_y(pts[0].y());
                    genericPath->add_action()->set_allocated_move(move);
                    break;
                }
                case SkPath::kLine_Verb: {
                    Path::Line* line = new Path::Line;
                    line->set_end_x(pts[1].x());
                    line->set_end_y(pts[1].y());
                    genericPath->add_action()->set_allocated_line(line);
                    break;
                }
                case SkPath::kQuad_Verb: {
                    Path::Quad* quad = new Path::Quad;
                    quad->set_control_x(pts[1].x());
                    quad->set_control_y(pts[1].y());
                    quad->set_end_x(pts[2].x());
                    quad->set_end_y(pts[2].y());
                    genericPath->add_action()->set_allocated_quad(quad);
                    break;
                }
                case SkPath::kConic_Verb: {
                    Path::Conic* conic = new Path::Conic;
                    conic->set_control_x(pts[1].x());
                    conic->set_control_y(pts[1].y());
                    conic->set_end_x(pts[2].x());
                    conic->set_end_y(pts[2].y());
                    conic->set_weight(pathIter.conicWeight());
                    genericPath->add_action()->set_allocated_conic(conic);
                    break;
                }
                case SkPath::kCubic_Verb: {
                    Path::Cubic* cubic = new Path::Cubic;
                    cubic->set_control_1_x(pts[1].x());
                    cubic->set_control_1_y(pts[1].y());
                    cubic->set_control_2_x(pts[2].x());
                    cubic->set_control_2_y(pts[2].y());
                    cubic->set_end_x(pts[3].x());
                    cubic->set_end_y(pts[3].y());
                    genericPath->add_action()->set_allocated_cubic(cubic);
                    break;
                }
                case SkPath::kClose_Verb: {
                    genericPath->add_action()->set_allocated_close(new Path::Close);
                    break;
                }
                case SkPath::kDone_Verb:
                    break;
            }
        }
        protoPath->set_allocated_generic_path(genericPath);
    }
    RemoteCommands::DefinePath* definePath = new RemoteCommands::DefinePath;
    definePath->set_allocated_path(protoPath);
    definePath->set_id(id.val());
    fCommands.add_remote_commands()->set_allocated_define_path(definePath);
    return id;
}
SkRemote::ID ProtoEncoder::define(const SkRemote::Stroke&) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kStroke));
    return id;
}
SkRemote::ID ProtoEncoder::define(const SkTextBlob*) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kTextBlob));
    return id;
}

// TODO(halcanary): stop using flattenable.  dyn_cast?
template <class P, class S>
static P* flatten(const S* flattenable) {
    if (!flattenable) { return nullptr; }
    const char* name = flattenable->getTypeName();
    if (!name) { return nullptr; }
    SkAutoTUnref<SkPixelSerializer> serializer(
            SkImageEncoder::CreatePixelSerializer());
    SkWriteBuffer writer;
    writer.setPixelSerializer(serializer);
    ((const SkFlattenable*)flattenable)->flatten(writer);
    P* p = new P;
    p->set_flattenedobject(writer.getWriter32()->contiguousArray(),
                           SkToS32(writer.bytesWritten()));
    p->set_factoryname(name);
    return p;
}

template <class D, class P, class S>
static D* definer(S* obj, SkRemote::ID id) {
    D* d = new D;
    d->set_id(id.val());
    d->set_allocated_obj(flatten<P>(obj));
    return d;
}

SkRemote::ID ProtoEncoder::define(SkPathEffect* obj) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kPathEffect));
    fCommands.add_remote_commands()->set_allocated_define_path_effect(
            definer<RemoteCommands::DefinePathEffect, PathEffect>(obj, id));
    return id;
}

SkRemote::ID ProtoEncoder::define(SkShader* obj) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kShader));
    fCommands.add_remote_commands()->set_allocated_define_shader(
            definer<RemoteCommands::DefineShader, Shader>(obj, id));
    return id;
}

static void addmode(Xfermode* x, SkXfermode::Mode mode) {
    switch (mode) {
        case SkXfermode::kClear_Mode:
            x->set_allocated_clear_xfermode     (new Xfermode::ClearMode);
            return;
        case SkXfermode::kSrc_Mode:
            x->set_allocated_src_xfermode       (new Xfermode::Src);
            return;
        case SkXfermode::kDst_Mode:
            x->set_allocated_dst_xfermode       (new Xfermode::Dst);
            return;
        case SkXfermode::kSrcOver_Mode:
            x->set_allocated_srcover_xfermode   (new Xfermode::SrcOver);
            return;
        case SkXfermode::kDstOver_Mode:
            x->set_allocated_dstover_xfermode   (new Xfermode::DstOver);
            return;
        case SkXfermode::kSrcIn_Mode:
            x->set_allocated_srcin_xfermode     (new Xfermode::SrcIn);
            return;
        case SkXfermode::kDstIn_Mode:
            x->set_allocated_dstin_xfermode     (new Xfermode::DstIn);
            return;
        case SkXfermode::kSrcOut_Mode:
            x->set_allocated_srcout_xfermode    (new Xfermode::SrcOut);
            return;
        case SkXfermode::kDstOut_Mode:
            x->set_allocated_dstout_xfermode    (new Xfermode::DstOut);
            return;
        case SkXfermode::kSrcATop_Mode:
            x->set_allocated_srcatop_xfermode   (new Xfermode::SrcATop);
            return;
        case SkXfermode::kDstATop_Mode:
            x->set_allocated_dstatop_xfermode   (new Xfermode::DstATop);
            return;
        case SkXfermode::kXor_Mode:
            x->set_allocated_xor_xfermode       (new Xfermode::Xor);
            return;
        case SkXfermode::kPlus_Mode:
            x->set_allocated_plus_xfermode      (new Xfermode::Plus);
            return;
        case SkXfermode::kModulate_Mode:
            x->set_allocated_modulate_xfermode  (new Xfermode::Modulate);
            return;
        case SkXfermode::kScreen_Mode:
            x->set_allocated_screen_xfermode    (new Xfermode::Screen);
            return;
        case SkXfermode::kOverlay_Mode:
            x->set_allocated_overlay_xfermode   (new Xfermode::Overlay);
            return;
        case SkXfermode::kDarken_Mode:
            x->set_allocated_darken_xfermode    (new Xfermode::Darken);
            return;
        case SkXfermode::kLighten_Mode:
            x->set_allocated_lighten_xfermode   (new Xfermode::Lighten);
            return;
        case SkXfermode::kColorDodge_Mode:
            x->set_allocated_colordodge_xfermode(new Xfermode::ColorDodge);
            return;
        case SkXfermode::kColorBurn_Mode:
            x->set_allocated_colorburn_xfermode (new Xfermode::ColorBurn);
            return;
        case SkXfermode::kHardLight_Mode:
            x->set_allocated_hardlight_xfermode (new Xfermode::HardLight);
            return;
        case SkXfermode::kSoftLight_Mode:
            x->set_allocated_softlight_xfermode (new Xfermode::SoftLight);
            return;
        case SkXfermode::kDifference_Mode:
            x->set_allocated_difference_xfermode(new Xfermode::Difference);
            return;
        case SkXfermode::kExclusion_Mode:
            x->set_allocated_exclusion_xfermode (new Xfermode::Exclusion);
            return;
        case SkXfermode::kMultiply_Mode:
            x->set_allocated_multiply_xfermode  (new Xfermode::Multiply);
            return;
        case SkXfermode::kHue_Mode:
            x->set_allocated_hue_xfermode       (new Xfermode::Hue);
            return;
        case SkXfermode::kSaturation_Mode:
            x->set_allocated_saturation_xfermode(new Xfermode::Saturation);
            return;
        case SkXfermode::kColor_Mode:
            x->set_allocated_color_xfermode     (new Xfermode::Color);
            return;
        case SkXfermode::kLuminosity_Mode:
            x->set_allocated_luminosity_xfermode(new Xfermode::Luminosity);
            return;
    }
}

SkRemote::ID ProtoEncoder::define(SkXfermode* xfermode) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kXfermode));
    SkXfermode::Mode mode;
    if (SkXfermode::AsMode(xfermode, &mode)) {
        Xfermode* x = new Xfermode;
        addmode(x, mode);
        RemoteCommands::DefineXfermode* d = new RemoteCommands::DefineXfermode;
        d->set_allocated_obj(x);
        d->set_id(id.val());
        fCommands.add_remote_commands()->set_allocated_define_xfermode(d);
        return id;
    } else {
        // FIXME
        return id;
    }
}
SkRemote::ID ProtoEncoder::define(SkMaskFilter* obj) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kMaskFilter));
    fCommands.add_remote_commands()->set_allocated_define_mask_filter(
            definer<RemoteCommands::DefineMaskFilter, MaskFilter>(obj, id));
    return id;
}
SkRemote::ID ProtoEncoder::define(SkColorFilter* obj) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kColorFilter));
    fCommands.add_remote_commands()->set_allocated_define_color_filter(
            definer<RemoteCommands::DefineColorFilter, ColorFilter>(obj, id));
    return id;
}
SkRemote::ID ProtoEncoder::define(SkRasterizer* obj) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kRasterizer));
    fCommands.add_remote_commands()->set_allocated_define_rasterizer(
            definer<RemoteCommands::DefineRasterizer, Rasterizer>(obj, id));
    return id;
}
SkRemote::ID ProtoEncoder::define(SkDrawLooper* obj) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kDrawLooper));
    fCommands.add_remote_commands()->set_allocated_define_draw_looper(
            definer<RemoteCommands::DefineDrawLooper, DrawLooper>(obj, id));
    return id;
}
SkRemote::ID ProtoEncoder::define(SkImageFilter* obj) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kImageFilter));
    fCommands.add_remote_commands()->set_allocated_define_image_filter(
            definer<RemoteCommands::DefineImageFilter, ImageFilter>(obj, id));
    return id;
}

static Annotation* to_annotation(SkAnnotation* annotation) {
    if (!annotation) {
        return nullptr;
    }
    SkData* data = nullptr;
    Annotation::Key key;
    if ((data = annotation->find(SkAnnotationKeys::URL_Key()))) {
        key = Annotation::URL;
    } else if ((data = annotation->find(
                        SkAnnotationKeys::Define_Named_Dest_Key()))) {
        key = Annotation::DEFINE_NAMED_DEST;
    } else if ((data = annotation->find(
                        SkAnnotationKeys::Link_Named_Dest_Key()))) {
        key = Annotation::LINK_NAMED_DEST;
    } else {
        return nullptr;
    }
    SkASSERT(data);
    Annotation* a = new Annotation;
    a->set_key(key);
    a->set_value(data->data(), data->size());
    return a;
}

SkRemote::ID ProtoEncoder::define(SkAnnotation* annot) {
    SkRemote::ID id(fIDs.next(SkRemote::Type::kAnnotation));
    RemoteCommands::DefineAnnotation* d = new RemoteCommands::DefineAnnotation;
    d->set_allocated_annotation(to_annotation(annot));
    d->set_id(id.val());
    fCommands.add_remote_commands()->set_allocated_define_annotation(d);
    return id;
}
static RemoteCommands::IDType to_type(SkRemote::Type type) {
    switch (type) {
        case SkRemote::Type::kMatrix:      return RemoteCommands::ID_TYPE_MATRIX;
        case SkRemote::Type::kMisc:        return RemoteCommands::ID_TYPE_MISC;
        case SkRemote::Type::kPath:        return RemoteCommands::ID_TYPE_PATH;
        case SkRemote::Type::kStroke:      return RemoteCommands::ID_TYPE_STROKE;
        case SkRemote::Type::kTextBlob:    return RemoteCommands::ID_TYPE_TEXT_BLOB;
        case SkRemote::Type::kPathEffect:  return RemoteCommands::ID_TYPE_PATH_EFFECT;
        case SkRemote::Type::kShader:      return RemoteCommands::ID_TYPE_SHADER;
        case SkRemote::Type::kXfermode:    return RemoteCommands::ID_TYPE_XFERMODE;
        case SkRemote::Type::kMaskFilter:  return RemoteCommands::ID_TYPE_MASK_FILTER;
        case SkRemote::Type::kColorFilter: return RemoteCommands::ID_TYPE_COLOR_FILTER;
        case SkRemote::Type::kRasterizer:  return RemoteCommands::ID_TYPE_RASTERIZER;
        case SkRemote::Type::kDrawLooper:  return RemoteCommands::ID_TYPE_DRAW_LOOPER;
        case SkRemote::Type::kImageFilter: return RemoteCommands::ID_TYPE_IMAGE_FILTER;
        case SkRemote::Type::kAnnotation:  return RemoteCommands::ID_TYPE_ANNOTATION;
    }
    return RemoteCommands::ID_TYPE_MATRIX;
}

void ProtoEncoder::undefine(SkRemote::ID id) {
    RemoteCommands::Undefine* undefine =  new RemoteCommands::Undefine;
    undefine->set_type(to_type(id.type()));
    undefine->set_id(id.val());
    fCommands.add_remote_commands()->set_allocated_undefine(undefine);
}
void ProtoEncoder::saveLayer(SkRemote::ID bounds,
                             SkRemote::Encoder::CommonIDs,
                             uint32_t flags)  {}
void ProtoEncoder::save()  {
    fCommands.add_remote_commands()->set_allocated_save(
            new RemoteCommands::Save);
}
void ProtoEncoder::restore()  {
    fCommands.add_remote_commands()->set_allocated_restore(
            new RemoteCommands::Restore);
}
void ProtoEncoder::setMatrix(SkRemote::ID matrix)  {
    // FIXME
}
void ProtoEncoder::clipPath(SkRemote::ID path, SkRegion::Op, bool aa)  {
    // FIXME
}
void ProtoEncoder::fillPath(SkRemote::ID path, SkRemote::Encoder::CommonIDs)  {
    // FIXME
}
void ProtoEncoder::strokePath(SkRemote::ID path,
                              SkRemote::Encoder::CommonIDs,
                              SkRemote::ID stroke)  {
    // FIXME
}
void ProtoEncoder::  fillText(SkRemote::ID text,
                              SkPoint,
                              SkRemote::Encoder::CommonIDs)  {
    // FIXME
}
void ProtoEncoder::strokeText(SkRemote::ID text,
                              SkPoint,
                              SkRemote::Encoder::CommonIDs,
                              SkRemote::ID stroke)  {
    // FIXME
}

}  // namespace
////////////////////////////////////////////////////////////////////////////////
SkRemote::Encoder* ProtocolRemoteEncoder::CreateEncoder(SkWStream* s) {
    return new ProtoEncoder(s);
}
bool ProtocolRemoteEncoder::Decode(SkRemote::Encoder* dst, SkStream* src) {
    return false;
}
