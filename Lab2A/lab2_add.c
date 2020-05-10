// NAME: Anh Mac
// EMAIL: anhmvc@gmail.com
// UID: 905111606

#include <stdlib.h>
#include <stdio.h> // for fprintf(3)
#include <errno.h> // for errno(3)
#include <getopt.h> // for getopt_long(3)
#include <pthread.h>
#include <time.h> // for clock_gettime(3)
#include <unistd.h>
#include <string.h>

long long counter = 0; // global variable to be shared between threads
int numThreads = 1; // default 1
int numIterations = 1; // default 1
int withLock = 0;
int spinlock = 0;
pthread_mutex_t mutex;
char lock_type;
int opt_yield;

void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}

void add_mutex(long long *pointer, long long value) {
  pthread_mutex_lock(&mutex);
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
  pthread_mutex_unlock(&mutex);
}

void add_spinlock(long long *pointer, long long value) {
  while (__sync_lock_test_and_set(&spinlock,1));
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
  __sync_lock_release(&spinlock);
}

void add_cns(long long *pointer, long long value) {
  long long old_value, sum;
  do {
    old_value = *pointer;
    sum = old_value + value;
    if (opt_yield)
      sched_yield();
  } while (__sync_val_compare_and_swap(pointer, old_value, sum) != old_value);
}

void* thread_routine(void* arg) {
  int i;
  for (i = 0; i < numIterations; i++) {
    if (lock_type == 'm') {
      add_mutex(&counter,1);
      add_mutex(&counter,-1);
    }
    else if (lock_type == 's') {
      add_spinlock(&counter,1);
      add_spinlock(&counter,-1);
    }
    else if (lock_type == 'c') {
      add_cns(&counter,1);
      add_cns(&counter,-1);
    }
    else {
      add(&counter, 1);
      add(&counter, -1);
    }
  }
  return arg;
}

void errorcheck(char* msg) {
  fprintf(stderr, "%s with error: %s\n", msg, strerror(errno));
  exit(1);
}

int main(int argc, char *argv[]) {
  static const struct option long_options[] = {
    {"threads", required_argument, 0, 't'},
    {"iterations", required_argument, 0, 'i'},
    {"yield", no_argument, 0, 'y'},
    {"sync", required_argument, 0, 's'},
    {0,0,0,0}
  };

  char opt;
  while (1) {
    int option_index = 0;
    opt = getopt_long(argc, argv, "t:i:ys:", long_options, &option_index);
    
    if (opt == -1)
      break;

    switch (opt) {
      case 't':
        numThreads = atoi(optarg);
        break;
      case 'i':
        numIterations = atoi(optarg);
        break;
      case 'y':
        opt_yield = 1;
        break;
      case 's':
        lock_type = optarg[0];
        withLock = 1;
        break;
      default:
        fprintf(stderr, "ERROR: Unrecognized argument. Correct usage: lab2_add '--threads=#' '--iterations=#' '--yield' '--sync=[m][s][c]\n");
        exit(1);
    }  
  }

  // notes the (high resolution) starting time for the run
  struct timespec startTime, endTime;
  clock_gettime(CLOCK_MONOTONIC, &startTime);

  pthread_t* tid = malloc(numThreads * sizeof(pthread_t));
  if (tid == NULL)
    errorcheck("Error, failed to allocate memory for threads ids");

  // initializes mutex
  if (lock_type == 'm') {
    if (pthread_mutex_init(&mutex, NULL) != 0)
      errorcheck("Failed to initialize mutex");
  }

  // create threads
  int i;
  for (i = 0; i < numThreads; i++) {
      if (pthread_create(&(tid[i]), NULL, &thread_routine, NULL) != 0) 
        errorcheck("Failed to create threads");
  }

  for (i = 0; i < numThreads; i++) {
    if (pthread_join(tid[i], NULL) != 0) 
      errorcheck("Failed to join threads");
  }

  // stop timer
  clock_gettime(CLOCK_MONOTONIC, &endTime);

  // print to stdout csv record
  int numOps = numThreads * numIterations * 2;
  int totalRuntime = (endTime.tv_sec - startTime.tv_sec) * 1000000000L + (endTime.tv_nsec - startTime.tv_nsec);
  int avgPerOp = totalRuntime / numOps;

  fprintf(stdout, "add");

  if (opt_yield)
    fprintf(stdout, "-yield");

  if (withLock) {
    switch (lock_type){
      case 'm':
        fprintf(stdout, "-m");
        break;
      case 's':
        fprintf(stdout, "-s");
        break;
      case 'c':
        fprintf(stdout, "-c");
        break;
      default:
        break;
    }
  }

  else
    fprintf(stdout, "-none");

  fprintf(stdout, ",%d,%d,%d,%d,%d,%lld\n", numThreads, numIterations, numOps, totalRuntime, avgPerOp, counter); 
    
  if (lock_type == 'm')
    pthread_mutex_destroy(&mutex);

  free(tid);
  return 0;  
}
