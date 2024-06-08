#include "util.h"
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <linux/limits.h>
#include <ostream>
#include <iostream>

int open_img_file(char* dir, const char* file, int flags) {
  char path[PATH_MAX];
  strcpy(path, dir);
  strcat(path, "/");
  strcat(path, file);
  strcat(path, ".img");

  return open(path, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
}

char* read_alloc(FILE *f, int size) {
  if (size == 0) {
    return (char*)0;
  }

  void* data = malloc(size);
  fread(data, size, 1, f);

  return (char*)data;
}

void log_err(const char *section, const char *msg) {
  std::cerr << "ERROR: " << section << ": " << msg << std::endl;
}
