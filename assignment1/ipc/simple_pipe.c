#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[100];
    const char *message = "Hello from parent!";

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        close(pipefd[1]);
        ssize_t bytes = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytes == -1) {
            perror("read failed");
            return 1;
        }
        buffer[bytes] = '\0';
        printf("Child received: %s\n", buffer);
        close(pipefd[0]);
    } else {
        close(pipefd[0]);
        write(pipefd[1], message, strlen(message));
        close(pipefd[1]);
        wait(NULL);
    }

    return 0;
}
