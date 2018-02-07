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

/* Global Variables */
const char *arguments[15];

/* Function */
void setup_GUI();
int prompt_input();
void commhandler();
void pipehandler();
int start_process();

/* Define Statements */
#define clear() printf("\e[1;1H\e[2J");

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
    commhandler();

  }

  return 0;
}

/*
=============== SETUP_GUI =================
Documentation:
Function to display greeting message for
myShell.
===========================================
*/
void setup_GUI() {
  // Clear the terminal window to make room for the shell program
  clear();
  printf("\n\n ==================================================\n");
  printf("\n\n                 Welcome to myShell                \n");
  printf("\n\n ==================================================\n");
}

/*
============== PROMPT_INPUT ===============
Documentation:
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
============== COMMHANDLER ================
Documentation:

===========================================
*/
void commhandler() {
  if (strcmp(arguments[0], "cd") == 0) {
    chdir(arguments[1]);
  }
}

/*
============= START_PROCESS ===============
Documentation:

===========================================

int start_process(int program_number) {
  pid_t pid;

  if ((pid = fork()) == 0 ) {
    // Child process

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
*/
