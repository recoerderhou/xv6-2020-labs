#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc < 2){
  	printf("the sleep funciton needs 1 more argument\n");
  	exit(1);
  }
  
  int times = atoi(argv[1]);
  if(times < 0)
  	exit(1);
  sleep(times);
  exit(0);
  return 0;
}
