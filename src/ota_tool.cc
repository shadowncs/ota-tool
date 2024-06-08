#include "ota_tool.h"
#include "payload.h"
#include "util.h"
#include <argp.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>


const char *argp_program_version = "ota-tool 2.0-dev";
const char *argp_program_bug_address = "<emily@redcoat.dev>";

struct arguments args = { NULL, NULL, NULL, NULL, get_nprocs_conf() };

payload update;

INIT_FUNC(usage) {
  std::cout << "Usage: ota-tool [subcommand] [flags]\n";

  return 0;
}

INIT_FUNC(list) {
  if (argc != 3) {
    usage(argc, argv);
    return 1;
  }

  FILE *f = open_from_filename(argv[2], &update);

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
  {"threads",   'c', "4", 0, "Number of threads to spawn"},
  { 0 }
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
    case 'c':
      arguments->threads = atoi(arg);
      if (arguments->threads <= 0) {
        std::cerr << "Invalid Number of Threads: " << arg << std::endl;
        argp_usage(state);
      }
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num != 0)
        /* Too many arguments. */
        argp_usage (state);

      arguments->update_file = arg;

      break;

    case ARGP_KEY_END:
      if (!arguments->update_file)
        argp_usage(state);
      if (!arguments->input)
        argp_usage(state);
      if (!arguments->output)
        argp_usage(state);
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

partition *jobs;
int total_operations = 0;
int job_count = 0;

void launch_apply(char *partition) {
  for (int j = 0; j < update.manifest.partitions_size(); j++) {
    chromeos_update_engine::PartitionUpdate part = update.manifest.partitions(j);
    const char *partition_name = part.partition_name().data();

    if (partition == NULL || strcmp(partition_name, partition) == 0) {

      int in = open_img_file(args.input, partition_name, O_RDONLY);
      if (in < 0) {
        if (partition != NULL) {
          log_err(partition, "Could not open the source image for reading");
        }
        continue;
      }
      int out = open_img_file(args.output, partition_name, O_WRONLY | O_CREAT | O_TRUNC);
      if (out < 0) {
        log_err(partition, "Could not open the destination image for writing");
        continue;
      }

      jobs[job_count].part_number = j;
      jobs[job_count].in = in;
      jobs[job_count++].out = out;
      total_operations += part.operations_size();

      if (partition != NULL) {
        return;
      }
    }
  }
}
void* run_apply(void *a) {
  section *queue = (section*)a;

  FILE *f = fopen(args.update_file, "rb");

  while (queue->part != NULL) {
    apply_section(&update, queue, f);

    queue++;
  }

  fclose(f);

  // The calling function immediately forgets about the memory it has
  // allocated to us, so it's the thread's responsibility to clean it up
  // properly.
  free(a);
  return NULL;
}

INIT_FUNC(apply) {

  argp_parse (&argp, argc - 1, &(argv[1]), 0, 0, &args);

  FILE *f = open_from_filename(args.update_file, &update);

  struct stat sb;
  if (stat(args.output, &sb)) {
    // We couldn't stat it - either because it doesn't exist or because
    // of access permissions. Naively try to create it last minute.
    if (mkdir(args.output, S_IRWXU | S_IRWXG)) {
      std::cerr << "Could not ensure output directory exists: " << args.output << std::endl;
      return 1;
    }
  } else if ((sb.st_mode & S_IFMT) != S_IFDIR) {
    std::cerr << "Output is not a directory: " << args.output << std::endl;
    return 1;
  }

  jobs = malloc_t(partition, update.manifest.partitions_size());

  if (args.partitions == NULL) {
    launch_apply(NULL);
  } else {
    char *partition = strtok(args.partitions, ",");
    do {
      launch_apply(partition);
      partition = strtok(NULL, ",");
    } while(partition != NULL);
  }

  int ops_per_thread = total_operations / args.threads + 1;
  int ops_needed = ops_per_thread;
  int used = 0;
  int job = 0;
  int section_i = 0;
  int cur_thread = 0;
  pthread_t *tid = malloc_t(pthread_t, args.threads);
  section *thread_queue = malloc_t(section, job_count + 1);

  while (job != job_count) {
    thread_queue[section_i].part = &jobs[job];
    thread_queue[section_i].start = used;

    int ops_in_part = update.manifest.partitions(jobs[job].part_number).operations_size();
    int ops_left = ops_in_part - used;
    if (ops_left <= ops_needed) {
      thread_queue[section_i].end = ops_in_part - 1;
      job++;
      used = 0;
      ops_needed -= ops_left;
    } else {
      used += ops_needed;
      thread_queue[section_i].end = used;
      ops_needed = 0;
    }

    section_i++;

    // If the thread is full, or we've run out of jobs to assign, we can
    // start it and start assigning to the next one.
    if (ops_needed == 0 || job == job_count) {
      thread_queue[section_i].part = NULL;
      pthread_create(&tid[cur_thread++], NULL, run_apply, thread_queue);
      section_i = 0;
      ops_needed = ops_per_thread;

      // Get the next thread job list ready if we still have things to
      // process
      if (job != job_count) {
        thread_queue = malloc_t(section, job_count + 1);
      }
    }
  }

  while (--cur_thread >= 0) {
    void *ret;
    pthread_join(tid[cur_thread], &ret);
  }

  for (job = 0; job < job_count; job++) {
    close(jobs[job].in);
    close(jobs[job].out);
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
