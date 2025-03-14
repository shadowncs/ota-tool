#include "payload.h"
#include "util.h"
#include "algos.h"
#include <bzlib.h>
#include <bsdiff/bspatch.h>
#include <cstring>
#include <utility>
#include <lzma.h>
#include <openssl/sha.h>

FILE* open_from_filename(const char *name, payload *update) {
  FILE *f = fopen(name, "rb");
  if (!f) {
    std::cerr << "Could not open update file for reading: " << name << std::endl;
    exit(1);
  }

  int len = strlen(name);
  if(len > 3 && !strcmp(name + len - 4, ".zip")) {
    init_payload_from_zip(update, f);
  } else {
    init_payload(update, f);
  }

  return f;
}

void init_payload_from_zip(payload *update, FILE *f) {
  zip_header header;

  while (fread(&header, sizeof(zip_header), 1, f)) {
    if (header.magic != ZIP_MAGIC) {
      std::cerr << "Not a valid zip archive" << std::endl;
      exit(1);
    }

    char filename[NAME_MAX];
    fread(filename, header.filename_length, 1, f);

    if (strncmp("payload.bin", filename, header.filename_length) != 0) {
      fseek(f, header.extra_field_length + header.compressed_size, SEEK_CUR);
    } else {
      fseek(f, header.extra_field_length, SEEK_CUR);
      long offset = ftell(f);
      init_payload(update, f);
      update->data_offset += offset;
      return;
    }
  }

  std::cerr << "Could not find payload.bin in the zip archive" << std::endl;
  exit(1);
}

void init_payload(payload *update, FILE *f) {
  fread(&(update->header), sizeof(payload_header), 1, f);

  // Header ints are in network byte (Big Endian) order. If we are
  // compiling for a little endian system (eg x86) we need to swap the
  // bytes round
# if __BYTE_ORDER == __LITTLE_ENDIAN
  update->header.major_version = __bswap_32(update->header.major_version);
  update->header.manifest_size = __bswap_32(update->header.manifest_size);
  update->header.signature_len = __bswap_32(update->header.signature_len);
# endif

  update->data_offset = sizeof(payload_header) + update->header.manifest_size + update->header.signature_len;

  char *raw_manifest = read_alloc(f, update->header.manifest_size);
  
  update->manifest.ParseFromArray(raw_manifest, update->header.manifest_size);

  free(raw_manifest);
}

void write_out(int out,
    const chromeos_update_engine::InstallOperation *op,
    char* data
  ) {
  int data_offset = 0;
  PB_LOOP(op, dst_extent, Extent)
    pwrite(out, &(data[data_offset]), b2o(dst_extent.num_blocks()), b2o(dst_extent.start_block()));
    data_offset += b2o(dst_extent.num_blocks());
  }
}

char* output_buffer(const chromeos_update_engine::InstallOperation *op, unsigned int *size) {
  *size = 0;
  PB_LOOP(op, dst_extent, Extent)
    *size += b2o(dst_extent.num_blocks());
  }

  return (char*)malloc(*size);
}

char* get_src(int in, unsigned int *size,
  const chromeos_update_engine::InstallOperation *op) {
  *size = 0;
  PB_LOOP(op, src_extent, Extent)
    *size += b2o(src_extent.num_blocks());
  }

  if (*size == 0 || in < 0) {
    return (char*)0;
  }

  char *src = (char*)malloc(*size);
  int offset = 0;

  PB_LOOP(op, src_extent, Extent)
    pread(in, &(src[offset]), b2o(src_extent.num_blocks()), b2o(src_extent.start_block()));
    offset += b2o(src_extent.num_blocks());
  }

  return src;
}

int apply_section(payload *update, section *section, FILE *data_file) {
  int ret = 1;
  const char* part_name = update->manifest.partitions(section->part->part_number).partition_name().data();
  while (section->start < section->end) {
    chromeos_update_engine::InstallOperation op = update->manifest.partitions(section->part->part_number).operations(section->start++);

    fseek(data_file, update->data_offset + op.data_offset(), SEEK_SET);

    unsigned char hash[32] = {0};
    unsigned int output_size, src_size;
    char *output;
    char *src = get_src(section->part->in, &src_size, &op);
    char *data = read_alloc(data_file, op.data_length());
    uint64_t mem_limit = 1410065407;
    size_t next = 0;
    size_t out = 0;

    if (src_size && !src) {
      log_err(part_name, "Incremental OTA package, requiring a base image. Either none was given, or it is inaccessible.");
      goto end;
    }

    if (op.has_src_sha256_hash()) {
      sha256_bytes(src, src_size, hash);
      if (memcmp(hash, op.src_sha256_hash().data(), 32) != 0) {
        log_err(part_name, "Source Hash Mismatch. The source image is corrupted or you are running the wrong patch version against it");
        section->end = 0;
        goto end;
      }
    }
    if (op.has_data_sha256_hash()) {
      sha256_bytes(data, op.data_length(), hash);
      if (memcmp(hash, op.data_sha256_hash().data(), 32) != 0) {
        log_err(part_name, "Data Hash Mismatch. The update file is corrupted");
        section->end = 0;
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
        log_err(part_name, "Unknown Operation Type");
        goto end;
    }

    write_out(section->part->out, &op, output);

    if (output != src && output != data) {
      free(output);
    }

    ret = 0;
end:
    if (data != 0) {
      free(data);
    }
    if (src != 0) {
      free(src);
    }
  }
  return ret;
}
