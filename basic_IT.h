
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#include<setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <ctype.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SERV_PORT	12002 // porta del server
#define BACKLOG		10 // numero di connessioni pendenti che possono essere in coda
#define MAXLINE		1024 // massimi caratteri su una linea
#define PAGE_SIZE   4096 // grandezza di una pagina

#define N_PAGE      1 // numero di pagine
#define SIZE        N_PAGE*PAGE_SIZE // pagina * numero di pagine

#define SIZE_NAME   100  // grandezza massima di un username
#define SIZE_OBJ    20   // numero massimo di caratteri per oggetti e comandi (non vogliamo oggetti troppo grossi )
#define MAX_CON     100 // numero massimo di connessioni sul server
#define UPPER_BOUND 4096 // numero massimo di caratteri scrivibili in bacheca questo valore dovra essere minore seppur di qualche byte della SIZE

#define MAX_FAULT   3   // numero massimo di errori di fault che il server puo avere poi lo mando in crash per manutenzione per via dei troppi fault

#define timeout_bck    600// ogni 60 secondi il thread main si fa il backup dei file aggiornati
#define timeout_con    360// Tempo di sessione del client se non fa nulla entro questo tempo chiudo la connessione
#define timeout_con2   10// Tempo di sessione del client se non fa nulla entro questo tempo chiudo la connessione


char *ack="ACK"; // inserirlo per gestire la latenza di stampa della bacheca fra client e server

 //  insertMessage(char *name ,  int fd,  char *msg)
char *str0 = "inserisci l'oggetto del messaggio :  ";
char *str1 = "inserisci il testo del messaggio o scrivi 'exit' per uscire dall inserimento :  ";
char *str2 = "carattere non valido nell'oggetto passato riprova : ";
char *str6 = "hai inserito un oggetto troppo grande riprova";
char *str24= "hai inserito un testo troppo grande in bacheca riprova";

//int  removeMessage(int  fd , char *msg , struct information_connect  *host){
char *str3 = "inserisci l'oggetto del messaggio che vuoi eliminare :  ";
char *str4 = "autenticazione fallita ";
char *str5 = "remove fallita controlla che l'oggetto esista o sia il tuo \n";

//  void *thread_function(void * arg)
char *str7  = "RETRY";
char *str8  = "NEW";
char *str9  = "RETRY -> per riprovare ,  NEW-> per creare un nuovo utente , LOGOUT -> per uscire dal server";
char *str10 = "LIST";
char *str11 = "POST";
char *str12 = "REMOVE";
char *str13 = "messaggio inserito";
char *str14 = "Nessun comando trovato \n";
char *str15 = "remove eseguita con successo \n";
char *str16 = "il contenuto della bacheca e vuoto\n";
char *str17 = "LOGOUT";
char *str18 = "Logout eseguito con successo\n";
char *str19 = "WHOAMI";
char *str20 = "utente gia connesso \n"; // si trova nell autenticator
char *str21 = "nome utente non disponibile \n"; // si trova in register_new_user
char *str22 = "Se sei un nuovo utente inserisci qualsiasi carattere da tastiera e premi invio.\nse sei gia registrato inserisci il nome utente.\nusername: "; // si trova in autenticator
char *str23 = "password: "; // si trova in autenticator
char *str25 = "fine della bacheca";

//int register_new_user(struct information_connect  *host)
char *str26 = "inserisci il tuo nome utente : ";
char *str27 = "inserisci la tua password : ";


char *str28 = "sono nella bacheca\n\n";
char *str29 = "le tue credenziali sono \n";


char *str30 = "pronto";
char *str31 = "exit";

char *legend= "\nLe azioni permesse sono :                                               cmd\n1)  Leggere tutti i tuoi messaggi presenti sulla bacheca elettronica -> LIST\n2)  Depositare un nuovo messaggio sulla bacheca elettronica          -> POST\n3)  Rimuovere un tuo messaggio dalla bacheca                         -> REMOVE\n4)  Per effettuare il logout                                         -> LOGOUT\n5)  Stampare i dati del tuo account                                  -> WHOAMI\n";

char *file_2 = "user_login.txt"; // descriptor [0]
char *file_3 = "pwd_login.txt"; // descriotir [1];
char *file_4 = "bacheca.txt"; // descriptor[2];
char *file_5 = "key_bacheca.txt" ; // descriptor[3];
char *file_6 = "user_login_backup.txt"; // des[0]
char *file_7 = "pwd_login_backup.txt"; // des[1]
char *file_8 = "bacheca_backup.txt"; // des[2]
char *file_9 = "key_bacheca_backup.txt" ; // des[3]
char *file_10 = "temporary.txt"; // file temporaneo di appoggio per copiare la bacheca
char *file_11 = "text.txt"; // file di inseriemento del testo della bacheca usato dal client

// client

char *cli_str0 = "Sessione Interretta dal client\n";
char *cli_str1 = "Sessione Scaduta\n";
char *cli_str2 = "input non valido ripova\n";
char *cli_str3 = "Errore di segmentazione\n";
char *cli_str4 = "Segnale si SIGQUIT ricevuto \n";
char *cli_str5 = "il server ha chiuso il canale di comunicazione\n";

