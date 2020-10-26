#include "shell.h"

// Globals
command_t* commands;
int** pipefd;

/**
 * Hooks termios to our terminal
 */
void set_shortcuts(){
    if (!isatty (STDIN_FILENO)) exit (0); // Check if stdin is a terminal

    tcgetattr(STDIN_FILENO, &term); // Get config
    term.c_cc[VTIME] = 0;
    term.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &term); // Set config
    return;
}

/**
 * Initializes pipefd pipes for every command in our buffer
 */
void init_pipes(){
    int i;
    pipefd = (int**)malloc(commands->length * sizeof(int*));
    for(i = 0; i < commands->length; i++){
        pipefd[i] = (int*)malloc(2 * sizeof(int));
    }

    return;
}

/**
 * Reads line of given size from input file and places it in buffer
 */
void read_line(FILE *file, char *buffer, int size){
    buffer[0] = '\0';
    buffer[size - 1] = '\0';             
    char* temp;

    if (fgets(buffer, size, file) == NULL) {
        *buffer = '\0';                  
        return;
    }else if ((temp = strrchr(buffer, '\n')) != NULL) {
        *temp = '\0';
    }
    return;
}

/**
 * Handles SIGINT stop signal, only killing the child (if it exists)
 */
void signal_stop(int signal){
    kill(SIGINT, 0);
    return;
}

/**
 * Changes current directory to argument given to cd
 */
void change_dir(){

    // If command was just cd, go to /home
    if(commands[0].arguments[0] == NULL){
        chdir("/home");
        return;
    }

    // Change dir
    if(chdir(commands[0].arguments[0]) == -1){
        printf("uwush: %s: no such directory\n", commands[0].arguments[0]); // Couldn't change dir
    }

    return;
}

/**
 * Executes command at index and deals with pipes and redirections
 * 
 * index    -   index of command to be executed
 */
void execute_command(int index){
    char* args[MAX_ARGS];
    char* arg;
    int i = 0;
    int j = 1;
    pid_t cpid;
    int status;
    int piping_forward = 0;
    int fd;
    int wasFile = 0;

    // Create args array
    args[0] = (char*)malloc(MAX_STRING * sizeof(char));
    strcpy(args[0], commands[index].command);

    while((arg = commands[index].arguments[i]) != NULL){
        args[j] = (char*)malloc(MAX_STRING * sizeof(char));
        strcpy(args[j++], arg);
        i++;
    }
    args[j] = NULL;

    // Ready pipe if we're piping forward
    if((index != commands->length - 1) && commands[index].redirect != NULL && (strcmp(commands[index].redirect, PIPE) == 0)){
        piping_forward = 1;
        pipe(pipefd[index]);
    }

    // Spawn child and deal with pipes / redirections
    cpid = fork();
    if(cpid == 0){

        // Piping forward
        if(piping_forward){

            dup2(pipefd[index][1], STDOUT_FILENO);
        }
        
        // Piping backwards
        if((index >= 1) && (commands[index - 1].redirect != NULL) && (strcmp(commands[index - 1].redirect, PIPE) == 0)){

            dup2(pipefd[index - 1][0], STDIN_FILENO);
        }

        // Redirects
        if(commands[index].redirect != NULL){
            
            // Check type
            if(strcmp(commands[index].redirect, REDIR_TYPE_1) == 0){ // <
                
                if((index + 1 <= commands->length) && (commands[index + 1].command != NULL)){     
                     
                    // Set input
                    fd = open(commands[index + 1].command, O_RDONLY, 0600);
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }else{
                    // Error
                    printf("uwush: syntax error\n");
                }
            }else if(strcmp(commands[index].redirect, REDIR_TYPE_2) == 0){ // >
                
                if((index + 1 <= commands->length) && (commands[index + 1].command != NULL)){

                    // Get file
                    fd = open(commands[index + 1].command, O_CREAT | O_TRUNC | O_WRONLY, 0600);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }else{
                    // Error
                    printf("uwush: syntax error\n");
                }
            }else if(strcmp(commands[index].redirect, REDIR_TYPE_3) == 0){ // >>

                if((index + 1 <= commands->length) && (commands[index + 1].command != NULL)) {

                    // Get file
                    fd = open(commands[index + 1].command, O_CREAT | O_APPEND | O_WRONLY, 0600);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }else{
                    // Error
                    printf("uwush: syntax error\n");
                }       
            }
        }

        // Case of file
        if(index > 0 && (strcmp(commands[index - 1].redirect, REDIR_TYPE_1) == 0 || strcmp(commands[index - 1].redirect, REDIR_TYPE_2) == 0 || strcmp(commands[index - 1].redirect, REDIR_TYPE_3) == 0)){
            wasFile = 1;
        }

        // Run command
        if(execvp(args[0], args) == -1){
            if(!wasFile) {
                printf("uwush: %s: command not found\n", args[0]);
            }
            exit(0); // Exit in the case exec failed so we don't forkbomb =)
        }
        exit(0); // Double exit double forkbomb protection, right?
    }else{ 

        // Piping forward close
        if(piping_forward){

            close(pipefd[index][1]);
        }
        
        // Piping backwards close
        if((index >= 1) && (commands[index - 1].redirect != NULL) && (strcmp(commands[index - 1].redirect, PIPE) == 0)){

            close(pipefd[index - 1][0]);
        }

        // Wait for child
        waitpid(-1, NULL, 0);
    }

    return;
}

/**
 * Handles the global command variable, looping through all of them and executing one by one
 */
void handle_command(){
    int z;

    // Empty line
    if(commands[0].command == NULL) return;

    // Basic commands
    if(commands[0].command && !strcmp(commands[0].command, "exit")){ // exit
        
        exit(0);
    }else if(commands[0].command && !strcmp(commands[0].command, "cd")){ // cd

        change_dir();
        return;
    }

    // Loop though commands and execute one by one
    z = 0;
    while(z < commands->length){
        if(commands[z].command != NULL) execute_command(z);
        z++;
    }
}

int main(int argc, char* argv[]){
    char path[MAX_STRING];
    char* name = getlogin();
    char* buffer = (char*)malloc(MAX_ARGS * sizeof(char));

    // Signals
    signal(SIGINT, signal_stop);
    set_shortcuts();

    while(TRUE){
        commands = NULL;
      
        // Empty the line buffer
		memset (buffer, '\0', MAX_STRING);

        getcwd(path, sizeof(path)); // Get path
        printf("uwush ~ %s %s $ ", name, path); // Print shell
  
        // Get user input and tokenize it
        read_line(stdin, buffer, MAX_COMMANDS);
        commands = tokenize_input(buffer);
        if(!commands) continue;

        // Init pipes
        init_pipes();

        // Command handler
        handle_command();
    }
    return 0;
}