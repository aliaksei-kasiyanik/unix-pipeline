#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
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

int spawnProcess(int i, int pipefds[][2], int pCount, char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        for (int j = 0; j < i - 1; ++j) {
            if (close(pipefds[j][0]) < 0 ||
                close(pipefds[j][1]) < 0) {
                fprintf(stderr, "ERR: Close\n");
                return 1;
            }
        }

        if (i != pCount - 1) {
            if (dup2(pipefds[i][1], 1) < 0) {
                fprintf(stderr, "ERR: dup2\n");
                return 1;
            }
            if (close(pipefds[i][0]) < 0) {
                fprintf(stderr, "ERR: close\n");
                return 1;
            }
        }

        if (i != 0) {
            if (dup2(pipefds[i - 1][0], 0) < 0) {
                fprintf(stderr, "ERR: dup2\n");
                return 1;
            }
            if (close(pipefds[i - 1][1]) < 0) {
                fprintf(stderr, "ERR: close\n");
                return 1;
            }
        }


        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "ERR: Wrong command: %s \n", args[0]);
            return 1;
        }
    }
    return pid;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "ERR: Pass arguments, please\n");
        return 2;
    }

    int pCount = countPipeProcesses(argc, argv);

    char ***PP_ARGS = new char **[pCount];
    int parseCode = parsePipeArgs(argc, argv, pCount, PP_ARGS);
    if (parseCode == 1) {
        fprintf(stderr, "ERR: Invalid arguments\n");
        return 2;
    }

    pid_t children[pCount - 1];
    int pipefds[pCount - 1][2];

    for (int i = 0; i < pCount; ++i) {

        if (pipe(pipefds[i]) == -1) {
            fprintf(stderr, "ERR: Pipe\n");
            return 1;
        }

        pid_t pid = spawnProcess(i, pipefds, pCount, PP_ARGS[i]);

        if (pid > 0) {
            children[i] = pid;
        } else {
            fprintf(stderr, "ERR: Fork\n");
            return 1;
        }
    }

    for (int i = 0; i < pCount - 1; ++i) {
        if (close(pipefds[i][0]) < 0 ||
            close(pipefds[i][1]) < 0) {
            fprintf(stderr, "ERR: Close\n");
            return 1;
        }

    }

    int exitCode = 0;
    int status;
    for (int j = 0; j < pCount; ++j) {
        waitpid(children[j], &status, 0);
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                exitCode = 1;
            }
        }
    }

    for (int i = 0; i < pCount; ++i) {
        delete[] PP_ARGS[i];
    }
    delete[] PP_ARGS;

    return exitCode;
}



