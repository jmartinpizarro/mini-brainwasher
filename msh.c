//P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

//#include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8
#define MAX_EXECUTABLE_COMMANDS 3


// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("****  Exiting MSH **** \n");
	//signal(SIGINT, siginthandler);
	exit(0);
}

/* myhistory */

/* myhistory */

struct command
{
  // Store the number of commands in argvv
  int num_commands;
  // Store the number of arguments of each command
  int *args;
  // Store the commands
  char ***argvv;
  // Store the I/O redirection
  char filev[3][64];
  // Store if the command is executed in background or foreground
  int in_background;
};

int history_size = 20;
struct command * history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd)
{
    if((*cmd).argvv != NULL)
    {
        char **argv;
        for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
        {
            for (argv = *(*cmd).argvv; argv && *argv; argv++)
            {
                if(*argv){
                    free(*argv);
                    *argv = NULL;
                }
            }
        }
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command* cmd)
{
    int num_commands = 0;
    while(argvv[num_commands] != NULL){
        num_commands++;
    }

    for(int f=0;f < 3; f++)
    {
        if(strcmp(filev[f], "0") != 0)
        {
            strcpy((*cmd).filev[f], filev[f]);
        }
        else{
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands-1;
    (*cmd).argvv = (char ***) calloc((num_commands) ,sizeof(char **));
    (*cmd).args = (int*) calloc(num_commands , sizeof(int));

    for( int i = 0; i < num_commands; i++)
    {
        int args= 0;
        while( argvv[i][args] != NULL ){
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **) calloc((args+1) ,sizeof(char *));
        int j;
        for (j=0; j<args; j++)
        {
            (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]),sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j] );
        }
    }
}


/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
	//reset first
	for(int j = 0; j < 8; j++)
		argv_execvp[j] = NULL;

	int i = 0;
	for ( i = 0; argvv[num_command][i] != NULL; i++)
		argv_execvp[i] = argvv[num_command][i];
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
	/**** Do not delete this code.****/
	int end = 0; 
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];

	if (!isatty(STDIN_FILENO)) {
		cmd_line = (char*)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF){
			if(strlen(cmd_line) <= 0) return 0;
			cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush (stdin);
			fflush(stdout);
		}
	}

	/*********************************/

	char ***argvv = NULL;
	int num_commands;

	history = (struct command*) malloc(history_size *sizeof(struct command));
	int run_history = 0;

	while (1) 
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		if (run_history)
    {
        run_history=0;
    }
    else{
        // Prompt 
        write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

        // Get command
        //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
        executed_cmd_lines++;
        if( end != 0 && executed_cmd_lines < end) {
            command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
        }
        else if( end != 0 && executed_cmd_lines == end)
            return 0;
        else
            command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
    }
		//************************************************************************************************


		/************************ STUDENTS CODE ********************************/
        static int internal_accumulator = 0;
        // error handler
	    if (command_counter > 0) {
			if (command_counter > MAX_COMMANDS){
				printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
			}
            if (command_counter > MAX_EXECUTABLE_COMMANDS){
                perror("Maximum number of executable commands is 3");
            }
        }


        // execute commands
        int fd[2];
        int fd_in = 0;
        int fd_out = 1;

        
        for (int i = 0; i < command_counter; ++i){
            getCompleteCommand(argvv, i); // num_command is the index of the command in argvv
            store_command(argvv, filev, in_background, &history[tail]);

            if (n_elem == history_size) {
                        // Liberar el comando más antiguo
                        free_command(&history[head]);
                        head = (head + 1) % history_size;
            } else {
                n_elem++;
            }
            
            tail = (tail + 1) % history_size;
            pipe(fd);

            if (strcmp(argvv[i][0], "mycalc") == 0)
                {
                if (argvv[i][1] == NULL || argvv[i][2] == NULL || argvv[i][3] == NULL) 
                    {
                    printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
                    }
                else 
                    {
                    int num1, num2,result;
                    char *endptr;
                    char *operator = argvv[i][2];
                    num1 = atoi(argvv[i][1]);
                    num2 = atoi(argvv[i][3]);
                    result = 0;
                    if (strcmp(operator, "add") == 0)
                        {
                            result = num1 + num2;
                            internal_accumulator = internal_accumulator + result;
                            printf("[OK] %d + %d = %d; Acc %d\n", num1, num2, result, internal_accumulator);
                        }
                    else if (strcmp(operator, "mul") == 0)
                        {
                            result = num1 * num2;
                            printf("[OK] %d * %d = %d\n", num1, num2, result);
                        }
                    else if (strcmp(operator, "div") == 0)
                        {
                            result = num1 / num2;
                            int remainder = num1 % num2;
                            printf("[OK] %d / %d = %d; Remainder %d\n", num1, num2, result, remainder);
                        }
                    } 
            } else if (strcmp(argvv[i][0], "myhistory") == 0)
                    {
                        if (argvv[i][1] == NULL) // if there is not any argument
                            {
                                if (n_elem == 0){
                                    perror("Commands not found\n");
                                }
                                else {
                                    for (int k = 0; k < n_elem; k++) {
                                        printf("%d ", k);
                                        // iteration and more iterations through a struct that I do not understand :)
                                        for (int cmd_index = 0; cmd_index < history[k].num_commands; cmd_index++) {
                                            for (int arg_index = 0; arg_index < history[k].args[cmd_index]; arg_index++) {
                                                printf("%s ", history[k].argvv[cmd_index][arg_index]);
                                            }
                                        }
                                        printf("\n");
                                    }
                                }
                            }
                        else //if there is a number with an argument
                            {
                                int commandToRun = atoi(argvv[i][1]);
                                if (commandToRun >= 0 && commandToRun < n_elem){
                                    printf("Running command %d\n", commandToRun);
                                    getCompleteCommand(history[commandToRun].argvv, 0);
                                    pid_t pid = fork(); // if not, the program will execute the command but then stop
                                    if (pid == 0) { 
                                        execvp(argv_execvp[0], argv_execvp);
                                        exit(0);
                                    } else if (pid < 0) { 
                                        perror("fork failed");
                                    } else { 
                                        wait(NULL);
                                    }
                                }
                            }
                    } else {

            pid_t pid = fork(); // Create a new process
            if (pid == 0) { // In the child process

            if (dup2(fd_in, STDIN_FILENO) == -1) { // Replace STDIN with fd_in
                    perror("dup2 failed");
                    exit(1);
                }
            if (i != command_counter - 1) { // If this is not the last command, replace STDOUT with fd[1]
                if (dup2(fd[1], STDOUT_FILENO) == -1) {
                    perror("dup2 failed");
                    exit(1);
                }
            }
                
            if (strcmp(filev[0], "0") != 0) { // If there is input redirection
                int fd_in = open(filev[0], O_RDONLY);
                if (fd_in == -1) {
                    perror("open failed");
                    exit(1);
                }
                if (dup2(fd_in, STDIN_FILENO) == -1) {
                    perror("dup2 failed");
                    exit(1);
                }
                close(fd_in);
            }

            if (strcmp(filev[1], "0") != 0) { // If there is output redirection
                int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out == -1) {
                    perror("open failed");
                    exit(1);
                }
                if (dup2(fd_out, STDOUT_FILENO) == -1) {
                    perror("dup2 failed");
                    exit(1);
                }
                close(fd_out);
            }

            execvp(argv_execvp[0], argv_execvp);
            
             // Execute the command
            } 
            else if (pid < 0) { // Fork failed
                perror("fork failed");
            } 
            else { // In the parent process
                if (in_background) { // If the command should be executed in the background
                    printf("[%d]\n", pid); // Print the PID of the child process
                } else {
                wait(NULL); // Wait for the child process to finish
                }
            }
            fd_in = fd[0];
            close(fd[1]);
            if (i == command_counter - 1 && strcmp(filev[1], "0") != 0) {
                close(fd_out);
            }
            }
			}
    }
    return 0;
}	


