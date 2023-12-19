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

//Struttura che contiene informazioni sul corso il quale è definito da ID, nome e crediti
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

void rispondi_studente(int connfd);
void invia_esame_server_u();
int contaEsami(const char *nomeFile);
CORSO crea_pacchetto_esami();
CORSO *richiediCorsiServerU(int *numCorsi);

/*------------------------------------
   	  IMPLEMENTAZIONE DEL MAIN
------------------------------------*/


int main(int argc, char const *argv[]) {

    int listen_fd, connfd, scelta;

    struct sockaddr_in serv_addr;
    pid_t pid;

    // Creo il descrittore del socket, valorizzo la struttura server_addr, assegno la porta al server e metto il socket in ascolto

    listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(1024);

    Bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    Listen(listen_fd, 1024);

    if ((pid = fork()) < 0) {
    	perror("fork() error");
        exit(1);
    }

    if (pid == 0) {

    	printf("\033[1;32m[Segreteria]: Server started\033[1;0m\n\n");

    	for (;;) {

    		connfd = Accept(listen_fd, NULL, NULL);

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

    sleep(1);

    while(1){
    	printf("Selezione: \n\n------------------------------\n\n1 - inserisci un nuovo esame\n\n------------------------------\n\nDigita la tua scelta: ");
    	scanf("%d", &scelta);
    	while (getchar() != '\n');
    
    	if(scelta == 1)
    		invia_esame_server_u();
    	else
    		printf("\033[1;91m\n\n[!]Errore: Scelta non valida. Riprovare.\033[1;0m\n\n");
    }
}


/*------------------------------------
   IMPLEMENTAZIONE DELLE FUNZIONI
------------------------------------*/

void rispondi_studente(int connfd){

	char welcome_message[] = "Benvenuto nella segreteria studenti!";
	int welcome_size = strlen(welcome_message) + 1;
	ESAME *esami;
	int numCorsi;

	//Invio la dimensione in byte del messaggio di benvenuto
	FullWrite(connfd, &welcome_size, sizeof(int));
	// Invio il messaggio di benvenuto contenuto in welcome_message
	FullWrite(connfd, welcome_message, welcome_size);

	esami = richiediCorsiServerU(&numCorsi);

	FullWrite(connfd, &numCorsi, sizeof(int));
	FullWrite(connfd, esami, numCorsi * sizeof(CORSO));

	sleep(1);

	 /*
	int num_esami = 3;

	// Invio preventivamente il numero di esami al client studente
	if(full_write(connfd, &num_esami, sizeof(int)) < 0) {
		perror("full_write() error");
		exit(1);
	}

	// Invio l'array contenente tutti gli esami al client studente
	if(full_write(connfd, &esami, sizeof(esami)) < 0) {
		perror("full_write() error");
		exit(1);
	} */

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

void invia_esame_server_u(){

	ESAME esame;
	int socket_fd;
	struct sockaddr_in server_addr_u;
	char bit_iniziale = '1';

	socket_fd = Socket(AF_INET, SOCK_STREAM, 0);

	server_addr_u.sin_family = AF_INET;
	server_addr_u.sin_port = htons(1025);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr_u.sin_addr) <= 0) {
		perror("inet_pton() error");
	    exit(1);
	}

	while (1) {
		printf("Nome esame: ");

		if (fgets(esame.corso.nome, MAX_SIZE, stdin) == NULL) {
			perror("fgets() error");
	        exit(1);
	    }

		esame.corso.nome[strlen(esame.corso.nome)-1] = 0; // Sostituisco il carattere a capo con 0

		break;
	}

	printf("Crediti esame: ");
	scanf("%d", &esame.corso.crediti);
	printf("Invio [%d] %s con %d crediti", esame.ID, esame.corso.nome, esame.corso.crediti);

	Connect(socket_fd, (struct sockaddr *)&server_addr_u, sizeof(server_addr_u));
	FullWrite(socket_fd, &bit_iniziale, sizeof(char));
	FullWrite(socket_fd, &esame, sizeof(esame));

	close(socket_fd);

}

CORSO *richiediCorsiServerU(int *numCorsi){

	int socket_fd, dim;
	struct sockaddr_in server_addr_u;
	char bit_iniziale = '2';


	socket_fd = Socket(AF_INET, SOCK_STREAM, 0);
	server_addr_u.sin_family = AF_INET;
	server_addr_u.sin_port = htons(1025);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr_u.sin_addr) <= 0) {
		perror("inet_pton() error");
		exit(1);
	}

	Connect(socket_fd, (struct sockaddr *)&server_addr_u, sizeof(server_addr_u));

	FullWrite(socket_fd, &bit_iniziale, sizeof(char));
	FullRead(socket_fd, &dim, sizeof(int));

	//printf("%d", dim);

	*numCorsi = dim;

	CORSO *esami = (CORSO *)malloc(dim*sizeof(CORSO));

	FullRead(socket_fd, esami, dim*sizeof(CORSO));

	for(int i=0; i<dim; i++){
		printf("%s, ", esami[i].nome);
	}

	return esami;

}


