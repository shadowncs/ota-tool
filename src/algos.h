
int64_t ExecuteSourcePuffDiffOperation(void *data, size_t data_size,
    void *patch, size_t patch_size,
    void *output, size_t output_size);

int64_t ExecuteSourceBsdiffOperation(void *data, size_t data_size,
    void *patch, size_t patch_size,
    void *output, size_t output_size);

int64_t ExecuteSourceZucchiniOperation(void *data, size_t data_size,
    void *patch, size_t patch_size,
    void *output, size_t output_size);

void sha256_bytes(char *data, unsigned int data_size, unsigned char *hash);
