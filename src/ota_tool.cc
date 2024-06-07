#include "ota_tool.h"
#include "apply.h"
#include <argp.h>


const char *argp_program_version = "ota-tool 2.0-dev";
const char *argp_program_bug_address = "<emily@redcoat.dev>";

INIT_FUNC(usage) {
  std::cout << "Usage: ota-tool [subcommand] [flags]\n";

  return 0;
}

INIT_FUNC(list) {
  if (argc != 3) {
    usage(argc, argv);
    return 1;
  }

  FILE *f = fopen(argv[2], "rb");

  payload update;
  init_payload(&update, f);

  for (int j = 0; j < update.manifest.partitions_size(); j++) {
    std::cout << update.manifest.partitions(j).partition_name() << std::endl;
  }

  fclose(f);

  return 0;
}

static struct argp_option options[] = {
  {"output",   'o', "DIR", 0, "Directory to place updated images into"},
  {"input",    'i', "DIR", 0, "Directory where existing images are"},
  {"partitions", 'p', "", 0, "List of partitions to extract"},
  { 0 }
};

struct arguments
{
  char *output;
  char *input;
  char *partitions;
  char *update_file;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{

  struct arguments *arguments = (struct arguments*)state->input;

  switch (key)
    {
    case 'o':
      arguments->output = arg;
      break;
    case 'i':
      arguments->input = arg;
      break;
    case 'p':
      arguments->partitions = arg;
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num != 0)
        /* Too many arguments. */
        argp_usage (state);

      arguments->update_file = arg;

      break;

    case ARGP_KEY_END:
      if (state->arg_num < 1)
        /* Not enough arguments. */
        argp_usage (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  "UPDATE_FILE",
  "Applies the upload to existing partition images"
};

FILE* open_img_file(char* dir, const char* file, char* mode) {
  char path[PATH_MAX];
  strcpy(path, dir);
  strcat(path, "/");
  strcat(path, file);
  strcat(path, ".img");

  return fopen(path, mode);
}

void launch_apply(
  payload *update,
  struct arguments *args,
  char *partition
  ) {

  FILE *f = fopen(args->update_file, "rb");

  for (int j = 0; j < update->manifest.partitions_size(); j++) {
    const char *partition_name = update->manifest.partitions(j).partition_name().data();

    if (partition == NULL || strcmp(partition_name, partition) == 0) {
      std::cout << partition_name << std::endl;

      FILE *in = open_img_file(args->input, partition_name, "rb");
      if (in == NULL) {
        continue;
      }

      FILE *out = open_img_file(args->output, partition_name, "wcb");

      apply_partition(update, &update->manifest.partitions(j), f, in, out);

      fclose(out);
      fclose(in);

      if (partition != NULL) {
        goto end;
      }
    }
  }

end:
  fclose(f);
}

INIT_FUNC(apply) {
  struct arguments arguments = { NULL, NULL, NULL, NULL };

  argp_parse (&argp, argc - 1, &(argv[1]), 0, 0, &arguments);

  FILE *f = fopen(arguments.update_file, "rb");
  payload update;
  init_payload(&update, f);
  fclose(f);

  if (arguments.partitions == NULL) {
    launch_apply(&update, &arguments, NULL);
  } else {
    char *partition = strtok(arguments.partitions, ",");
    do {
      launch_apply(&update, &arguments, partition);
      partition = strtok(NULL, ",");
    } while(partition != NULL);
  }

  return 0;
}

INIT_FUNC(main) {
  if (argc < 2) {
    usage(argc, argv);
    return 1;
  }

  if (strcmp(argv[1], "list") == 0) {
    return list(argc, argv);
  } else if (strcmp(argv[1], "apply") == 0) {
    return apply(argc, argv);
  } else if (strcmp(argv[1], "help") == 0) {
    usage(argc, argv);
    return 0;
  }

  return 2;
}
