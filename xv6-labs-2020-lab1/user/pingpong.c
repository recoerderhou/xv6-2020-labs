#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p1[2];
  int p2[2];
	pipe(p1);
	pipe(p2);
	if (fork() == 0)
	{
			char pingpong[10];
			int son_pid = getpid();
    	close(p2[0]);
    	close(p1[1]);
    	if(read(p1[0], pingpong, 8) < 8){
    		printf("son process could not read from pipe1\n");
    		exit(1);
    	}
    	else{
    		printf("%d: received ping\n", son_pid);
    	}
    	if(write(p2[1], "DAAAAAAY", 8) < 8){
    		printf("son process could not write into pipe2\n");
    		exit(1);
    	}
    	exit(0);
	}
	else
	{
	    close(p1[0]);
	    close(p2[1]);
	    if(write(p1[1], "GOOOOOOD", 8) < 8){
	    	printf("father process could not write into pipe1\n");
	    	wait(0);
	    	exit(1);
	    }
	    char pingpong_f[10];
			int vater_pid = getpid();
    	if(read(p2[0], pingpong_f, 8) < 8){
    		printf("father process could not read from pipe2\n");
    		wait(0);
    		exit(1);
    	}
    	else{
    		printf("%d: received pong\n", vater_pid);
    	}
    	wait(0);
    	exit(0);
	}

}


