#ifndef _H_PAYLOAD
#define _H_PAYLOAD 1

#include "update_metadata.pb.h"

#define PB_LOOP(from, field, type) for (int i = 0; i < from->field##s_size(); i++) { \
  chromeos_update_engine::type field = from->field##s(i);

#define BLOCK_SIZE 4096

#define b2o(block) block * BLOCK_SIZE

/* The header at the top of Android / ChromeOS Payloads */
typedef struct __attribute__((__packed__)) {
  char magic[4];
  uint32_t __pad1;
  uint32_t major_version;
  uint32_t __pad2;
  uint32_t manifest_size;
  uint32_t signature_len;
} payload_header;

/* We have a payload object which holds our payload header, as well as
 * the parsed DeltaArchiveManifest.
 */
typedef struct {
  payload_header header;
  int data_offset;
  chromeos_update_engine::DeltaArchiveManifest manifest;
} payload;

void init_payload(payload *update, FILE *f);

void write_out(FILE *out,
    const chromeos_update_engine::InstallOperation *op,
    char* data
  );

char* output_buffer(const chromeos_update_engine::InstallOperation *op, unsigned int *size);

char* get_src(FILE *in, unsigned int *size,
  const chromeos_update_engine::InstallOperation *op);
 
char* read(FILE *f, int size);

#endif /* ifndef _H_PAYLOAD */
