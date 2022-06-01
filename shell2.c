#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <stdlib.h>


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


int main() {
    // if(signal(SIGINT, intHandler) == SIG_ERR){
    //     printf("SIGINT ERROR");
    // }
    char command[1024], prev_command[1024], name[1024];
    char *token;
    char *outfile;
    char stat[3];
    int i, j, fd, amper, redirect, retid, status, new_command;
    char *argv[10];
    status = 1;
    new_command = 1;
    strcpy(name, "hello");
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

        /* parse command line */
        i = 0;
        token = strtok (command," ");
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok (NULL, " ");
            i++;
        }
        argv[i] = NULL;

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
            execvp(argv[0], argv);
        }
        /* parent continues here */
        if (amper == 0){
            retid = wait(&status);
        }
        argvtocommand(argv, prev_command);
    }
}
