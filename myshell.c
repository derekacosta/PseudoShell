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
#include <string.h>
#include <readline/readline.h> /* for readline() */
#include <readline/history.h> /* for readline() */

/* Define Statements */
#define clear() printf("\e[1;1H\e[2J")
#define MAX_ARGS 15

/* Global Variables */
const char *arguments[MAX_ARGS];

/* Function */
void setup_GUI();
int prompt_input();
int is_pipe();
void commhandler();
void pipehandler(int);
int start_process();

int main(int argc, char *argv[])
{
  // Variables used by shell:
  char user_input[2048];

  // Setup the shell GUI
  setup_GUI();

  // Run the shell until exit
  while (1) {
    while (prompt_input() == 0) {
      prompt_input();
    }
    int pipes = is_pipe();
    if (pipes > 0) {
      // There is at least one pipe present
      //pipehandler(pipes);
    } else {
      commhandler();
    }

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
  printf("\n\n ============================================================\n");
  printf("\n\n                      Welcome to myShell                     \n");
  printf("\n\n ============================================================\n");
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
  char working_dir[2048];
  getcwd(working_dir, sizeof(working_dir));
  printf("Working Directory: %s\n", working_dir);

  input = readline("$");
  if (strlen(input) > 0) {
    // IMPLEMENT HISTORY
    // Process the input string
    char *token;
    token = strtok(input, " ");
    int j = 0;
    while( token != NULL ) {
      arguments[j] = token;
      token = strtok(NULL, " ");
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
  //Check if arguments[0] is an executable file
  if (access(arguments[0], X_OK) == 0) {
    // File exists and is executable.
    start_process();
  }

  if (strcmp(arguments[0], "cd") == 0) {
    chdir(arguments[1]);
  } else if (strcmp(arguments[0], "exit") == 0) {
    exit(0);
  }
}

/*
============== PIPEHANDLER ================
Handles commands with pipes. Support for
1+ pipes is (not yet) included.
pipefd[0] = read end, pipefd[1] = write end
===========================================

void pipehandler(int num_pipes) {
  // Set the array of file descriptors (2 are needed per pipe)
  int pipefd[num_pipes * 2];

  // Create the necessary pipes
  for (int i = 0; i < count; i++) {
    if(pipe(pipefd + i*2) < 0) {
      printf("Error: %s\n", "Pipe was unsuccessful.");
      exit(EXIT_FAILURE);
    }
  }

  pid_t cpid;
  char buf;

  // Create the child process
  cpid = fork();
  if (cpid == -1) {
    printf("Error: %s\n", "Fork was unsuccessful.");
    exit(EXIT_FAILURE);
  }
  if (cpid == 0) {
    // Child reads from pipe
    close(pipefd[1]);

    while (read(pipefd[0], &buf, 1) > 0) {
      write(STDOUT_FILENO, &buf, 1);
    }
    write(STDOUT_FILENO, "\n", 1);
    close(pipefd[0]);
    _exit(EXIT_SUCCESS);

  } else {
    close(pipefd[0]);
    write(pipefd[1], argv[1], strlen(argv[1]));
    close(pipefd[1]);
    wait(NULL);
    exit(EXIT_SUCCESS);
  }
}
*/

/*
============= START_PROCESS ===============
Called when the command is an executable
file. File is executed with given command
line arguments. (NOT TESTED, warning
generated at compile) --- needs fix
===========================================
*/
int start_process(int program_number) {
  pid_t pid;

  if ((pid = fork()) == 0 ) {
    // Child process
    char *args[MAX_ARGS];
    for (int i = 1; i < MAX_ARGS; i++) {
      args[i] = arguments[i];
    }
    execv(arguments[0], args);
    printf("Error: %s\n", "execv() failed to execute");
    exit(127);
  } else if (pid < 0) {
    // Fork was unsuccessful
    perror("Error: ");
  } else {
    // Parent process
    // wait for child to exit
    waitpid(pid,0,0);
  }

  return 0;
}
