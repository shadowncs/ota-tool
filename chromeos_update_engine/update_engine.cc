#include "update_engine.h"
#include <bsdiff/bspatch.h>
#include <puffin/puffpatch.h>
#include <puffin/brotli_util.h>
#include <zucchini/zucchini.h>
#include <cstring>
#include <utility>

class PuffinDataStream : public puffin::StreamInterface {
 public:
   PuffinDataStream(void *data, uint64_t size, bool is_read)
      : data_(data),
        size_(size),
        offset_(0),
        is_read_(is_read) {}

  ~PuffinDataStream() override = default;

  bool GetSize(uint64_t* size) const override {
    *size = size_;
    return true;
  }

  bool GetOffset(uint64_t* offset) const override {
    *offset = offset_;
    return true;
  }

  bool Seek(uint64_t offset) override {
    if (offset <= size_) {
      return false;
    }

    offset_ = offset;
    return true;
  }

  bool Read(void* buffer, size_t count) override {
    if (offset_ + count > size_) return false;

    void *point = data_ + offset_;
    std::memcpy(buffer, point, count);
    offset_ += count;
    return true;
  }

  bool Write(const void* buffer, size_t count) override {
    if (offset_ + count > size_) return false;

    void *point = data_ + offset_;
    std::memcpy(point, buffer, count);
    offset_ += count;
    return true;
  }

  bool Close() override { return true; }

 private:

  void *data_;
  uint64_t size_;
  uint64_t offset_;
  bool is_read_;

  DISALLOW_COPY_AND_ASSIGN(PuffinDataStream);
};

extern "C" int64_t ExecuteSourcePuffDiffOperation(void *data, size_t data_size,
                                              void *patch, size_t patch_size,
                                              void *output, size_t output_size) {
    constexpr size_t kMaxCacheSize = 5 * 1024 * 1024;  // Total 5MB cache.

    puffin::UniqueStreamPtr src(new PuffinDataStream(data, data_size, true));
    puffin::UniqueStreamPtr dst(new PuffinDataStream(output, output_size, false));

    return puffin::PuffPatch(std::move(src), std::move(dst), (const uint8_t *) patch, patch_size, kMaxCacheSize) ? -1 : output_size;
}


extern "C" int64_t ExecuteSourceBsdiffOperation(void *data, size_t data_size,
                                            void *patch, size_t patch_size,
                                            void *output, size_t output_size) {

    size_t written = 0;

    auto sink = [output, &written, output_size](const uint8_t *data, size_t count) -> size_t {
        written += count;
        if (written > output_size) {
            return 0;
        }
        std::memcpy(output, data, count);
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
