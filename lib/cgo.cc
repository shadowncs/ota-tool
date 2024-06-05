#include "cgo.h"
#include <bzlib.h>
#include <bsdiff/bspatch.h>
#include <puffin/puffpatch.h>
#include <puffin/brotli_util.h>
#include <puffin/memory_stream.h>
#include <zucchini/zucchini.h>
#include <cstring>
#include <utility>

extern "C" int64_t ExecuteSourcePuffDiffOperation(void *data, size_t data_size,
    void *patch, size_t patch_size,
    void *output, size_t output_size) {
  constexpr size_t kMaxCacheSize = 5 * 1024 * 1024;  // Total 5MB cache.

  puffin::Buffer src_buf, dst_buf;
  src_buf.assign((uint8_t*)data, (uint8_t*)data + data_size);
  dst_buf.assign((uint8_t*)output, (uint8_t*)output + data_size);

  return puffin::PuffPatch(
    puffin::MemoryStream::CreateForRead(src_buf),
    puffin::MemoryStream::CreateForWrite(&dst_buf),
    (const uint8_t *) patch, patch_size,
    kMaxCacheSize
  ) ? output_size : -1;
}


extern "C" int64_t ExecuteSourceBsdiffOperation(void *data, size_t data_size,
                                            void *patch, size_t patch_size,
                                            void *output, size_t output_size) {

    size_t written = 0;

    auto sink = [output, &written, output_size](const uint8_t *data, size_t count) -> size_t {
        char *point = (char *)output + written;
        written += count;
        if (written > output_size) {
            return 0;
        }

        std::memcpy(point, data, count);
        return count;
    };

    int result = bsdiff::bspatch((const uint8_t *) data, data_size, (const uint8_t *) patch, patch_size, sink);

    return result == 0 ? written : -result;

}

extern "C" int64_t ExecuteSourceZucchiniOperation(void *data, size_t data_size,
                                            void *patch, size_t patch_size,
                                            void *output, size_t output_size) {

	zucchini::ConstBufferView old_image((const uint8_t *) data, data_size);
	zucchini::MutableBufferView new_image((uint8_t *) output, output_size);

	std::vector<uint8_t> zucchini_patch;
  puffin::BrotliDecode((const uint8_t *)patch, patch_size, &zucchini_patch);
	zucchini::BufferSource patch_file((const uint8_t *)zucchini_patch.data(), zucchini_patch.size());

	auto patch_reader = zucchini::EnsemblePatchReader::Create(patch_file);
	if (!patch_reader.has_value()) {
		return -1;
	}

	return zucchini::ApplyBuffer(old_image, *patch_reader, new_image);
}

extern "C" int Bzip2Decompress(void *data, size_t data_size, void *output, uint32_t output_size) {
  return BZ2_bzBuffToBuffDecompress((char*)output, &output_size, (char*)data, data_size, 0, 0);
}
