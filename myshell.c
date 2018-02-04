//
//  myshell.c
//
//  COSC 255 Spring 2018
//  Project 1
//
//  Due on: Feb 4, 2018
//  Authors: Derek Acosta and Austin Lee

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include <string.h>

int main(int argc, char *argv[])
{
    if(argc == 3){
    	// Spawn a child to run the program.
 		pid_t pid=fork();
 		if (pid==0) { /* child process */
            char str[30];
            strcpy(str, "./");
            strcat(str, argv[1]);

            char *temp = clean(str);

			execlp(temp, temp, argv[2], (char *)NULL);
			exit(127); /* only if execv fails */
		}
 		else { /* pid!=0; parent process */
 		    waitpid(pid,0,0); /* wait for child to exit */
 		}
		return 0;
    }

}
