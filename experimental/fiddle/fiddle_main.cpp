/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include <GL/osmesa.h>

#include "fiddle_main.h"

// Globals externed in fiddle_main.h
SkBitmap source;
SkImage* image(nullptr);

const char* FLAGS_source(nullptr);
int32_t FLAGS_width(256);
int32_t FLAGS_height(256);
bool FLAGS_gpu(false);
bool FLAGS_noraster(false);
bool FLAGS_pdf(false);

static void help() {
    const char options[] =
            "\nOptions:\n"
            "    --width WIDTH   Set the canvas width (default is 256).\n"
            "    --height HEIGHT Set the canvas height (default is 256).\n"
            "    --source PATH   Set the source image file.\n"
            "    --noraster      Disable the raster backend.\n"
            "    --gpu           Enable the GPU backend.\n"
            "    --pdf           Enable the PDF backend.\n\n";
    fputs(options, stderr);
    exit(2);
}

static void parse_flags(int, char** argv) {
    ++argv; // skip argv[0]
    while (*argv) {  // argv is always NULL-terminated
        if (0 == strcmp(*argv, "--width")) {
            if (!*++argv) {
                help();
            }
            FLAGS_width = atoi(*argv);
        } else if (0 == strcmp(*argv, "--height")) {
            if (!*++argv) {
                help();
            }
            FLAGS_height = atoi(*argv);
        } else if (0 == strcmp(*argv, "--source")) {
            if (!*++argv) {
                help();
            }
            FLAGS_source = *argv;
        } else if (0 == strcmp(*argv, "--gpu")) {
            FLAGS_gpu = true;
        } else if (0 == strcmp(*argv, "--noraster")) {
            FLAGS_noraster = true;
        } else if (0 == strcmp(*argv, "--pdf")) {
            FLAGS_pdf = true;
        } else {
            help();
        }
        ++argv;
    }
}

static void encode_to_base64(const void* data, size_t size, FILE* out) {
    const uint8_t* input = reinterpret_cast<const uint8_t*>(data);
    const uint8_t* end = &input[size];
    static const char codes[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz0123456789+/";
    while (input != end) {
        uint8_t b = (*input & 0xFC) >> 2;
        fputc(codes[b], out);
        b = (*input & 0x03) << 4;
        ++input;
        if (input == end) {
            fputc(codes[b], out);
            fputs("==", out);
            return;
        }
        b |= (*input & 0xF0) >> 4;
        fputc(codes[b], out);
        b = (*input & 0x0F) << 2;
        ++input;
        if (input == end) {
            fputc(codes[b], out);
            fputc('=', out);
            return;
        }
        b |= (*input & 0xC0) >> 6;
        fputc(codes[b], out);
        b = *input & 0x3F;
        fputc(codes[b], out);
        ++input;
    }
}

static void dump_output(SkData* pngData, const char* name, bool last = true) {
    if (pngData) {
        printf("\t\"%s\": \"", name);
        encode_to_base64(pngData->data(), pngData->size(), stdout);
        fputs(last ? "\"\n" : "\",\n", stdout);
    }
}

static SkData* encode_snapshot(SkSurface* surface) {
    SkAutoTUnref<SkImage> img(surface->newImageSnapshot());
    return img ? img->encode() : nullptr;
}

static OSMesaContext create_osmesa_context() {
    OSMesaContext osMesaContext =
        OSMesaCreateContextExt(OSMESA_BGRA, 0, 0, 0, nullptr);
    if (osMesaContext != nullptr) {
        static uint32_t buffer[16 * 16];
        OSMesaMakeCurrent(osMesaContext, &buffer, GL_UNSIGNED_BYTE, 16, 16);
    }
    return osMesaContext;
}

static GrContext* create_mesa_grcontext() {
    SkAutoTUnref<const GrGLInterface> mesa(GrGLCreateMesaInterface());
    intptr_t backend = reinterpret_cast<intptr_t>(mesa.get());
    return backend ? GrContext::Create(kOpenGL_GrBackend, backend) : nullptr;
}

int main(int argc, char** argv) {
    parse_flags(argc, argv);
    if (FLAGS_source) {
        SkAutoTUnref<SkData> data(SkData::NewFromFileName(FLAGS_source));
        if (!data) {
            perror("Unable to open the source image file.");
        } else {
            image = SkImage::NewFromEncoded(data);
            bool success = SkInstallDiscardablePixelRef(data, &source);
            if (!image || !success) {
                perror("Unable to decode the source image.");
            }
        }
    }
    SkISize size = SkISize::Make(FLAGS_width, FLAGS_height);
    SkAutoTUnref<SkData> rasterData, gpuData, pdfData;
    if (!FLAGS_noraster) {
        SkAutoTUnref<SkSurface> rasterSurface(
                SkSurface::NewRaster(SkImageInfo::MakeN32Premul(size)));
        draw(rasterSurface->getCanvas());
        rasterData.reset(encode_snapshot(rasterSurface));
    }
    if (FLAGS_gpu) {
        OSMesaContext osMesaContext = create_osmesa_context();
        SkAutoTUnref<GrContext> grContext(create_mesa_grcontext());
        if (!grContext) {
            fputs("Unable to get Mesa GrContext.\n", stderr);
        } else {
            SkAutoTUnref<SkSurface> surface(
                    SkSurface::NewRenderTarget(
                            grContext,
                            SkSurface::kNo_Budgeted,
                            SkImageInfo::MakeN32Premul(size)));
            if (!surface) {
                fputs("Unable to get render surface.\n", stderr);
                exit(1);
            }
            draw(surface->getCanvas());
            gpuData.reset(encode_snapshot(surface));
        }
        if (osMesaContext) {
            OSMesaDestroyContext(osMesaContext);
        }
    }
    if (FLAGS_pdf) {
        SkDynamicMemoryWStream pdfStream;
        SkAutoTUnref<SkDocument> document(SkDocument::CreatePDF(&pdfStream));
        draw(document->beginPage(FLAGS_width, FLAGS_height));
        document->close();
        pdfData.reset(pdfStream.copyToData());
    }

    printf("{\n");
    dump_output(rasterData, "Raster", !gpuData && !pdfData);
    dump_output(gpuData, "Gpu", !pdfData);
    dump_output(pdfData, "Pdf");
    printf("}\n");

    SkSafeSetNull(image);
    return 0;
}
