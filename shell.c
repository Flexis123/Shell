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
#define PIPE_RD 0
#define PIPE_WR 1

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

    case NODE_PIPE:;
        int p[2];
        pipe(p);
        
            pid_t lpid = fork();
            int lpid_status;
            if(lpid == 0){
                close(p[PIPE_RD]);
                close(STDOUT_FILENO);
                dup(p[PIPE_WR]);

                node_t* cmd1 = node->pipe.parts[0];
                lpid_status = execvp(cmd1->command.program, cmd1->command.argv);
            }

            pid_t rpid = fork();
            int rpid_status;
            if(rpid == 0){
                close(p[PIPE_WR]);
                close(STDIN_FILENO);
                dup(p[PIPE_RD]);

                node_t* cmd1 = node->pipe.parts[1];
                rpid_status = execvp(cmd1->command.program, cmd1->command.argv);
            }

            waitpid(lpid, &lpid_status, 0);
            waitpid(rpid, &rpid_status, 0);

            if(lpid < 0 || rpid < 0){
                print_err(lpid_status);
                print_err(rpid_status);
                return;
            }
        
        break;
    
    default:
        break;
    }
}
