#include "parser/ast.h"
#include "shell.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

void sigint(int signum){
    if(signum == SIGINT) {
        printf("SIGINT");

    }
    
}

void initialize(void)
{
    /* This code will be called once at startup */
    if (prompt)
        prompt = "vush$ ";

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

    if(strcmp(prog_name, "exit") == 0) exit(atoi(node->command.argv[1]));
    else if(fork() == 0)
    {
        if(strcmp(prog_name, "cd") == 0) status = get_status(chdir(node->command.argv[1]), -2);
        else status = get_status(execvp(prog_name, node->command.argv), -3);
    }
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
        prompt = "vush$ ";

    switch (node->type)
    {
    case NODE_COMMAND:
        exec_cmd_print_err(node);
        break;

    case NODE_SEQUENCE: ;
        node_t* seq_n = node;

       
            while(1)
            {
                print_tree(seq_n);
                exec_cmd_print_err(seq_n->sequence.first);
                seq_n = seq_n->sequence.second;

                if(seq_n->type == NODE_COMMAND){
                    //exec_cmd_print_err(seq_n);
                    break;
                }
            }
    
        break;
    
    default:
        break;
    }
}
