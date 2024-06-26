// P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

// #include "parser.h"
#include <stddef.h> /* NULL */
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

// to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
    printf("****  Exiting MSH **** \n");
    // signal(SIGINT, siginthandler);
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
struct command *history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd)
{
    if ((*cmd).argvv != NULL)
    {
        char **argv;
        for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
        {
            for (argv = *(*cmd).argvv; argv && *argv; argv++)
            {
                if (*argv)
                {
                    free(*argv);
                    *argv = NULL;
                }
            }
        }
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command *cmd)
{
    int num_commands = 0;
    while (argvv[num_commands] != NULL)
    {
        num_commands++;
    }

    for (int f = 0; f < 3; f++)
    {
        if (strcmp(filev[f], "0") != 0)
        {
            strcpy((*cmd).filev[f], filev[f]);
        }
        else
        {
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands - 1;
    (*cmd).argvv = (char ***)calloc((num_commands), sizeof(char **));
    (*cmd).args = (int *)calloc(num_commands, sizeof(int));

    for (int i = 0; i < num_commands; i++)
    {
        int args = 0;
        while (argvv[i][args] != NULL)
        {
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **)calloc((args + 1), sizeof(char *));
        int j;
        for (j = 0; j < args; j++)
        {
            (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]), sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j]);
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
void getCompleteCommand(char ***argvv, int num_command)
{
    // reset first
    for (int j = 0; j < 8; j++)
        argv_execvp[j] = NULL;

    int i = 0;
    for (i = 0; argvv[num_command][i] != NULL; i++)
        argv_execvp[i] = argvv[num_command][i];
}

/**
 * Main sheell  Loop
 */
int main(int argc, char *argv[])
{
    /**** Do not delete this code.****/
    int end = 0;
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];

    if (!isatty(STDIN_FILENO))
    {
        cmd_line = (char *)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF)
        {
            if (strlen(cmd_line) <= 0)
                return 0;
            cmd_lines[end] = (char *)malloc(strlen(cmd_line) + 1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush(stdin);
            fflush(stdout);
        }
    }

    /*********************************/

    char ***argvv = NULL;
    int num_commands;

    history = (struct command *)malloc(history_size * sizeof(struct command));
    int run_history = 0;

    while (1)
    {
        int status = 0;
        int command_counter = 0;
        int in_background = 0;
        signal(SIGINT, siginthandler);

        if (run_history)
        {
            run_history = 0;
        }
        else
        {
            // Prompt
            write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

            // Get command
            //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
            executed_cmd_lines++;
            if (end != 0 && executed_cmd_lines < end)
            {
                command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
            }
            else if (end != 0 && executed_cmd_lines == end)
                return 0;
            else
                command_counter = read_command(&argvv, filev, &in_background); // NORMAL MODE
        }
        //************************************************************************************************

        /************************ STUDENTS CODE ********************************/
        static int internal_accumulator = 0;
        if (n_elem == history_size)
        {
            free_command(&history[head]);
            head = (head + 1) % history_size;
        }
        if (argvv != NULL)
        {
            store_command(argvv, filev, in_background, &history[tail]);
            tail = (tail + 1) % history_size;
            n_elem++;
        }
        // error handler
        if (command_counter > 0)
        {
            if (command_counter > MAX_COMMANDS)
            {
                printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
            }
            if (command_counter > MAX_EXECUTABLE_COMMANDS)
            {
                perror("Maximum number of executable commands is 3");
            }
        }

        for (int i = 0; i < command_counter; i++)
        {
            getCompleteCommand(argvv, i);
        }

        if (strcmp(argv_execvp[0], "mycalc") == 0)
        {
            if (argv_execvp[1] == NULL || argv_execvp[2] == NULL || argv_execvp[3] == NULL)
            {
                printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
            }
            else
            {

                int num1, num2, result;
                // check if we are working with numbers
                if (sscanf(argv_execvp[1], "%d", &num1) != 1)
                {
                    printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
                }
                else if (sscanf(argv_execvp[3], "%d", &num2) != 1)
                {
                    printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
                }

                else
                {
                    char *operator= argv_execvp[2];
                    num1 = atoi(argv_execvp[1]);
                    num2 = atoi(argv_execvp[3]);
                    result = 0;
                    if (strcmp(operator, "add") == 0)
                    {
                        result = num1 + num2;
                        internal_accumulator = internal_accumulator + result;
                        char acc[10]; // for the env var
                        sprintf(acc, "%d", internal_accumulator);
                        setenv("Acc", acc, 1);
                        char output[100];
                        sprintf(output, "[OK] %d + %d = %d; Acc %s\n", num1, num2, result, getenv("Acc"));
                        fprintf(stderr, "%s", output);
                    }
                    else if (strcmp(operator, "mul") == 0)
                    {
                        result = num1 * num2;
                        char output[100];
                        sprintf(output, "[OK] %d * %d = %d\n", num1, num2, result);
                        fprintf(stderr, "%s", output);
                    }
                    else if (strcmp(operator, "div") == 0)
                    {
                        result = num1 / num2;
                        int remainder = num1 % num2;
                        char output[100];
                        sprintf(output, "[OK] %d / %d = %d; Remainder %d\n", num1, num2, result, remainder);
                        fprintf(stderr, "%s", output);
                    }
                    else
                    {
                        printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
                    }
                }
            }
        }
        else if (strcmp(argv_execvp[0], "myhistory") == 0)
        {
            // if no argument, then we must print the last 20 commands
            if (argv_execvp[1] == NULL)
            {
                for (int k = 0; k < n_elem  - 1; k++)
                {
                    int number = k;
                    fprintf(stderr, "%d ", number);
                    for (int i = 0; i < history[k].num_commands; i++)
                    {
                        for (int j = 0; j < history[k].args[i]; j++)
                        {
                            char output[100];
                            sprintf(output, "%s ", history[k].argvv[i][j]);
                            fprintf(stderr, "%s", output);
                        }
                        // Si hay más comandos, imprimir un pipe
                        if (i < history[k].num_commands - 1)
                        {
                            char output[100] = "| ";
                            fprintf(stderr, "%s", output);
                        }
                    }
                    char output[100] = "\n";
                    fprintf(stderr, "%s", output);
                }
            }
            // if we have an argument, then we must print the last n commands
            else
            {
                int n = atoi(argv_execvp[1]);
                struct command cmd = history[n];
                if (n >history_size){
                    printf("ERROR: Command not found\n");
                }
                char output[100];
                sprintf(output, "Running command %d\n", n);
                fprintf(stderr, "%s", output);

                // pipe for each even command
                int pipes[cmd.num_commands - 1][2];
                for (int i = 0; i < cmd.num_commands - 1; i++)
                {
                    if (pipe(pipes[i]) == -1)
                    {
                        perror("msh");
                    }
                }

                for (int i = 0; i < cmd.num_commands; i++)
                {
                    pid_t pid = fork();
                    if (pid == 0)
                    {

                        // if not first command, edit input descriptor
                        if (i != 0)
                        {
                            if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                            {
                                perror("pipe error");
                            }
                        }

                        // if not the last command, edit output descriptor
                        if (i != cmd.num_commands - 1)
                        {
                            if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
                            {
                                perror("pipe error");
                            }
                        }

                        // Close everything
                        for (int j = 0; j < cmd.num_commands - 1; j++)
                        {
                            close(pipes[j][0]);
                            close(pipes[j][1]);
                        }
                        // execute command
                        if (execvp(cmd.argvv[i][0], cmd.argvv[i]) == -1)
                        {
                            perror("error executing command");
                        }

                        // fatal crash here, not my problem
                        exit(EXIT_FAILURE);
                    }
                    else if (pid < 0)
                    {
                        perror("fork failed");
                    }
                }

                // close pipes for the parent
                for (int i = 0; i < cmd.num_commands - 1; i++)
                {
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }

                for (int i = 0; i < cmd.num_commands; i++)
                {
                    wait(NULL);
                }
            }
        }
        else if (command_counter == 1)
        {
            int pid = fork();
            if (pid < 0)
            {
                perror("fork error\n");
                return (-1);
            }

            if (pid == 0)
            {
                // input redirection
                if (strcmp(filev[0], "0") != 0)
                { // If there is input redirection
                    int fd_in = open(filev[0], O_RDONLY);
                    if (fd_in == -1)
                    {
                        perror("open failed");
                        exit(1);
                    }
                    if (dup2(fd_in, STDIN_FILENO) == -1)
                    {
                        perror("dup2 failed");
                        exit(1);
                    }
                    close(fd_in);
                }
                // output redirection
                if (strcmp(filev[1], "0") != 0)
                { // If there is output redirection
                    int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd_out == -1)
                    {
                        perror("open failed");
                        exit(1);
                    }
                    if (dup2(fd_out, STDOUT_FILENO) == -1)
                    {
                        perror("dup2 failed");
                        exit(1);
                    }
                    close(fd_out);
                }
                // error redirection, where is this used for? idk
                if (strcmp(filev[2], "0") != 0)
                {
                    int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd_out == -1)
                    {
                        perror("open failed");
                        exit(1);
                    }
                    if (dup2(fd_out, STDOUT_FILENO) == -1)
                    {
                        perror("dup2 failed");
                        exit(1);
                    }
                    close(fd_out);
                }
                // if -1, then we got a big problem here. Not my problem anyway
                if (execvp(argv_execvp[0], argv_execvp) < 0)
                {
                    perror("command exec is down\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (in_background)
                {
                    printf("[%d]\n", getpid());
                }
                else
                {
                    wait(NULL);
                }
            }
        }
        else
        {
            int n = command_counter;
            int fd[2];        // for pipes
            int pid, status2; // status 2 is for the same reason as status, but for not renaming it
            int filehandle = 0;

            int in;

            if ((in = dup(0)) < 0)
            {
                perror("fail in dup\n");
            }

            for (int i = 0; i < n; i++)
            {
                // if last command, do not create a pipe
                if (i != n - 1)
                {
                    if (pipe(fd) < 0)
                    {
                        perror("error in pipe\n");
                        exit(0);
                    }
                }

                switch (pid = fork()) // cases for the fork, more convenient than if due to 200 houndred more lines of code :O
                {

                case -1:
                    perror("Error in fork\n");

                    if ((close(fd[0])) < 0)
                    {
                        perror("fail in dup");
                    }
                    if ((close(fd[1])) < 0)
                    {
                        perror("fail in dup");
                    }
                    exit(0);
                case 0:
                    if (strcmp(filev[2], "0") != 0)
                    {
                        if ((filehandle = open(filev[2], O_TRUNC | O_WRONLY | O_CREAT, 0644)) < 0)
                        {
                            perror("could not open the file\n");
                        }
                        else if (dup2(filehandle, 2) < 0)
                        {
                            perror("fail in dup2");
                        }
                    }

                    if (i == 0 && strcmp(filev[0], "0") != 0)
                    {
                        if ((filehandle = open(filev[0], O_RDWR, 0644)) < 0)
                        {
                            perror("could not open the file\n");
                        }
                        else if (dup2(filehandle, 0) < 0)
                        {
                            perror("fail in dup2");
                        }
                    }
                    else
                    {
                        if (dup2(in, 0) < 0)
                        {
                            perror("fail in dup2\n");
                        }
                    }

                    if (i != n - 1) // pipes generation for the next command (if it is not the last one)
                    {
                        if (dup2(fd[1], 1) < 0)
                        {
                            perror("fail in dup2\n");
                        }
                        if ((close(fd[0])) < 0)
                        {
                            perror("fail in close dup");
                        }
                        if ((close(fd[1])) < 0)
                        {
                            perror("fail in close dup");
                        }
                    }
                    else
                    {
                        if (strcmp(filev[1], "0") != 0)
                        {
                            if ((close(1)) < 0)
                            {
                                perror("fail in close dup");
                            }

                            if ((filehandle = open(filev[1], O_TRUNC | O_WRONLY | O_CREAT, 0644)) < 0)
                            {
                                perror("could not open the file\n");
                            }
                        }
                    }

                    getCompleteCommand(argvv, i);
                    if (in_background)
                    {
                        printf("[%d]\n", getpid());
                    }

                    if (execvp(argv_execvp[0], argv_execvp) < 0)
                    {
                        perror("fail in executing\n");
                    }
                    break;

                default:

                    if ((close(in)) < 0)
                    {
                        perror("fail in close dup");
                    }
                    if (i != n - 1)
                    {
                        if ((in = dup2(fd[0], in)) < 0)
                        {
                            perror("fail in dup2\n");
                        }
                        if ((close(fd[1])) < 0)
                        {
                            perror("fail in close dup");
                        }
                    }
                }
            }
            if (filehandle != 0)
            {
                if ((close(filehandle)) < 0)
                {
                    perror("fail in close dup");
                }
            }
            if (!in_background) // this for executing the first, then the second, .... commands in that order
            {
                wait(NULL);
            }
        }
    }
    return 0;
}