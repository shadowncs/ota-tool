#include "payload.h"
#include "util.h"

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

  if (*size == 0) {
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
