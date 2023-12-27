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
#include <time.h>
#include "Common/protocol.h"

#define MAX_SIZE 1024
#define MAX_100	 100
#define MAX_CORSI 128
#define MAT_SIZE 11

/*------------------------------------
   	 DEFINIZIONI DELLE STRUTTURE
------------------------------------*/

//Struttura che contiene i dati di uno studente
typedef struct Studente {
	char matricola[MAT_SIZE];
	char nome[50];
	char cognome[50];
} STUDENTE;

//Struttura che contiene informazioni sul corso il quale è definito da ID, nome e crediti
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
//int Socket(int namespace, int style, int protocol);		// Funzione wrapped per la socket
//void Bind(int listen_fd, struct sockaddr *addr, socklen_t lenght);	// Funzione wrapped per la bind
//void Listen(int listen_fd, int n);		// Funzione wrapped per la listen

void rispondiStudente(int connfd);
void invia_esame_server_u();
int contaEsami(const char *nomeFile);
ESAME *richiediEsamiServerU(const char *nomeCorso, int *numEsami);
CORSO *richiediCorsiServerU(int *numCorsi);
int controllaStudente(STUDENTE *studente);
int controllaData(int day, int month, int year);


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

    	printf("\033[1;32m[Segreteria]: Server online\033[1;0m\n\n");

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
    			rispondiStudente(connfd);

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

void rispondiStudente(int connfd){

	char welcome_message[] = "BENVENUTO NELLA SEGRETERIA STUDENTI!\nEFFETTUA IL LOGIN CON LA MATRICOLA\n\n";
	int welcome_size = strlen(welcome_message) + 1;
	char err_login_message[] = "LOGIN FALLITA. STUDENTE INESISTENTE.";
	int err_login_size = strlen(err_login_message) + 1;
	CORSO *corsi;
	ESAME *esami;
	int numCorsi, numEsami;
	STUDENTE studente;
	char matricola[MAT_SIZE];
	char nome_corso[MAX_SIZE];

	//Invio la dimensione in byte del messaggio di benvenuto
	FullWrite(connfd, &welcome_size, sizeof(int));
	// Invio il messaggio di benvenuto contenuto in welcome_message
	FullWrite(connfd, welcome_message, welcome_size);

	FullRead(connfd, &matricola, sizeof(char)*MAT_SIZE);
	strcpy(studente.matricola, matricola);

	int result = controllaStudente(&studente);

	//INVIO IL RISULTATO DELLA LOGIN ALLO STUDENTE
	FullWrite(connfd, &result, sizeof(int));

	if(result){
		welcome_size = snprintf(NULL, 0, "Bentornato %s %s ! \n\n", studente.nome, studente.cognome);
		printf("welcome_size = %d", welcome_size);
		char *bentornato_message = (char *)malloc(welcome_size);
		snprintf(bentornato_message, welcome_size, "Bentornato %s %s ! \n\n", studente.nome, studente.cognome);

		//Invio la dimensione della conferma di accesso
		FullWrite(connfd, &welcome_size, sizeof(int));
		//Invio una conferma di accesso
		FullWrite(connfd, bentornato_message, welcome_size);

		free(bentornato_message);

		corsi = richiediCorsiServerU(&numCorsi);

		FullWrite(connfd, &numCorsi, sizeof(int));
		FullWrite(connfd, corsi, numCorsi * sizeof(CORSO));

		FullRead(connfd, &welcome_size, sizeof(int));
		FullRead(connfd, &nome_corso, welcome_size);

		esami = richiediEsamiServerU(nome_corso, &numEsami);

		FullWrite(connfd, &numEsami, sizeof(int));
		FullWrite(connfd, esami, numEsami*sizeof(ESAME));


	}
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
	int result;

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

	while(1){
		printf("Data esame (nel formato dd/mm/aaaa): ");
		result = scanf("%d/%d/%d", &esame.data.day, &esame.data.month, &esame.data.year);
		if(controllaData(esame.data.day, esame.data.month, esame.data.year) && result == 3)
			break;
		else {
			printf("\nData non corretta. Riprovare.\n");
			int c;
			while ((c = getchar()) != '\n' && c != EOF);
		}
	}

	printf("Invio %s con %d crediti del %d/%d/%d\n", esame.corso.nome, esame.corso.crediti, esame.data.day, esame.data.month, esame.data.year);

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

	*numCorsi = dim;

	CORSO *corsi = (CORSO *)malloc(dim*sizeof(CORSO));

	FullRead(socket_fd, corsi, dim*sizeof(CORSO));

	for(int i=0; i<dim; i++){
		printf("%s, ", corsi[i].nome);
	}

	close(socket_fd);
	return corsi;

}

ESAME *richiediEsamiServerU(const char *nomeCorso, int *numEsami){

	int socket_fd, dim;
	struct sockaddr_in server_addr_u;
	char bit_iniziale = '3';


	socket_fd = Socket(AF_INET, SOCK_STREAM, 0);
	server_addr_u.sin_family = AF_INET;
	server_addr_u.sin_port = htons(1025);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr_u.sin_addr) <= 0) {
		perror("inet_pton() error");
		exit(1);
	}

	Connect(socket_fd, (struct sockaddr *)&server_addr_u, sizeof(server_addr_u));

	FullWrite(socket_fd, &bit_iniziale, sizeof(char));

	dim = strlen(nomeCorso)+1;

	FullWrite(socket_fd, &dim, sizeof(int));
	FullWrite(socket_fd, nomeCorso, dim);

	FullRead(socket_fd, &dim, sizeof(int));

	*numEsami = dim;

	ESAME *esami = (ESAME *)malloc(dim*sizeof(ESAME));
	FullRead(socket_fd, esami, dim*sizeof(ESAME));

	close(socket_fd);
	return esami;

}

int controllaStudente(STUDENTE *studente){

	char c, *buffer = (char *)malloc(MAX_SIZE);
	int conteggio = 0;
	STUDENTE tmp_studente;
	int check = 0;

	int fd = open("studenti.txt", O_RDONLY);

	while(read(fd, &c, 1) > 0){

		if (c == '\n'){

			buffer[conteggio] = '\0';
			sscanf(buffer, "%10[^;];%49[^;];%49[^;]", tmp_studente.matricola, tmp_studente.nome, tmp_studente.cognome);

			printf("confronto %s - %s", studente->matricola, tmp_studente.matricola);
			if (strcmp(studente->matricola, tmp_studente.matricola) == 0){
				strcpy(studente->nome, tmp_studente.nome);
				strcpy(studente->cognome, tmp_studente.cognome);
				check = 1;
				break;
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

	close(fd);
	free(buffer);
	return check;

}

int controllaData(int day, int month, int year){

	int days;
	time_t currentTime;
	struct tm *localTime;

	time(&currentTime);     // Ottiengo il tempo corrente in secondi
	localTime = localtime(&currentTime); // Converto currentTime in una struttura tm

	if (year < localTime->tm_year+1900)
		return 0; //anno non valido
	else if (year == localTime->tm_year+1900 && month < localTime->tm_mon+1)
		return 0; //mese non valido
	else if (year == localTime->tm_year+1900 && month == localTime->tm_mon+1 && day < localTime->tm_mday)
		return 0; //giorno non valido

	if (month == 2)
		days = 29;
	else if (month == 4 || month == 6 || month == 9 || month == 11)
		days = 30;
	else
		days = 31;

	if (day < 1 || day > days) {
	        return 0; // Giorno non valido
	}

	//se tutti i controlli hanno successo ritorno 1
	return 1;

}
