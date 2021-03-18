#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

#define MAX_CMD_SIZE 10
#define MAX_ARG_SIZE 10
#define MAX_STR_SIZE 64

char **tokenize(char *line);
char ***tokenToCommand(char **tokens);
void doCommand(char ***cmd);

int main(int argc, char* argv[])
{
    FILE *fp;
    pid_t pId;
    int i = 0, j = 0;
    char line[MAX_INPUT_SIZE];      
    char **tokens, ***cmd;

    if (argc == 2) {
        fp = fopen(argv[1], "r");
        if (fp < 0) {
            printf("File doesn't exists.");
            return -1;
        }
    }

    while (1) {
        bzero(line, sizeof(line));
        if (argc == 2) { 
            if (fgets(line, sizeof(line), fp) == NULL) { 
                break;	
            }
            line[strlen(line) - 1] = '\0';
        } 
        else { 
            printf("$ ");
            scanf("%[^\n]", line);
            getchar();
        }
        line[strlen(line)] = '\n'; 
        tokens = tokenize(line);
        cmd = tokenToCommand(tokens);
        doCommand(cmd);

        for (i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }

    return 0;
}

char **tokenize(char *line)
{
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for (i = 0; i < strlen(line); i++) {
        char readChar = line[i];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t') {
            token[tokenIndex] = '\0';
            if (tokenIndex != 0) {
                tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0; 
            }
        } 
        else {
            token[tokenIndex++] = readChar;
        }
    }

    free(token);
    tokens[tokenNo] = NULL;
    return tokens;
}

char ***tokenToCommand(char **tokens)
{
    char ***cmd = (char ***)malloc(MAX_CMD_SIZE * sizeof(char **));
    int tok_idx = 0, cmd_idx = 0, arg_idx = 0;

    for (int i = 0; i < MAX_CMD_SIZE; i++) {
        cmd[i] = (char **)malloc(MAX_ARG_SIZE * sizeof(char *));
        for (int j = 0; j < MAX_ARG_SIZE; j++) {
            cmd[i][j] = (char *)malloc(MAX_STR_SIZE * sizeof(char));
        }
    }

    while (tokens[tok_idx] != NULL) {
        if (tokens[tok_idx][0] == '|') {
            cmd[cmd_idx][arg_idx] = NULL;
            arg_idx = 0;
            cmd_idx++;
            tok_idx++;
        }
        cmd[cmd_idx][arg_idx++] = tokens[tok_idx++];
    }
    cmd[cmd_idx][arg_idx] = NULL;
    cmd[++cmd_idx] = NULL;

    return cmd;
}

void doCommand(char ***cmd)
{
    int fd[2];
    pid_t pid;
    int tempFd = 0;

    while (*cmd != NULL) {
        if (pipe(fd) < 0) {
            fprintf(stderr, "pipe error\n");
            exit(1);
        }

        if ((pId = fork()) < 0) {
            fprintf(stderr, "fork error\n");
            exit(1);
        }

        else if (pId == 0) {
            dup2(tempFd, 0); 
            if (*(cmd + 1) != NULL) {
                dup2(fd[1], 1);
            }                
            close(fd[0]);
            execv((*cmd)[0], *cmd);
            exit(1);
        }

        else {
            wait(NULL);
            close(fd[1]);
            tempFd = fd[0]; 
            cmd++;
        }
    }
}

