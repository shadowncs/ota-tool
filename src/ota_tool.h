
#define INIT_FUNC(name) int name(int argc, char *argv[])

struct arguments
{
  char *output;
  char *input;
  char *partitions;
  char *update_file;
  int threads;
};

INIT_FUNC(usage);
INIT_FUNC(list);
INIT_FUNC(apply);
INIT_FUNC(main);
