#include "shell.h"

/**
 *  Tokenizes given input according to defined delimiter 
 * 
 *  input   -   input buffer
 */
command_t* tokenize_input(char* input){
    char* tokens[MAX_ARGS];
    const char delim[2] = " ";
    char* token;
    int cur_command = 0, cur_arg = 0;
    int redirect = 1;
    command_t* commands = (command_t*)malloc(MAX_COMMANDS * sizeof(command_t)); // Commands

    // Tokenize with delimiter
    token = strtok(input, delim);
    if(!token) return NULL;

    // Some kind of fix???
    if(token[0] == '-' || token[0] == '|' || token[0] == '>' || token[0] == '<') {
        printf("Wrong input\n");
        return NULL;
    };

    // Loop tokens
    while(token){

        if(!strcmp(token, PIPE) || !strcmp(token, REDIR_TYPE_1) || !strcmp(token, REDIR_TYPE_2) || !strcmp(token, REDIR_TYPE_3) ){ // Redirection
            int l;

            // Token was redirection
            redirect = 1;

            commands[cur_command].redirect = (char*)malloc(MAX_STRING * sizeof(char));

            // Fix str
            for(l = 0; l < MAX_STRING; l++){
                commands[cur_command].redirect[l] = '\0';
            }   

            strcpy(commands[cur_command].redirect, token);

            cur_command++;
            commands[cur_command].command = NULL;
        }else if(token[0] == '-' || redirect == 0){ // Argument

            // Token was argument
            commands[cur_command].arguments[cur_arg] = (char*)malloc(MAX_STRING * sizeof(char));
            strcpy(commands[cur_command].arguments[cur_arg], token);
            cur_arg++;
        }else{ // Command
            int l;

            // Token was command
            commands[cur_command].command = (char*)calloc(MAX_STRING, sizeof(char));

            // Fix str
            for(l = 0; l < MAX_STRING; l++){
                commands[cur_command].command[l] = '\0';
            }    

            strcpy(commands[cur_command].command, token);

            // Create fields
            commands[cur_command].arguments = (char**)calloc(MAX_ARGS, sizeof(char*));
            commands[cur_command].redirect = NULL;

            cur_arg = 0;
            redirect = 0;
        }

        token = strtok(NULL, delim); // Next token
    }

    // Intented bug, please no segmentation
    commands->length = cur_command + 1;

    return commands;
}