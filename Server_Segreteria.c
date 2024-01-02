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
		char *bentornato_message = (char *)malloc(welcome_size);
		snprintf(bentornato_message, welcome_size, "Bentornato %s %s ! \n\n", studente.nome, studente.cognome);

		//Invio la dimensione della conferma di accesso
		FullWrite(connfd, &welcome_size, sizeof(int));
		//Invio una conferma di accesso
		FullWrite(connfd, bentornato_message, welcome_size);

		// Libero la memoria allocata a bentornato_message
		free(bentornato_message);

		// Richiedo i corsi per i quali esistono appelli al server universitario e li memorizzo in corsi
		corsi = richiediCorsiServerU(&numCorsi);

		// Invio al client studente il numero dei corsi con appelli memorizzato in numCorsi
		FullWrite(connfd, &numCorsi, sizeof(int));
		// Invio al client studente la struttura di tipo CORSO contenente i corsi con appelli disponibili
		FullWrite(connfd, corsi, numCorsi * sizeof(CORSO));

		if (numCorsi > 0){

			// Ricevo dal client studente la dimensione della stringa contenente il nome del corso per il quale si vogliono conoscere gli appelli
			FullRead(connfd, &welcome_size, sizeof(int));
			// Ricevo il nome del corso per il quale si vogliono conoscere gli appelli
			FullRead(connfd, &nome_corso, welcome_size);

			// Richiedo al server universitario gli appelli per il corso nome_corso, salvo il numero degli appelli in numEsami e gli appelli in esami
			esami = richiediEsamiServerU(nome_corso, &numEsami);

			// Invio numEsami al client studente
			FullWrite(connfd, &numEsami, sizeof(int));
			// Invio la struttura di tipo ESAME esami contenente gli appelli al client studente
			FullWrite(connfd, esami, numEsami*sizeof(ESAME));


			// Ricevo dal client studente l'ID dell'appello per il quale vuole prenotarsi
			FullRead(connfd, &welcome_size, sizeof(int));
			// Inoltro l'ID dell'appello per il quale lo studente vuole prenotarsi al server universitario
			int ack = inoltraPrenotazioneServerU(welcome_size, studente);
			// Inoltro l'ack al client studente
			FullWrite(connfd, &ack, sizeof(int));
		}
	}
}

/* invia_esame_server_u stabilisce una connessione con il server universitario al quale invia
 * un nuovo appello da inserire sul server.
 */
void invia_esame_server_u(){

	ESAME esame;
	int socket_fd;
	struct sockaddr_in server_addr_u;
	char bit_iniziale = '1';	// bit_iniziale inviato al server universitario = 1 per far capire che devo inviargli un appello
	int result;

	socket_fd = Socket(AF_INET, SOCK_STREAM, 0);

	server_addr_u.sin_family = AF_INET;
	server_addr_u.sin_port = htons(1025);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr_u.sin_addr) <= 0) {
		perror("inet_pton() error");
	    exit(1);
	}

	// Leggo in input da tastiera il nome del corso al quale fa riferimento l'appello
	while (1) {
		printf("Nome esame: ");

		if (fgets(esame.corso.nome, MAX_SIZE, stdin) == NULL) {
			perror("fgets() error");
	        exit(1);
	    }

		esame.corso.nome[strlen(esame.corso.nome)-1] = '\0'; // Sostituisco il carattere a capo con 0

		break;
	}

	// Leggo in input da tastiera il numero dei crediti dell'appello
	printf("Crediti esame: ");
	scanf("%d", &esame.corso.crediti);

	// Leggo in input da tastiera la data dell'appello nel formato dd/mm/aaaa
	while(1){
		printf("Data esame (nel formato dd/mm/aaaa): ");
		result = scanf("%d/%d/%d", &esame.data.day, &esame.data.month, &esame.data.year);
		//Con la funzione controllaData controllo la validità della data inserita
		if(controllaData(esame.data.day, esame.data.month, esame.data.year) && result == 3)
			break;
		else {
			printf("\nData non corretta. Riprovare.\n");
			int c;
			while ((c = getchar()) != '\n' && c != EOF);
		}
	}


	printf("Invio %s con %d crediti del %d/%d/%d\n", esame.corso.nome, esame.corso.crediti, esame.data.day, esame.data.month, esame.data.year);

	// Mi connetto al server universitario
	Connect(socket_fd, (struct sockaddr *)&server_addr_u, sizeof(server_addr_u));
	// Invio il bit iniziale = 1
	FullWrite(socket_fd, &bit_iniziale, sizeof(char));
	// Invio la struttura esame contenente tutti i dati dell'appello inseriti
	FullWrite(socket_fd, &esame, sizeof(esame));

	// Chiudo la connessione
	close(socket_fd);

}

/* La funzione richiediCorsiServerU, richiede i corsi con appelli disponibili al
 * server universitario e li salva in una struttura di tipo CORSO. Ritorna i corsi ricevuti
 * e la loro quantità in *numCorsi passato come argomento.
 */
CORSO *richiediCorsiServerU(int *numCorsi){

	int socket_fd, dim;
	struct sockaddr_in server_addr_u;
	char bit_iniziale = '2'; // bit iniziale = 2 per far capire al server universitario che devo richiedere corsi con appelli disponibili


	socket_fd = Socket(AF_INET, SOCK_STREAM, 0);
	server_addr_u.sin_family = AF_INET;
	server_addr_u.sin_port = htons(1025);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr_u.sin_addr) <= 0) {
		perror("inet_pton() error");
		exit(1);
	}

	// Mi connetto al server universitario
	Connect(socket_fd, (struct sockaddr *)&server_addr_u, sizeof(server_addr_u));

	// Invio il bit iniziale = 2
	FullWrite(socket_fd, &bit_iniziale, sizeof(char));
	// Ricevo dal server universitario il numero totale dei corsi con appelli disponibili
	FullRead(socket_fd, &dim, sizeof(int));

	*numCorsi = dim;	// Salvo il numero dei corsi in numCorsi passato come argomento

	CORSO *corsi = (CORSO *)malloc(dim*sizeof(CORSO)); // Creo la struttura dati per contenere dim corsi

	// Ricevo dal server universitario i corsi con appelli disponibili e li salvo nella struttura corsi creata in precedenza
	FullRead(socket_fd, corsi, dim*sizeof(CORSO));

	// Chiudo il socket
	close(socket_fd);
	return corsi;

}

/* La funzione richiediEsamiServerU, richiede gli appelli al server universitario del corsoù
 * passato come argomento in *nomeCorso, ritorna la struttura ESAME contenente tutti gli
 * appelli e il numero totale degli appelli in *numEsami passato come argomento.
 */
ESAME *richiediEsamiServerU(const char *nomeCorso, int *numEsami){

	int socket_fd, dim;
	struct sockaddr_in server_addr_u;
	char bit_iniziale = '3'; // bit iniziale = 3 per far capire al server universitario che devo richiedere gli appelli di un determinato corso


	socket_fd = Socket(AF_INET, SOCK_STREAM, 0);
	server_addr_u.sin_family = AF_INET;
	server_addr_u.sin_port = htons(1025);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr_u.sin_addr) <= 0) {
		perror("inet_pton() error");
		exit(1);
	}

	// Si connette al server universitario
	Connect(socket_fd, (struct sockaddr *)&server_addr_u, sizeof(server_addr_u));

	// Invio il bit iniziale al server universitario
	FullWrite(socket_fd, &bit_iniziale, sizeof(char));

	// Salvo la lunghezza del nome del corso ricevuto dal client studente (contenuto in *nomeCorso) in dim
	dim = strlen(nomeCorso)+1;

	// Invio dim al server universitario
	FullWrite(socket_fd, &dim, sizeof(int));
	// Invio nomeCorso contenente il nome del corso al server universitario
	FullWrite(socket_fd, nomeCorso, dim);
	// Ricevo dal server universitario il numero di appelli totali per il corso inviato in precedenza
	FullRead(socket_fd, &dim, sizeof(int));

	*numEsami = dim;	// Ritorno il numero di appelli totali in numEsami

	// Creo la struttura dati di tipo ESAME* esami per contenere tutti gli appelli che riceverò dal server universitario
	ESAME *esami = (ESAME *)malloc(dim*sizeof(ESAME));
	// Ricevo dal server universitario gli appelli del corso richiesto con relative informazioni e li salvo in esami
	FullRead(socket_fd, esami, dim*sizeof(ESAME));

	// Chiudo la connessione
	close(socket_fd);
	return esami;

}

int inoltraPrenotazioneServerU(int id_appello, STUDENTE studente){

	int socket_fd, dim, ack;
	struct sockaddr_in server_addr_u;
	char bit_iniziale = '4'; // bit iniziale = 3 per far capire al server universitario che devo richiedere gli appelli di un determinato corso


	socket_fd = Socket(AF_INET, SOCK_STREAM, 0);
	server_addr_u.sin_family = AF_INET;
	server_addr_u.sin_port = htons(1025);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr_u.sin_addr) <= 0) {
		perror("inet_pton() error");
		exit(1);
	}

	// Mi connetto al server universitario
	Connect(socket_fd, (struct sockaddr *)&server_addr_u, sizeof(server_addr_u));

	// Invio il bit iniziale al server universitario per fargli capire che gli sto inoltrando una prenotazione
	FullWrite(socket_fd, &bit_iniziale, sizeof(char));

	// Invio al server universitario l'ID dell'appello per il quale lo studente vuole prenotarsi
	FullWrite(socket_fd, &id_appello, sizeof(int));
	// Invio al server universitario la matricola dello studente che vuole prenotarsi
	FullWrite(socket_fd, &studente.matricola, MAT_SIZE);
	// Ricevo l'ack da parte del server universitario
	FullRead(socket_fd, &ack, sizeof(int));

	return ack;
}

/* La funzione controllaStudente, effettua il controllo della LOGIN attraverso la
 * matricola ricevuta dal client studente. Se lo studente esiste memorizza i dati contenuti
 * in studenti.txt nella struttura STUDENTE *studente passata come argomento e ritorna 1,
 * altrimenti ritorna 0.
 */
int controllaStudente(STUDENTE *studente){

	char c, *buffer = (char *)malloc(MAX_SIZE);
	int conteggio = 0;
	STUDENTE tmp_studente;	// Struttura temporanea per contenere lo studente letto in studenti.txt
	int check = 0;

	int fd = open("studenti.txt", O_RDONLY);

	/* Scorro tutte le righe in studenti.txt (ogni riga è uno studente) e confronto le
	 * matricole, se la matricola ricevuta dal client studenti esiste, memorizzo nome e
	 * cognome relativi a quella matricola nella struttura *studente passata come
	 * argomento, e imposto check = 1.
	 */
	while(read(fd, &c, 1) > 0){

		if (c == '\n'){

			buffer[conteggio] = '\0';
			sscanf(buffer, "%10[^;];%49[^;];%49[^;]", tmp_studente.matricola, tmp_studente.nome, tmp_studente.cognome);

			/* Con strcmp confronto la matricola ricevuta dal client studente con quella
			 * letta in studenti.txt. Se sono uguali salvo tutto nella struttura *studente
			 * e imposto check = 1.
			 */
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

	// Chiudo il file descriptor per studenti.txt
	close(fd);
	// Libero la memoria allocata a buffer
	free(buffer);
	// Ritorno check = 0 se lo studente non esiste, altrimenti 1
	return check;

}

/* La funzione controllaData, controlla che la data inserita per caricare un nuovo appello
 * nel server universitario sia corretta. Ritorno 1 se la data è corretta altrimenti 0.
 */
int controllaData(int day, int month, int year){

	int days;
	time_t currentTime;
	struct tm *localTime;

	time(&currentTime);     // Ottiengo il tempo corrente in secondi
	localTime = localtime(&currentTime); // Converto currentTime in una struttura tm

	if (year < localTime->tm_year+1900)
		return 0; // anno non valido perchè minore di quello attuale
	else if (year == localTime->tm_year+1900 && month < localTime->tm_mon+1)
		return 0; // mese non valido perchè minore di quello attuale
	else if (year == localTime->tm_year+1900 && month == localTime->tm_mon+1 && day < localTime->tm_mday)
		return 0; // giorno non valido perchè minore di quello attuale

	/* Controllo quanti giorni ha il mese inserito e salvo il numero dei giorni in days */
	if (month == 2) // Mese di febbraio supponiamo abbia sempre 29 giorni
		days = 29;
	else if (month == 4 || month == 6 || month == 9 || month == 11) // Mesi con 30 giorni
		days = 30;
	else	// Mesi con 31 giorni
		days = 31;

	if (day < 1 || day > days) {
	        return 0; // Giorno non valido poichè quella data in quel mese non può esistere
	}

	//se tutti i controlli hanno successo ritorno 1
	return 1;

}
