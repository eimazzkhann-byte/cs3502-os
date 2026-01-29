#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int pipe1[2];
    int pipe2[2];
    pid_t pid;

    if (pipe(pipe1) == -1) { perror("pipe1"); return 1; }
    if (pipe(pipe2) == -1) { perror("pipe2"); return 1; }

    pid = fork();
    if (pid == -1) { perror("fork"); return 1; }

    if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        char buf[200];
        ssize_t n = read(pipe1[0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("Child got: %s\n", buf);
        }

        const char *reply = "Hello parent, message received!";
        write(pipe2[1], reply, strlen(reply));

        close(pipe1[0]);
        close(pipe2[1]);
    } else {
        close(pipe1[0]);
        close(pipe2[1]);

        const char *msg = "Hello child, this is parent.";
        write(pipe1[1], msg, strlen(msg));

        char buf[200];
        ssize_t n = read(pipe2[0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("Parent got: %s\n", buf);
        }

        close(pipe1[1]);
        close(pipe2[0]);

        wait(NULL);
    }

    return 0;
}
