#include "apply.h"
#include "payload.h"
#include "util.h"
#include "algos.h"
#include <bzlib.h>
#include <bsdiff/bspatch.h>
#include <cstring>
#include <utility>
#include <lzma.h>
#include <openssl/sha.h>

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

    unsigned char hash[32] = {0};
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
        log_err(p->partition_name().data(), "Source Hash Mismatch. The source image is corrupted or you are running the wrong patch version against it");
        end_at = 0;
        goto end;
      }
    }
    if (op.has_data_sha256_hash()) {
      sha256_bytes(data, op.data_length(), hash);
      if (memcmp(hash, op.data_sha256_hash().data(), 32) != 0) {
        log_err(p->partition_name().data(), "Data Hash Mismatch. The update file is corrupted");
        end_at = 0;
        goto end;
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
        log_err(p->partition_name().data(), "Unknown Operation Type");
        return;
    }

    write_out(out_file, &op, output);

    if (output != src && output != data) {
      free(output);
    }
end:
    if (data != 0) {
      free(data);
    }
    if (src != 0) {
      free(src);
    }
  }
}


