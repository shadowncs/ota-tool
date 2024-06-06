#ifndef BSDIFF4_IMPL_H
#define BSDIFF4_IMPL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t ExecuteSourceBsdiffOperation(void *data, size_t data_size,
                                     void *patch, size_t patch_size,
                                     void *output, size_t output_size);

int64_t ExecuteSourcePuffDiffOperation(void *data, size_t data_size,
                                       void *patch, size_t patch_size,
                                       void *output, size_t output_size);


int64_t ExecuteSourceZucchiniOperation(void *data, size_t data_size,
                                       void *patch, size_t patch_size,
                                       void *output, size_t output_size);

int Bzip2Decompress(void *data, size_t data_size, void *output, uint32_t output_size);

void show();

#ifdef __cplusplus
}
#endif

#endif /* BSDIFF4_IMPL_H */
