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

int64_t ExecuteSourcePuffDiffOperation(void *data, size_t data_size,
    void *patch, size_t patch_size,
    void *output, size_t output_size) {
  constexpr size_t kMaxCacheSize = 5 * 1024 * 1024;  // Total 5MB cache.
                                                     //
                                                     //
  puffin::Buffer src_buf, dst_buf;
  src_buf.assign((uint8_t*)data, (uint8_t*)data + data_size);
  dst_buf.assign((uint8_t*)output, (uint8_t*)output + output_size);

  return puffin::PuffPatch(
    puffin::MemoryStream::CreateForRead(src_buf),
    puffin::MemoryStream::CreateForWrite(&dst_buf),
    (const uint8_t *) patch, patch_size,
    kMaxCacheSize
  ) ? output_size : -1;
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
    FILE *data_file,
    FILE *in_file,
    FILE *out_file
  ) {
  for (int i = 0; i < p->operations_size(); i++) {
    chromeos_update_engine::InstallOperation op = p->operations(i);

    fseek(data_file, update->data_offset + op.data_offset(), SEEK_SET);

    BYTE hash[32] = {0};
    unsigned int output_size, src_size;
    char *output;
    char *src = get_src(in_file, &src_size, &op);
    char *data = read(data_file, op.data_length());
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

