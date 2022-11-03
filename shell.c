#include "parser/ast.h"
#include "shell.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define PROMPT_NAME "vush$ "

void sigint(int signum){
    if(signum == SIGINT) {
        printf("SIGINT\n");
       // prompt = PROMPT_NAME;
    }
    
}

void initialize(void)
{
    /* This code will be called once at startup */
    if (prompt)
        prompt = PROMPT_NAME;

    signal(SIGINT, sigint);
}

int get_status(int returned_status, int err_status){
    if(returned_status == -1) return err_status;
    else return returned_status;
}

void print_err(int status){
    switch (status)
    {
    case -2:
        perror("Problem with cd");
        break;
    case -3:
        perror("Problem with executing program");
        break;
    default:
        break;
    }
}


int exec_cmd(node_t *node)
{
    int status; 
    char* prog_name = node->command.program;
    char** prog_argv = node->command.argv;

    if(strcmp(prog_name, "exit") == 0) exit(atoi(prog_argv[1]));
    else if(strcmp(prog_name, "cd") == 0)  status = get_status(chdir(prog_argv[1]), -2);
    else if(fork() == 0) status = get_status(execvp(prog_name, prog_argv), -3);
    else wait(&status);
    
    return status;
}

int exec_cmd_print_err(node_t *node){
    int status = exec_cmd(node);
    if (status < 0) print_err(status);
    return status;
}

void run_command(node_t *node)
{
    /* Print parsed input for testing - comment this when running the tests! */

   
    if (prompt)
        prompt = PROMPT_NAME;

    
    switch (node->type)
    {
    case NODE_COMMAND:
        exec_cmd_print_err(node);
        break;
    case NODE_SEQUENCE: 
        exec_cmd(node->sequence.first);
        run_command(node->sequence.second);
        break;
    
    default:
        break;
    }
}
