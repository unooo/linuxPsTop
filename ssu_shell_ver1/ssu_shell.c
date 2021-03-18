#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> // for wait()
#include <stdbool.h>
#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

/* Splits the string by space and returns the array of tokensFwa
*
*/
int doProcess(char **tokens);
int getPipeNum(char **tokens);
void callProcess(char *orderPath, char **options);
char **tokenize(char *line)
{
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for (i = 0; i < strlen(line); i++)
    {

        char readChar = line[i];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t')
        {
            token[tokenIndex] = '\0';

            if (tokenIndex != 0)
            {
                tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        }
        else
        {
            token[tokenIndex++] = readChar;
        }
    }

    free(token);
    tokens[tokenNo] = NULL;
    return tokens;
}

int main(int argc, char *argv[])
{

    char line[MAX_INPUT_SIZE];
    char **tokens;
    int i;

    FILE *fp;
    if (argc == 2)
    {
        fp = fopen(argv[1], "r");
        if (fp < 0)
        {
            printf("File doesn't exists.");
            return -1;
        }
    }

    while (1)
    {
        /* BEGIN: TAKING INPUT */
        bzero(line, sizeof(line));
        if (argc == 2)
        { // batch mode
            if (fgets(line, sizeof(line), fp) == NULL)
            { // file reading finished
                break;
            }
            line[strlen(line) - 1] = '\0';
        }
        else
        { // interactive mode
            printf("$ ");
            scanf("%[^\n]", line);
            getchar();
        }

        line[strlen(line)] = '\n'; //terminate with new line
        tokens = tokenize(line);
        doProcess(tokens);

        for (i = 0; tokens[i] != NULL; i++)
        {
            free(tokens[i]);
        }
        free(tokens);
    }
    fclose(fp);
    return 0;
}

int doProcess(char **tokens)
{
    int pipeNum = getPipeNum(tokens);
    int fd[2 * pipeNum];
    int ret = 0;
    int startIdx = 0;
    char *const envp[] = {"PATH", NULL};
    int i;
    for (i = 0; i < pipeNum; i++)
    {
        if (pipe(fd + i * 2) < 0)
        {
            perror("파이프만들기오류");
            exit(EXIT_FAILURE);
        }
    }

    int pipeFlag = 0;
    int routineNum = 0;
    while (1)
    {

        if (tokens[startIdx] == NULL)
        {
            if (startIdx == 0)
                return 0;
        }
        char *order = tokens[startIdx];
        char *orderPath = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
        strcpy(orderPath, "");

        if (order == NULL)
        {
            return 0;
        }
        strcat(orderPath, order);

        char **options = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
        bzero(options, sizeof(options));
        options[0] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char)); //메모리누수 처리필요
        strcpy(options[0], orderPath);
        int i;
        for (i = 1; i < MAX_NUM_TOKENS; i++)
        {
            if (tokens[startIdx + i] == NULL)
            {
                int pid;
                int status = 0;
                pid = fork();
                int pidChild;
                if (pid < 0)
                {
                    exit(-1);
                }
                else if (pid == 0)
                {

                    if (pipeFlag == 1)
                    {

                        dup2(fd[(routineNum - 1) * 2], 0);
                        close(fd[(routineNum - 1) * 2]);
                        close(fd[(routineNum - 1) * 2 + 1]);
                    }

                    callProcess(orderPath, options);
                }
                else
                {
                     
                   close(fd[(routineNum - 1) * 2 + 1]);
                   close(fd[(routineNum - 1) * 2]);
                  
                      pidChild = wait(&status);
                    if (!WIFEXITED(status))
                    {
                        printf("자식프로세스 비정상 종료 %d\n", status);
                    }

                    return 0;
                }
            }
            else if (strcmp(tokens[startIdx + i], "|") == 0)
            {

                startIdx += i + 1;
                int pid;
                int status = 0;
                pid = fork();
                int pidChild;
                if (pid < 0)
                {
                    exit(-1);
                }
                else if (pid == 0)
                {
                    if (pipeFlag == 1)
                    {
                        dup2(fd[(routineNum - 1) * 2], 0);
                        close(fd[(routineNum - 1) * 2 + 1]);
                        close(fd[(routineNum - 1) * 2]);
                    }
                    
                    dup2(fd[routineNum * 2 + 1], 1);
                    close(fd[routineNum * 2 + 1]);
                    close(fd[routineNum * 2]);
                    callProcess(orderPath, options);
                }
                else
                {
                  
                    if (pipeFlag == 1)
                    {
                        close(fd[(routineNum - 1) * 2 + 1]);
                        close(fd[(routineNum - 1) * 2]);
                    }
                      pidChild = wait(&status);
                    close(fd[(routineNum - 1) * 2 + 1]);
                        close(fd[(routineNum - 1) * 2]);
                    if (!WIFEXITED(status))
                    {
                        printf("자식프로세스 비정상 종료 %d\n", status);
                    }
                    pipeFlag = 1; //여기서해야 반영됨.
                    routineNum++;
                    break;
                }
            }
            else
            {
                char *option = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
                bzero(option, sizeof(option));
                strcpy(option, tokens[startIdx + i]);
                options[i] = option;
            }
        }
        free(orderPath);
        free(options[0]);
        free(options);
    }

 
    return ret;
}

int getPipeNum(char **tokens)
{
    int ret = 0;
    int i;
    for (i = 0; i < MAX_NUM_TOKENS; i++)
    {
        if (tokens[i] == NULL)
            break;
        if (strcmp(tokens[i], "|") == 0)
            ret++;
    }
    return ret;
}
void callProcess(char *orderPath, char **options)
{
    int exeRet;
    if (strcmp(orderPath, "ttop") == 0 || strcmp(orderPath, "pps") == 0)
    {
        char pwd[1024];
        getcwd(pwd, sizeof(pwd));
        strcat(pwd, "/");
        strcat(pwd, orderPath);
        exeRet = execv(pwd, options);
    }
    else
    {
        exeRet = execvp(orderPath, options);
    }

    if (exeRet < 0)
    {
        perror("SSUShell : Incorrect command\n");
        exit(EXIT_FAILURE);
    }
    else
    {

        exit(EXIT_SUCCESS);
    }
}