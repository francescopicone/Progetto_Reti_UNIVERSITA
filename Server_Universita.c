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
#define MAX_CORSI 128

/*------------------------------------
   	 DEFINIZIONI DELLE STRUTTURE
------------------------------------*/

//Struttura che contiene un esame il quale è definito dal nome e dai crediti
typedef struct Corso {
	int ID;
    char nome[50];
    int crediti;
} CORSO;

typedef struct Esame{
	int ID;
	CORSO corso;

} ESAME;



/*------------------------------------
   	  PROTOTIPI DELLE FUNZIONI
------------------------------------*/
//int Socket(int namespace, int style, int protocol);		// Funzione wrapped per la socket
//void Bind(int listen_fd, struct sockaddr *addr, socklen_t lenght);	// Funzione wrapped per la bind
//void Listen(int listen_fd, int n);		// Funzione wrapped per la listen

void ricevi_esame(int connfd);
int contaEsami(const char *nomeFile);
ESAME *creaPacchettoEsami(const char *nomeFile);
void inviaCorsiSegreteria(const char *nomeFile, int connfd);

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

            //Ricevo il bit iniziale: se = 1, la segreteria vuole aggiungere un esame; se = 0, la segeteria inoltra una prenotazione
            FullRead(connfd, &bit_iniziale, sizeof(char));


            if(bit_iniziale == '1'){
            	ricevi_esame(connfd);
            }
            else if (bit_iniziale == '2')
            	inviaCorsiSegreteria("esami.txt", connfd);

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

	esame.ID = contaEsami("esami.txt")+1;

	memorizza_esame(esame);

	printf("[Server universitario] \033[1;32mHo aggiunto il seguente esame %s - %d\033[1;0m\n", esame.corso.nome, esame.corso.crediti);

	close(connfd);

}

void memorizza_esame(ESAME esame){

	int fd = open("esami.txt", O_WRONLY | O_APPEND, 0777);

	if(flock(fd, LOCK_EX) < 0) {
		perror("flock() error");
	    exit(1);
	}

	dprintf(fd, "%d;%s;%d\n", esame.ID, esame.corso.nome, esame.corso.crediti);

	if (flock(fd, LOCK_UN) < 0) {
	    perror("flock() unlock error");
	    exit(1);
	}

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

ESAME *creaPacchettoEsami(const char *nomeFile){

	int numero_esami = contaEsami("esami.txt");
	ESAME *esami = (ESAME *)malloc(numero_esami*sizeof(CORSO));
	char *buffer = (char *)malloc(MAX_SIZE);

	int fd = open("esami.txt", O_RDONLY);

	if(flock(fd, LOCK_EX) < 0) {
			perror("flock() error");
		    exit(1);
	}

	lseek(fd, 0, SEEK_SET);

	int conteggio = 0, i = 0;
	char c;

	while(read(fd, &c, 1) > 0){
		if (c == '\n'){

			buffer[conteggio] = '\0';
			sscanf(buffer, "%d;%49[^;];%d", &esami[i].ID, esami[i].corso.nome, &esami[i].corso.crediti);
			i++;

			free(buffer);
			buffer = (char *)malloc(MAX_SIZE);
			conteggio = 0;

		}
		else {
			buffer[conteggio] = c;
			conteggio++;
		}

	}

	if (flock(fd, LOCK_UN) < 0) {
		    perror("flock() unlock error");
		    exit(1);
	}

	close(fd);

	return esami;

}

void inviaCorsiSegreteria(const char *nomeFile, int connfd){

	CORSO tmp_corsi[MAX_CORSI];
	int numCorsi = 0, duplicato=0, conteggio=0, i=0;
	char c, *buffer = (char *)malloc(MAX_SIZE);

	int fd = open(nomeFile, O_RDONLY);

	while(read(fd, &c, 1) > 0){

		int duplicato = 0;

		if (c == '\n'){
			buffer[conteggio] = '\0';

			sscanf(buffer, "%d;%49[^;];%d", &tmp_corsi[i].ID, tmp_corsi[i].nome, &tmp_corsi[i].crediti);

			for(int j=0; j<numCorsi; j++){
				if(strcmp(tmp_corsi[i].nome, tmp_corsi[j].nome) == 0){
					duplicato = 1;
					break;
				}
			}

			if(!duplicato){
				i++;
				numCorsi++;
			}

			free(buffer);
			buffer = (char *)malloc(MAX_SIZE);
			conteggio = 0;
		}

		else {
			buffer[conteggio] = c;
			conteggio++;
		}

	}

	CORSO *esami = malloc(numCorsi*sizeof(CORSO));

	for(int i=0; i<numCorsi; i++){
			strcpy(esami[i].nome, tmp_corsi[i].nome);
			printf("%s, ", esami[i].nome);
	}

	close(fd);

	FullWrite(connfd, &numCorsi, sizeof(int));
	FullWrite(connfd, esami, numCorsi*sizeof(CORSO));

}
