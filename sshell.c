#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CMDLINE_MAXCHAR 512
#define CMDLINE_MAXARGNUM 16
#define CMDLINE_MAXTOKLEN 32

/* Structure for parsing commands */
struct inputCmd {
    char *cmdStr;
    struct inputCmd *next;
};

/* free the allocated space */
void freeLinkedList(struct inputCmd *head) {
    struct inputCmd *ptr = head;
    while (ptr) {
        struct inputCmd *node = ptr->next;
        free(ptr->cmdStr);
        free(ptr);
        ptr = node;
    }
}

/* Parse the cmds in linked-list (fangyunh) */
struct inputCmd* parseCmdInList(char* cmd) {
    //char** commands = (char**) calloc(CMDLINE_MAXARGNUM, sizeof(char*));
    int cursor = 0;
    char* token = (char*) calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    char* pos;
    int count = 0;
    struct inputCmd *head = NULL;
    struct inputCmd *tail = NULL;
    if (cmd == NULL || *cmd == '\0') {
        fprintf(stderr, "Empty command input.\n");
        free(token);
        return NULL;
    }
    while (1) {
        if (count > CMDLINE_MAXARGNUM) {
            fprintf(stderr, "Command exceeds the limit of number of tokens.\n");
            free(token);
            //freeLinkedList(head);
            return NULL;
        }

        sscanf(cmd + cursor, "%[^ ]", token);
        if (strlen(token) > CMDLINE_MAXTOKLEN) {
            fprintf(stderr, "Command exceeds the limit of token max length\n");
            free(token);
            //freeLinkedList(head);
            return NULL;
        }
        if (!token) {
            break;
        }

        struct inputCmd *newNode = (struct inputCmd*) calloc(1, sizeof(struct inputCmd));
        newNode->cmdStr = (char *) calloc(strlen(token) + 1, sizeof(char));
        if (!newNode->cmdStr) {
            fprintf(stderr, "newNode memory allocation fail");
            free(newNode);
            //freeLinkedList(head);
            return NULL;
        }
        if (strcmp(token, "\0") != 0) {
            strcpy(newNode->cmdStr, token);
            newNode->next = NULL;

            if (!head) {
                head = newNode;
                tail = newNode;
            } else {
                tail->next = newNode;
                tail = newNode;
            }
            count++;
        }


        pos = strchr(cmd + cursor, ' ');
        while (pos != NULL && *(pos+1) == ' ') {
            pos++;
        }
        if (pos == NULL) {
            break;
        }
        cursor = pos - cmd + 1;
    }
    free(token);

    return head;
}

/* cd built-in function */
void builtinCd (struct inputCmd *head) {
    struct inputCmd *filePath = head->next;
    int status = chdir(filePath->cmdStr);
    if (status == -1) {
        fprintf(stderr, "cd error");
    }
}


/* Determine and execute bulit-in functions */
bool builtinFunctions(struct inputCmd *head) {
    if (!strcmp(head->cmdStr, "cd")) {
        builtinCd(head);
        return true;
    } else if (!strcmp(head->cmdStr, "pwd")) {
        return true;
    }
    return false;
}

int main(void)
{
        char cmd[CMDLINE_MAXCHAR];

        while (1) {
            char *nl;
            int retval;
            pid_t pid;
            struct inputCmd *commandList;
            bool builtinFlag = false;

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
            commandList = parseCmdInList(cmd);

            /* Builtin command */
            if (!strcmp(cmd, "exit")) {
                fprintf(stderr, "Bye...\n");
                fprintf(stderr, "+ completed '%s': [%d]\n",
                        cmd, 0);
                break;
            }
            builtinFlag = builtinFunctions(commandList);
            if (builtinFlag) {
                fprintf(stderr, "+ completed '%s': [%d]\n",
                        cmd, 0);
                continue;
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
            freeLinkedList(commandList);
        }

        return EXIT_SUCCESS;
}
