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

void intHandler(int n)
{
    printf("\nYou typed Control-C!\n");
}

int charInStr(char *str, char c, int size)
{
    int count = 0;
    for (int i = 0; str[i] && i < size; i++)
    {
        if (str[i] == c)
        {
            count++;
        }
    }
    return count;
}

void addVar(char**** vars, int* num_of_vars, char* name_of_var, char* value_of_var){
    if(!(*num_of_vars)){
        (*vars) = (char***)(malloc(sizeof(char**)));
        (*vars)[0] = (char**)(malloc(sizeof(2*sizeof(char*))));
        (*vars)[0][0] = (char*)(malloc(strlen(name_of_var) + 1));
        strcpy((*vars)[0][0], name_of_var);
        (*vars)[0][1] = (char*)(malloc(strlen(value_of_var) + 1));
        strcpy((*vars)[0][1], value_of_var);
        (*num_of_vars)++;
    }
    else{
        for(int r = (*num_of_vars) - 1; r >= 0; r--){
            if(! strcmp((*vars)[r][0], name_of_var)){
                free((*vars)[r][1]);
                (*vars)[r][1] = (char*)(malloc(strlen(value_of_var + 1)));
                strcpy((*vars)[r][1], value_of_var);
                return;
            }
            else{
                    (*num_of_vars)++;
                    (*vars) = (char***)(realloc((*vars), (*num_of_vars) * sizeof(char**)));
                    (*vars)[(*num_of_vars) - 1] = (char**)(malloc(sizeof(2*sizeof(char*))));
                    (*vars)[(*num_of_vars) - 1][0] = (char*)(malloc(strlen(name_of_var) + 1));
                    strcpy((*vars)[(*num_of_vars) - 1][0], name_of_var);
                    (*vars)[(*num_of_vars) - 1][1] = (char*)(malloc(strlen(value_of_var) + 1));
                    strcpy((*vars)[(*num_of_vars) - 1][1], value_of_var);
                }
        }
    }
}

int main()
{
    strcpy(name, "hello");
    if (signal(SIGINT, intHandler) == SIG_ERR)
    {
        printf("SIGINT ERROR");
    }
    char command[1024], prev_command[1024], read_var_name[1024], read_var_val[1024];
    char *token;
    char *outfile;
    char stat[3];
    int i, j, fd, amper, redirect, retid, status, pipes_num, pi, pj, len;
    char *argv[10];
    char ***pargv;
    status = 1;
    pipes_num = 0;
    int fildes[1024][2];

    char*** vars;
    int num_of_vars = 0;
    read_var_name[0] = '$';

    while (1)
    {
        printf("%s: ", name);

        if(fgets(command, 1024, stdin) == NULL){
            clearerr(stdin);
            printf("\n");
            continue;
        }
        command[strlen(command) - 1] = '\0';

        // !!
        if (strcmp(command, "!!"))
        {
            strcpy(prev_command, command);
        }
        else
        {
            strcpy(command, prev_command);
        }

        pipes_num = charInStr(command, '|', 1024);
        if (pipes_num > 0)
        {
            pargv = (char ***)(malloc(pipes_num * sizeof(char ***)));
            for (int r = 0; r < pipes_num; r++)
            {
                pargv[r] = (char **)(malloc(10 * sizeof(char *)));
            }
        }

        /* parse command line */
        i = 0;
        token = strtok(command, " ");
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok(NULL, " ");
            i++;
            if (token && !strcmp(token, "|"))
            {
                break;
            }
        }
        argv[i] = NULL;

        if (pipes_num > 0)
        {
            token = strtok(NULL, " ");
            pi = 0;
            pj = 0;
            while (token != NULL)
            {
                pargv[pi][pj] = token;
                token = strtok(NULL, " ");
                pj++;
                if (token && !strcmp(token, "|"))
                {
                    token = strtok(NULL, " ");
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
        if (!strcmp(argv[i - 1], "&"))
        {
            amper = 1;
            argv[i - 1] = NULL;
        }
        else
        {
            amper = 0;
        }
        if (i > 2 && !strcmp(argv[i - 2], ">"))
        {
            redirect = 1;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
        }
        else if (i > 2 && !strcmp(argv[i - 2], "2>"))
        {
            redirect = 2;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
        }
        else
        {
            redirect = 0;
        }
        /* built in commands */

        // =
        if (i > 2 && !strcmp(argv[0], "prompt") && !strcmp(argv[1], "="))
        {
            strcpy(name, argv[2]);
            continue;
        }

        // echo
        if (!strcmp(argv[0], "echo"))
        {
            j = 1;
            while (argv[j])
            {
                if (!strcmp(argv[j], "$?"))
                {
                    printf("%d", status);
                }
                else if(argv[j][0] == '$' && argv[j][1] != '\0'){
                    for(int r = 0; r <num_of_vars; r++){
                        if(!strcmp(vars[r][0], argv[j])){
                            printf("%s", vars[r][1]);
                            break;
                        }
                    }
                }
                else
                {
                    printf("%s", argv[j]);
                }
                j++;
                if (argv[j])
                {
                    printf(" ");
                }
            }
            printf("\n");
            continue;
        }

        // cd
        if (!strcmp(argv[0], "cd"))
        {
            if (!argv[1])
            {
                chdir(getenv("HOME"));
            }
            else if (chdir(argv[1]) == -1)
            {
                printf("No such file or directory\n");
            }
            continue;
        }

        // quit
        if (!strcmp(argv[0], "quit"))
        {
            for(int r = 0; r < num_of_vars; r++){
                free(vars[r][0]);
                free(vars[r][1]);
                free(vars[r]);
            }
            free(vars);
            exit(0);
        }

        // Variables
        if (i > 2 && argv[0][0] == '$' && argv[0][1] && strcmp(argv[0], "$?") && !strcmp(argv[1], "=")){
            addVar(&vars, &num_of_vars, argv[0], argv[2]);
            continue;
        }

        // Read
        if(i > 1 && !strcmp(argv[0], "read") && strcmp(argv[1], "?")){
            if(fgets(read_var_val, 1024, stdin) == NULL){
                clearerr(stdin);
                printf("\n");
                continue;
            }
            strcat(read_var_name, argv[1]);
            read_var_val[strlen(read_var_val) - 1] = '\0';
            addVar(&vars, &num_of_vars, read_var_name, read_var_val);
            read_var_name[1] = '\0';
            continue;
        }

        /* for commands not part of the shell command language */

        if (fork() == 0)
        {
            /* redirection of IO ? */
            if (redirect)
            {
                fd = creat(outfile, 0660);
                close(redirect);
                dup(fd);
                close(fd);
            }
            if (pipes_num > 0)
            {
                for (int r = 0; r < pipes_num; r++)
                {
                    pipe(fildes[r]);
                    if (fork() == 0)
                    {
                        close(STDOUT_FILENO);
                        dup(fildes[r][1]);
                        close(fildes[r][1]);
                        close(fildes[r][0]);
                        if (r == 0)
                        {
                            if (execvp(argv[0], argv) == -1)
                                printf("command %s not found\n", pargv[r][0]);
                            exit(0);
                        }
                        else
                        {
                            if (execvp(pargv[r - 1][0], pargv[r - 1]) == -1)
                            {
                                printf("command %s not found\n", pargv[r - 1][0]);
                                exit(0);
                            }
                        }
                    }
                    close(STDIN_FILENO);
                    dup(fildes[r][0]);
                    close(fildes[r][0]);
                    close(fildes[r][1]);
                    if (r == pipes_num - 1 && execvp(pargv[r][0], pargv[r]) == -1)
                    {
                        printf("command %s not found\n", pargv[r][0]);
                        exit(0);
                    }
                }
            }
            else if (execvp(argv[0], argv) == -1)
            {
                printf("command %s not found\n", argv[0]);
                exit(0);
            }
        }
        /* parent continues here */
        if (amper == 0)
        {
            retid = wait(&status);
        }
        if (pipes_num > 0)
        {
            for (int r = 0; r < pipes_num; r++)
            {
                free(pargv[r]);
            }
            free(pargv);
        }
    }
    for(int r = 0; r < num_of_vars; r++){
        free(vars[r][0]);
        free(vars[r][1]);
        free(vars[r]);
    }
    free(vars);
    return 0;
}
