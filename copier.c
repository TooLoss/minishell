#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 4096

void error_open(char *file) {
    char msg[100];
    sprintf(msg, "Erreur à l'ouverture du fichier %s \n", file);
    write(STDERR_FILENO, msg, strlen(msg));
}

int main(int argc, char *argv[]) {
    int desc_in;
    int desc_out;

    if ( (desc_in = open(argv[1], O_RDONLY)) == -1 ) {
        error_open(argv[1]);
        exit(EXIT_FAILURE);
    }

    if ( (desc_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
        error_open(argv[2]);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFSIZE];
    ssize_t nb_lus;

    while ( (nb_lus = read(desc_in, buffer, BUFSIZE)) > 0 ) {
        ssize_t nb_ecrit = 0;
        while (nb_ecrit < nb_lus) {
            ssize_t ret;
            if ( (ret = write(desc_out, buffer + nb_ecrit, nb_lus - nb_ecrit)) == -1 ) {
                close(desc_out);
                close(desc_in);
                exit(EXIT_FAILURE);
            }
            nb_ecrit += ret;
        }
    }

    close(desc_out);
    close(desc_in);

    return EXIT_SUCCESS;
}
