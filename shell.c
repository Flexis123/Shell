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
        printf("Ctrl+C user terminated program\n");
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


int exec_cmd(node_t *node, int (*exec)(char*, char**))
{
    int status = 0; 
    char* prog_name = node->command.program;
    char** prog_argv = node->command.argv;

    if(strcmp(prog_name, "exit") == 0) exit(atoi(prog_argv[1]));
    else if(strcmp(prog_name, "cd") == 0)  status = get_status(chdir(prog_argv[1]), -2);
    else if(exec != NULL) status = exec(prog_name, prog_argv);
    
    return status;
}

int exec_cmd_print_err(node_t *node, int (*exec)(char*, char**)){
    int status = exec_cmd(node, exec);
    if (status < 0) print_err(status);
    return status;
}

int cmd_work(char* prog_name, char** argv){
    int status = 0;
    if(fork() == 0) status = get_status(execvp(prog_name, argv), -3);
    else wait(&status);
    return status;
}

int pipe_work(char* prog_name, char** argv){
    return execvp(prog_name, argv);
}


void run_command(node_t *node)
{
    /* Print parsed input for testing - comment this when running the tests! */
   
    if (prompt)
        prompt = PROMPT_NAME;

    
    switch (node->type)
    {
    case NODE_COMMAND:
        exec_cmd_print_err(node, cmd_work);
        break;
    case NODE_SEQUENCE: 
        exec_cmd_print_err(node->sequence.first, cmd_work);
        run_command(node->sequence.second);
        break;

    case NODE_PIPE:;
        pid_t lid, rid;
        int lid_status, rid_status;
        for(size_t i = 0; i < node->pipe.n_parts - 1; i++){
            int p[2];
            pipe(p);

            lid = fork();
            if(lid == 0){
                if(close(p[PIPE_RD]) || close(STDOUT_FILENO)) {
                    perror("error on replacing STD IO with pipe IO");
                    exit(-1);
                }
                else if(dup(p[PIPE_WR]) == -1) {
                    perror("error on pipe write end dupe");
                    exit(-1);
                }
                else if(exec_cmd(node->pipe.parts[i], pipe_work) == -1) {
                    perror("lhs");
                    exit(-1);
                }else exit(0);
                

            }
            
            rid = fork();
            if(rid == 0){
                if(close(p[PIPE_WR]) || close(STDIN_FILENO)) perror("error on replacing STD IO with pipe IO");
                else if(dup(p[PIPE_RD]) == -1) perror("error on pipe read end dupe");
                else if(exec_cmd(node->pipe.parts[i + 1], pipe_work) == -1) {
                    perror("rhs");
                    exit(-1);
                }else exit(0);

            }

            close(p[PIPE_RD]);
            close(p[PIPE_WR]);

            waitpid(lid, NULL, 0);
            waitpid(rid, NULL, 0);   
        }
        
        break;
    
    default:
        break;
    }
}
