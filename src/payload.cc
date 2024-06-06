#include "payload.h"

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

  char *raw_manifest = read(f, update->header.manifest_size);
  
  update->manifest.ParseFromArray(raw_manifest, update->header.manifest_size);

  free(raw_manifest);
}

char* read(FILE *f, int size) {
  if (size == 0) {
    return (char*)0;
  }

  void* data = malloc(size);
  fread(data, size, 1, f);

  return (char*)data;
}

void write_out(FILE *out,
    const chromeos_update_engine::InstallOperation *op,
    char* data
  ) {
  int data_offset = 0;
  PB_LOOP(op, dst_extent, Extent)
    fseek(out, b2o(dst_extent.start_block()), SEEK_SET);
    fwrite(&(data[data_offset]), b2o(dst_extent.num_blocks()), 1, out);
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

char* get_src(FILE *in, unsigned int *size,
  const chromeos_update_engine::InstallOperation *op) {
  *size = 0;
  PB_LOOP(op, src_extent, Extent)
    *size += b2o(src_extent.num_blocks());
  }

  if (*size == 0) {
    return (char*)0;
  }

  char *src = (char*)malloc(*size);
  int offset = 0;

  PB_LOOP(op, src_extent, Extent)
    fseek(in, b2o(src_extent.start_block()), SEEK_SET);
    fread(&(src[offset]), b2o(src_extent.num_blocks()), 1, in);
    offset += b2o(src_extent.num_blocks());
  }

  return src;
}
