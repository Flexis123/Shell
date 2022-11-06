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

#define DEFAULT_PROMPT_NAME "vush$ "
#define PIPE_RD 0
#define PIPE_WR 1

void sigint(int signum)
{
    if(signum == SIGINT) printf("Ctrl+C user terminated program\n");
}

void prompt_cat(char* src, size_t *dest_end, size_t src_n){
    size_t f = 0;
    for(size_t i = *dest_end + 1; i <= *dest_end + src_n; i++){
        prompt[i] = src[f];
        f++;
    }
    *dest_end += sizeof(src);
}

void set_prompt_name()
{
    char* new_prompt = getenv("PS1");


    if(!new_prompt) prompt = DEFAULT_PROMPT_NAME;
    else{
        prompt = malloc(1024 * sizeof(char*));
        memset(prompt, '\0', 1024);
        char* tok = strtok(new_prompt, "\\");
        
        if(tok != NULL)
        {
            strcpy(prompt, tok);

            while((tok = strtok(NULL, "\\")))
            { 

                if(tok[0] == 'u')
                {
                    strcat(prompt, getlogin());
                    strcat(prompt, tok + 1);
                }
                else if(tok[0] == 'h')
                {
                    strcat(prompt, getenv("NAME"));
                    strcat(prompt, tok + 1);
                }
                else if(tok[0] == 'w')
                {
                    strcat(prompt, getenv("PWD"));
                    strcat(prompt, tok + 1);
                }
                else strcat(prompt, tok);
            }
        }
    }
}

void initialize(void)
{   
    
    if(getenv("USER") == NULL){
        char **s = __environ;

        for (; *s; s++) {
            printf("%s\n", *s);
        }
    }
    
    set_prompt_name();
    signal(SIGINT, sigint);
}

int exec_cmd(node_t *node)
{
    int status = 0; 
    char* prog_name = node->command.program;
    char** prog_argv = node->command.argv;

    if(strcmp(prog_name, "exit") == 0) exit(atoi(prog_argv[1]));
    else if(strcmp(prog_name, "cd") == 0)  status = chdir(prog_argv[1]);
    else if(strcmp(prog_name, "set") == 0) 
    {
        char* var = strtok(prog_argv[1], "=");
        char* var_val = strtok(NULL, "\"");

        status = setenv(var, var_val, 1);

    }
    else if(strcmp(prog_name, "get") == 0) printf("%s\n", getenv("NAME"));
    else if(strcmp(prog_name, "unset") == 0) status = unsetenv(prog_argv[1]);
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

int exec_subshell(node_t *node, int detached){
    pid_t pid = fork();
    int status = -1;

    if(pid == -1) perror("problem with subshell process");
    else if(pid == 0) {
        run_command(node->subshell.child);
        exit(0);
    }
    else if(detached != 1) wait(&status);

    return status;
}

void run_command(node_t *node)
{
    set_prompt_name();

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

    case NODE_SUBSHELL:
        exec_subshell(node, 0);
        break;

    case NODE_REDIRECT:
        print_tree(node);
        break;

    case NODE_DETACH:;
        exec_subshell(node, 1);
        break;

    default:
        perror("Invalid command shell input");
    }
}
