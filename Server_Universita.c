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
#include <fcntl.h>
#include <unistd.h>
#include "Common/protocol.h"

#define MAX_SIZE 1024
#define MAX_100	 100

int max_id = 1;

/*------------------------------------
   	 DEFINIZIONI DELLE STRUTTURE
------------------------------------*/

//Struttura che contiene un esame il quale è definito dal nome e dai crediti
typedef struct Esame {
	int ID;
    char corso[50];
    int crediti;
} ESAME;


/*------------------------------------
   	  PROTOTIPI DELLE FUNZIONI
------------------------------------*/
//int Socket(int namespace, int style, int protocol);		// Funzione wrapped per la socket
//void Bind(int listen_fd, struct sockaddr *addr, socklen_t lenght);	// Funzione wrapped per la bind
//void Listen(int listen_fd, int n);		// Funzione wrapped per la listen

void ricevi_esame(int connfd);
int contaEsami(const char *nomeFile);

/*------------------------------------
   	  IMPLEMENTAZIONE DEL MAIN
------------------------------------*/


int main(int argc, char const *argv[]) {

    int listen_fd, connfd;
    char bit_iniziale;
    struct sockaddr_in serv_addr;
    pid_t pid;

    // Creo il descrittore del socket, valorizzo la struttura server_addr, assegno la porta al server e metto il socket in ascolto

    listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(1025);

    Bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    Listen(listen_fd, 1024);

    for (;;) {

    printf("[Server universitario] Attendo nuove richieste\n");

        connfd = Accept(listen_fd, NULL, NULL);

        // Creo il figlio con la fork per gestire la connessione accettata in precedenza
        if ((pid = fork()) < 0) {
            perror("fork() error");
            exit(1);
        }
		
	    // Codice eseguito dal figlio per gestire la richiesta
        if (pid == 0) {

            close(listen_fd);

            //Ricevo il bit iniziale: se = 0, la segreteria vuole aggiungere un esame; se = 1, la segeteria inoltra una prenotazione
            FullRead(connfd, &bit_iniziale, sizeof(char));

            if(bit_iniziale == '1')
            	ricevi_esame(connfd);
            else
            	//ricevi_prenotazione(connfd);

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

void ricevi_esame(int connfd){

	ESAME esame;

	FullRead(connfd, &esame, sizeof(ESAME));

	memorizza_esame(esame);

	printf("\nHo ricevuto il seguente esame %s - %d", esame.corso, esame.crediti);

	close(connfd);

}

void memorizza_esame(ESAME esame){

	int fd = open("esami.txt", O_WRONLY | O_APPEND, 0777);

	dprintf(fd, "%d/%s/%d\n", esame.ID, esame.corso, esame.crediti);

	close(fd);
}

int contaEsami(const char *nomeFile){

	int fd = open(nomeFile, O_RDONLY);

	 if (fd < 0) {
		 perror("open() error");
	     	 exit(1);
	 }

	 int conteggio = 0;
	 char c;

	 while(read(fd, &c, 1) > 0){
		 if (c == '\n')
			 conteggio++;
	 }

	 close(fd);

	 return conteggio;

}


