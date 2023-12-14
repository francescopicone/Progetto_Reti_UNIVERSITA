/* Tipi di dato comuni */

#define MAX_SIZE 1024   //buffer size
#define ID_SIZE 11      //Size dell'ID (10byte) +1byte del terminatore
#define ACK_SIZE 61     //Size del messaggio di ACK ricevuto dal CentroVaccinale

//Definiamo il pacchetto applicazione per l'user da inviare al centro vaccinale
typedef struct {
    char name[MAX_SIZE];
    char surname[MAX_SIZE];
    char ID[ID_SIZE];
} VAX_PCKG_REQ;


//Struct del pacchetto dell'ASL contenente il numero di tessera sanitaria di un green pass ed il suo referto di validità
typedef struct  {
    char ID[ID_SIZE];
    char stato;
} ASL_PCKG;

//Struct che permette di salvare una data, formata dai campi: giorno, mese ed anno
typedef struct {
    int day;
    int month;
    int year;
} DATE;

//Struct del pacchetto inviato dal centro vaccinale al server vaccinale contentente il numero di tessera sanitaria dell'utente, la data di inizio e fine validità del GP
typedef struct {
    char ID[ID_SIZE];
    char stato; //Stato del green pass, 0 se il green pass non è valido, 1 se il green pass è valido
    DATE start_date;
    DATE expire_date;
} GP_REQ_PCKG;




