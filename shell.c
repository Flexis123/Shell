#include "parser/ast.h"
#include "shell.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>

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

int exec_cmd(node_t *node)
{
    int status = 0; 
    char* prog_name = node->command.program;
    char** prog_argv = node->command.argv;

    if(strcmp(prog_name, "exit") == 0) exit(atoi(prog_argv[1]));
    else if(strcmp(prog_name, "cd") == 0)  status = chdir(prog_argv[1]);
    else if(fork() == 0) status = execvp(prog_name, prog_argv);
    else wait(&status);
   
    if(status == -1) perror(prog_name);
    
    return status;
}


void exec_pipe(node_t *node){
    int p[2];
    pid_t pid;
    int fd_in = STDIN_FILENO;

    pid_t childs[node->pipe.n_parts];

    for(size_t i = 0; i < node->pipe.n_parts; i++){
        pipe(p);

        pid = fork();
        if(pid == -1){
            perror("problem with creating new child for pipeline process");
            break;
        }else if(pid == 0){
            int child_pipe_io_status = dup2(fd_in, STDIN_FILENO);

            if(i + 1 < node->pipe.n_parts){
                child_pipe_io_status += dup2(p[PIPE_WR], STDOUT_FILENO);
            }
            child_pipe_io_status += close(p[PIPE_RD]);

            if(child_pipe_io_status < 0){
                perror("Problem with replacing STD IO with Pipe IO ABORTING");
                break;
            }

            node_t* part = node->pipe.parts[i];
            run_command(part);
            exit(0);
        }else{
            childs[i] = pid;
            if(close(p[PIPE_WR]) < 0){
                perror("Problem with replacing STD IO with Pipe IO ABORTING");
                break;
            }
            fd_in = p[PIPE_RD];
        }
    }

    for(size_t i = 0; i < node->pipe.n_parts; i++){
        waitpid(childs[i], NULL, 0);
    }
}

void run_command(node_t *node)
{
    /* Print parsed input for testing - comment this when running the tests! */
   
    if (prompt)
        prompt = PROMPT_NAME;


    
    switch (node->type)
    {
    case NODE_COMMAND:
        exec_cmd(node);
        break;
    case NODE_SEQUENCE: 
        run_command(node->sequence.first);
        run_command(node->sequence.second);
        break;

    case NODE_PIPE:
        exec_pipe(node);
        break;

    default:
        perror("Invalid command shell input");
    }
        
}
