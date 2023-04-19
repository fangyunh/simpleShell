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
#define MAX_BUFFER_SIZE 256
#define ORIGINAL_INPUT -1
#define ORIGINAL_OUTPUT -1
#define ORIGINAL_ERROR -1



/* Structure for parsing commands */
struct inputCmd
{
    char *cmdStr;
    struct inputCmd *next;
};

struct cmdType
{
    int binFunc;
    int pip;
    int metaORdir;
    int oRdir;
    int metaPip;
};

/* free the allocated space of linked list*/
void freeLinkedList(struct inputCmd *head)
{
    struct inputCmd *ptr = head;
    while (ptr)
    {
        struct inputCmd *node = ptr->next;
        free(ptr->cmdStr);
        free(ptr);
        ptr = node;
    }
}

/* free the allocated space of array */
void freeArray(char **head)
{
    for (int i = 0; i < CMDLINE_MAXARGNUM + 1; i++) {
        free(head[i]);
    }
    free(head);
}

/* Parse the cmds in linked-list (fangyunh) */
struct inputCmd *parseCmdInList(char *cmd)
{
    int cursor = 0;
    char *token = (char *)calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    char *pos;
    int count = 0;
    struct inputCmd *head = NULL;
    struct inputCmd *tail = NULL;
    if (cmd == NULL || *cmd == '\0')
    {
        fprintf(stderr, "Empty command input.\n");
        free(token);
        return NULL;
    }
    while (1)
    {
        if (count > CMDLINE_MAXARGNUM)
        {
            fprintf(stderr, "Command exceeds the limit of number of tokens.\n");
            free(token);
            return NULL;
        }

        sscanf(cmd + cursor, "%[^ ]", token);
        if (strlen(token) > CMDLINE_MAXTOKLEN)
        {
            fprintf(stderr, "Command exceeds the limit of token max length\n");
            free(token);
            return NULL;
        }

        struct inputCmd *newNode = (struct inputCmd *)calloc(2, sizeof(struct inputCmd));
        newNode->cmdStr = (char *)calloc(strlen(token) + 1, sizeof(char));
        if (!newNode->cmdStr)
        {
            fprintf(stderr, "newNode memory allocation fail");
            free(newNode);
            // freeLinkedList(head);
            return NULL;
        }
        if (strcmp(token, "\0") != 0)
        {
            strcpy(newNode->cmdStr, token);
            newNode->next = NULL;

            if (!head)
            {
                head = newNode;
                tail = newNode;
            }
            else
            {
                tail->next = newNode;
                tail = newNode;
            }
            count++;
        }

        pos = strchr(cmd + cursor, ' ');
        while (pos != NULL && *(pos + 1) == ' ')
        {
            pos++;
        }
        if (pos == NULL)
        {
            break;
        }
        cursor = pos - cmd + 1;
    }
    free(token);

    return head;
}

/* Parse the command line in an array with a NULL in the end */
char** parseCmdInArr(char* cmd) {
    char** commands = (char**) calloc(CMDLINE_MAXARGNUM+1, sizeof(char*));
    int cursor = 0;
    char* token = (char*) calloc(CMDLINE_MAXTOKLEN, sizeof(char));
    char* pos;
    int count = 0;
    int i = 0;

    if (cmd == NULL || *cmd == '\0')
    {
        fprintf(stderr, "Empty command input.\n");
        free(token);
        return NULL;
    }

    while (cmd[i] == ' ')
    {
        i++;
    }
    cmd += i;

    while (1) {
        // check does cmd reached the end
        int status = sscanf(cmd + cursor, "%[^ ]", token);

        if (status > 0)
        {
            commands[count] = (char*) calloc(strlen(token) + 1, sizeof(char));
            strcpy(commands[count], token);
            count++;
        } else {
            break;
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
    commands[count] = NULL; // Satisfy the execvp() requirement

    return commands;
}

/* Initialize the type structure */
void typeInit(struct cmdType *type)
{
    type->binFunc = 0;
    type->oRdir = 0;
    type->metaORdir = 0;
    type->pip = 0;
    type->metaPip = 0;
}

/* Classify the command into different types */
void deterType(struct inputCmd *list, char *cmd, struct cmdType *type)
{
    char *builtin[] = {"cd", "pwd", "exit"};

    for (int i = 0; i < 3; i++)
    {
        if (strcmp(list->cmdStr, builtin[i]) == 0)
        {
            type->binFunc = 1;
        }
    }

    if (strchr(cmd, '>'))
    {
        if (strstr(cmd, ">&"))
        {
            type->metaORdir = 1;
        }
        type->oRdir = 1;
    }

    if (strchr(cmd, '|'))
    {
        if (strstr(cmd, "|&"))
        {
            type->metaPip = 1;
        }
        type->pip = 1;
    }
}

/* Return the standrd input, output, and error back to standard */
void returnStd(int saveIn, int saveOut, int saveErr)
{
    if (saveIn != -1)
    {
        dup2(saveIn, STDIN_FILENO);
    }
    close(saveIn);

    if (saveOut != -1)
    {
        dup2(saveOut, STDOUT_FILENO);
    }
    close(saveOut);

    if (saveErr != -1)
    {
        dup2(saveErr, STDERR_FILENO);
    }
    close(saveErr);
}

/* Execute regular command and $PATH environment commands */
void execRegCmd(char **instru, char *cmd, int saveIn, int saveOut, int saveErr)
{
    int retval;
    pid_t pid;
    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork error\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        execvp(instru[0], instru);
        fprintf(stderr, "execvp error\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        waitpid(pid, &retval, 0);
        returnStd(saveIn, saveOut, saveErr);
        fprintf(stderr, "+ completed '%s': [%d]\n",
                cmd, WEXITSTATUS(retval));
    }
}

/* Divide meta characters string */
void dviMeta(char *meta, char **arr, char *cmd) {
    char *curPosition = cmd;
    int count = 0;
    char *findMeta = strstr(cmd, meta);

    while (findMeta != NULL)
    {
        *findMeta = '\0';
        arr[count] = curPosition;
        count++;
        curPosition = findMeta + strlen(meta);
        findMeta = strstr(curPosition, meta);
    }
    arr[count] = curPosition;
}

/* Put the file to FD = STDOUT_FILENO */
void conductRedirection(char **file, char **instruc, char *first, int meta)
{
    pid_t pid;
    int fd;
    int status;

    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork error");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        fd = open(file[0], O_WRONLY | O_CREAT, 0644);
        if (fd < 0) {
            perror("open or create file error");
            exit(EXIT_FAILURE);
        }

        int saveOut = dup(STDOUT_FILENO);
        int saveErr = dup(STDERR_FILENO);
        dup2(fd, STDOUT_FILENO);
        if (meta)
        {
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
void outputRedirection(char *cmd, int meta)
{
    char **cmdArr = (char **) calloc(CMDLINE_MAXTOKLEN, sizeof(char *));
    if (meta) {
        dviMeta(">&", cmdArr, cmd);
    } else {
        cmdArr[0] = (char *) calloc(strlen(cmd), sizeof(char));
        cmdArr[1] = (char *) calloc(strlen(cmd), sizeof(char));
        strcpy(cmdArr[0], strtok(cmd, ">"));
        strcpy(cmdArr[1], strtok(NULL, ">"));
    }

    char **instruc = parseCmdInArr(cmdArr[0]);
    char **file = parseCmdInArr(cmdArr[1]);

    conductRedirection(file, instruc, cmdArr[0], meta);
}

/* PWD built-in function */
void builtinPWD()
{
    char buffer[MAX_BUFFER_SIZE];

    if (getcwd(buffer, sizeof(buffer)) != NULL)
    {
        printf("%s\n", buffer);
    }
    else
    {
        fprintf(stderr, "PWD error");
    }
}

/* cd built-in function */
void builtinCd(struct inputCmd *head)
{
    struct inputCmd *filePath = head->next;
    int status = chdir(filePath->cmdStr);
    if (status == -1)
    {
        fprintf(stderr, "cd error");
    }
}

/* Determine and execute bulit-in functions */
void builtinFunctions(struct inputCmd *head)
{
    if (!strcmp(head->cmdStr, "cd"))
    {
        builtinCd(head);
    }
    else if (!strcmp(head->cmdStr, "pwd"))
    {
        builtinPWD();
    }
}

int main(void)
{
    char cmd[CMDLINE_MAXCHAR];

    while (1)
    {
        char *nl;
        struct inputCmd *commandList;
        struct cmdType *type = (struct cmdType *) calloc(3, sizeof(struct cmdType));
        typeInit(type);

        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAXCHAR, stdin);
        if (cmd[strlen(cmd) - 1] != '\n')
        {
            fprintf(stderr, "Command exceeds the limit.\n");
            continue;
        }

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO))
        {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';

        /* Parse the commands in a Linked list */
        commandList = parseCmdInList(cmd);

        /* Determine the type of command */
        deterType(commandList, cmd, type);

        /* Execute the command according to their types */
        if (type->binFunc)
        {
            /* Builtin command */
            if (!strcmp(commandList->cmdStr, "exit"))
            {
                fprintf(stderr, "Bye...\n");
                fprintf(stderr, "+ completed '%s': [%d]\n",
                        cmd, 0);
                break;
            }
            builtinFunctions(commandList);
            fprintf(stderr, "+ completed '%s': [%d]\n",
                        cmd, 0);
        } else {
            if (type->oRdir)
            {
                /* Output Redirection */
                outputRedirection(cmd, type->metaORdir);
            } else if (type->pip){
                /* Pipe */
            } else {
                /* Regular command, $PATH environment commands*/
                char **cmdArr = parseCmdInArr(cmd);
                execRegCmd(cmdArr, cmd, ORIGINAL_INPUT, ORIGINAL_OUTPUT, ORIGINAL_ERROR);
                freeArray(cmdArr);
            }

        }


        /***
        pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "fork error");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // Child process
            char *instru[] = {"sh", "-c", cmd, NULL};
            execvp("sh", instru);
            fprintf(stderr, "execvp error");
            exit(EXIT_FAILURE);
        }
        else
        {
            // Parent process
            wait(&retval);
            fprintf(stderr, "+ completed '%s': [%d]\n",
                    cmd, WEXITSTATUS(retval));
        }
         ***/

        freeLinkedList(commandList);
    }

    return EXIT_SUCCESS;
}