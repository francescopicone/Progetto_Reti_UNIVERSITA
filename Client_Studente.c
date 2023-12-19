/*----------------------------------------------------------------------------
file: Client_Studente.c
autore: Francesco Picone

Implementa il client dal quale lo studente può chiedere la lista degli esami
disponibili al server e/o inviare una richiesta di prenotazione
----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include "Common/protocol.h"

#define MAX_SIZE 1024

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
   	  IMPLEMENTAZIONE DEL MAIN
------------------------------------*/

int main(int argc, char **argv) {

	int socket_fd, welcome_size;
	struct hostent *data;
	struct sockaddr_in server_addr;
	char **alias;
	char *addr;
	char buffer[MAX_SIZE];


	if (argc != 2) {
	        perror("usage: <host name>");
	        exit(1);
	}

	// Creo il descrittore del socket

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	        perror("socket() error");
	        exit(1);
	}

	// Valorizzo la struttura server_addr

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(1024);


	// Con la gethostbyname mi ricavO l'indirizzo IP dell'hostname passato come argomento da terminale e memorizzo tutto in data
	if ((data = gethostbyname(argv[1])) == NULL) {
	        herror("gethostbyname() error");
			exit(1);
	}

	// Salvo l'indirizzo in network byte order nella variabile alias
	alias = data -> h_addr_list;


	// Converto l'indirizzo IP in network byte in tipo char e la salvo nella variabile addr
	if ((addr = (char *)inet_ntop(data -> h_addrtype, *alias, buffer, sizeof(buffer))) < 0) {
	        perror("inet_ntop() error");
	        exit(1);
	}

	// Converto l'indirizzo da testo a formato binario
	if (inet_pton(AF_INET, (char *)addr, &server_addr.sin_addr) <= 0) {
	        perror("inet_pton() error");
	        exit(1);
	}

	//Mi connetto al server della segreteria
	if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
	        perror("connect() error");
	        exit(1);
	}

	/* RICEVO LA DIMENSIONE DEL MESSAGGIO DI BENVENUTO */
	if (full_read(socket_fd, &welcome_size, sizeof(int)) < 0) {
	        perror("full_read() error");
	        exit(1);
	}

	/* RICEVO IL MESSAGGIO DI BENVENUTO E LO STAMPO A SCHERMO */

	if (full_read(socket_fd, buffer, welcome_size) < 0) {
	        perror("full_read() error");
	        exit(1);
	}

	printf("%s\n", buffer);
	welcome_size = 0;

	/* RICEVO IL NUMERO DI CORSI */
	FullRead(socket_fd, &welcome_size, sizeof(int));

	CORSO *corsi_disp = (CORSO *)malloc(welcome_size * sizeof(CORSO));

	/* RICEVO GLI ESAMI E LI MEMORIZZO NELLA STRUTTURA CREATA PRECEDENTEMENTE */
	FullRead(socket_fd, corsi_disp, sizeof(CORSO) * welcome_size);

	printf("Numero corsi disponibili: %d \nLista dei corsi: \n\n", welcome_size);

	for(int i=0; i<welcome_size; i++){
		printf("%d - %s\n", i+1, corsi_disp[i].nome);
	}

	return 0;

}
