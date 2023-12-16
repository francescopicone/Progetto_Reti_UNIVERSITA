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

void rispondi_studente(int connfd);
void invia_esame_server_u();
int contaEsami(const char *nomeFile);
ESAME crea_pacchetto_esami();

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

    	for (;;) {

    		printf("[Segreteria] Attendo nuove richieste\n");

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
    			//rispondi_studente(connfd);

    			close(connfd);
    			exit(0);

    		} else
    			close(connfd);
    	}

    	exit(0);
    }

    while(1){
    	printf("Selezione: \n\n1 - inserisci un nuovo esame\n\n Scelta: ");
    	scanf("%d", &scelta);
    	while (getchar() != '\n');
    
    	if(scelta == 1)
    		invia_esame_server_u();
    	else
    		printf("Scelta non valida");
    }
}


/*------------------------------------
   IMPLEMENTAZIONE DELLE FUNZIONI
------------------------------------*/

void rispondi_studente(int connfd){

	char welcome_message[] = "Benvenuto nella segreteria studenti!";
	int welcome_size = strlen(welcome_message) + 1;

	ESAME esami[] = {{0, "Reti dei calcolatori", 9}, {1, "Programmazione III", 6}, {2, "Basi di Dati", 9}};

	//Invio la dimensione in byte del messaggio di benvenuto
	FullWrite(connfd, &welcome_size, sizeof(int));
	// Invio il messaggio di benvenuto contenuto in welcome_message
	FullWrite(connfd, welcome_message, welcome_size);

	sleep(1);

	int num_esami = 3;

	// Invio preventivamente il numero di esami al client studente
	if(full_write(connfd, &num_esami, sizeof(int)) < 0) {
		perror("full_write() error");
		exit(1);
	}

	printf("\n %s, %d\n", esami->corso, esami->crediti);

	// Invio l'array contenente tutti gli esami al client studente
	if(full_write(connfd, &esami, sizeof(esami)) < 0) {
		perror("full_write() error");
		exit(1);
	}




	close(connfd);

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

ESAME crea_pacchetto_esami(){

	int num_esami = contaEsami("esami.txt");
	int fd = open("esami.txt", O_RDONLY, 0777);

	ESAME temp_esami[num_esami];

	write(fd, &temp_esami, sizeof(temp_esami));

	printf("%c", temp_esami->corso);

	return *temp_esami;

	/*
	int num_esami = contaEsami("esami.txt");
	ESAME temp_esami[num_esami];

	int fd = open("esami.txt", O_RDONLY, 0777);

	char buffer[MAX_SIZE];
	int byte_letti;

	while((byte_letti = read(fd, buffer, sizeof(buffer))) > 0){

		int indice = 0;

		while(buffer[indice] != '/'){
			temp_esami.data[indice] = buffer[indice];
			indice++;
		}

		temp_esami.data[indice] = '\0';

		while(buffer[indice] != '/'){
				indice++;
		}

		indice = 0;

		while(buffer)*/

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

	Connect(socket_fd, (struct sockaddr *)&server_addr_u, sizeof(server_addr_u));

	FullWrite(socket_fd, &bit_iniziale, sizeof(char));

	while (1) {
		printf("Nome esame: ");

		if (fgets(esame.corso, MAX_SIZE, stdin) == NULL) {
			perror("fgets() error");
	        exit(1);
	    }

		esame.corso[strlen(esame.corso)-1] = 0; // Sostituisco il carattere a capo con 0

		break;
	}

	printf("Crediti esame: ");
	scanf("%d", &esame.crediti);
	printf("Invio [%d] %s con %d crediti", esame.ID, esame.corso, esame.crediti);

	FullWrite(socket_fd, &esame, sizeof(esame));
	close(socket_fd);

}

/*------------------------------------
IMPLEMENTAZIONE DELLE FUNZIONI WRAPPED
------------------------------------*/


