//
//  myshell.c
//
//  COSC 255 Spring 2018
//  Project 1
//
//  Due on: Feb 4, 2018
//  Authors: Derek Acosta and Austin Lee
//
//  References:
//  - Class notes, stack overflow,
//  - https://goo.gl/MA8hj3

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for fork, chdir */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include <fcntl.h>
#include <string.h>
#include <readline/readline.h> /* for readline() */
#include <readline/history.h> /* for readline() */
#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>


/* Define Statements */
#define clear() printf("\e[1;1H\e[2J")
#define MAX_ARGS 15

/* Global Variables */
char *arguments[MAX_ARGS];
char *bw_pipe_args[MAX_ARGS];  // Used when pipes are present
#define MAX_HISTORY 100

//history
char *history[MAX_HISTORY];
int histInd = 0;
bool hasWrapped = false;
char *token;

/* Function */
void setup_GUI();
int prompt_input();
int is_pipe();
void commhandler();
void pipehandler(int);
int start_process();
int binify(int);
int comm_swap();
void historyController();

int main(int argc, char *argv[])
{
        // Variables used by shell:
        char user_input[2048];

        // Setup the shell GUI
        setup_GUI();

        memset(history, 0, sizeof(history));

        // Run the shell until exit
        while (1) {


                while (prompt_input() == 0) {
                        prompt_input();
                }

                int pipes = is_pipe();
                if (pipes > 0) {
                        // There is at least one pipe present
                        pipehandler(pipes);
                } else {
                        commhandler();
                }

                memset(arguments, 0, sizeof arguments);
                memset(bw_pipe_args, 0, sizeof bw_pipe_args);

        }

        return 0;
}

/*
   =============== SETUP_GUI =================
   Function to display greeting message for
   myShell.
   ===========================================
 */
void setup_GUI() {
        // Clear the terminal window to make room for the shell program
        clear();
        printf("\n ==============================================================================\n");
        printf("\n                               Welcome to myShell                              \n");
        printf("\n ==============================================================================\n");
}

/*
   ============== PROMPT_INPUT ===============
   Prompts the user with $, takes following
   input and copies it to input_string. If no
   input provided, returns with value of 0.
   ===========================================
 */
int prompt_input() {
        char *input;
        /*
        char working_dir[2048];
        getcwd(working_dir, sizeof(working_dir));
        printf("Working Directory: %s\n", working_dir);
        */

        input = readline("$");
        // fgets(input,2048, stdin);
        if (strlen(input) > 0) {
                // IMPLEMENT HISTORY

                //allocating space in array for history
        		free(history[histInd]);
        		history[histInd] = malloc(strlen(input)+1);

        		//copies buffer into history array
        		strcpy(history[histInd], input);

        		//maintaining index
        		histInd++;
        		if (histInd >= MAX_HISTORY){
        			//checks if circular arr wrapped back
        			hasWrapped = true;
        		}
                histInd = histInd % MAX_HISTORY;

                // Process the input string
                char *_token;
                _token = strtok(input, " ");
                int j = 0;
                while( _token != NULL ) {
                        arguments[j] = _token;
                        _token = strtok(NULL, " ");
                        j++;
                }

                return 1;
        } else {
                // There was no readable input
                return 0;
        }
}

/*
   ================ IS_PIPE ==================
   Checks whether or not a pipe is present in
   the input. Returns number of pipes if pipe
   is present, or 0 if no pipe is present.
   ===========================================
 */
int is_pipe() {
        int pipe_counter = 0;

        int a = 0;
        while (arguments[a] != NULL) {
                if (strcmp(arguments[a], "|") == 0) {
                        pipe_counter++;
                }
                a++;
        }

        return pipe_counter;
}

/*
   ============== COMMHANDLER ================
   Handles built-in commands stored in
   arguments[0], and calls the respective sys
   function.
   ===========================================
 */
void commhandler() {
        // EXIT, CD, AND HISTORY
        if (strcmp(arguments[0], "cd") == 0) {
                chdir(arguments[1]);
        } else if (strcmp(arguments[0], "exit") == 0) {
                exit(0);

        } else if (strcmp(arguments[0], "history") == 0) {
                historyController();
        }


        // EXECUTABLE FILE
        else if (access(arguments[0], X_OK) == 0) {
                start_process();
        } else if (binify(1) > 0) {
                // File exists and is executable.
                start_process();
        } else {
                // File does not exist.
                printf("Error: Executable file not found.\n");
        }

}

/*
   ============== PIPEHANDLER ================
   Handles commands with pipes. Support for
   1+ pipes is (not yet) included.
   pipefd[0] = read end, pipefd[1] = write end
   Reference: https://goo.gl/9jVSU6
   ===========================================
 */
void pipehandler(int num_pipes) {
        // Set variables
        int pipefd[num_pipes * 2];
        pid_t pid;
        int num_comms = num_pipes + 1;
        int comm_idx = 0;

        int i;
        // Create the necessary pipes
        for (i = 0; i < num_pipes; i++) {
                if(pipe(pipefd + i*2) < 0) {
                        printf("Error: %s\n", "Pipe was unsuccessful.");
                        exit(EXIT_FAILURE);
                }
        }

        int d = 0;
        int j;
        for (j = 0; j < num_comms; j++) {
                // Switches out the bw_pipe_args array with the next command
                comm_idx = comm_swap(comm_idx);

                pid = fork();

                if (pid < 0) {
                        // Fork was unsuccessful
                        printf("Error: %s\n", "fork() was unsuccessful.");
                } else if (pid == 0) {
                        // Child Process
                        // Case 1: Not last command, Write pipes
                        if (j < num_comms - 1) {
                                if (dup2(pipefd[d + 1], 1) < 0) {
                                        printf("Error: %s\n", "dup2() was unsuccessful.");
                                        exit(EXIT_FAILURE);
                                }
                        }
                        // Case 2: if not first command, d is not zero, Read pipes
                        if (j != 0) {
                                if (dup2(pipefd[d - 2], 0) < 0) {
                                        printf("Error: %s\n", "dup2() was unsuccessful.");
                                        exit(EXIT_FAILURE);
                                }
                        }

                        // Close pipes
                        int p;
                        for (p = 0; p < 2 * num_pipes; p++) {
                                close(pipefd[p]);
                        }

                        // Execute
                        if (binify(2) > 0) {
                                // File exists in /bin/ and is executable
                                execv(bw_pipe_args[0], bw_pipe_args);
                                printf("Error: %s\n", "execv() failed to execute. File may not exist.");
                                exit(127);
                        } else {
                                // File does not exist.
                                printf("Error: Executable file not found.\n");
                        }
                } // End else if(pid == 0)
                d += 2;

        } // End for
        // Close pipes
        int p;
        for (p = 0; p < 2 * num_pipes; p++) {
                close(pipefd[p]);
        }

        // Wait
        int w;
        for (w = 0; w < num_pipes + 1; w++) {
                waitpid(pid,0,0);
        }

}

/*
   ================ COMM_SWAP ================
   Switches out the bw_pipe_args array with
   the correct command and its args.
   ===========================================
 */
int comm_swap(int index) {
        memset(bw_pipe_args, 0, sizeof bw_pipe_args);
        int g = 0;
        while (arguments[index] != NULL) {
                if (strcmp(arguments[index], "|") == 0) {
                        return index+1;
                }
                bw_pipe_args[g] = arguments[index];
                index++;
                g++;
        }

        return -1;
}

/*
   ================= BINIFY ==================
   Appends the /bin/ path to arguments[0];
   ===========================================
 */
int binify(int flag) {
        char *bin = "/bin/";
        char *usrbin = "/usr/bin/";
        if (flag == 1) {
                // use arguments array
                size_t arglen = strlen(arguments[0]);
                size_t usrbinlen = strlen(usrbin);
                size_t binlen = strlen(bin);

                char *cattedbin = malloc(arglen + binlen + 1);
                char *cattedusrbin = malloc(arglen + usrbinlen + 1);

                memcpy(cattedbin, bin, binlen);
                memcpy(cattedbin + binlen, arguments[0], arglen + 1);

                memcpy(cattedusrbin, usrbin, usrbinlen);
                memcpy(cattedusrbin + usrbinlen, arguments[0], arglen + 1);

                if (access(cattedbin, X_OK) == 0) {
                        // File exists in /bin/ and is executable
                        arguments[0] = cattedbin;
                        return 1;
                } else if (access(cattedusrbin, X_OK) == 0) {
                        // File exists in /usr/bin/ and is executable
                        arguments[0] = cattedusrbin;
                        return 1;
                }

        } else if (flag == 2) {
                // use bw_pipe_args array
                size_t arglen = strlen(bw_pipe_args[0]);
                size_t usrbinlen = strlen(usrbin);
                size_t binlen = strlen(bin);

                char *cattedbin = malloc(arglen + binlen + 1);
                char *cattedusrbin = malloc(arglen + usrbinlen + 1);

                memcpy(cattedbin, bin, binlen);
                memcpy(cattedbin + binlen, bw_pipe_args[0], arglen + 1);

                memcpy(cattedusrbin, usrbin, usrbinlen);
                memcpy(cattedusrbin + usrbinlen, bw_pipe_args[0], arglen + 1);

                if (access(cattedbin, X_OK) == 0) {
                        // File exists in /bin/ and is executable
                        bw_pipe_args[0] = cattedbin;
                        return 1;
                } else if (access(cattedusrbin, X_OK) == 0) {
                        // File exists in /usr/bin/ and is executable
                        bw_pipe_args[0] = cattedusrbin;
                        return 1;
                }
        }
        return 0;

}

/*
   ============= START_PROCESS ===============
   Called when the command is an executable
   file. File is executed with given command
   line arguments.
   ===========================================
 */
int start_process() {
        pid_t pid;

        if ((pid = fork()) == 0 ) {
                // Child process
                execv(arguments[0], arguments);
                printf("Error: %s\n", "execv() failed to execute. File may not exist.");
                exit(127);
        } else if (pid < 0) {
                // Fork was unsuccessful
                printf("Error: %s\n", "fork() was unsuccessful.");
        } else {
                // Parent process
                // wait for child to exit
                waitpid(pid,0,0);
        }

        return 0;
}

void historyController(){
        // int histInd = (&histInd);
        int cmdInd;
        // bool hasWrapped = (*waddr)
        // token = strtok(NULL, " ");

        if (arguments[1] != NULL) {
//		printf("token is not null\n");
                //clears history
                if (strcmp(arguments[1], "-c") == 0) {
//			printf("clearing history\n");

                        memset(history, 0, sizeof(history));
                        histInd = 0;
                        hasWrapped = false;
                        // *haddr = 0;
                        // *waddr = false;
                }
                //executes command at target index from history
                else if (arguments[1]) {
                        cmdInd = atoi(arguments[1]);
                        if (cmdInd >= histInd && hasWrapped == false)
                                printf("error: %s\n", "offset is not valid");
                        else if(cmdInd < MAX_HISTORY && hasWrapped == true) {
                                char *_token;
                                _token = strtok(history[(histInd+cmdInd-1)%MAX_HISTORY] , " ");
                                memset(arguments, 0, sizeof(arguments));
                                int j = 0;
                                while( _token != NULL ) {
                                        arguments[j] = _token;
                                        _token = strtok(NULL, " ");
                                        j++;
                                }
                                commhandler();
                        }
                        else {
                                // printf("Command to be executed: %s at offset %d", history[cmdInd], cmdInd);
//                printf("token=%s", token);
                                char *_token;
                                _token = strtok(history[cmdInd], " ");
                                memset(arguments, 0, sizeof(arguments));
                                int j = 0;
                                while( _token != NULL ) {
                                        arguments[j] = _token;
                                        _token = strtok(NULL, " ");
                                        j++;
                                }
                                commhandler();
//              cancels carriage return when history prints again, needs to be fixed
                        }
                }
                //not a valid modifier
                else{
                        printf("error: %s\n", "not a valid modifier");
                }
        }
        //if not extra arguments print history
        else{
//		printf("token is null\n");
                int i;
                if (hasWrapped == false) {
                        for(i = 0; i < histInd; i++) {
                                printf("%d %s\n",i, history[i]);
                        }
                }
                else{
                        int j;
                        for(i = 0; i < MAX_HISTORY; i++) {
                                j = (histInd + i) % MAX_HISTORY;
                                printf("%d %s\n", i, history[j]);
                        }
                }
        }
	// printf("\n");

}
