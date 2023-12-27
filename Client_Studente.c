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
#define MAT_SIZE 11

/*------------------------------------
   	 DEFINIZIONI DELLE STRUTTURE
------------------------------------*/

//Struttura che contiene i dati di uno studente
typedef struct Studente{
	char matricola[MAT_SIZE];
} STUDENTE;

//Struttura che contiene un esame il quale è definito dal nome e dai crediti
typedef struct Corso {
	int ID;
    char nome[50];
    int crediti;
} CORSO;

//Struttura per memorizzare una data
typedef struct {
    int day;
    int month;
    int year;
} DATE;

typedef struct Esame{
	int ID;
	CORSO corso;
	DATE data;
} ESAME;



/*------------------------------------
   	  PROTOTIPI DELLE FUNZIONI
------------------------------------*/

void pulisciSTDINBuffer();


/*------------------------------------
   	  IMPLEMENTAZIONE DEL MAIN
------------------------------------*/

int main(int argc, char **argv) {

	int socket_fd, welcome_size, login_result, dim;
	struct hostent *data;
	struct sockaddr_in server_addr;
	char **alias;
	char *addr;
	char buffer[MAX_SIZE];
	char matricola[MAT_SIZE];


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


	// Con la gethostbyname mi ricavo l'indirizzo IP dell'hostname passato come argomento da terminale e memorizzo tutto in data
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
	FullRead(socket_fd, &welcome_size, sizeof(int));

	/* RICEVO IL MESSAGGIO DI BENVENUTO E LO STAMPO A SCHERMO */
	FullRead(socket_fd, buffer, welcome_size);

	printf("%s", buffer);


	while(1){
		printf("Inserisci la matricola: ");
		fgets(matricola, MAX_SIZE, stdin);
		if(strlen(matricola) != MAT_SIZE)
			printf("\033[1;91m[!] Errore: la matricola deve essere di 10 caratteri !\033[1;0m\n");
		else{
			// Rimuovo il carattere newline (\n) alla fine della stringa
			matricola[MAT_SIZE - 1] = '\0';
			break;
		}
	}

	/* INVIO LA MATRICOLA ALLA SEGRETERIA */
	FullWrite(socket_fd, &matricola, MAT_SIZE);
	/* RICEVO L'ESITO DEL LOGIN */
	FullRead(socket_fd, &login_result, sizeof(int));

	if(login_result){
		FullRead(socket_fd, &welcome_size, sizeof(int));
		FullRead(socket_fd, &buffer, welcome_size);
		printf("\033[1;32m%s\033[1;0m", buffer);

		/* RICEVO IL NUMERO DI CORSI */
		FullRead(socket_fd, &welcome_size, sizeof(int));

		CORSO *corsi_disp = (CORSO *)malloc(welcome_size * sizeof(CORSO));

		/* RICEVO GLI ESAMI E LI MEMORIZZO NELLA STRUTTURA CREATA PRECEDENTEMENTE */
		FullRead(socket_fd, corsi_disp, sizeof(CORSO) * welcome_size);

		printf("Sono disponibili appelli per i seguenti corsi:\n\n");
		for(int i=0; i<welcome_size; i++){
			printf("%d - %s\n", i+1, corsi_disp[i].nome);
		}
		printf("\nTotale corsi: %d\n------------------------------", welcome_size);

		buffer[0] = '\0';

		while(1){
			printf("\nDigita il nome del corso per conoscerne le date: ");
			fgets(buffer, MAX_SIZE, stdin);
			if(strlen(buffer) > 0){
				buffer[strlen(buffer) - 1] = '\0';
				break;
			}
			else {
				printf("\n\033[1;91m[!] Errore: Inserire un nome valido !\033[1;0m");
			}
		}

		welcome_size = strlen(buffer)+1;

		FullWrite(socket_fd, &welcome_size, sizeof(int));
		FullWrite(socket_fd, &buffer, welcome_size);

		FullRead(socket_fd, &welcome_size, sizeof(int));
		ESAME *esami_disp = (ESAME *)malloc(welcome_size * sizeof(ESAME));
		FullRead(socket_fd, esami_disp, welcome_size * sizeof(ESAME));

		printf("\nSono disponibili i seguenti appelli per %s: \n\n", buffer);
		for(int i=0; i<welcome_size; i++){
			printf("\nID:%d - %s DATA:%d/%d/%d CFU: %d", esami_disp[i].ID, esami_disp[i].corso.nome, esami_disp[i].data.day, esami_disp[i].data.month, esami_disp[i].data.year, esami_disp[i].corso.crediti);
		}
		printf("\n\nTotale appelli: %d\n------------------------------", welcome_size);

	}

	else {
		printf("\033[1;91mLogin Fallita! Matricola non registrata.\n");
	}

	return 0;

}


void pulisciSTDINBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
