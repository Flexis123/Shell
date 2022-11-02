#include "parser/ast.h"
#include "shell.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>


void initialize(void)
{
    /* This code will be called once at startup */
    if (prompt)
        prompt = "vush$ ";
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

    setuid(0);

    if(strcmp(prog_name, "exit") == 0) exit(atoi(node->command.argv[1]));
    else if(fork() == 0)
    {
        if(strcmp(prog_name, "cd") == 0) status = get_status(chdir(node->command.argv[1]), -2);
        else {
            
            /*
            char* prog_path = "/usr/bin/";
            char* prog = malloc(strlen(prog_name) + strlen(prog_path) + 1);
            prog[0] = '\0';
            strcat(prog, prog_path);
            prog[strlen(prog_path)] = '\0';
            strcat(prog, prog_name);
            */


            status = get_status(execvp(prog_name, node->command.argv), -3);
        }
    }
    else wait(&status);
    
    return status;
}

void run_command(node_t *node)
{
    /* Print parsed input for testing - comment this when running the tests! */
    //print_tree(node);

    int status;

    if (prompt)
        prompt = "vush$ ";

    switch (node->type)
    {
    case NODE_COMMAND:
        status = exec_cmd(node);
        if (status < 0) print_err(status);
        break;
    
    default:
        break;
    }
}
