#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

volatile sig_atomic_t shutdown_flag = 0;
volatile sig_atomic_t usr1_flag = 0;

static void handle_sigint(int sig) {
    (void)sig;
    shutdown_flag = 1;
}

static void handle_sigusr1(int sig) {
    (void)sig;
    usr1_flag = 1;
}

int main(int argc, char *argv[]) {
    long max_lines = -1;
    int verbose = 0;

    int opt;
    while ((opt = getopt(argc, argv, "n:v")) != -1) {
        switch (opt) {
            case 'n':
                max_lines = atol(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n max_lines] [-v]\n", argv[0]);
                return 1;
        }
    }

    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_usr1;
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    unsigned long long lines = 0;
    unsigned long long chars = 0;

    int c;
    while (!shutdown_flag && (c = getchar()) != EOF) {
        chars++;

        if (verbose) putchar(c);

        if (c == '\n') {
            lines++;
            if (max_lines >= 0 && (long)lines >= max_lines) break;
        }

        if (usr1_flag) {
            fprintf(stderr, "[consumer] lines=%llu chars=%llu\n", lines, chars);
            usr1_flag = 0;
        }
    }

    fprintf(stderr, "[consumer] done. lines=%llu chars=%llu\n", lines, chars);
    return 0;
}
