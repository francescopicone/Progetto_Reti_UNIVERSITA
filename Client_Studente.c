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

	int socket_fd, welcome_size, login_result, dim, scelta, ack;
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

	/* LEGGO IN INPUT LA MATRICOLA DELLO STUDENTE PER LA LOGIN */
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
	/* RICEVO L'ESITO DEL LOGIN e lo memorizzo in login_result*/
	FullRead(socket_fd, &login_result, sizeof(int));

	// Se l'esito della login è positivo, login_result = 1
	if(login_result){
		FullRead(socket_fd, &welcome_size, sizeof(int));
		FullRead(socket_fd, &buffer, welcome_size);
		printf("\033[1;32m%s\033[1;0m", buffer);

		/* RICEVO IL NUMERO DI CORSI E LI MEMORIZZO IN welcome_size */
		FullRead(socket_fd, &welcome_size, sizeof(int));

		CORSO *corsi_disp = (CORSO *)malloc(welcome_size * sizeof(CORSO));

		/* RICEVO I CORSI CON APPELLI DISPONIBILI E LI MEMORIZZO NELLA STRUTTURA CREATA PRECEDENTEMENTE */
		FullRead(socket_fd, corsi_disp, sizeof(CORSO) * welcome_size);

		/* SE CI SONO APPELLI DISPONIBILI ENTRO NELL'IF */
		if(welcome_size > 0){
			/* STAMPO A VIDEO I CORSI */
			printf("Sono disponibili appelli per i seguenti corsi:\n\n");
			for(int i=0; i<welcome_size; i++){
				printf("%d - %s\n", i+1, corsi_disp[i].nome);
			}
			printf("\nTotale corsi: %d\n------------------------------", welcome_size);

			buffer[0] = '\0';

			/* PRENDO IN INPUT IL NOME DEL CORSO DEL IL QUALE MI INTERESSANO LE DATI DI APPELLO */
			while(1){
				printf("\nDigita il nome del corso per conoscerne le date: ");
				fgets(buffer, MAX_SIZE, stdin);
				if(strlen(buffer) > 0){
					buffer[strlen(buffer) - 1] = '\0'; // Sostituisco il carattere newline con \0
					break;
				}
				else {
					printf("\n\033[1;91m[!] Errore: Inserire un nome valido !\033[1;0m");
				}
			}


			welcome_size = strlen(buffer)+1; // Inserisco in welcome_size la dimensione del nome del corso (buffer) inserito in precedenza

			FullWrite(socket_fd, &welcome_size, sizeof(int)); // Invio la dimensione di buffer al server segreteria
			FullWrite(socket_fd, &buffer, welcome_size); // Invio il nome del corso memorizzato in buffer al server segreteria

			FullRead(socket_fd, &welcome_size, sizeof(int));	// Ricevo la quantità di appelli disponibili per quel corso e la salvo in welcome_size
			ESAME *esami_disp = (ESAME *)malloc(welcome_size * sizeof(ESAME)); // Creo la struttura dati di tipo ESAME che possa contenere welcome_size appelli.
			FullRead(socket_fd, esami_disp, welcome_size * sizeof(ESAME)); // Ricevo la struttura dati contenente tutti gli appelli con relative info

			if(welcome_size > 0){
				/* Stampo a video tutti gli appelli ricevuti */
				printf("\nSono disponibili i seguenti appelli per %s: \n\n", buffer);
				for(int i=0; i<welcome_size; i++){
					printf("\nID:%d - %s DATA:%d/%d/%d CFU: %d", esami_disp[i].ID, esami_disp[i].corso.nome, esami_disp[i].data.day, esami_disp[i].data.month, esami_disp[i].data.year, esami_disp[i].corso.crediti);
				}
				printf("\n\nTotale appelli: %d\n------------------------------", welcome_size);
				printf("\nDigita l'ID dell'appello per il quale prenotarti: ");
				scanf("%d", &scelta);

				// Invio al server della segreteria l'ID dell'appello per il quale lo studente vuole prenotarsi
				FullWrite(socket_fd, &scelta, sizeof(int));
				// Ricevo l'ack dal server segreteria
				FullRead(socket_fd, &ack, sizeof(int));

				if (ack == 1)
					printf("\033[1;32m\nPrenotazione effettuata correttamente! Uscita in corso...\n\n\033[1;0m");
				else
					printf("\033[1;91m\nErrore durante la prenotazione. Uscita in corso...\033[1;0m");
			}
			else
				printf("\033[1;91m\nNessun appello disponibile per %s. Uscita in corso...\n\n\033[1;0m", buffer);

		}
		/* SE NON CI SONO APPELLI STAMPO UN MESSAGGIO ED ESCO */
		else {
			printf("\033[1;91m\nNessun appello disponibile al momento. Uscita in corso...\033[1;0m\n\n");
		}
	}

	// Se la login fallisce login_result = 0 entro nell'else
	else {
		// Stampo il messaggio di login fallita
		printf("\033[1;91mLogin Fallita! Matricola non registrata.\n");
	}

	// Chiudo il socket
	close(socket_fd);
	return 0;

}


void pulisciSTDINBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
