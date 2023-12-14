/*----------------------------------------------------------------------------
file: Server_Segreteria.c
autore: Francesco Picone

Implementa il server della segreteria
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "Common/protocol.h"

#define MAX_SIZE 1024

/*------------------------------------
   	  PROTOTIPI DELLE FUNZIONI
------------------------------------*/

void rispondi_studente(int connfd);

/*------------------------------------
   	  IMPLEMENTAZIONE DEL MAIN
------------------------------------*/


int main(int argc, char const *argv[]) {

    int listen_fd, connfd;

    struct sockaddr_in serv_addr;
    pid_t pid;

    // Creo il descrittore del socket, valorizzo la struttura server_addr, assegno la porta al server e metto il socket in ascolto

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(1024);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind() error");
        exit(1);
    }

    if (listen(listen_fd, 1024) < 0) {
        perror("listen() error");
        exit(1);
    }

    for (;;) {

    printf("[Segreteria] Attendo nuove richieste\n");

        // Accetto una nuova connessione
        if ((connfd = accept(listen_fd, (struct sockaddr *)NULL, NULL)) < 0) {
            perror("accept() error");
            exit(1);
        }

        // Creo il figlio con la fork per gestire la connessione accettata in precedenza
        if ((pid = fork()) < 0) {
            perror("fork() error");
            exit(1);
        }
		
	    // Codice eseguito dal figlio per gestire la richiesta
        if (pid == 0) {

            close(listen_fd);

            // Gestisco l'utente connesso con la funzione dichiarata in precedenza gestisci_utente

            rispondi_studente(connfd);
            close(connfd);
            exit(0);

        } else
        	close(connfd);
    }
    
    exit(0);
}


/*------------------------------------
   IMPLEMENTAZIONE DELLE FUNZIONI
------------------------------------*/

void rispondi_studente(int connfd){

	char welcome_message[] = "Benvenuto nella segreteria studenti!";
	int welcome_size = strlen(welcome_message) + 1;

	//Invio la dimensione in byte del messaggio di benvenuto
	if(full_write(connfd, &welcome_size, sizeof(int)) < 0) {
		perror("full_write() error");
	    exit(1);
	}

	// Invio il messaggio di benvenuto contenuto in welcome_message
	if(full_write(connfd, welcome_message, welcome_size) < 0) {
		perror("full_write() error");
	    exit(1);
	}

	close(connfd);

}
