#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define N 100

void handler(int sig) {
    printf("\nSignal %d reçu, je commence la lecture...\n", sig);
}

int main() {
    char tab[N];
    sprintf(tab, "Le message à envoyer à l'autre pipe");
    int pipefd[2];
    int write_ret;

    if (pipe(pipefd) == -1) {
        return 1;
    }

    if (fork() == 0) {
        close(pipefd[1]);
        signal(SIGINT, handler);
        pause();
        char buffer_read[10 * N];
        read(pipefd[0], buffer_read, 10 * N);
        printf("Message : %s\n", buffer_read);
        close(pipefd[0]);
        exit(0);
    } else {
        close(pipefd[0]);
        while ((write_ret = write(pipefd[1], &tab, N)) > 0) {
            sleep(1);
            printf("Valeur du write : %d\n", write_ret);
        }
        close(pipefd[1]);
        return 0;
    }
}
