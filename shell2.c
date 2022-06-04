#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <stdlib.h>

char name[1024];

void intHandler(int n) {
    printf("\nYou typed Control-C!\n");
}

void argvtocommand(char* argv[10], char command[1024]){
    command[0] = '\0';
    int i = 0;
    while (argv[i])
    {
        strcat(command, argv[i]);
        i++;
        if(argv[i])
        {
            strcat(command, " ");
        }
    }
}

int charInStr(char* str, char c, int size){
    int count = 0;
    for(int i = 0; str[i] && i < size; i++){
        if(str[i] == c){
            count++;
        }
    }
    return count;
}

int main() {
    strcpy(name, "hello");
    if(signal(SIGINT, intHandler) == SIG_ERR){
        printf("SIGINT ERROR");
    }
    char command[1024], prev_command[1024];
    char *token;
    char *outfile;
    char stat[3];
    int i, j, fd, amper, redirect, retid, status, new_command, pipes_num, pi, pj;
    char *argv[10];
    char ***pargv;
    status = 1;
    new_command = 1;
    pipes_num = 0;
    int fildes[1024][2];
    while (1)
    {
        if(new_command){
            printf("%s: ", name);
            fgets(command, 1024, stdin);
            command[strlen(command) - 1] = '\0';
        }
        else{
            new_command = 1;
        }

        pipes_num = charInStr(command, '|', 1024);
        if(pipes_num > 0){
            pargv = (char***)(malloc(pipes_num * sizeof(char***)));
            for(int r = 0; r < pipes_num; r++){
                pargv[r] = (char**)(malloc(10 * sizeof(char*)));
            }
        }
        
        /* parse command line */
        i = 0;
        token = strtok (command," ");
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok (NULL, " ");
            i++;
            if (token && ! strcmp(token, "|")) {
                break;
            }
        }
        argv[i] = NULL;

        if(pipes_num > 0){
            token = strtok (NULL, " ");
            pi = 0;
            pj = 0;
            while (token != NULL)
            {
                pargv[pi][pj] = token;
                token = strtok (NULL, " ");
                pj++;
                if(token && ! strcmp(token, "|")){
                    token = strtok (NULL, " ");
                    pargv[pi][pj] = NULL;
                    pi++;
                    pj = 0;
                }
            }
        }

        /* Is command empty */
        if (argv[0] == NULL)
            continue;

        /* Does command line end with & */ 
        if (! strcmp(argv[i - 1], "&")) {
            amper = 1;
            argv[i - 1] = NULL;
        }
        else {
            amper = 0; 
        }
        if (i > 2 && ! strcmp(argv[i - 2], ">")) {
            redirect = 1;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
        }
        else if(i > 2 && !strcmp(argv[i - 2], "2>")){
            redirect = 2;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
        }
        else {
            redirect = 0; 
        }
        /* built in commands */

        // =
        if(i > 2 && ! strcmp(argv[0], "prompt") && ! strcmp(argv[1], "=")){
            strcpy(name, argv[2]);
            continue;
        }
        
        // echo
        if(! strcmp(argv[0], "echo")){
            j = 1;
            while (argv[j])
            {
                if(!strcmp(argv[j], "$?")){
                    printf("%d", status);
                }
                else{
                    printf("%s", argv[j]);
                }
                j++;
                if(argv[j]){
                    printf(" ");
                }
            }
            printf("\n");
            argvtocommand(argv, prev_command);
            continue;
        }

        // cd
        if(! strcmp(argv[0], "cd"))
        {
            if(!argv[1]){
                chdir(getenv("HOME"));
            }
            else if(chdir(argv[1]) == -1){
                printf("No such file or directory\n");
            }
            argvtocommand(argv, prev_command);
            continue;
        }

        // !!
        if(! strcmp(argv[0], "!!")){
            new_command = 0;
            strcpy(command, prev_command);
            continue;
        }

        // quit
        if(! strcmp(argv[0], "quit")){
            exit(0);
        }

        /* for commands not part of the shell command language */ 

        if (fork() == 0) { 
            /* redirection of IO ? */
            if (redirect) {
                fd = creat(outfile, 0660); 
                close (redirect) ; 
                dup(fd); 
                close(fd); 
            }
            if(pipes_num > 0){
                for(int r = 0; r < pipes_num; r++){
                    pipe(fildes[r]);
                    if(fork() == 0){
                        close(STDOUT_FILENO);
                        dup(fildes[r][1]);
                        close(fildes[r][1]);
                        close(fildes[r][0]);
                        if(r == 0){
                            if(execvp(argv[0], argv) == -1)
                                printf("command %s not found\n", pargv[r][0]);
                                exit(0);
                        }
                        else{ 
                            if(execvp(pargv[r - 1][0], pargv[r - 1]) == -1){
                                printf("command %s not found\n", pargv[r - 1][0]);
                                exit(0);
                            }
                        }
                    }
                    close(STDIN_FILENO);
                    dup(fildes[r][0]);
                    close(fildes[r][0]);
                    close(fildes[r][1]);
                    if(execvp(pargv[r][0], pargv[r]) == -1){
                        printf("command %s not found\n", pargv[r][0]);
                        exit(0);
                    }
                }
            }
            else if(execvp(argv[0], argv) == -1){
                printf("command %s not found\n", argv[0]);
                exit(0);
            }
        }
        /* parent continues here */
        if (amper == 0){
            retid = wait(&status);
        }
        argvtocommand(argv, prev_command);
    }
}
