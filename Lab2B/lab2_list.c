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
#include <signal.h>
#include <string.h>
#include "SortedList.h"

int numThreads = 1; // default 1
int numIterations = 1; // default 1
int numLists = 1; // default 1
int numElements;
int opt_yield = 0;
int yield_flag = 0;
char* yieldopts;
char syncopts;
SortedListElement_t* elements; // shared list of elements

// specified number of sub-lists with own list header and synchronization object
typedef struct SubList {
  SortedList_t* head;
  pthread_mutex_t mutex;
  int spinLock;
} SubList_t;

SubList_t* list_arr;

typedef struct lock_stat {
  long long nsec;
  int n;
} LockStat_t;
// hash function that hashes key into correspondent subList
int hash_function(const char* key, int numLists) {
  int hash_value = 0;
  while (*key != '\0') {// while not EOF 
    hash_value += *key;
    key++;
  }
  return hash_value % numLists;
}

void segfault_handler(int signal) {
  if (signal == SIGSEGV) {
    fprintf(stderr, "ERROR: Segmentation fault.\n");
    exit(2);
  }
}
void errorcheck(char* msg) {
  fprintf(stderr, "%s with error: %s\n", msg, strerror(errno));
  exit(1);
}

void *thread_routine(void *arg) {
  int num = *((int*) arg);
  int i;
  int index;
  SubList_t* currList;

  int numLocks = 0;
  long long lock_nsec = 0;
  struct timespec startTime,endTime;
  // insert intialized elements
  for (i = num * numIterations; i < (num+1)*numIterations; i++) {
    index = 0;
    if (numLists != 1)
      index = hash_function(elements[i].key, numLists);
    currList = &list_arr[index];
    if (syncopts == 'm') {
      if (clock_gettime(CLOCK_MONOTONIC, &startTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      pthread_mutex_lock(&currList->mutex);
      if (clock_gettime(CLOCK_MONOTONIC, &endTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      lock_nsec += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
      numLocks += 1;
    }
      
    else if (syncopts == 's') {
      if (clock_gettime(CLOCK_MONOTONIC, &startTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      while (__sync_lock_test_and_set(&currList->spinLock,1));
      if (clock_gettime(CLOCK_MONOTONIC, &endTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      lock_nsec += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
      numLocks += 1;
    } 
    SortedList_insert(currList->head, &elements[i]); 

    if (syncopts == 'm')
      pthread_mutex_unlock(&currList->mutex);

    else if (syncopts == 's')
      __sync_lock_release(&currList->spinLock);
  }

  // get the list length
  int len;
  int sum;
  for (i = 0; i < numLists; i++) {
    currList = &list_arr[i];
    if (syncopts == 'm') {
      if (clock_gettime(CLOCK_MONOTONIC, &startTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      pthread_mutex_lock(&currList->mutex);
      if (clock_gettime(CLOCK_MONOTONIC, &endTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      lock_nsec += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
      numLocks += 1;
    }

    else if (syncopts == 's') {
      if (clock_gettime(CLOCK_MONOTONIC, &startTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      while (__sync_lock_test_and_set(&currList->spinLock,1));
      if (clock_gettime(CLOCK_MONOTONIC, &endTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      lock_nsec += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
      numLocks += 1;
    }

    len = SortedList_length(currList->head);
    if (len == -1) {
      fprintf(stderr, "List is corrupted!\n");
      exit(2);
    }

    if (syncopts == 'm')
      pthread_mutex_unlock(&currList->mutex);

    else if (syncopts == 's')
      __sync_lock_release(&currList->spinLock);
    sum += len;
  }

  // looks up and delete all the keys it had previously inserted
  for (i = num * numIterations; i < (num+1)*numIterations; i++) {
    index = 0;
    if (numLists != 1)
      index = hash_function(elements[i].key, numLists);
    currList = &list_arr[index];

    if (syncopts == 'm') {
      if (clock_gettime(CLOCK_MONOTONIC, &startTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      pthread_mutex_lock(&currList->mutex);
      if (clock_gettime(CLOCK_MONOTONIC, &endTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      lock_nsec += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
      numLocks += 1;
    }
      
    else if (syncopts == 's') {
      if (clock_gettime(CLOCK_MONOTONIC, &startTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      while (__sync_lock_test_and_set(&currList->spinLock,1));
      if (clock_gettime(CLOCK_MONOTONIC, &endTime) < 0)
        errorcheck("FAILED TO GET CLOCKTIME.");
      lock_nsec += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
      numLocks += 1;
    }
    SortedListElement_t *target = SortedList_lookup(currList->head,elements[i].key);
    if (!target) {
      fprintf(stderr, "Failed to lookup element\n");
      exit(2);
    }

    if (SortedList_delete(target) == 1) {
      fprintf(stderr, "Failed to delete target\n");
      exit(2);
    }

    if (syncopts == 'm')
      pthread_mutex_unlock(&currList->mutex);

    else if (syncopts == 's')
      __sync_lock_release(&currList->spinLock);
  }

  LockStat_t* temp = (LockStat_t*) malloc(sizeof(LockStat_t));
  temp->nsec = lock_nsec;
  temp->n = numLocks;
  return temp;
}

void randomizerstr(char* buf, const int len) {

    static const char a[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int i;
    for (i = 0; i < len; ++i) {
        buf[i] = a[rand() % strlen(a)];
    }
    buf[len] = '\0';
}

int opt_threads, opt_iters;

int main(int argc, char* argv[])
{
	static const struct option long_options[] = {
    {"threads", required_argument, 0, 't'},
    {"iterations", required_argument, 0, 'i'},
    {"yield", required_argument, 0, 'y'},
    {"sync", required_argument, 0 , 's'},
    {"lists", required_argument, 0, 'l'},
    {0,0,0,0}
  };

  char opt;
  while (1) {
    int option_index = 0;
    opt = getopt_long(argc, argv, "t:i:y:s:l:", long_options, &option_index);
    
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
      yield_flag = 1;
      yieldopts = optarg;
      break;
    case 's':
      syncopts = optarg[0];
      break;
    case 'l':
      numLists = atoi(optarg);
      break;
    default:
      fprintf(stderr, "ERROR: Unrecognized argument. Correct usage: lab2_add '--threads=#' '--iterations=#' '--yield=[idl]' '--lists=#' '--sync=[s][m]\n");
      exit(1);
    }  
  }

  signal(SIGSEGV, segfault_handler);

  if (yield_flag) {
    int i;
    int len_yieldopts = strlen(yieldopts);
    for (i = 0; i < len_yieldopts; i++) {
      if (yieldopts[i] == 'i')
        opt_yield |= INSERT_YIELD;
      else if (yieldopts[i] == 'd')
        opt_yield |= DELETE_YIELD;
      else if (yieldopts[i] == 'l')
        opt_yield |= LOOKUP_YIELD;
    }
  }

  // initialize list arrays
  list_arr = (SubList_t*) malloc(numLists * sizeof(SubList_t));
  if (!list_arr)
    errorcheck("Failed to allocate memory to initialize lists array");
  int i;
  for (i = 0; i < numLists; i++) {
    SubList_t* sublist = &list_arr[i];
    sublist->head = (SortedList_t*) malloc(sizeof(SortedList_t));
    sublist->head->key = NULL;
    sublist->head->prev = sublist->head;
    sublist->head->next = sublist->head;

    // initializes spin lock
    if (syncopts == 's')
      sublist->spinLock = 0;
    // initializes mutex
    if (syncopts == 'm') {
      if (pthread_mutex_init(&sublist->mutex, NULL) != 0)
        errorcheck("Failed to initialize mutex");
    }
  }

	// create and initialize the required number of elements
  numElements = numThreads * numIterations;
	elements = (SortedListElement_t*) malloc(numElements * sizeof(SortedListElement_t));
	int randLen;
	for (i = 0; i < numElements; i++) {
		randLen = rand() % 10 + 10;
		char* random = (char*) malloc((randLen + 1) * sizeof(char));
		randomizerstr(random, randLen);
		elements[i].key = random;
	}

  // allocate memory for threads id
  pthread_t* tid = (pthread_t*) malloc(numThreads * sizeof(pthread_t));
  if (!tid)
    errorcheck("Failed to allocate memory for threads ids");

  // notes the (high resolution) starting time for the run
  struct timespec startTime, endTime;
  if (clock_gettime(CLOCK_MONOTONIC, &startTime) < 0)
    errorcheck("FAILED TO GET CLOCKTIME");

  // create threads
  int nums[numThreads];
  for (i = 0; i < numThreads; i++) {
    nums[i] = i;
    if (pthread_create(&tid[i], NULL, &thread_routine, &nums[i]) != 0) 
      errorcheck("Failed to create threads");
  }

  long long total_lock_nsec = 0;
  int total_numLocks = 0;
  LockStat_t* ret;
  for (i = 0; i < numThreads; i++) {
    if (pthread_join(tid[i], (void**)&ret) != 0)
      errorcheck("Failed to join threads");
    total_lock_nsec += ret->nsec;
    total_numLocks += ret->n;
    free(ret);
  }

  if (clock_gettime(CLOCK_MONOTONIC, &endTime) < 0)
    errorcheck("FAILED TO GET CLOCKTIME");

  // checks the length of the list to confirm that it's zero
  int length;
  for (i = 0; i < numLists; i++) {
    length = SortedList_length(list_arr[i].head);
    if (length != 0) {
      fprintf(stderr, "Length of list is not zero\n");
      exit(2);
    }
  }
	
	// print out .csv values
  int numOps = numThreads * numIterations * 3;
  long long totalRuntime = (endTime.tv_sec - startTime.tv_sec) * 1000000000L + (endTime.tv_nsec - startTime.tv_nsec);
  long long avgPerOp = totalRuntime / numOps;
  long long waitTime = 0;
  if (syncopts == 's' || syncopts == 'm')
    waitTime = total_lock_nsec / total_numLocks;

  fprintf(stdout, "list-");
  if (opt_yield) {
    if (opt_yield & INSERT_YIELD)
      fprintf(stdout, "i");
    if (opt_yield & DELETE_YIELD)
      fprintf(stdout, "d");
    if (opt_yield & LOOKUP_YIELD)
      fprintf(stdout, "l");
  }

  else
    fprintf(stdout, "none");

  switch (syncopts) {
  case 0:
    fprintf(stdout, "-none");
    break;
  case 's':
    fprintf(stdout, "-s");
    break;
  case 'm':
    fprintf(stdout, "-m");
    break;
  default:
    break;
  }

  fprintf(stdout, ",%d,%d,%d,%d,%lld,%lld,%lld\n", numThreads, numIterations, numLists, numOps, totalRuntime, avgPerOp, waitTime);

  for (i = 0; i < numElements; ++i)
		free((void*)elements[i].key);
  for (i = 0; i < numLists; i++)
    free(list_arr[i].head);
  free(list_arr);
  free(elements);
  free(tid);
  exit(0);
}
