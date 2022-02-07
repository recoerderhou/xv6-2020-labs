#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
	char buf[512];
	char* full_argv[MAXARG];
	for(int i = 1; i < argc; ++ i){
		full_argv[i - 1] = argv[i];
	}
	
	full_argv[argc] = 0;

	while(1){
		int i = 0;
		while(1){
			if(read(0, &buf[i], 1) <= 0){
				break;
			}
			if(buf[i] == '\n')
				break;
			++ i;
		}
		if(i == 0)
			break;
		buf[i] = 0;
		full_argv[argc - 1] = buf;
		if(fork() == 0){
			exec(full_argv[0], full_argv);
			exit(0);
		}
		else{
			wait(0);
		}
	}
	exit(0);
	
	
}
