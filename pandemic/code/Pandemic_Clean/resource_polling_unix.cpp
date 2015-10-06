#include <sys/time.h>
#include <sys/resource.h>
#include <cstdio>


size_t get_memory_usage()
{
  int who = RUSAGE_SELF;
  struct rusage usageStruct;
  
  getrusage(who, &usageStruct);

  //printf("max_rss: %ld\n", usageStruct.ru_maxrss);
  
  size_t ret = (size_t) usageStruct.ru_maxrss;
  ret = ret << 10; //unix gives us kb, parent func expects bytes
  return ret;
}