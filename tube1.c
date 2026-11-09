#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    pid_t pid;
    int pipefd[2];
    int nb_envoyer = 10;
    int nb_recu;

    if (pipe(pipefd) == -1) {
        return 1;
    }

    if ((pid = fork()) == 0) {
        close(pipefd[1]); // ferme écriture
        read(pipefd[0], &nb_recu, sizeof(int));
        printf("Reçu : %d\n", nb_recu);
        close(pipefd[0]);
    } else {
        close(pipefd[0]); // ferme lecture
        write(pipefd[1], &nb_envoyer, sizeof(int));
        close(pipefd[1]);
        wait(NULL); // attendre le fils
    }
}
