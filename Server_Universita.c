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
#include <sys/file.h>
#include "Common/protocol.h"

#define MAX_SIZE 1024
#define MAX_100	 100
#define MAX_CORSI 128
#define MAT_SIZE 11

/*------------------------------------
   	 DEFINIZIONI DELLE STRUTTURE
------------------------------------*/

//Struttura che contiene un esame il quale è definito dal nome e dai crediti
typedef struct Corso {
	int ID;
    char nome[50];
    int crediti;
} CORSO;

//Struttura per memorizzare una data
typedef struct Date{
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

void ricevi_esame(int connfd);
int memorizza_esame(ESAME esame);
int contaEsami(const char *nomeFile);
void inviaEsamiSegreteria(int connfd);
void inviaCorsiSegreteria(const char *nomeFile, int connfd);
int controllaPrenotazioneEsistente(const char *matricola, int id_appello);

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

    printf("\033[1;32m[Server universitario]: Server online\033[1;0m\n");

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

            //Ricevo il bit iniziale
            FullRead(connfd, &bit_iniziale, sizeof(char));

            /* Se il bit iniziale è
             * 1 - Il client segreteria vuole aggiungere un nuovo appello
             * 2 - Il client segreteria richiede la lista dei corsi per i quali sono disponibili appelli
             * 3 - Il client segreteria richiede la lista degli appelli per un determinato corso
             * 4 - Il client segreteria inoltra la prenotazione di un appello per uno studente
             */

            if(bit_iniziale == '1'){
            	ricevi_esame(connfd);
            }
            else if (bit_iniziale == '2')
            	inviaCorsiSegreteria("esami.txt", connfd);
            else if (bit_iniziale == '3')
            	inviaEsamiSegreteria(connfd);
            else
            	riceviPrenotazione(connfd);

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
/* Funzione che riceve un appello dal client segreteria */
void ricevi_esame(int connfd){

	char ack_message[] = "\033[1;32m\nEsame aggiunto con successo!";
	char ack_message_err[] = "\033[1;91m\nSi è verificato un errore sul server. Riprovare.";
	int ack_size, result;

	ESAME esame;

	FullRead(connfd, &esame, sizeof(ESAME));	// Ricevo la struttura di tipo ESAME che contiene le info sull'appello

	esame.ID = contaEsami("esami.txt")+1;	// Assegno come ID all'appello il numero di appelli già presenti + 1, se il file esami non esiste ritorna -1

	result = memorizza_esame(esame);	// Con memorizza_esame salvo l'appello nel file esami.txt, se il salvataggio fallisce ritorna 0 altrimenti 1

	if (result){
		printf("\n[Server universitario] Aggiunto l'esame %s - %d\033[1;0m\n", esame.corso.nome, esame.corso.crediti);
		ack_size = strlen(ack_message)+1;
		FullWrite(connfd, &ack_size, sizeof(int));
		FullWrite(connfd, &ack_message, ack_size);
	}

	else {
		ack_size = strlen(ack_message_err)+1;
		FullWrite(connfd, &ack_size, sizeof(int));
		FullWrite(connfd, &ack_message_err, ack_size);
	}

	close(connfd);

}

void riceviPrenotazione(int connfd){

	int id_appello, ack = 1, fd;
	char matricola[MAT_SIZE];

	// Ricevo dal server della segreteria l'ID dell'appello per il quale lo studente vuole prenotarsi
	FullRead(connfd, &id_appello, sizeof(int));
	// Ricevo dal server della segreteria la matricola dello studente che vuole prenotare l'appello
	FullRead(connfd, &matricola, MAT_SIZE);

	/* con controllaPrenotazioneEsistente controllo se esiste già una prenotazione per lo studente matricola dell'appello id_appello
	   ritorna 1 se non esiste, 0 se esiste, -1 in caso di errore */
	ack = controllaPrenotazioneEsistente(matricola, id_appello);

	// Se ack = 1 non esiste alcuna prenotazione ed il server procede a memorizzarla in prenotazioni.txt
	if (ack == 1){
		if ((fd = open("prenotazioni.txt", O_WRONLY | O_APPEND | O_CREAT, 0777)) == -1) {
			perror("Errore durante l'apertura del file");
			ack = -1;
		}

		// Accedo in mutua esclusione al file
		if(flock(fd, LOCK_EX) < 0) {
			perror("flock() error");
			ack = -1;
		}

		// Scrivo la prenotazione nel file
		dprintf(fd, "%s;%d\n", matricola, id_appello);

		if (flock(fd, LOCK_UN) < 0) {
			perror("flock() unlock error");
			ack = -1;
		}

		// Chiudo il file descriptor
		close(fd);

		// Stampo a schermo la prenotazione salvata
		printf("\n[Server universitario] Ricevuta la prenotazione di %s per l'esame %d", matricola, id_appello);
	}

	// Invio l'ack al server della segreteria
	FullWrite(connfd, &ack, sizeof(int));


}

/* La funzione memorizza_esame prende in input un appello contenuto nella struttura di
 * tipo ESAME e lo salva nel file esami.txt
 */
int memorizza_esame(ESAME esame){

	int fd;

	if ((fd = open("esami.txt", O_WRONLY | O_APPEND | O_CREAT, 0777)) == -1) {
		perror("Errore durante l'apertura del file");
		return 0;
	}

	// Accedo in mutua esclusione al file
	if(flock(fd, LOCK_EX) < 0) {
		perror("flock() error");
	    return 0;
	}

	// Scrivo le info dell'appello nel file esami.txt
	dprintf(fd, "%d;%s;%d;%d;%d;%d\n", esame.ID, esame.corso.nome, esame.corso.crediti, esame.data.day, esame.data.month, esame.data.year);

	if (flock(fd, LOCK_UN) < 0) {
	    perror("flock() unlock error");
	    return 0;
	}

	close(fd);
	return 1;
}

/* La funzione contaEsami conta il numero totale di appelli memorizzati nel file
 * esami.txt. Ritorna il numero degli appelli.
 */
int contaEsami(const char *nomeFile){

	int fd = open(nomeFile, O_RDONLY);

	 if (fd < 0) {
		 perror("open() error");
	     return -1;
	 }

	 int conteggio = 0;
	 char c;

	 // Scorro il file e ad ogni riga letta aumento il conteggio di 1
	 while(read(fd, &c, 1) > 0){
		 if (c == '\n')
			 conteggio++;
	 }

	 close(fd);
	 return conteggio;

}

/* La funzione contaEsamiN è una variante di contaEsami che restituisce il numero di appelli
 * totale per un determinato corso identificato da *nomeCorso passato come argomento.
 */
int contaEsamiN(const char *nomeCorso){

	char *buffer = (char *)malloc(MAX_SIZE);
	ESAME tmp_esame;
	int numEsami = 0;

	int fd = open("esami.txt", O_RDONLY);

	if (fd < 0) {
		perror("open() error");
		exit(1);
	}

	if(flock(fd, LOCK_EX) < 0) {
		perror("flock() error");
		exit(1);
	}

	// Mi posiziono all'inizio del file
	lseek(fd, 0, SEEK_SET);

	int conteggio = 0;
	char c;

	// Scorro il file e se trovo un appello del corso identificato da *nomeCorso incremento il conteggio
	while(read(fd, &c, 1) > 0){
		if (c == '\n'){
			buffer[conteggio] = '\0';
		 	sscanf(buffer, "%d;%49[^;];%d;%d;%d;%d", &tmp_esame.ID, tmp_esame.corso.nome, &tmp_esame.corso.crediti, &tmp_esame.data.day, &tmp_esame.data.month, &tmp_esame.data.year);

		 	// Confronto nomeCorso con il nome del corso relativo all'appello letto
		 	if (strcmp(nomeCorso, tmp_esame.corso.nome) == 0){
		 		numEsami++;
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

	if (flock(fd, LOCK_UN) < 0) {
		perror("flock() unlock error");
		exit(1);
	}

	// Libero la memoria allocata a buffer
	free(buffer);
	close(fd);
	return numEsami;
}


/* La funzione controllaPrenotazioneEsistente controlla nel file prenotazioni.txt se esiste
 * già la prenotazione dello studente per un determinato esame. Ritorna 1 se non esiste, 0 se
 * esiste, -1 in caso di errore.
 */
int controllaPrenotazioneEsistente(const char *matricola, int id_appello) {

	char *buffer = (char *)malloc(MAX_SIZE);
	int result = 1, fd, conteggio = 0;
	char c, tmp_matricola[MAT_SIZE];
	int tmp_id_appello;

	// Apro il file prenotazioni.txt, se non esiste non esistono prenotazioni e ritorno 1
	if ((fd = open("prenotazioni.txt", O_RDONLY)) == -1) {
		perror("Errore durante l'apertura del file");
		return 1;
	}

	// Accedo in mutua esclusione al file prenotazioni.txt
	if(flock(fd, LOCK_EX) < 0) {
		perror("flock() error");
		return -1;
	}

	// Scorro il file e se trovo una prenotazione esistente dello studente matricola per quell'appello imposto result = 0
	while(read(fd, &c, 1) > 0){
		if (c == '\n'){
			buffer[conteggio] = '\0';
			sscanf(buffer, "%10[^;];%d", tmp_matricola, &tmp_id_appello);

			// Confronto nomeCorso con il nome del corso relativo all'appello letto
			if (strcmp(tmp_matricola, matricola) == 0){
				if (tmp_id_appello == id_appello){
					free(buffer);
			 		result = 0;
			 		break;
			 	}
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

	if (flock(fd, LOCK_UN) < 0) {
		perror("flock() unlock error");
		return -1;
	}

	close(fd);
	return result;

}


void inviaEsamiSegreteria(int connfd){

	int numero_esami, dim;
	ESAME tmp_esame;
	char *buffer = (char *)malloc(MAX_SIZE);

	FullRead(connfd, &dim, sizeof(int));
	char *nome_corso = (char *)malloc(dim);
	FullRead(connfd, nome_corso, dim);

	numero_esami = contaEsamiN(nome_corso);
	ESAME *esami = (ESAME *)malloc(numero_esami*sizeof(CORSO));

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
			sscanf(buffer, "%d;%49[^;];%d;%d;%d;%d", &tmp_esame.ID, tmp_esame.corso.nome, &tmp_esame.corso.crediti, &tmp_esame.data.day, &tmp_esame.data.month, &tmp_esame.data.year);

			if (strcmp(nome_corso, tmp_esame.corso.nome) == 0){
				esami[i].ID = tmp_esame.ID;
				esami[i].corso = tmp_esame.corso;
				esami[i].data = tmp_esame.data;
				i++;
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

	if (flock(fd, LOCK_UN) < 0) {
		    perror("flock() unlock error");
		    exit(1);
	}

	close(fd);

	FullWrite(connfd, &numero_esami, sizeof(int));
	FullWrite(connfd, esami, sizeof(ESAME)*numero_esami);


}

void inviaCorsiSegreteria(const char *nomeFile, int connfd){

	CORSO tmp_corsi[MAX_CORSI];
	int numCorsi = 0, duplicato=0, conteggio=0, i=0;
	char c, *buffer = (char *)malloc(MAX_SIZE);
	int d,m,y;

	int fd = open(nomeFile, O_RDONLY);

	while(read(fd, &c, 1) > 0){

		int duplicato = 0;

		if (c == '\n'){
			buffer[conteggio] = '\0';

			sscanf(buffer, "%d;%49[^;];%d;%d;%d;%d", &tmp_corsi[i].ID, tmp_corsi[i].nome, &tmp_corsi[i].crediti, &d, &m, &y);

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
	}

	close(fd);

	FullWrite(connfd, &numCorsi, sizeof(int));
	FullWrite(connfd, esami, numCorsi*sizeof(CORSO));

	free(esami);
	free(buffer);

}
