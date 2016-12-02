#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

const char *DELIMITER = "|";

int countPipeProcesses(int argc, char *const *argv) {
    int pipeProcessCount = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], DELIMITER) == 0) {
            pipeProcessCount++;
        }
    }
    return pipeProcessCount;
}

int parsePipeArgs(int argc, char **argv, int pipeProcessCount, char ***PP_ARGS) {
    int ppArgsCount[pipeProcessCount];
    int ppIndex = 0;
    int argsCount = 0;
    int exitCode = 0;
    for (int i = 1; i <= argc; i++) {
        if (i == argc || strcmp(argv[i], DELIMITER) == 0) {
            if (argsCount == 0) {
                exitCode = 1;
                break;
            }
            PP_ARGS[ppIndex] = new char *[argsCount + 1]; // +1 for last NULL arg for execvp
            PP_ARGS[ppIndex][argsCount] = NULL;
            for (int j = 0; j < argsCount; j++) {
                PP_ARGS[ppIndex][j] = argv[i - argsCount + j];
            }
            ppArgsCount[ppIndex] = argsCount + 1;
            argsCount = 0;
            ppIndex++;
        } else {
            argsCount++;
        }
    }
    if (exitCode == 1) {
        for (int i = 0; i < ppIndex; i++) {
            delete []PP_ARGS[i];
        }
        delete []PP_ARGS;
    }
    return exitCode;
}

int spawnProcess(int in, int out, char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        if (in != 0) {
            dup2(in, 0);
            close(in);
        }
        if (out != 1) {
            dup2(out, 1);
            close(out);
        }
        execvp(args[0], args);
    }
    return pid;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "ERR: Pass arguments, please\n");
        return 1;
    }

    int pCount = countPipeProcesses(argc, argv);

    char ***PP_ARGS = new char **[pCount];
    int parseCode = parsePipeArgs(argc, argv, pCount, PP_ARGS);
    if (parseCode == 1) {
        fprintf(stderr, "ERR: Invalid arguments\n");
        return 1;
    }

    pid_t children[pCount - 1];
    int in, fd[2];

    /* The first process should get its input from the original file descriptor 0.  */
    in = 0;

    /* Note the loop bound, we spawn here all, but the last stage of the pipeline.  */
    int i;
    for (i = 0; i < pCount - 1; ++i) {
        pipe(fd);

        /* f [1] is the write end of the pipe, we carry `in` from the prev iteration.  */
        children[i] = spawnProcess(in, fd[1], PP_ARGS[i]);

        /* No need for the write end of the pipe, the child will write here.  */
        close(fd[1]);

        /* Keep the read end of the pipe, the next child will read from there.  */
        in = fd[0];
    }

    /* Last stage of the pipeline - set stdin be the read end of the previous pipe
       and output to the original file descriptor 1. */
    if (in != 0) {
        dup2(in, 0);
    }

    int exitCode = 0;
    int status;
    for (int j = 0; j < pCount - 1; ++j) {
        waitpid(children[j], &status, 0);
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                exitCode = 1;
            }
        }
    }
    if (exitCode == 1) {
        return exitCode;
    }

    /* Execute the last stage with the current process. */
    return execvp(PP_ARGS[i][0], PP_ARGS[i]);
}



