#include <iostream>
#include "SkBitmapDevice.h"
#include "SkCanvas.h"
#include "SkDevice.h"
#include "SkGraphics.h"
#include "SkImageEncoder.h"
#include "SkOSFile.h"
#include "SkPdfConfig.h"
#include "SkPdfRenderer.h"
#include "SkPicture.h"
#include "SkStream.h"
#include "SkTypeface.h"
#include "SkTArray.h"
#include "SkNulCanvas.h"
#include <fstream>

namespace
{
	// Most pointers returned by Skia are derived from SkRefCnt,
	// meaning we need to call ->unref() on them when done rather than delete them.
	template <typename T> std::shared_ptr<T> adopt(T* ptr) {
		return std::shared_ptr<T>(ptr, [](T* p) { p->unref(); });
	}

	class StdOutWStream : public SkWStream {
	public:
		StdOutWStream() : fBytesWritten(0) {}
		bool write(const void* buffer, size_t size) final {
			fBytesWritten += size;
			return size == fwrite(buffer, 1, size, stdout);
		}
		size_t bytesWritten() const final { return fBytesWritten; }

	private:
		size_t fBytesWritten;
	};

	SkStreamAsset* open_for_reading(const char* path) {
		if (!path || !path[0] || 0 == strcmp(path, "-")) {
			return new SkFILEStream(stdin, SkFILEStream::kCallerRetains_Ownership);
		}
		return SkStream::NewFromFile(path);
	}

	SkWStream* open_for_writing(const char* path) {
		if (!path || !path[0] || 0 == strcmp(path, "-")) {
			return new StdOutWStream;
		}
		return new SkFILEWStream(path);
	}
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		std::cout << "pdf_to_bmp <pdf file> <bmp file>\n";
		return 0;
	}

#if 0
	SkAutoTDelete<SkPdfRenderer> pdf( SkPdfRenderer::CreateFromFile(argv[1]) );
	if (pdf.get() == nullptr)
	{
		std::cout << "Failure loading file " << argv[1] << "\n";
		return -1;
	}

	if (!pdf->pages())
	{
		std::cout << "WARNING: Empty PDF Document " << argv[1] << "\n";
		SkASSERT(false);
		return 0;
	}
#endif

	SkBitmap bm;
	SkAutoTDelete<SkStream> in(open_for_reading(argv[1]));
	if (SkPDFNativeRenderToBitmap(in, &bm, 0))
	{
		const auto img  = SkImage::MakeFromBitmap(bm);
		const auto png = adopt(img->encode(SkImageEncoder::kPNG_Type, 100));
		
		std::ofstream(argv[2], std::ios::out | std::ios::binary)
			.write(static_cast<const char*>(png->data()), png->size());

		std::cout << "Wrote " << argv[2] << std::endl;
	}

	return 0;
}
