#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDLINE_MAXCHAR 512
#define CMDLINE_MAXARGNUM 16
#define CMDLINE_MAXTOKLEN 32
#define MAX_BUFFER_SIZE 256
#define MAX_ENVI_VAR 26
#define ORIGINAL_INPUT (-1)
#define ORIGINAL_OUTPUT (-1)
#define ORIGINAL_ERROR (-1)
#define ERR_STATE 1
#define SUC_STATE 0

char *enviVar[MAX_ENVI_VAR] = {""};

/* Structure for parsing commands */
struct inputCmd {
    char *cmdStr;
    struct inputCmd *next;
};

struct cmdType {
    int binFunc;
    int pip;
    int metaORdir;
    int oRdir;
    int metaPip;
};

/* free the allocated space of linked list*/
void freeLinkedList(struct inputCmd *head) {
    struct inputCmd *ptr = head;
    while (ptr) {
        struct inputCmd *node = ptr->next;
        free(ptr->cmdStr);
        free(ptr);
        ptr = node;
    }
}

/* free the allocated space of array */
void freeArray(char **head) {
    for (int i = 0; i < CMDLINE_MAXARGNUM + 1; i++) {
        free(head[i]);
    }
    free(head);
}

/* Parse the cmds in linked-list (fangyunh) */
struct inputCmd *parseCmdInList(char *cmd) {
    int cursor = 0;
    char *token = (char *) calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    char *pos;
    int count = 0;
    struct inputCmd *head = NULL;
    struct inputCmd *tail = NULL;

    while (1) {
        if (count > CMDLINE_MAXARGNUM) {
            fprintf(stderr, "Error: too many process arguments\n");
            free(token);
            return NULL;
        }

        sscanf(cmd + cursor, "%[^ ]", token);
        if (strlen(token) > CMDLINE_MAXTOKLEN) {
            fprintf(stderr, "Error: too many process arguments\n");
            free(token);
            return NULL;
        }

        struct inputCmd *newNode = (struct inputCmd *) calloc(2, sizeof(struct inputCmd));
        newNode->cmdStr = (char *) calloc(strlen(token) + 1, sizeof(char));
        if (!newNode->cmdStr) {
            perror("calloc error:");
            free(newNode);
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
        while (pos != NULL && *(pos + 1) == ' ') {
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

/* Parse the command line in an array with a NULL in the end */
char **parseCmdInArr(char *cmd) {
    char **commands = (char **) calloc(CMDLINE_MAXARGNUM + 1, sizeof(char *));
    int cursor = 0;
    char *token = (char *) calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    char *pos;
    int count = 0;
    int i = 0;

    while (cmd[i] == ' ') {
        i++;
    }
    cmd += i;

    while (1) {
        // check does cmd reached the end
        int status = sscanf(cmd + cursor, "%[^ ]", token);

        if (status > 0) {
            commands[count] = (char *) calloc(strlen(token) + 1, sizeof(char));
            strcpy(commands[count], token);
            count++;
        } else {
            break;
        }

        pos = strchr(cmd + cursor, ' ');
        while (pos != NULL && *(pos + 1) == ' ') {
            pos++;
        }
        if (pos == NULL) {
            break;
        }
        cursor = pos - cmd + 1;
    }
    commands[count] = NULL; // Satisfy the execvp() requirement

    return commands;
}

/* Initialize the type structure */
void typeInit(struct cmdType *type) {
    type->binFunc = 0;
    type->oRdir = 0;
    type->metaORdir = 0;
    type->pip = 0;
    type->metaPip = 0;
}

/* Classify the command into different types */
void deterType(struct inputCmd *list, char *cmd, struct cmdType *type) {
    char *builtin[] = {"cd", "pwd", "exit", "set"};

    for (int i = 0; i < 4; i++) {
        if (strcmp(list->cmdStr, builtin[i]) == 0) {
            type->binFunc = 1;
        }
    }

    if (strchr(cmd, '>')) {
        if (strstr(cmd, ">&")) {
            type->metaORdir = 1;
        }
        type->oRdir = 1;
    }

    if (strchr(cmd, '|')) {
        if (strstr(cmd, "|&")) {
            type->metaPip = 1;
        }
        type->pip = 1;
    }
}

/* Return the standrd input, output, and error back to standard */
void returnStd(int saveIn, int saveOut, int saveErr) {
    if (saveIn != -1) {
        dup2(saveIn, STDIN_FILENO);
    }
    close(saveIn);

    if (saveOut != -1) {
        dup2(saveOut, STDOUT_FILENO);
    }
    close(saveOut);

    if (saveErr != -1) {
        dup2(saveErr, STDERR_FILENO);
    }
    close(saveErr);
}

/* Check valid variables names */
bool isValidName(const char *name) {
    char validName = name[0];
    if (strlen(name) != 1) {
        return false;
    }

    if (validName >= 'a' && validName <= 'z') {
        return true;
    }
    return false;
}

/* Detect environmental variables and replace them in list */
int replaceEnvirVarList(struct inputCmd *list) {
    struct inputCmd *cmdPtr = list;
    char *content = (char *) calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    int position = 0;
    bool checkNameFlag = false;

    while (cmdPtr != NULL) {
        char *curCmd = cmdPtr->cmdStr;
        if (curCmd[0] == '$') {
            curCmd++;
            checkNameFlag = isValidName(curCmd);
            if (!checkNameFlag) {
                fprintf(stderr, "Error: invalid variable name\n");
                return ERR_STATE;
            }
            position = curCmd[0] - 'a';
            if (enviVar[position] != NULL) {
                strcpy(content, enviVar[position]);
            } else {
                strcpy(content, "");
            }
            cmdPtr->cmdStr = content;
        }
        cmdPtr = cmdPtr->next;
    }
    return SUC_STATE;
}

/* Detect environmental variables and replace them in array */
void replaceEnvirVarArr(char **cmdArr) {
    //char *cmdBuffer = (char *) calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    //char *replaceStr = (char *) calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    char *cmdBuffer = (char *) malloc(sizeof(char));
    char *replaceStr = (char *) malloc(sizeof(char));

    int varPos = 0;
    int i = 0;

    while (cmdArr[i] != NULL) {
        strcpy(cmdBuffer, cmdArr[i]);
        if (cmdBuffer[0] == '$') {
            varPos = cmdBuffer[1] - 'a';
            if (enviVar[varPos] != NULL) {
                strcpy(replaceStr, enviVar[varPos]);
            } else {
                strcpy(replaceStr, "");
            }
            strcpy(cmdArr[i], replaceStr);
        }
        i++;
    }

    free(cmdBuffer);
    free(replaceStr);
}

/* Execute regular command and $PATH environment commands */
void execRegCmd(char **instru, char *cmd, int saveIn, int saveOut, int saveErr) {
    int retval;
    pid_t pid;
    pid = fork();
    replaceEnvirVarArr(instru);
    if (pid < 0) {
        fprintf(stderr, "fork error\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(instru[0], instru);
        fprintf(stderr, "Error: command not found\n");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, &retval, 0);
        returnStd(saveIn, saveOut, saveErr);
        fprintf(stderr, "+ completed '%s' [%d]\n",
                cmd, WEXITSTATUS(retval));
    }
}

/* Detect are there missing parts in commands of output redirection and pipe */
bool detectMissing(const char *instruc, const char *file) {
    if (instruc == NULL) {
        fprintf(stderr, "Error: missing command\n");
        return true;
    } else if (file == NULL) {
        fprintf(stderr, "Error: no output file\n");
        return true;
    }

    return false;
}

/* Divide meta characters string */
void divMeta(char *meta, char **arr, char *cmd) {
    char *curPosition = cmd;
    int count = 0;
    char *findMeta = strstr(cmd, meta);

    while (findMeta != NULL) {
        *findMeta = '\0';
        arr[count] = curPosition;
        count++;
        curPosition = findMeta + strlen(meta);
        findMeta = strstr(curPosition, meta);
    }
    arr[count] = curPosition;
}

/* Put the file to FD = STDOUT_FILENO */
void conductRedirection(char **file, char **instruc, char *first, int meta) {
    pid_t pid;
    int fd;
    int status;

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork error");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        fd = open(file[0], O_WRONLY | O_CREAT, 0644);
        if (fd < 0) {
            fprintf(stderr, "Error: no output file\n");
            exit(EXIT_FAILURE);
        }

        int saveOut = dup(STDOUT_FILENO);
        int saveErr = dup(STDERR_FILENO);
        dup2(fd, STDOUT_FILENO);
        if (meta) {
            dup2(fd, STDERR_FILENO);
        }
        close(fd);
        execRegCmd(instruc, first, ORIGINAL_INPUT, saveOut, saveErr);
        exit(EXIT_SUCCESS);
    } else {
        waitpid(pid, &status, 0);
    }
}

/* Output Redirection Structure */
void outputRedirection(char *cmd, int meta) {
    bool missFlag = false;
    char **cmdArr = (char **) calloc(CMDLINE_MAXTOKLEN, sizeof(char *));
    if (meta) {
        divMeta(">&", cmdArr, cmd);
    } else {
        cmdArr[0] = strtok(cmd, ">");
        cmdArr[1] = strtok(NULL, ">");
    }

    missFlag = detectMissing(cmdArr[0], cmdArr[1]);
    if (!missFlag) {
        char **instruc = parseCmdInArr(cmdArr[0]);
        char **file = parseCmdInArr(cmdArr[1]);
        conductRedirection(file, instruc, cmdArr[0], meta);
    }
}

/* PWD built-in function */
int builtinPWD() {
    char buffer[MAX_BUFFER_SIZE];

    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        printf("%s\n", buffer);
    } else {
        fprintf(stderr, "PWD error");
        return ERR_STATE;
    }
    return SUC_STATE;
}

/* cd built-in function */
int builtinCd(struct inputCmd *head) {
    struct inputCmd *filePath = head->next;
    int status = chdir(filePath->cmdStr);
    if (status == -1) {
        fprintf(stderr, "Error: cannot cd into directory\n");
        return ERR_STATE;
    }
    return SUC_STATE;
}

/* Set global variables */
int builtinSet(struct inputCmd *cmdList) {
    char *name = (char *) calloc(1, sizeof(char));
    char *content = (char *) calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    char validName;
    bool isValid;
    int position = 0;

    if (cmdList != NULL && cmdList->next != NULL && cmdList->next->cmdStr != NULL) {
        strcpy(name, cmdList->next->cmdStr);
    } else {
        strcpy(name, "");
    }

    isValid = isValidName(name);

    if (!isValid) {
        fprintf(stderr, "Error: invalid variable name\n");
        return ERR_STATE;
    }

    validName = name[0];

    position = validName - 'a';
    strcpy(content, cmdList->next->next->cmdStr);
    enviVar[position] = realloc(enviVar[position], strlen(content) + 1);
    strcpy(enviVar[position], content);
    free(name);
    free(content);
    return SUC_STATE;
}

/* Determine and execute bulit-in functions */
int builtinFunctions(struct inputCmd *head) {
    int status = 0;
    if (!strcmp(head->cmdStr, "cd")) {
        status = builtinCd(head);
    } else if (!strcmp(head->cmdStr, "pwd")) {
        status = builtinPWD();
    } else if (!strcmp(head->cmdStr, "set")) {
        status = builtinSet(head);
    }
    return status;
}

/* Detect mislocated output redirection */
bool isMisloactedOutRedir(char *cmd) {
    char *pipPos = strchr(cmd, '|');
    char *orPos = strchr(cmd, '>');

    while (pipPos != NULL) {
        if (orPos < pipPos) {
            return true;
        }
        pipPos++;
        pipPos = strchr(pipPos, '|');
    }
    return false;
}

int main(void) {
    char cmd[CMDLINE_MAXCHAR];

    while (1) {
        char *nl;
        bool mislocOutRedirFlag = false;
        struct inputCmd *commandList;
        struct cmdType *type = (struct cmdType *) calloc(5, sizeof(struct cmdType));
        int builtinStat = 0;
        int repEnvirVarStat = 0;
        typeInit(type);


        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAXCHAR, stdin);
        if (cmd[strlen(cmd) - 1] != '\n') {
            fprintf(stderr, "Error: too many process arguments\n");
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

        /* Handle empty input */
        if (*cmd == '\0') {
            continue;
        }

        /* Detect mislocated output redirection */
        mislocOutRedirFlag = isMisloactedOutRedir(cmd);
        if (mislocOutRedirFlag) {
            fprintf(stderr, "Error: mislocated output redirection\n");
            continue;
        }

        /* Parse the commands in a Linked list */
        commandList = parseCmdInList(cmd);

        /* Replace strings to environment variables */
        repEnvirVarStat = replaceEnvirVarList(commandList);
        if (repEnvirVarStat == ERR_STATE) {
            continue;
        }

        /* Determine the type of command */
        deterType(commandList, cmd, type);

        /* Execute the command according to their types */
        if (type->binFunc) {
            /* Builtin command */
            if (!strcmp(commandList->cmdStr, "exit")) {
                fprintf(stderr, "Bye...\n");
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, 0);
                break;
            }
            builtinStat = builtinFunctions(commandList);
            fprintf(stderr, "+ completed '%s' [%d]\n",
                    cmd, builtinStat);
        } else {
            if (type->oRdir) {
                /* Output Redirection */
                outputRedirection(cmd, type->metaORdir);
            } else if (type->pip) {
                /* Pipe */
            } else {
                /* Regular command, $PATH environment commands*/
                char **cmdArr = parseCmdInArr(cmd);
                execRegCmd(cmdArr, cmd, ORIGINAL_INPUT, ORIGINAL_OUTPUT, ORIGINAL_ERROR);
                freeArray(cmdArr);
            }

        }

        freeLinkedList(commandList);
    }

    return EXIT_SUCCESS;
}