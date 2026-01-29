#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
    char *filename = NULL;
    int buffer_size = 4096;

    int opt;
    while ((opt = getopt(argc, argv, "f:b:")) != -1) {
        switch (opt) {
            case 'f':
                filename = optarg;
                break;
            case 'b':
                buffer_size = atoi(optarg);
                if (buffer_size <= 0) buffer_size = 4096;
                break;
            default:
                fprintf(stderr, "Usage: %s [-f file] [-b size]\n", argv[0]);
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

    FILE *in = stdin;
    if (filename != NULL) {
        in = fopen(filename, "r");
        if (in == NULL) {
            perror("fopen failed");
            return 1;
        }
    }

    char *buf = malloc(buffer_size);
    if (buf == NULL) {
        fprintf(stderr, "malloc failed\n");
        if (in != stdin) fclose(in);
        return 1;
    }

    size_t total_bytes = 0;

    while (!shutdown_flag) {
        size_t n = fread(buf, 1, buffer_size, in);
        if (n > 0) {
            fwrite(buf, 1, n, stdout);
            total_bytes += n;
        }
        if (n < (size_t)buffer_size) {
            if (feof(in)) break;
            if (ferror(in)) {
                fprintf(stderr, "read error\n");
                break;
            }
        }

        if (usr1_flag) {
            fprintf(stderr, "[producer] bytes_sent=%zu\n", total_bytes);
            usr1_flag = 0;
        }
    }

    fprintf(stderr, "[producer] exiting. bytes_sent=%zu\n", total_bytes);

    free(buf);
    if (in != stdin) fclose(in);
    return 0;
}
