#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CMDLINE_MAXCHAR 512
#define CMDLINE_MAXARGNUM 16
#define CMDLINE_MAXTOKLEN 32

char** parseCmdInArr(char *cmd);
void freeArray(char** arr, int elementNum);

int main(void)
{
        char cmd[CMDLINE_MAXCHAR];

        while (1) {
            char *nl;
            int retval;
            pid_t pid;
            char** commandArr;

            /* Print prompt */
            printf("sshell@ucd$ ");
            fflush(stdout);

            /* Get command line */
            fgets(cmd, CMDLINE_MAXCHAR, stdin);
            if (cmd[strlen(cmd) - 1] != '\n') {
                fprintf(stderr, "Command exceeds the limit.\n");
                continue;
            }

            /* Print command line if stdin is not provided by terminal */
            if (!isatty(STDIN_FILENO)) {
                printf("%s", cmd);
                fflush(stdout);
            }

            /* Remove trailing newline from command line */
            nl = strchr(cmd, '\n');
            if (nl)
                *nl = '\0';

            /* Parse the commands in an array */
            commandArr = parseCmdInArr(cmd);
            if (commandArr == NULL) {
                continue;
            }


            /* Builtin command */
            if (!strcmp(cmd, "exit")) {
                fprintf(stderr, "Bye...\n");
                break;
            }

            /* Regular command, $PATH environment commands*/
            pid = fork();
            if (pid < 0) {
                fprintf(stderr, "fork error");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Child process
                char* instru[] = {"sh", "-c", cmd, NULL };
                execvp("sh", instru);
                fprintf(stderr, "execvp error");
                exit(EXIT_FAILURE);
            } else {
                // Parent process
                wait(&retval);
                fprintf(stderr, "+ completed '%s': [%d]\n",
                        cmd, WEXITSTATUS(retval));
            }
            freeArray(commandArr, CMDLINE_MAXARGNUM);
        }

        return EXIT_SUCCESS;
}

/* Parse the cmds in array (fangyunh) */
char** parseCmdInArr(char* cmd) {
    char** commands = (char**) calloc(CMDLINE_MAXARGNUM, sizeof(char*));
    int cursor = 0;
    char* token = (char*) calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    char* pos;
    int count = 0;

    while (cmd != NULL) {
        if (count > CMDLINE_MAXARGNUM) {
            fprintf(stderr, "Command exceeds the limit of number of tokens.\n");
            free(token);
            freeArray(commands, count);
            return NULL;
        }

        sscanf(cmd + cursor, "%[^ ]", token);
        if (strlen(token) > CMDLINE_MAXTOKLEN) {
            fprintf(stderr, "Command exceeds the limit of token max length\n");
            free(token);
            freeArray(commands, count);
            return NULL;
        }
        commands[count] = (char*) calloc(strlen(token) + 1, sizeof(char));
        strcpy(commands[count], token);
        count++;

        pos = strchr(cmd + cursor, ' ');
        while (pos != NULL && *(pos+1) == ' ') {
            pos++;
        }
        if (pos == NULL) {
            break;
        }
        cursor++;
        cursor = pos - cmd + 1;
    }
    free(token);

    return commands;
}

/* free the allocated space */
void freeArray(char** arr, int elementNum) {
    for (int i = 0; i < elementNum; i++) {
        free(arr[i]);
    }
    free(arr);
}


