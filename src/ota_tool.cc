#include "ota_tool.h"
#include "apply.h"

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

INIT_FUNC(apply) {
  if (argc < 3) {
    usage(argc, argv);
    return 1;
  }

  FILE *f = fopen(argv[2], "rb");
  FILE *out = fopen("/tmp/out", "wc");
  FILE *in = fopen("/home/emily/dev/r1/dumps/system.img", "rb");

  payload update;
  init_payload(&update, f);

  for (int j = 0; j < update.manifest.partitions_size(); j++) {
    if (update.manifest.partitions(j).partition_name() == "system") {
      apply_partition(&update, &update.manifest.partitions(j), f, in, out);
      break;
    }
  }

  fclose(f);
  fclose(out);
  fclose(in);

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
