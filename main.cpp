#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

char* ARGS[2][5] = {
        { "ls", "-l", "/etc/", NULL },
        { "wc", "-l", NULL }
};

int main() {
    int pipefds[2];
    if (pipe(pipefds) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t children[2];

    for (int i = 0; i < 2; ++i) {
        pid_t pid = fork();

        if (pid == 0) {
            char** args = ARGS[i];
            if (i == 0) {
                dup2(pipefds[1], 1);
                //close(pipefds[1]);
                close(pipefds[0]);
            } else if (i == 1) {
                dup2(pipefds[0], 0);
                //close(pipefds[0]);
                close(pipefds[1]);
            }
            execvp(args[0], args);
        } else {
            children[i] = pid;
        }
    }

    for (int i = 0; i < 2; ++i) {
        close(pipefds[i]);
    }

    int exitCode = 0;
    int status;
    for (int i = 0; i < 2; ++i) {
        waitpid(children[i], &status, 0);
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                exitCode = 1;
            }
        }
    }

    return exitCode;
}
