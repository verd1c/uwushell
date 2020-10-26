#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>

#define TRUE 1
#define DEBUG 0
#define MAX_STRING 50
#define MAX_ARGS 500
#define MAX_COMMANDS 300
#define PIPE ">"
#define REDIR_TYPE_1 "|"
#define REDIR_TYPE_2 "||"
#define REDIR_TYPE_3 "|||"

static struct termios term;

/**
 * Struct for every command, contains all it's arguments
 */
typedef struct command{
 
    int length;
 
    char* command;

    char* redirect;
    char** arguments;
} command_t;

command_t* tokenize_input(char* input);

#endif