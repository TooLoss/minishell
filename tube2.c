#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    pid_t pid;
    int pipefd[2];
    int nb_envoyer = 10;
    int nb_recu;

    // crée tube
    if (pipe(pipefd) == -1) {
        return 1;
    }

    // écrit entier dans le tube
    write(pipefd[1], &nb_envoyer, sizeof(int));

    // crée un processus fils
    if ((pid = fork()) == 0) {
        close(pipefd[1]); // ferme écriture
        // fils lit dans le tube
        read(pipefd[0], &nb_recu, sizeof(int));
        printf("Reçu : %d\n", nb_recu);
        close(pipefd[0]);
    } else {
        close(pipefd[1]);
        close(pipefd[0]);
    }
}
