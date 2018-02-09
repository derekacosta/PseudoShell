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
char *arguments[MAX_ARGS];
char *bw_pipe_args[MAX_ARGS];  // Used when pipes are present

/* Function */
void setup_GUI();
int prompt_input();
int is_pipe();
void commhandler();
void pipehandler(int);
int start_process();
void binify(int);
int comm_swap();

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
  // EXIT, CD, AND HISTORY
  if (strcmp(arguments[0], "cd") == 0) {
    chdir(arguments[1]);
  } else if (strcmp(arguments[0], "exit") == 0) {
    exit(0);
  }

  // EXECUTABLE FILE
  else if (access(arguments[0], X_OK) == 0) {
    // File exists and is executable.
    start_process();
  } else {
    // File may exist, but dir path not given
    binify(1);
    if (access(arguments[0], X_OK) == 0) {
      // File exists in /bin/ and is executable
      start_process();
    } else {
      binify(2);
      if (access(arguments[0], X_OK) == 0) {
        // File exists in /usr/bin/ and is executable
        printf("%s\n", arguments[0]);
        start_process();
      } else {
        // File does not exist.
        printf("Error: Executable file not found.");
      }
    }
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
  printf("In pipehandler\n");

  // Set variables
  int pipefd[num_pipes * 2];
  pid_t pid;
  int num_comms = num_pipes + 1;
  int comm_idx = 0;

  // Create the necessary pipes
  for (int i = 0; i < num_pipes; i++) {
    if(pipe(pipefd + i*2) < 0) {
      printf("Error: %s\n", "Pipe was unsuccessful.");
      exit(EXIT_FAILURE);
    }
  }

  int d = 0;
  for (int j = 0; j < num_comms; j++) {
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
        if (dup2(pipefd[j + 1], 1) < 0) {
          printf("Error: %s\n", "dup2() was unsuccessful.");
          exit(EXIT_FAILURE);
        }
      }
      // Case 2: if not first command, d is not zero, Read pipes
      if (d != 0) {
        if (dup2(pipefd[d - 2], 0) < 0) {
          printf("Error: %s\n", "dup2() was unsuccessful.");
          exit(EXIT_FAILURE);
        }
      }

      // Close pipes
      for (int p = 0; p < 2 * num_pipes; p++) {
        close(pipefd[p]);
      }

      printf("%s\n", bw_pipe_args[0]);
      // Execute
      binify(1);
      if (access(arguments[0], X_OK) == 0) {
        // File exists in /bin/ and is executable
        execv(bw_pipe_args[0], bw_pipe_args);
        printf("Error: %s\n", "execv() failed to execute. File may not exist.");
        exit(127);
      } else {
        binify(2);
        if (access(arguments[0], X_OK) == 0) {
          // File exists in /usr/bin/ and is executable
          execv(bw_pipe_args[0], bw_pipe_args);
          printf("Error: %s\n", "execv() failed to execute. File may not exist.");
          exit(127);
        } else {
          // File does not exist.
          printf("Error: Executable file not found.");
        }
      }

    }

    d += 2;

  }
  // Close pipes
  for (int p = 0; p < 2 * num_pipes; p++) {
    close(pipefd[p]);
  }

  // Wait
  for (int w = 0; w < num_pipes + 1; w++) {
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
  for (size_t i = 0; i < sizeof arguments; i++) {
    printf("Value of args in comm swap: %s\n", arguments[i]);
  }

  int g = 0;
  while (arguments[index] != NULL) {
    if (strcmp(arguments[index], "|") == 0) {
      printf("%s\n", "In string comp");
      return index+1;
    }
    bw_pipe_args[g] = arguments[index];
    printf("Value of bpa in loop: %s\n", bw_pipe_args[g]);
    index++;
    g++;
  }

  for (size_t i = 0; i < sizeof bw_pipe_args; i++) {
    printf("Value of bpa in comm swap: %s\n", bw_pipe_args[i]);
  }

  printf("%s\n", "Exiting comm_swap");
  return -1;
}

/*
================= BINIFY ==================
Appends the /bin/ path to arguments[0];
===========================================
*/
void binify(int flag) {
  char *bin;
  if (flag == 1) {
    bin = "/bin/";
  } else if (flag == 2) {
    bin = "/usr/bin/";
  }

  size_t arglen = strlen(arguments[0]);
  size_t binlen = strlen(bin);

  char *catted = malloc(arglen + binlen + 1);

  memcpy(catted, bin, binlen);
  memcpy(catted + binlen, arguments[0], arglen + 1);

  arguments[0] = catted;
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
