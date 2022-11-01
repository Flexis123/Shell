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


int exec_cmd(node_t *node)
{
    int status;
    char* prog = node->command.program;

    if(strcmp(prog, "exit") == 0) exit(atoi(node->command.argv[1]));
    else if(fork() == 0)
    {
        if(strcmp(prog, "cd") == 0) chdir(node->command.argv[1]);
        else status = execvp(prog, node->command.argv);
        
    }
    else wait(&status);
    
    return status;
}

void run_command(node_t *node)
{
    /* Print parsed input for testing - comment this when running the tests! */
    //print_tree(node);

    if (prompt)
        prompt = "vush$ ";

    switch (node->type)
    {
    case NODE_COMMAND:
        exec_cmd(node);
        break;
    
    default:
        break;
    }
}
