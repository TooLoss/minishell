#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    int pipefd[2];
    int read_num;

    if (pipe(pipefd) == -1) {
        return 1;
    }

    if (fork() == 0) {
        close(pipefd[1]);
        while (read(pipefd[0], &read_num, sizeof(int)) > 0) {
            printf("Lecture entier boucle %d.\n", read_num);
        }
        printf("Sortie de boucle.\n");
        close(pipefd[0]);
        exit(0);
    } else {
        close(pipefd[0]);
        for (int i = -3; i < 6; i++) {
            write(pipefd[1], &i, sizeof(int));
        }
        close(pipefd[1]);
        sleep(10);
        return 0;
    }
}
