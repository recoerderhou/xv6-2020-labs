#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primes(int p[]){
	// it starts as a child process
	int n, prime;
	int p1[2];
	close(p[1]); // close the write side of the parent pipe
	// if there's no number in parent pipe
	if(read(p[0], &prime, 4) == 0){
		close(p[0]); // close the read side of the parent pipe
		exit(0); // exit
	}
	else{
		// the first number which passed all the ancestor pipe is definitly the new prime
		printf("prime %d\n", prime);
		pipe(p1); // start its own new pipe as a parent
		if(fork() == 0){
			close(p[0]); // no longer take need to read directly from the parent pipe(only accept what is fed to it)
			close(p1[1]);
			primes(p1);
			exit(0);
		}
		else{
			close(p1[0]); // the parent process does not need to read from this parent-child pipe
			while(1){
				if(read(p[0], &n, 4) == 0)
					break;
				// if the new number can go through this pipe
				// then hand it to the child pipe
				if(n % prime != 0)
					write(p1[1], &n, 4);
			}
			close(p[0]);
			close(p1[1]);
			wait(0); // it must be a parent process for some process
		}
	}
	exit(0);
}

int
main(int argc, char *argv[])
{
	int p[2]; // creat the initial pipe
	pipe(p);
	int i = 2;
	if(fork() == 0){
  		primes(p);
	}
	else{
		close(p[0]);
		for(i = 2; i <= 35; ++ i){
  			write(p[1], &i, 4);
  		}
  		close(p[1]);
  		// wait until all the process has finished their jobs
  		wait(0);
	}
	exit(0);
  
}
