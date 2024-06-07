#include "apply.h"
#include "payload.h"
#include <bzlib.h>
#include <bsdiff/bspatch.h>
#include <puffin/puffpatch.h>
#include <puffin/brotli_util.h>
#include <puffin/memory_stream.h>
#include <zucchini/zucchini.h>
#include <cstring>
#include <utility>
#include <lzma.h>

extern "C" {
#include "sha256.h"
}

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
    if (offset > size_) {
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



int64_t ExecuteSourcePuffDiffOperation(void *data, size_t data_size,
                                              void *patch, size_t patch_size,
                                              void *output, size_t output_size) {
    constexpr size_t kMaxCacheSize = 5 * 1024 * 1024;  // Total 5MB cache.

    puffin::UniqueStreamPtr src(new PuffinDataStream(data, data_size, true));
    puffin::UniqueStreamPtr dst(new PuffinDataStream(output, output_size, false));

    return puffin::PuffPatch(std::move(src), std::move(dst), (const uint8_t *) patch, patch_size, kMaxCacheSize) ? output_size : -1;
}

int64_t ExecuteSourceBsdiffOperation(void *data, size_t data_size,
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

int64_t ExecuteSourceZucchiniOperation(void *data, size_t data_size,
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

void sha256_bytes(char *data, unsigned int data_size, BYTE *hash) {
  SHA256_CTX ctx;
  sha256_init(&ctx);
  sha256_update(&ctx, (const BYTE*)data, data_size);
  sha256_final(&ctx, hash);
}

void apply_partition(
    payload *update,
    const chromeos_update_engine::PartitionUpdate *p,
    int start_at,
    int end_at,
    FILE *data_file,
    int in_file,
    int out_file
  ) {
  while (start_at < end_at) {
    chromeos_update_engine::InstallOperation op = p->operations(start_at++);

    fseek(data_file, update->data_offset + op.data_offset(), SEEK_SET);

    BYTE hash[32] = {0};
    unsigned int output_size, src_size;
    char *output;
    char *src = get_src(in_file, &src_size, &op);
    char *data = read_alloc(data_file, op.data_length());
    uint64_t mem_limit = 1410065407;
    size_t next = 0;
    size_t out = 0;

    if (op.has_src_sha256_hash()) {
      sha256_bytes(src, src_size, hash);
      if (memcmp(hash, op.src_sha256_hash().data(), 32) != 0) {
        std::cerr << "Source Hash Mismatch\n";
      }
    }
    if (op.has_data_sha256_hash()) {
      sha256_bytes(data, op.data_length(), hash);
      if (memcmp(hash, op.data_sha256_hash().data(), 32) != 0) {
        std::cerr << "Data Hash Mismatch\n";
      }
    }

    if (op.type() != chromeos_update_engine::InstallOperation_Type_REPLACE && op.type() != chromeos_update_engine::InstallOperation_Type_SOURCE_COPY) {
      output = output_buffer(&op, &output_size);
    }

    switch (op.type()) {
      case chromeos_update_engine::InstallOperation_Type_REPLACE:
        output = data;
        break;
      case chromeos_update_engine::InstallOperation_Type_REPLACE_BZ:
        BZ2_bzBuffToBuffDecompress(output, &output_size, data, op.data_length(), 0, 0);
        break;
      case chromeos_update_engine::InstallOperation_Type_REPLACE_XZ:
        lzma_stream_buffer_decode(&mem_limit, 0, NULL, (const uint8_t*)data, &next, op.data_length(),
            (uint8_t*)output, &out, output_size);
        break;
      case chromeos_update_engine::InstallOperation_Type_ZERO:
        memset(output, 0, output_size);
        break;
      case chromeos_update_engine::InstallOperation_Type_BROTLI_BSDIFF:
        ExecuteSourceBsdiffOperation(src, src_size, data, op.data_length(), output, output_size);
        break;
      case chromeos_update_engine::InstallOperation_Type_PUFFDIFF:
        ExecuteSourcePuffDiffOperation(src, src_size, data, op.data_length(), output, output_size);
        break;
      case chromeos_update_engine::InstallOperation_Type_ZUCCHINI:
        ExecuteSourceZucchiniOperation(src, src_size, data, op.data_length(), output, output_size);
        break;
      case chromeos_update_engine::InstallOperation_Type_SOURCE_COPY:
        output = src;
        break;
      default:
        std::cout << "UNKNOWN\n";
        return;
    }

    write_out(out_file, &op, output);

    if (data != 0) {
      free(data);
    }
    if (data != output) {
      free(output);
    }
    if (src != 0 && output != src) {
      free(src);
    }
  }
}


