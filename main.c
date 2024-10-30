#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

void split_command(char *cmd, char **argv)
{
    char *token = strtok(cmd, " ");
    int i = 0;
    while (token != NULL)
    {
        argv[i++] = token;
        token = strtok(NULL, " ");
    }
    argv[i] = NULL;
}

char *find_command_path(char *cmd)
{
    char *path = getenv("PATH");
    char *token = strtok(path, ":");
    static char full_path[1024];

    while (token != NULL)
    {
        snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd);
        if (access(full_path, X_OK) == 0)
        {
            return full_path;
        }
        token = strtok(NULL, ":");
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <inp> <cmd> <out>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *inp = argv[1];
    char *cmd = argv[2];
    char *out = argv[3];

    int inp_fd = STDIN_FILENO;
    int out_fd = STDOUT_FILENO;

    if (strcmp(inp, "-") != 0)
    {
        inp_fd = open(inp, O_RDONLY);
        if (inp_fd < 0)
        {
            perror("open input file");
            exit(EXIT_FAILURE);
        }
    }

    if (strcmp(out, "-") != 0)
    {
        out_fd = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0)
        {
            perror("open output file");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        if (dup2(inp_fd, STDIN_FILENO) < 0)
        {
            perror("dup2 input");
            exit(EXIT_FAILURE);
        }
        if (dup2(out_fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 output");
            exit(EXIT_FAILURE);
        }

        char *cmd_argv[1024];
        split_command(cmd, cmd_argv);

        char *cmd_path = find_command_path(cmd_argv[0]);
        if (cmd_path == NULL)
        {
            fprintf(stderr, "Command not found: %s\n", cmd_argv[0]);
            exit(EXIT_FAILURE);
        }

        execv(cmd_path, cmd_argv);
        perror("execv");
        exit(EXIT_FAILURE);
    } else
    {
        wait(NULL);
    }

    if (inp_fd != STDIN_FILENO) close(inp_fd);
    if (out_fd != STDOUT_FILENO) close(out_fd);

    return 0;
}