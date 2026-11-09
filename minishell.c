#include <stdio.h>
#include <stdlib.h>
#include <readcmd.h>
#include <stdbool.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

unsigned short int is_pid_fg;
pid_t fg_pid;
int pipe_previous = -1;
int fg_termine = 0;

// Methode pour changer repertoire avec une commande
// NULL : retour à $HOME
// !NULL : chemin = commande
void changer_repertoire(char *commande) {
    char *chemin = commande == NULL ? getenv("$HOME") : commande;
    if ( chdir(chemin) == -1 ) {
        printf("Erreur changer de répertoire");
    }
}

// Methode affiche le répertoire en paramètre
void afficher_repertoire(char *repertoire) {
    if (repertoire == NULL) {
        repertoire = ".";
    }

    DIR *dir = opendir(repertoire);
    if (dir == NULL) {
        printf("Erreur repertoire introuvable lire");
    }

    struct dirent *entree;
    while ( (entree = readdir(dir)) != NULL ) {
        printf("%s\n", entree->d_name);
    }

    if (closedir(dir) == -1) {
        printf("Erreur dossier introuvable");
    }
}

// Récupère les signaux
void traitement(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED|WCONTINUED)) > 0) {
        if (pid == fg_pid) {
            // signal reçu processus au premier plan
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                // Mort normal
                fg_termine = 1; // fg terminé, on débloque le Shell
                printf("\n[Fils %d] Terminé signal %d\n", pid, sig);
            } else if (WIFSTOPPED(status)) {
                printf("\n[Fils %d] Suspendu\n", pid);
                fg_termine = 1;
            } else if (WIFCONTINUED(status)) {
                printf("\n[Fils %d] Repris\n", pid);
            }
        } else {
            // signal terminaison processus arrière plan
            if (WIFEXITED(status)) printf("\n[Arrière-plan %d] Terminé\n", pid);
        }
    }
}

// Modifie les signaux, permet de les bloquer
void modifierSignaux(int how) {
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGINT);
    sigaddset(&sig_mask, SIGTSTP);
    sigprocmask(how, &sig_mask, NULL);
}

// Récupérer les signaux et rediriger vers méthode traitement
void traiter_signal() {
    struct sigaction act;
    act.sa_handler = &traitement;
    sigset_t mask;
    sigemptyset(&mask);
    act.sa_mask = mask;
    act.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &act, NULL) == -1) {
        exit(EXIT_FAILURE);
    }
}

// Redirection entrée standart et sortie standart avec ">"
void redirection_entree_sortie(cmdline cmd) {
    if (cmd->out != NULL) {
        int fileOut;
        if ( (fileOut = open(cmd->out, O_WRONLY | O_CREAT | O_TRUNC, 0644)) != -1) {
            dup2(fileOut, STDOUT_FILENO);
            close(fileOut);
        }
    }
    if (cmd->in != NULL) {
        int fileIn;
        if ( (fileIn = open(cmd->in, O_RDONLY)) != -1) {
            dup2(fileIn, STDIN_FILENO);
            close(fileIn);
        }
    }
}

// Prend une commande et l'execute. Gestion du pipe, backgrounded
void executer_commande(cmdline cmd, char *commande[], int exist_next_command) {
    int pid_fils;
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        printf("[PIPE] Impossible d'ouvrire le pipe.");
    }

    if ((pid_fils = fork()) == 0) {
        // ENFANT
        // Débloquer SIGINT/SIGTSTP pour le premier plan
        modifierSignaux(SIG_UNBLOCK);

        if (cmd->backgrounded) {
            // désactive signal pour fils en arrière plan
            setpgrp();
            signal(SIGINT, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);
        }

        // si pipe_previous == -1, il y a pas de pipe à faire
        if (pipe_previous != -1) {
            // Valeur non initiale de pipe_previous
            // C'est donc pas la première commande
            // On lie donc l'entrée et la sortie
            dup2(pipe_previous, STDIN_FILENO);
            close(pipe_previous);
        }
        if (exist_next_command) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
        }

        redirection_entree_sortie(cmd);
        execvp(commande[0], commande);
        perror("Erreur de execvp");
        exit(EXIT_FAILURE);
    }

    // PERE

    if (pipe_previous != -1) {
        close(pipe_previous);
    }
    if (exist_next_command) {
        // Enregistre la sortie chez le père pour le mettre sur l'autre processus
        close(pipefd[1]);
        pipe_previous = pipefd[0];
    } else {
        // Pas de commande après, on lie à rien
        pipe_previous = -1;
    }

    if (!cmd->backgrounded) {
        // enregistre pid du fg (pour interrompre le fg uniquement)
        fg_pid = pid_fils;
        // bloque tant que le fg est pas terminé
        fg_termine = 0;
        while (!fg_termine) {
            pause();
        }
    } else {
        printf("[Processus %d] mis en arrière-plan\n", pid_fils);
    }
}

int main(void) {
    bool fini= false;
    char *ligne_commande;
    cmdline cmd; 
    cmderror err;
    is_pid_fg = 0;

    traiter_signal();
    modifierSignaux(SIG_BLOCK);

    while (!fini) {
        ligne_commande=  readline("sh -3.2 $ ");
        err= readcmd(ligne_commande, &cmd); // Interprétation de la ligne lue
        free(ligne_commande);               // readline effectue un malloc -> liberation de la memoire

        if (err != cmdNoError) {
            // Traitement d'erreur
            // fprintf(stderr, "erreur lecture commande: %s\n", cmd_errstring(err));
            exit(EXIT_FAILURE);
        } else {
            cmdfragment* les_commandes= cmd->fragments;
            while (les_commandes && !fini) {
                char **une_commande= les_commandes->command; // on récupère une commande
                if (strcmp(une_commande[0], "exit") == 0) {
                    fini= true;
                    printf("Au revoir ...\n");
                } else if (strcmp(une_commande[0], "cd") == 0) {
                    changer_repertoire(une_commande[1]);
                } else if (strcmp(une_commande[0], "rep") == 0) {
                    afficher_repertoire(une_commande[1]);
                } else {
                    executer_commande(cmd, une_commande, les_commandes->next ? 1 : 0);
                }
                les_commandes= les_commandes->next; // on passe à la commande suivante
            }
            cmd_dispose(cmd);
        }
    }
    return EXIT_SUCCESS;
}
