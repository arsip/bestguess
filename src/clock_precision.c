//  -*- Mode: C; -*-                                                       
// 
//  clock_precision.c
// 
// (C) Jamie A. Jennings, 2024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

int main(int argc, char **argv) {
  struct rusage start, stop;
  getrusage(RUSAGE_SELF, &start);
  do {
    getrusage(RUSAGE_SELF, &stop);
  } while (stop.ru_utime.tv_usec == start.ru_utime.tv_usec);

  printf("CLOCKS_PER_SEC = %8lu ticks/sec\n", CLOCKS_PER_SEC);
  printf("               = %8.2Lf μs/tick\n", (long double) (1000 * 1000) / (long double) CLOCKS_PER_SEC);

  printf("Start %ld sec, %d μsec\n", start.ru_utime.tv_sec, start.ru_utime.tv_usec);
  printf("Stop  %ld sec, %d μsec\n\n", stop.ru_utime.tv_sec, stop.ru_utime.tv_usec);

  printf("Granularity %d μsec\n", stop.ru_utime.tv_usec - start.ru_utime.tv_usec);

  struct timespec tp;
  if (clock_getres(CLOCK_PROCESS_CPUTIME_ID, &tp)) {
    printf("clock_getres failed!\n");
    exit(-1);
  }
  printf("Clock resolution from API is %ld ns\n",
	 tp.tv_nsec + tp.tv_sec * 1000 * 1000 * 1000);

}
