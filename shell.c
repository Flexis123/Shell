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

int exec_cmd(node_t *node, int (*exec)(char*, char**))
{
    int status = 0; 
    char* prog_name = node->command.program;
    char** prog_argv = node->command.argv;

    if(strcmp(prog_name, "exit") == 0) exit(atoi(prog_argv[1]));
    else if(strcmp(prog_name, "cd") == 0)  status = chdir(prog_argv[1]);
    else if(exec != NULL) status = exec(prog_name, prog_argv);

    if(status == -1) perror(prog_name);
    
    return status;
}


int cmd_work(char* prog_name, char** argv){
    int status = 0;
    if(fork() == 0) status = execvp(prog_name, argv);
    else wait(&status);
    return status;
}

int pipe_work(char* prog_name, char** argv){
    return execvp(prog_name, argv);
}

void perr_exit(const char* msg){
    perror(msg);
    exit(-1);
}


void run_command(node_t *node)
{
    /* Print parsed input for testing - comment this when running the tests! */
   
    if (prompt)
        prompt = PROMPT_NAME;

    
    switch (node->type)
    {
    case NODE_COMMAND:
        exec_cmd(node, cmd_work);
        break;
    case NODE_SEQUENCE: 
        exec_cmd(node->sequence.first, cmd_work);
        run_command(node->sequence.second);
        break;

    case NODE_PIPE:;
        pid_t lid, rid;
        
        for(size_t i = 0; i < node->pipe.n_parts - 1; i++){

            int p[2];
            if(pipe(p) < 0){
                perror("Problem with creating pipe");
                break;
            }

            lid = fork();
            if(lid == 0){
                if(close(p[PIPE_RD]) || close(STDOUT_FILENO)) perr_exit("error on replacing STD IO with pipe IO");
                else if(dup(p[PIPE_WR]) == -1) perr_exit("error on pipe write end dupe");
                else if(exec_cmd(node->pipe.parts[i], pipe_work) == -1) perr_exit("problem with lhs pipe program"); 
                else exit(0);
            }
            
            rid = fork();
            if(rid == 0){
                if(close(p[PIPE_WR]) || close(STDIN_FILENO)) perr_exit("error on replacing STD IO with pipe IO");
                else if(dup(p[PIPE_RD]) == -1) perr_exit("error on pipe read end dupe");
                else if(exec_cmd(node->pipe.parts[i + 1], pipe_work) == -1) perr_exit("problem with lhs pipe program");
                else exit(0);

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
