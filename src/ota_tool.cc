#include "ota_tool.h"
#include "apply.h"
#include "util.h"
#include <argp.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>


const char *argp_program_version = "ota-tool 2.0-dev";
const char *argp_program_bug_address = "<emily@redcoat.dev>";

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

  FILE *f = fopen(argv[2], "rb");

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
  {"threads",   'c', "4", 0, "Number of threads to spawn"},
  { 0 }
};

struct arguments
{
  char *output;
  char *input;
  char *partitions;
  char *update_file;
  int threads;
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

typedef struct {
  int part_number;
  int in;
  int out;
} task;

task *jobs;
int total_operations = 0;
int job_count = 0;

void launch_apply(
  payload *update,
  struct arguments *args,
  char *partition
  ) {

  for (int j = 0; j < update->manifest.partitions_size(); j++) {
    chromeos_update_engine::PartitionUpdate part = update->manifest.partitions(j);
    const char *partition_name = part.partition_name().data();

    if (partition == NULL || strcmp(partition_name, partition) == 0) {

      int in = open_img_file(args->input, partition_name, O_RDONLY);
      if (in < 0) {
        if (partition != NULL) {
          log_err(partition, "Could not open the source image for reading");
        }
        continue;
      }
      int out = open_img_file(args->output, partition_name, O_WRONLY | O_CREAT | O_TRUNC);
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

typedef struct {
  int start;
  int end;
  task* job;
  struct arguments *args;
} thread_job;

void* run_apply(void *a) {
  thread_job *t_job = (thread_job*)a;

  FILE *f = fopen(t_job->args->update_file, "rb");

  while (t_job->job != NULL) {
    apply_partition(
        &update,
        &update.manifest.partitions(t_job->job->part_number),
        t_job->start,
        t_job->end,
        f,
        t_job->job->in,
        t_job->job->out
    );

    t_job++;
  }

  fclose(f);

  // The calling function immediately forgets about the memory it has
  // allocated to us, so it's the thread's responsibility to clean it up
  // properly.
  free(a);
  return NULL;
}

INIT_FUNC(apply) {
  struct arguments arguments = { NULL, NULL, NULL, NULL, get_nprocs_conf() };

  argp_parse (&argp, argc - 1, &(argv[1]), 0, 0, &arguments);

  FILE *f = fopen(arguments.update_file, "rb");
  if (!f) {
    std::cerr << "Could not open update file for reading: " << arguments.update_file << std::endl;
    return 1;
  }

  int len = strlen(arguments.update_file);
  if(len > 3 && !strcmp(arguments.update_file + len - 4, ".zip")) {
    init_payload_from_zip(&update, f);
  } else {
    init_payload(&update, f);
  }

  struct stat sb;
  if (stat(arguments.output, &sb)) {
    // We couldn't stat it - either because it doesn't exist or because
    // of access permissions. Naively try to create it last minute.
    if (mkdir(arguments.output, S_IRWXU | S_IRWXG)) {
      std::cerr << "Could not ensure output directory exists: " << arguments.output << std::endl;
      return 1;
    }
  } else if ((sb.st_mode & S_IFMT) != S_IFDIR) {
    std::cerr << "Output is not a directory: " << arguments.output << std::endl;
    return 1;
  }

  jobs = malloc_t(task, update.manifest.partitions_size());

  if (arguments.partitions == NULL) {
    launch_apply(&update, &arguments, NULL);
  } else {
    char *partition = strtok(arguments.partitions, ",");
    do {
      launch_apply(&update, &arguments, partition);
      partition = strtok(NULL, ",");
    } while(partition != NULL);
  }

  int ops_per_thread = total_operations / arguments.threads + 1;
  int ops_needed = ops_per_thread;
  int used = 0;
  int job = 0;
  int thread_job_i = 0;
  int cur_thread = 0;
  pthread_t *tid = malloc_t(pthread_t, arguments.threads);
  thread_job *t = malloc_t(thread_job, job_count + 1);

  while (job != job_count) {
    t[thread_job_i].job = &jobs[job];
    t[thread_job_i].start = used;
    t[thread_job_i].args = &arguments;

    int ops_in_part = update.manifest.partitions(jobs[job].part_number).operations_size();
    int ops_left = ops_in_part - used;
    if (ops_left <= ops_needed) {
      t[thread_job_i].end = ops_in_part - 1;
      job++;
      used = 0;
      ops_needed -= ops_left;
    } else {
      used += ops_needed;
      t[thread_job_i].end = used;
      ops_needed = 0;
    }

    thread_job_i++;

    // If the thread is full, or we've run out of jobs to assign, we can
    // start it and start assigning to the next one.
    if (ops_needed == 0 || job == job_count) {
      t[thread_job_i].job = NULL;
      pthread_create(&tid[cur_thread++], NULL, run_apply, t);
      thread_job_i = 0;
      ops_needed = ops_per_thread;

      // Get the next thread job list ready if we still have things to
      // process
      if (job != job_count) {
        t = malloc_t(thread_job, job_count + 1);
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
