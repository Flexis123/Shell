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


char* add_env_var(char* tok, char* env_name){
    strcat(prompt, getenv(env_name));
    tok++;
    return tok;
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

                if(tok[0] == 'u') tok = add_env_var(tok, "USER");
                else if(tok[0] == 'h') tok = add_env_var(tok, "NAME");
                else if(tok[0] == 'w') tok = add_env_var(tok, "PWD");
                strcat(prompt, tok);
            }
        }

        //printf("%s\n", prompt);
    }
}

void initialize(void)
{   
    /*
    if(getenv("USER") == NULL){
        extern char **environ;
        char **s = environ;
        for (; *s; s++) {
            printf("%s\n", *s);
        }
    }

    if (prompt)
        prompt = "vush$ ";
    */
    
    set_prompt_name();
    signal(SIGINT, sigint);
}

int exec_cmd(node_t *node)
{
    int status = 0; 
    char* prog_name = node->command.program;
    char** prog_argv = node->command.argv;

    if(strcmp(prog_name, "exit") == 0) exit(atoi(prog_argv[1]));
    else if(strcmp(prog_name, "cd") == 0){
        status = chdir(prog_argv[1]);
        if(status == 0) status = setenv("PWD", prog_argv[1], 1);
    } 
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
            
            if(child_pipe_io_status < 0){
                perror("Problem with replacing STD IO with Pipe IO ABORTING");
                break;
            }

            node_t* part = node->pipe.parts[i];
            run_command(part);
            exit(0);
        }
        childs[i] = pid;
        if(close(p[PIPE_WR]) < 0){
            perror("Problem with replacing STD IO with Pipe IO ABORTING");
            break;
        }
        fd_in = p[PIPE_RD];
        
    }

    for(size_t i = 0; i < node->pipe.n_parts; i++){
        waitpid(childs[i], NULL, 0);
    }
}

void exec_pipe2(node_t *node){
    size_t fsd_size = node->pipe.n_parts * 2;
    int fds[fsd_size];
    fds[0] = STDIN_FILENO;
    fds[fsd_size - 1] = STDOUT_FILENO;

    int p[2];
    for(size_t i = 1; i < fsd_size - 1; i += 2){
        pipe(p);
        fds[i] = p[PIPE_WR];
        fds[i + 1] = p[PIPE_RD];
    }
    
    size_t child_count = node->pipe.n_parts;
    pid_t children[child_count];
    for(size_t i = 0; i < child_count; i++){
        int j = i * 2;
        pid_t pid = fork();
        if(pid == -1){
            perror("error with process when piping aborting");
            return;
        }else if(pid == 0){
            dup2(fds[j], STDIN_FILENO);
            dup2(fds[j + 1], STDOUT_FILENO);

            run_command(node->pipe.parts[i]);
            exit(0);
        }

        if(i + 1 != child_count) close(fds[j + 1]);
        children[i] = pid;
    }

    for(size_t i = 0; i < node->pipe.n_parts; i++){
        waitpid(children[i], NULL, 0);
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
    /*
    if(getenv("USER") == NULL){
        extern char **environ;
        char **s = environ;
        for (; *s; s++) {
            printf("%s\n", *s);
        }
    }
    

    if (prompt)
        prompt = "vush$ ";
    */

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
        exec_pipe2(node);
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

    set_prompt_name();
}
