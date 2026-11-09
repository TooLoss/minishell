#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#define BUFFSIZE 4096
#define NB_PAGE 3

int main() {
    int desc_tempo;
    char buffer[BUFFSIZE];
    pid_t fils;

    /* Ecriture de tempo */
    if ( (desc_tempo = open("./tempo", O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1 ) {
        perror("Impossible d'ouvrir le fichier tempo");
        exit(1);
    }

    for (int i = 0; i < BUFFSIZE; i++) {
        buffer[i] = 'a';
    }

    for (int page = 0; page < NB_PAGE; page++) {
        if ( (write(desc_tempo, &buffer, BUFFSIZE)) == -1) {
            perror("Erreur d'écriture");
            exit(1);
        }
    }

    close(desc_tempo);

    /* Ouverture de tempo en lecture */
    if ( (desc_tempo = open("./tempo", O_RDWR )) == -1 ) {
        perror("Impossible d'ouvrir tempo après écriture.");
        exit(1);
    }

    if ((fils = fork()) == 0) {
        // enfant
        char *addr;
        addr = mmap(NULL, NB_PAGE*BUFFSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, desc_tempo, 0);
        sleep(4);
        printf("Affichage 1 du fils\n");
        write(STDOUT_FILENO, addr, 1);
        printf("\n");
        write(STDOUT_FILENO, addr + BUFFSIZE, 1);
        printf("\n");
        write(STDOUT_FILENO, addr + 2*BUFFSIZE, 1);
        printf("\n");
        for (int page = 1; page < 2; page++) {
            for (int i = 0; i < BUFFSIZE; i++) {
                addr[i + page*BUFFSIZE] = 'd';
            }
        }
        printf("Affichage 2 du fils\n");
        write(STDOUT_FILENO, addr, 1);
        printf("\n");
        write(STDOUT_FILENO, addr + BUFFSIZE, 1);
        printf("\n");
        write(STDOUT_FILENO, addr + 2*BUFFSIZE, 1);
        printf("\n");
        sleep(8);
        munmap(addr, NB_PAGE*BUFFSIZE);
    } else {
        // parent
        char *addr;
        addr = mmap(NULL, NB_PAGE*BUFFSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, desc_tempo, 0);
        sleep(2);
        for (int page = 0; page < 2; page++) {
            for (int i = 0; i < BUFFSIZE; i++) {
                addr[i + page*BUFFSIZE] = 'b';
            }
        }
        sleep(6);
        printf("Affichage 1 du père\n");
        write(STDOUT_FILENO, addr, 1);
        printf("\n");
        write(STDOUT_FILENO, addr + BUFFSIZE, 1);
        printf("\n");
        for (int page = 1; page < 2; page++) {
            for (int i = 0; i < BUFFSIZE; i++) {
                addr[i + page*BUFFSIZE] = 'c';
            }
        }
        munmap(addr, NB_PAGE*BUFFSIZE);
    }

    close(desc_tempo);
}

/* Réponses */

/* L'affichage du fils devrait être bba puis bda. L'affichage du père devrait
 * être bba. Le fichier final devrai être bda car le fils effectue les modifs
 * en dernier. */
