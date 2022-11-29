#ifdef IT
#include  "basic_IT.h"

#else
#include  "basic_ENG.h"
#endif

/*
 devo sistemare

 2) fargli riconoscere 2 oggetti uguali per far scegliere l utente quale di questi desidera cancellare
 3) mandare dei SIGSEGV per testare al meglio come gestisce il sementation fault

 */

/////////////////////// STRUTTURA A CUI SARA ASSEGNATO OGNI CLIENT ////////////////////////

// racchiudera tutte le informazioni appartenenti al client che si connette
struct information_connect{
    long i;   // identificatore del thread
    int des_sock ; // descrittore del socket
    pid_t tid ;  // prrende l identificatore dle thread
    char *mem_private ; // un area di memoria di appoggio su cui scrivo e leggo cose ( stanziata 1 per ogni utente che si connette )
    char *login_name;  // nome dell utente che si sta connettendo
    char *login_pwd ; // password di login dell utente

};

/////////////////////////// dichiarazoni delle funzioni //////////////////////////////////
int restore(int num); // si occupa di fare il restore di eventuali file danneggiati
int control(int opt); // controlla l integrità dei file della bacheca
int control_usr_pwd(void); // controlla l integrita dei file di login
void backup(int sgnumber); // ha il compito di fare il backup dei file di bacheca
void connect_client( int sock_listen );// funzione che si occupa di accettare nuovi client
void signal_handler(int signum); // funzione di gestione della SIGINT (termina il server)
void  create_user_pwd_login(void); // crezione di 2 buffer globali con nome utente e password
int  reader(char *msg , int fd , int opt); //classico lettore di file generici
int  writer(char *msg , int fd , int opt); //classico scrittore su file generici
int  autenticator(struct information_connect  *host , int opt );// funzione di autenticazione dell utente
int register_new_user(struct information_connect  *host); // finzione che registra un nuovo utente
int insertMessage(char *name ,  int fd,  char *msg); // inserisce un messaggio in bacheca
void remapping( off_t key_start , long gap); // esegue un remap del file indici dopo una cancellazione
int  removeMessage(int  fd , struct information_connect  *host, char *msg); // rimuove messaggi in bacheca
char *upper( char *msg); // converte ogni stringa passata in maiuscola
void logout(int fd, struct information_connect  *client); // esegue la disconnessione dell utentedal server
void whoami(int fd, struct information_connect  *host); // indica all utente le sue credenziali
int reader_bacheca(char *msg ,int bacheca_fd, int fd_sock); // legge la bacheca
void *thread_function(void * arg); // funzione main di ogni thread child
int copyfile(int src_fd , int dst_fd , off_t start_src , off_t start_dst , off_t end, int opt); // copia un file partendo da start fino ad end e imposta l indice del file di destinazione
// il campo opt se 1 mi imposta gli offset altrimenti copiera indistintamente tutto il file

/////////////////////// VARIABILI GLOBALI ////////////////////////////////////////////

int login_fd; // descrittore dove sono i nomi di tutti gli utenti
int pwd_fd; // descrittore dove sono le password di tutti gli utenti
char *user[SIZE]; // array degli utenti registrati
char *pwd[SIZE]; // array di password  degli utenti
int num_user ; // numero di utenti registrati
int bacheca_fd; // descrittore della bacheca
int bacheca_key_fd; //descrittore del file che contiene le chiavi per cancellare i messaggi in bacheca
struct information_connect client[MAX_CON]; // di pari passo con le connessioni in parallelo che si possono accettare su server
sem_t s1; // questo semaforo sarà legato alla modifica dei file bacheca e key_bacheca (ogni modifica dovra essere fatta da un sono client per volta cosi da non creare problemi )
sem_t s2; // questo semaforo sarà legato alla modifica dei file utenti e password (ogni modifica dovra essere fatta da un sono client per volta cosi da non creare problemi )
int bitmask[MAX_CON] = {0};
int descriptor[4] ={0}; // descrittori dei file impostati inizialmente a 0


int max_fault;
jmp_buf env;
sigset_t set; // dichiaro in blobale la strutture di gestione degli eventi
pid_t tid_main ;
/////////////////////////////// FUNZIONI E PROCEDURE DI SERVIZIO  ///////////////////////////////////////////////////////

void signal_handler(int signum){ // signal handler per la termianzione del processo SERVER
   printf("close the server due to the signal %d\n",signum);
   backup(5); // in chiusura del serve mi interessa fare il backup
   exit(0);
}

// in caso di fault il server chiuderà tutte le connessioni e terminerà correttamente
/*
se il server accorre in un errore di segmentazione disconnettero il thread che lo ha captato perche dovrebbere essere stato lui
a tentare di accedere in un area di memoria invalida , e poi tornerò nella connect_client per riconctrollare eventualmente i file
se sono stati scritti ;
devo solo trovare
 */
void sigsegv_handler(int signum){ // signal handler per la termianzione del processo SERVER
    printf("Segmantation fault accurred %d return to main for check server files\n", signum); // messaggio di captazione del  segnale di errore di segmentazione
    pid_t tid = gettid(); // mi prendo l identiciatore del thread
    if(tid != tid_main){
        for(int i=0 ; i<MAX_CON  ; i++){
            if(bitmask[i]== 1){
                if(tid = client[i].tid){ // confronto identificatore del thread
                    logout(client->des_sock,&client[i]); // una volta identificato l utente che si e disconnesso chiamo la logout
                }
            }
        }
   }else if(!max_fault){
       printf("valore di max_fault raggiunto il server verrà chiuso a causa del ripetuto fault\n");
       exit(-1); // Codice di terminazione -1

}
   else {//se ad avere il fault è il main thread lo faccio tornare nella connect client per ricontrollare i file sel server
       max_fault--;
       longjmp(env , 1);
   }
}


/*
questa funzione deve essere invocata dal thread che capta la sigpipe
qui dentro il thread prenderà il suo identificativo , cercherà la sua struttura
e dovrà chiamare la logout per disconnettersi e terminare con successo
*/
void sigpipe_handler(int signum){ // signal handler per la termianzione del processo SERVER (alla captazione di questo segnale devo chiudere la conenssione )
   printf("channel comunication with client close receive SIGPIPE %d\n",signum);
   pid_t tid = gettid(); // mi prendo l identiciatore del thread
   for(int i=0 ; i<MAX_CON  ; i++){
       if(bitmask[i]== 1){
           if(tid = client[i].tid){ // confronto identificatore del thread
               logout(client->des_sock,&client[i]); // una volta identificato l utente che si e disconnesso chiamo la logout
          }
       }
   }
}

// funzione atta a bloccare i segnali
void signal_lock (void){

    sigprocmask(SIG_UNBLOCK, &set , NULL); //sblocchiamo la maschera di bit
    sigfillset(&set); // imposto tutta la maschera di segnali a 1(per disattivarli tutti
    sigprocmask(SIG_BLOCK, &set , NULL); // la ribolocchiamo con il nuovo set

}

void signal_unlock(void){

    // parte di codice che chiude i sengali e lascia aperto solamente la SIGINT e SIGALARM
   sigprocmask(SIG_UNBLOCK, &set , NULL); //sblocchiamo la maschera di bit
   sigfillset(&set); // blocco tutti i sengali per riattivare solo quelli necessari
   sigdelset(&set , 14); // sigalarm
   sigdelset(&set , 2);  // sigint
   sigdelset(&set , 1); // sighup
   sigdelset(&set , 11);  // sigsegv
   sigdelset(&set , 13); // sigpipe
   sigprocmask(SIG_BLOCK,&set,NULL);
}



//ritornare una stringa letta da un descrittore (lseek deve essere fatta dal chiamante che intende leggere qualcosa all inizio del file )
int  reader(char *msg , int fd , int opt){

    char buffercv[SIZE]; // dò l'indirizzo ad una stringa che fungera da dummy per allocarla
    int byte_r;
    memset(msg , 0 , SIZE ); // reset dell area di memoria
    if(opt)  // reimposta  la sessione al inzio cosi da leggerlo completamente
        lseek(fd , 0 , SEEK_SET); // mi riporto all inizio del file
    byte_r = read(fd, buffercv , SIZE);
    buffercv[byte_r]= '\0' ; // terminatore di stringa
    if(byte_r == -1){
        perror("error READ socket in reader");
            exit(-1);
    }
    memcpy(msg , buffercv , strlen(buffercv)); // salvo nell area di memoria riservata all utente
    return byte_r;
}


int  writer(char *msg , int fd , int opt){

    int len = strlen(msg); // prendo la lunghezza del messaggio passato
    int byte_w;
     if((byte_w = write(fd, msg , len))<0){   // lo scrivo nel descrittore che gli viene passato
            return -1;
        }
     if(opt==1){ // aggiungo all occorrenza un new_line
         dprintf(fd , "\n");
    }
    if(opt==2){ // aggiungo all occorrenza 2 new_line
         dprintf(fd , "\n\n");
    }
return 0;
}


int copyfile(int src_fd , int dst_fd , off_t start_src , off_t start_dst , off_t end ,int opt){
    char c;
    int byte_r , byte_w, count=0;
    char buff[SIZE];
    lseek(src_fd , start_src , SEEK_SET);
    lseek(dst_fd , start_dst , SEEK_SET);
    if(opt == 0 ) // imposta ad 1 end cosi da escludere sempre la prima condizione del while
        end = 1;

    while (end != 0 && (byte_r=read(src_fd, &c , 1))==1){ // leggo dal file sorgente byte a byte fino ad arrivare alla fine della riga
                buff[count] = c;
                count+=byte_r;
                if(opt)
                    end--;
                if(c== '\n' ){ // se arrivo alla new line ho finito la riga
                    if((byte_w=write(dst_fd, buff , count ))<0){ // scrivo su file destinazione
                         perror("error to WRITE socket in reader_bacheca");
                         return -1;
                     }
                    count =0 ; // reset del count
                }
        }if(byte_r <0){ // controllo errore della read
                perror("error to READ socket in reader_bacheca");
                exit(EXIT_FAILURE);
            }
return end;
}


// se cancello la prima riga nella riscrittura della riga potrebbe esserci uno scarto di numero dovuto al passaggio dalle centinaia alle decine ecc
// tale bug e controllato dal fatto che tokenizzo sempre in modo statico e la 4 stirnga tokenizzata e sempre l ultima
void remapping( off_t key_start , long gap){
    char buff[MAXLINE];
    char buff2[MAXLINE];
    char c;
    long inizio , fine;
    int count=0;
    int byte_r;
    char *str,*token;
    int j;
    char *buff1[MAXLINE];
    off_t end_key =0 , start_key;
    start_key = key_start;
    lseek(bacheca_key_fd , key_start , SEEK_SET ); // riporto ad inizio riga per scrivere
redo:
    while((byte_r = read(bacheca_key_fd, &c , 1))==1){ //leggo la linea qui punto alla fine della riga
        buff[count] = c;
        count+=byte_r;
        if(c== '\n' ){
                end_key = lseek(bacheca_key_fd , 0 , SEEK_CUR); // prendo la fine della riga
                count =0 ;
                break;
        }
    }if(byte_r == 0){ // condizione di uscita quando la read non leggera piu nulla
            return;
             }

    // tokenizzo la linea che ricavo dal file delle chiavi
    for (j = 0, str = buff; ; j++, str = NULL) {
            token = strtok(str, "|");
            if (token == NULL)
                break;
            buff1[j]=malloc(sizeof(strlen(token)));
            strcpy(buff1[j], token);
     }
     //salvo il tutto
     inizio = atol(buff1[2]);
     inizio -=gap;
     fine  = atol(buff1[3]);
     fine -= gap; // porto idietro gli offset
     snprintf(buff2, sizeof(buff2), "%s|%s|%ld|%ld|", buff1[0] , buff1[1] , inizio , fine ); //ricostruisco la stringa  modificata con i nuovi offset che devo salvare nel file delle chiavi
     lseek(bacheca_key_fd , start_key , SEEK_SET ); //punto la sessione del file all inizio della stringa che devo modificare
     writer(buff2 , bacheca_key_fd , 0 ); // scrive nel file delle chiavi la nuova stringa con offset modificati
     lseek(bacheca_key_fd , end_key , SEEK_SET); // spiazzo dall inizio fino alla nuova stringa da modificare
     start_key = end_key;
     goto redo;
}


char *upper( char *msg){ // funzione che converte una stringa passata in maiuscolo (evita che l utente debba preoccuparsi di inserire comandi in maiuscolo )
    size_t len = strlen(msg) , i;
    char *baseid = msg; // do a baseid l indirizzo base di msg
    for(i=0; i<len ; i++){
         (*baseid) = toupper((*baseid));
         baseid++;
    }
    return msg;
}
//////////////////////////////////////////////////////////// FUNZIONI INERENTI ALL'UTENTE E ALLA SUA REGRISTRAZIONE //////////////////////////////////


// l'intento e creare 2 bufferini che saranno letti con lo stesso indice
void  create_user_pwd_login(void){
    int byte_r;
    int j;
    char *str1 ,*token;
    char buff[SIZE]; // bufferini di appoggio  per i nomi utente
    char buff1[SIZE];
    int count = 0 ;
    lseek(login_fd,0,SEEK_SET); // faccio sempre puntare all inizio
    lseek(pwd_fd,0,SEEK_SET); // faccio sempre puntare all inizio

    while((byte_r = read(login_fd, &buff[count] , 1))==1){ // leggo completamente il file di login
            count+=byte_r;
    }
    if(byte_r == -1){
        perror("error to write socket in create user");
        exit(-1);
    }
    //tokenizza la stringa
    for (j = 0, str1 = buff; ; j++, str1 = NULL) { // tokenizzo ogni stringa
        token = strtok(str1, "\n");
        if (token == NULL)
            break;
        user[j]=malloc(sizeof(strlen(token)));
        strcpy(user[j], token); // inserisco nel buffer globale degli utenti
    }

    num_user = j ; // prende il numero dei utenti registrati
    count=0; // parte del codice che genera un bufferino di password
    j=0;

    while((byte_r = read(pwd_fd, &buff1[count] , 1))==1){
            count+=byte_r;
    }

     if(byte_r == -1){
        perror("error to write socket in create pwd ");
        exit(-1);
     }

    for (j = 0, str1 = buff1; ; j++, str1 = NULL) {
        token = strtok(str1, "\n");
        if (token == NULL)
            break;
        pwd[j]=malloc(sizeof(strlen(token)));
        strcpy(pwd[j], token); // copio nel
    }

    if(num_user != j){ // parte di codice che segnala solamente all amministratore del server che manca qualche nome utente
        if(num_user > j){
            printf("errore missing password \n");
            fflush(stdout);
            restore(2);
        }else{
            printf("errore missing username \n");
            fflush(stdout);
            restore(2);
        }
     }
}


int  autenticator(struct information_connect  *host , int opt ){
    int fd_sock = host -> des_sock;
    char *addr =  host -> mem_private;
    int j ,ret;
    memset(addr , 0 , SIZE ); // pulisce l area di memoria
    writer(str22 , fd_sock , 0); // scrive nella socket del utente
    ret =reader(addr , fd_sock ,1); // legge la risposta dalla socket condivisa dall utente // da qui addr avrà al suo interno il nome utente
    if(ret<=0){
        return -2;
    }
    if(opt){ // qeusto campo e riservato esclusivamente alla remove in quanto richiedo l'accesso al client che non puo impersonare nessun altro utente
        if((strcmp(addr , host->login_name))!=0){
            return -1;
        }
    }
    for(j=0 ; j<(host->i); j++){ // controllo se esiste l utente che ho passato
        if((strcmp(addr , client[j].login_name))==0){ // se trovo l utente gia loggato entro qui dentro
            writer(str20 , fd_sock , 0); // dico all utente che e gia connesso e non può loggarsi
            return -1;
        }
    }
    for(j=0 ; j<num_user; j++){ // controllo se esiste l utente che ho passato
        if((strcmp(addr , user[j]))==0){ // se trovo l utente registrato entro in questo if
            memset(addr , 0 , SIZE ); // reset dell area di memoria perche altrimenti da problemi
            writer(str23 , fd_sock ,0 ); // scrive nella socket del utente
            ret = reader(addr , fd_sock,1) ; // legge la risposta dalal socket condivisa dall utente // da qui addr avrà al suo interno il nome utente
            if(ret<=0){
                return -2;
            }

            if((strcmp(addr , pwd[j]))==0){ // faccio il match anche con la passwrod
                host -> login_name = malloc(sizeof(strlen(user[j]))); // caso affermativo entro e salvo nella struttura passata il nome utente
                strcpy( host -> login_name , user[j]);
                host -> login_pwd = malloc(sizeof(strlen(pwd[j]))); // salvo nella struttura la password dell utente
                strcpy(host -> login_pwd , pwd[j]);
                return 0 ;
            }
        }
    }
return -1; // se non trovo il nome utente restituisco -1
}
// funzione runnata solo dal child
int register_new_user(struct information_connect  *host){
    int j;
    int ret ;
    char c;
    int fd_sock = host -> des_sock;
    char *msg= malloc(SIZE);
    char *msg1= malloc(SIZE);
    char *cmp;
    memset(msg , 0 , SIZE);
    memset(msg1 , 0 , SIZE);
    /// inserimento dell utente //////////////////////////
retry:
    writer(str26 , fd_sock , 0); // scr
    ret = reader(msg, fd_sock ,1); // legge la risposta dell utente 

    if(ret<=0){ // provo a leggere la socket se torna 0 allora significhera che il client ha chiuso la conenssione
        free(msg);
        free(msg1);
        return -1;
    }
    for(j=0 ; j<num_user; j++){ // controllo se esiste l utente registrato (non posso accettare 2 nomi utente uguali )
        if((strcmp(msg , user[j]))==0){ // se trovo l utente registrato entro in questo if
               writer(str21 , fd_sock , 0); // dico all utente che il nome che sta segliendo e gia preso
               goto retry; // lo mando a inserire un nuovo nome utente
        }
    }

    cmp = msg; // assegno indirizzo base a cmp
    int len  = strlen(msg); // devo evitare di mettere il pipe anche nel nome utente in quanto porta problemi alla tokenizzazione 
    for(int i = 0 ; i<len ; i++){ // controllo sull oggetto inserito per evitare l uso di un  carattere speciale che mi serve a tokenizzare
         if((*cmp)== '|'){
                memset(msg , 0 , SIZE); // reset della memoria
                goto retry;
        } else
             cmp++; //incremento l indirizzo di memoria e leggo il prossimo byte
    }


    host -> login_name= malloc(sizeof(strlen(msg))); // alloco lo spazio necessario per salvare il nome utente in struttura host
    memcpy((host -> login_name) , msg, strlen(msg)); // salvo il nome (tale scelta e presa per dare un identità ad ogni client che adotta per cancellare messaggi senza che deve autenticarsi di nuovo)

    ////////////////// inserimento password//////////////////
    writer(str27 , fd_sock , 0 ); // scrive il messaggio all utente d della password
    ret = reader(msg1, fd_sock ,1 ); // legge la password dell utente
     if(ret<=0){ // stessa cosa di su se chiudo la connessione mentre devo inserire la password non leggero alcun carattere
        free(msg);
        free(msg1);
        return -1;
    }
    lseek(fd_sock,-1,SEEK_CUR);
    host -> login_pwd= malloc(sizeof(strlen(msg1))); // aloco scapzio per salvare la password
    memcpy((host -> login_pwd) , msg1, strlen(msg1)); // salvo la password dentro la struttura dell utente
// semaforo per l accesso esclusivo al file(questo codice e runnato da piu client contemporaneamente e quindi e soggetto a sincronizzazione)
    signal_lock(); // blocco tutti i segnali
    sem_wait(&s2); // per evitare conflitti di accesso al file utenti imposto un semaforo ad accesso esclusivo(mutex)
    lseek(login_fd , 0 , SEEK_END); // punto il file contenente i nomi utente alla fine
    lseek(pwd_fd , 0 , SEEK_END); // punto il file  contenente le password utente alla fine
    writer(msg1 , pwd_fd , 1); // scrivo la password appena presa nel file
    writer(msg , login_fd , 1 ); // scrivo il nome utente appena preso nel file
    sem_post(&s2); // rilasco il gettone per un client futuro
    signal_unlock();
    free(msg); // libero le aree di memoria che mi sono servite (evita i leak)
    free(msg1);
    return 0;
}


void logout(int fd, struct information_connect  *client){
     memset(client->mem_private , 0 , SIZE); // reset dell area di memoria
     client->des_sock = -1; // descrittore impostato a -1 (valore che non indica nulla )
     client->login_name = "NULL";  // nome dell utente che si sta connettendo reimpostata a NULL
     client->login_pwd = "NULL" ; // password di login dell utente (sara modificato in fase di autenticazione )
     writer(str18 , fd , 0); // risposta al client di uscita con successo
     close(fd); // chiude il descrittore della socket su cui e agganziato
     bitmask[client->i]=-1 ; // indice dell utente che non usa lo slot
     pthread_exit(0); // chiusura del thread
}

void whoami(int fd, struct information_connect  *host){ // costurisce una stringa semplice e poi la scrive nella socket per dire al utente la propia username e password
    char buff[MAXLINE];
    int len_1 = strlen(host->login_name);
    int len_2 = strlen(host->login_pwd);
    int count =0;
    char *string = "user: ";
    char *string2 = "password: ";
    int len_3 = strlen(string);
    int len_4 = strlen(string2);
    char *msg = malloc(sizeof(strlen(ack)));
    write(fd , str29 , strlen(str29)); // mando il messaggio che mi trovo nella whoami
    read(fd , msg , strlen(ack)); // leggo un ack di ricezione 
    memcpy(&buff[0] , string , len_3);
    count+=len_3;
    memcpy(&buff[count] , host->login_name , len_1);
    count += len_1;
    buff[count] = '\n';
    memcpy(&buff[++count] , string2 , len_4);
    count+=len_4;
    memcpy(&buff[count] , host->login_pwd , len_2);
    count+=len_2;
    buff[count] = '\0';
    writer(buff, fd , 0 ); // scrive nell socket
    free(msg);
}

//////////////////////////////////////////////////////////// FUNZIONI DELLA BACHECA //////////////////////////////////

// funzioen runnata solo dal child
int insertMessage(char *name ,  int fd,  char *msg){
     char  buff[MAXLINE];
     char  buff2[MAXLINE];
     time_t  ticks;
     off_t start;
     off_t end ;

     char c;
     char *object = malloc(SIZE);
     char *cmp = object;
     char *buff1[MAXLINE];
     char *str,*token;
     char buffer[MAXLINE];
     int counter = 0 ;
     int count=0;
     int  j , byte_r ;

     writer(str0 , fd , 0); // chiede al client di inserire l oggetto
    int len , ret;
retry:
     ret = reader(object , fd ,1 ) ; //legge il messaggio dell utente che inserira l oggetto del messaggio ()
     if((len  = strlen(object))>SIZE_OBJ){ // controllo sull oggetto piu grande che possa passare
           memset(object , 0 , SIZE); // reset della memoria
           writer(str6 , fd , 0 ); // scrivo all utente che il suo messaggio e troppo grande e dovra reinserirlo
           goto retry;
    }
    if(ret<=0){ // se leggo 0 nella socket allora il client ha chiuso la connessone
        free(object);
        return -1;
    }
     for(int i = 0 ; i<len ; i++){ // controllo sull oggetto inserito per evitare l uso di un  carattere speciale che mi serve a tokenizzare
         if((*cmp)== '|'){
                memset(object , 0 , SIZE); // reset della memoria
                writer(str2 , fd , 0 ); // messaggio all utente del carattere non valido
                goto retry;
        } else
             cmp++; //incremento l indirizzo di memoria e leggo il prossimo byte
    }
    ///////////// inizio della ricerca della stessa chiave //////////////
    lseek(bacheca_key_fd , 0 , SEEK_SET); // metto il file all inizio
riprova:
     while((byte_r = read(bacheca_key_fd, &c , 1))==1){ // leggo le chiavi di bacheca
            buffer[count] = c;
            count+=byte_r;
           if(c== '\n' ){
               count =0 ;
               break;
           }
     }
       if(byte_r ==0){ // arrivo alla fine della bacheca allora continuo
           goto pass;
        }
      //tokenizzo
      for (j = 0, str = buffer; ; j++, str = NULL) {
            token = strtok(str, "|");
            if (token == NULL ) // inutile che faccio memorizzare gli elementi successivi
                break;
            buff1[j]=malloc(sizeof(strlen(token))); // memorizzo  gli elementi
            strcpy(buff1[j], token);
     }
    if((strcmp(object , buff1[0]))==0){ // mathch  degli oggetti
        if((strcmp(name , buff1[1]))==0){ // metodo 1 -> match dell utente che sta inserendo l oggetto
             counter++; // se trovo il match fra lo stesso utente e oggetto allora lo aumento
             strncpy(buff2,object , len );
             memset(object , 0 , SIZE); // reset oggetto
             snprintf(object, sizeof(buff2)+sizeof(counter), "%s(%d)", buff2 ,counter ); // accoppio con il numeroS
             lseek(bacheca_key_fd , 0 , SEEK_SET); // reimposto a 0 l indice
              goto riprova;
         }else{
            goto riprova;
         }
    }else{ // se non
        goto riprova;
    }


pass:
    if(counter){ // se il contatore e diverso da 0 quindi ha trovato qualche doppione modifica la stringa  altrimenti va avanti come se nulla fosse

    }
 ///////////// FINE della ricerca della stessa chiave //////////////

redo:
    writer(str1 , fd , 0); // chiede al client di inserire il testo del messaggio
    ret = reader(msg , fd ,1) ; //legge il messaggio del client che e pronto a trasmettere il testo del messaggio
    if(strcmp(str30, msg)!=0){ // se viene passato un altra stringa cosa impossibile
        return -1;
    }

     signal_lock(); // blocco i segnali tutti
     sem_wait(&s1); // parte del codice che scrive nel file essendo gestito 1 fie per tutti gli utenti sincronizzare tale parte di codice e fondamentale
     start = lseek(bacheca_fd , 0 , SEEK_END); // aggiungo sempre in coda
     snprintf(buff, sizeof(buff), "user :%s  ", name ); // aggiungo il nome
     writer( buff , bacheca_fd , 0); // inserisce il nome
     // genero la stringa
     snprintf(buff, sizeof(buff), "object :%s  ", object ); // creo la stringa oggetto
     writer( buff, bacheca_fd , 0); // la scrivo in bacheca
     ticks = time(NULL);
     snprintf(buff, sizeof(buff), "date :  %.24s", ctime(&ticks)); // inserisco la data
     writer(buff, bacheca_fd , 1); // inserisco la data e il new_line

     // ciclo di inserimento del testo passato dal client dal suo file che sarà diretto su la bacheca
     memset(msg , 0 , SIZE);
     while((ret = read(fd , msg , SIZE))>0){ // legge SIZE byte dalla socket
        if(strcmp(str31 , msg)==0){ // se nella socket legge il solo messaggio di exit allora esce
            break;

        }else{
            writer(msg , bacheca_fd , 1); /// scrivo su la bacheca il messaggio
            writer(ack , fd , 0); // mando un riscontro di lettura al client per sincronizzarlo
            memset(msg , 0 , SIZE); // faccio un reset della memoria
        }
     }
     if(ret<0){
         perror("error to read socket");
         free(object); // evito il leak
        sem_post(&s1); // rilascio il token
        signal_unlock();
         return(-1);
    }

     end = lseek(bacheca_fd , 0 , SEEK_END); // prima SEEK_CUR (non dovrebbe cambiare nulla ) prendo l offset dall inizio fino al messaggio scritto servira per orientare il codice nella remove
     lseek(bacheca_key_fd , 0 , SEEK_END); // mi porto alla fine del file delle chiavi

     snprintf(buff, sizeof(buff), "%s|%s|%ld|%ld|", object , name , start , end ); // costruisce la stringa per fare il matching durante la remove
     writer(buff , bacheca_key_fd , 1 ); //scrivo la stringa nel codice chiave
     free(object); // evito il leak
     sem_post(&s1); // rilascio il token
     signal_unlock();
return 0;
}

int  removeMessage(int  fd , struct information_connect  *host,char *msg){
     char *name = host -> login_name;
     int byte_r,ret,byte_w;
     char buffer[MAXLINE];
     int count=0, temp;
     long j ;
     char *str,*token;
     char *buff1[MAXLINE];
     long start , end , gap;
     char c;
     memset(msg , 0 , SIZE); // Pulisco preventivamente l area di memoria
     writer(str3 , fd , 0);// chiede all utente di scrivere la stringa che desidera rimuovere
     ret=reader(msg , fd ,1); //acquisisco l oggetto che voglio togliere
     if(ret<=0){ // se chiudo la connessione il client manda un logout che verra preso qui e ritorna -1
        return -2;
    }
     off_t start_key ;
     off_t end_key =0 ;
     lseek(bacheca_key_fd , 0 , SEEK_SET); // metto il file all inizio

     signal_lock();
     sem_wait(&s1); // prendo io gettone per fermare chiunque stia modificando il fil
redo:

     start_key = end_key; // serve ad avere un puntatore dall inzio alla fine sul rigo che dovro eliminare dalle chaivi dopo aver eliminato il messaggio dalla bacheca

      while((byte_r = read(bacheca_key_fd, &c , 1))==1){
            buffer[count] = c;
            count+=byte_r;
           if(c== '\n' ){
                   end_key = lseek(bacheca_key_fd , 0 , SEEK_CUR);
                   count =0 ;
                    break;
           }
     }
       if(byte_r == 0){
                 sem_post(&s1);
                 signal_unlock();
                 return -1;
             }
      //tokenizzo
      for (j = 0, str = buffer; ; j++, str = NULL) {
            token = strtok(str, "|");
            if (token == NULL)
                break;
            buff1[j]=malloc(sizeof(strlen(token)));
            strcpy(buff1[j], token);
     }
     start = atol(buff1[2]);
     end  = atol(buff1[3]);

    if((strcmp(msg , buff1[0]))==0){ // mathch  degli oggetti
         printf("match object\n");
         fflush(stdout);
         //if((strcmp(name , buff1[1]))==0 && (ret = autenticator( host , 1 )) == 0){ // metodo 2 ->  chiedo le credenziali di accesso all utente per verificare che sia effettivamente lui
         if((strcmp(name , buff1[1]))==0){ // metodo 1 -> match dell utente che invoca la remove con l oggetto che vuole eliminare

                 printf("autentication SUCCESS\n");
                 fflush(stdout);
                 if((temp = open(file_10 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apro un file temporaneo di appoggio vuoto
                    perror("error to open LEGEND file in remove ");
                    sem_post(&s1);
                    signal_unlock();
                    exit(-1);
                    }
                    copyfile(bacheca_fd , temp ,  0 , 0 , start , 1 ); // copia la bacheca su sun file di appoggio da 0 a start
                    copyfile(bacheca_fd , temp ,  end , start , 0 , 0); // copia la bacheca su un file di appoggio dalla fine del messaggio da cancellare fino alla fine del file
                        // adesso temporary dovra essere la nuova becheca allora
                        close(bacheca_fd); // chiudo il file della bacheca
                        if((bacheca_fd = open(file_4 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file di bacheca con il troncamento
                            perror("error to open LEGEND file in remove ");
                            sem_post(&s1);
                            signal_unlock();
                            exit(-1);
                        }
                        copyfile(temp , bacheca_fd ,  0 , 0 , 0 , 0 ); // imposto i descrittori tutti a 0 cosi so che devono partire all origine

                        ///////////////////// parte della chiave ////////////////////////////////////
                        close(temp);
                         if((temp = open(file_10 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){// apro un file temporaneo di appoggio vuoto
                            perror("error to open LEGEND file in remove ");
                            sem_post(&s1);
                            signal_unlock();
                            exit(-1);
                        }
                        copyfile(bacheca_key_fd , temp ,  0 , 0 , start_key , 1 ); // copia il fil delle chiavi su sun file di appoggio da 0 a start_key
                        copyfile(bacheca_key_fd , temp ,  end_key , start_key , 0 , 0); // copio il file delle chiavi su un file di appoggio da end_key

                        close(bacheca_key_fd); // chiudo il file della bacheca
                        if((bacheca_key_fd = open(file_5 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file di bacheca con il troncamento
                            perror("error to open LEGEND file in remove ");
                            sem_post(&s1);
                            signal_unlock();
                            exit(-1);
                        }
                        copyfile(temp , bacheca_key_fd ,  0 , 0 , 0 , 0 ); // imposto i descrittori tutti a 0 cosi so che devono partire all origine

                    gap = end-start;
                    remapping( start_key , gap); //end key e la  nuova riga da dove devo partire a modificar
                    printf("END remove\n");
                    sem_post(&s1);
                    signal_unlock();
                    return 0;
           }else{
            goto redo; // altra chance su messaggi con oggetti omonimi ma scritti da persone diverse
           }
      }else{
        goto redo; // se non collide con l oggetto legge il prossimo rigo senza stampare messaggi
    }
}

int reader_bacheca(char *msg ,int bacheca_fd, int fd_sock){

    char buff[SIZE];
    int byte_r,byte_w,count =0,ret;
    char c;
    int flag=0;
    write(fd_sock , str28 , strlen(str28)); // mando "sono nella bacheca"
    read(fd_sock , msg , strlen(ack)); // leggo un ack dal client
    signal_lock();
    sem_wait(&s1);
    lseek(bacheca_fd,0 , SEEK_SET); // mi porto all inizio del file della bacheca
    // Di seguito 2 implementazioni che fanno la medesima cosa
    //la prima passa riga per riga , la seconda compie letture di al piu SIZE byte e li manda al client
    // ne conveniamo che la seconda implementazione è piu veloce anche per il minor costo computazionale e il limitato accesso al file per la lettura
    /*
        while ((byte_r=read(bacheca_fd, &c , 1))==1){ // leggo dal file sorgente byte a byte fino ad arrivare alla fine della riga
                buff[count] = c; // salvo qui dentro
                count+=byte_r;

                if(c== '\n' ){ // se arrivo alla new line ho finito la riga
                    if((byte_w=write(fd_sock, buff , count ))<0){ // scrivo su socket
                         perror("error to WRITE socket in reader_bacheca");
                         sem_post(&s1);
                         signal_unlock();
                         return -1;
                     }
                    ret=read(fd_sock , msg , strlen(ack)); // vado in attesa dell riscrontro del client dandomi l ok sul continuare a leggere la bacheca
                    if(ret<=0){ // in caso il client chiude la connessione nel mezzo della lettura esco con codice -1
                           sem_post(&s1);
                           signal_unlock();
                           return -1;
                    }
                    count =0 ; // reset del count
                }
        }if(byte_r <0){ // controllo errore della read
                perror("error to READ socket in reader_bacheca");
                sem_post(&s1);
                signal_unlock();
                exit(EXIT_FAILURE);
            }
*/
    while ((byte_r=read(bacheca_fd, buff , SIZE)) > 0){ //   leggo  SIZE byte
            flag =1; // variabile che indica che e stata eseguita almeno una lettura
             if((byte_w=write(fd_sock, buff , strlen(buff) ))<0){ // scrivo su socket
                 perror("error to WRITE socket in reader_bacheca");
                 sem_post(&s1);
                 signal_unlock();
                 return -1;
             }
             memset(buff, 0, SIZE); ; // reset del buffer con tutti 0
             ret=read(fd_sock , msg , strlen(ack)); // vado in attesa dell riscrontro del client dandomi l ok sul continuare a leggere la bachec
             if(ret<=0){ // in caso il client chiude la connessione nel mezzo della lettura esco con codice -1
                 sem_post(&s1);
                 signal_unlock();
                 return -1;
            }
     }if(byte_r <0){ // controllo errore della read
                perror("error to READ socket in reader_bacheca");
                sem_post(&s1);
                signal_unlock();
                exit(EXIT_FAILURE);
     }
    
sem_post(&s1);
signal_unlock();
return flag;
}

///////////////////////////////////////////////////////// FUNZIONI E PROCEDURE INERENTI AL BACKUP CONTROLLO E RESTORE DEI FILE /////////////////////////////////////////


// non rileva la mancanza di righe se il file delle chiavi le perde una riga intera

int control_usr_pwd(void){
     int byte_r;
     char c;
     int  n_line=6; // numero inventato
     signal_lock();
     sem_wait(&s2);
     lseek(pwd_fd , 0 ,SEEK_SET);
     lseek(login_fd , 0 ,SEEK_SET);
     while((byte_r = read(pwd_fd, &c , 1))==1){ //leggo tutte le linee di file del backup e incremento
                if(c== '\n'){
                    n_line++ ;
            }
        }
    while((byte_r = read(login_fd, &c , 1))==1){ //leggo tutte le linee di file del backup e decremento
                if(c== '\n'){
                    n_line-- ;
            }
        }
    if(n_line==6){
            sem_post(&s2);
            signal_unlock();
            return 0;
    }else{
        sem_post(&s2);
        signal_unlock();
        return -1;
    }
}
// attenzione il campo opt serve ad evitare che il file delle chiavi di backup invalidi il controllo previo backup successivo
int control(int opt){ // questo controlla la bacheca e il file delle chiavi della bacheca
     int byte_r, byte_r_1;
     int count=0;
     long j ,i , k;
     char *str,*token;
     char buff_1[MAXLINE];
     char *buff1[MAXLINE];
     char buff_2[MAXLINE];
     char *buff2[MAXLINE];
     char c;
    off_t start;
    int fd , n_line=0 ;
    signal_lock();
    sem_wait(&s1);
    if((fd = open(file_9 , O_CREAT|O_RDWR , 06660 )) == -1){ // apertura del file di nomi utente per il login
        perror("error to open USER_LOGIN file ");
        exit(-1);
    }
    while((byte_r = read(fd, &c , 1))==1){ //leggo tutte le linee di file del backup e incremento  (tale sistema ci aiuta a capire se manca qualche riga nei file )
                if(c== '\n'){
                    n_line++ ;
            }
        }
redo:
     while((byte_r = read(bacheca_key_fd, &c , 1))==1){ // leggo la linea delle chaivi
            buff_1[count] = c;
            count+=byte_r;
           if(c== '\n' ){
                   count =0 ;
                    break;
           }
     }
     if(byte_r == 0){ // se finisco di leggere il file delle chiavi allora significa che ho tutti i match
            if(opt==0){ // escludo il controllo incrociato con il file delle chiavi
                fflush(stdout);
                close(fd);
                sem_post(&s1);
                signal_unlock();
               return 0;

            }else if(n_line==0){
               close(fd);
               sem_post(&s1);
               signal_unlock();
               return 0;

            }else{
                close(fd);
                sem_post(&s1);
                signal_unlock();
                return -1;
            }
        }
      //tokenizzo le chaivi
      for (i = 0, str = buff_1; ; i++, str = NULL) {
            token = strtok(str, "|");
            if (token == NULL)
                break;
            buff1[i]=malloc(sizeof(strlen(token)));
            strcpy(buff1[i], token);
     }

    start = atol(buff1[2]);
    lseek(bacheca_fd ,start , SEEK_SET); // mi porto alla riga dove devo leggere la riga su la bacheca
    while((byte_r_1 = read(bacheca_fd, &c , 1))==1){ // ne leggo l intera riga
            buff_2[count] = c;
            count+=byte_r;
           if(c== '\n' ){
                   count =0 ;
                    break;
           }
     }
    if(byte_r_1 == 0){ // se finisco di leggere la bacheca ma ho ancora chiavi significa che qualcosa non va
        close(fd);
        sem_post(&s1);
        signal_unlock();
        return -1;
    }
      //tokenizzo
    for (j = 0, str = buff_2; ; j++, str = NULL) {
        token = strtok(str, " :");  // tokenizzo gli spazi
        if (token == NULL)
            break;
        buff2[j]=malloc(sizeof(strlen(token)));
        strcpy(buff2[j], token);
     }

    for(k=0; k<(j-6) ; k++){
      if((strcmp(buff1[0], buff2[k]))==0){ // se esce un match allora vado avanti
           fflush(stdout);
           n_line--;
            goto redo;
        }
    }

close(fd);
sem_post(&s1);
signal_unlock();
return -1; // return in caso che non si matchano il file delle chiavi e la bacheca
}


// questa funzione ha il compito di fare periodicamente un backup dei file di estrema importanza alla vita del server
void backup(int sgnumber){ // ha signumber perche e invocata con un alarm
	printf("start backup\n");
    char buff[SIZE];
    int des[4]={0};
    int i ,ret,byte_r,byte_w,count =0;
    char c;
    if(sgnumber!=5){
       if((ret = control(0))==-1){ // se trova problemi con i nuovi file allora adopeera un restore del vecchio backup
         printf("Corrupted BOARD files restore the last backup \n");
         restore(4); // fa il restore di tutit i file
         return;
      }else
         printf("The BOARD has been checked successfully\n");

      if ((control_usr_pwd()) == 0){// controlal se il file delle password e untenti sia giusto
         printf("check on USERS file successful\n");
      }else{
         printf("corrupt user files restore the latest backup \n");
         restore(2);
      }
    }
    if((des[0] = open(file_6 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file di nomi utente per il login
        perror("error to open USER_LOGIN file ");
        exit(-1);
    }
    if((des[1] = open(file_7 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file delle passwrod di login
        perror("error to open PWD_LOGIN file ");
        exit(-1);
    }
    if((des[2] = open(file_8 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file di bacheca
        perror("error to open BACHECA file ");
        exit(-1);
    }
    if((des[3] = open(file_9 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file di bacheca con le infromazioni dei messaggi
        perror("error to open KEY_BACHECA file ");
        exit(-1);
    }
    signal_lock();
    sem_wait(&s1); // devo bloccare qualsiasi client che tenda di modifcare il file cosi riesco a salvarlo correttamente

    for(i=0 ; i<4;i++){
        printf("backup file no. %d \n", i);
        fflush(stdout);
        lseek(descriptor[i],0 , SEEK_SET); // mi porto all inizio del file sorgente
        lseek(des[i],0 , SEEK_SET); // mi porto all inizio del file destinazione
            while ((byte_r=read(descriptor[i], &c , 1))==1){ // leggo dal file sorgente byte a byte fino ad arrivare alla fine della riga
                buff[count] = c; // salvo qui dentro
                count+=byte_r;
                if(c == '\n'  ){ // se arrivo alla new line ho finito la riga
                    if((byte_w=write(des[i], buff , count ))<0){ // scrivo sul file destinazione
                         perror("error to write file backup");
                         exit(EXIT_FAILURE);
                     }
                    count =0 ;
                }
            }if(byte_r <0){ // controllo errore della read
                perror("error to read file backup");
                exit(EXIT_FAILURE);
            }
    }
    sem_post(&s1);
    signal_unlock();


    printf("END backup\n");
     for(i=0 ; i<4;i++){
        close(des[i]);
     }
	alarm(timeout_bck); 
}

int restore(int num){
	printf("START restore\n");
    char buff[SIZE];
    int des[4]={0};
    int i =0 ,byte_r,byte_w,count =0;
    char c;
    if((des[0] = open(file_6 , O_CREAT|O_RDWR , 06660 )) == -1){ // apertura del file di nomi utente per il login_backup
        perror("error to open USER_LOGIN file ");
        exit(-1);
    }
    if((des[1] = open(file_7 , O_CREAT|O_RDWR , 06660 )) == -1){ // apertura del file delle passwrod di login_backup
        perror("error to open PWD_LOGIN file ");
        exit(-1);
    }
     if((des[2] = open(file_8 , O_CREAT|O_RDWR , 06660 )) == -1){ // apertura del file di bacheca_backup
        perror("error to open BACHECA file ");
        exit(-1);
    }
    if((des[3] = open(file_9 , O_CREAT|O_RDWR , 06660 )) == -1){ // apertura del file di bacheca_key_backup con le infromazioni dei messaggi
        perror("error to open KEY_BACHECA file ");
        exit(-1);
    }

    if(num==2){ // se passo 2 fa il restore dei file di login
        i=0;
        close(descriptor[0]);
        close(descriptor[1]);
        if((login_fd = open(file_2 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file di nomi utente per il login
            perror("error to open USER_LOGIN file ");
            exit(-1);
        }
        if((pwd_fd = open(file_3 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file delle passwrod di login
            perror("error to open PWD_LOGIN file ");
            exit(-1);
        }
        descriptor[0] = login_fd;
        descriptor[1] = pwd_fd;

    }else{ // se passo 4 fa il restore solo del file bacheca
        i=2;
         close(descriptor[2]);
        close(descriptor[3]);
        if((bacheca_fd = open(file_4 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file di bacheca
            perror("error to open BACHECA file ");
            exit(-1);
        }
        if((bacheca_key_fd = open(file_5 , O_CREAT|O_RDWR|O_TRUNC , 06660 )) == -1){ // apertura del file di bacheca con le infromazioni dei messaggi
            perror("error to open KEY_BACHECA file ");
            exit(-1);
        }
        descriptor[2] = bacheca_fd;
        descriptor[3] = bacheca_key_fd;
    }
    signal_lock();
    sem_wait(&s1); // devo bloccare qualsiasi client che tenda di modifcare il file cosi riesco a salvarlo correttamente
    for(i ; i<num;i++){
        printf("restore file no. %d \n", i);
        fflush(stdout);
        lseek(descriptor[i],0 , SEEK_SET); // mi porto all inizio del file sorgente
        lseek(des[i],0 , SEEK_SET); // mi porto all inizio del file destinazione
            while ((byte_r=read(des[i], &c , 1))==1){ // leggo dal file sorgente byte a byte fino ad arrivare alla fine della riga
                buff[count] = c;
                count+=byte_r;
                if(c== '\n' ){ // se arrivo alla new line ho finito la riga
                    if((byte_w=write(descriptor[i], buff , count ))<0){ // scrivo sul file destinazione
                         perror("error to write file backup");
                         sem_post(&s1);
                         signal_unlock();
                         exit(EXIT_FAILURE);
                     }
                    count =0 ;
                }
            }if(byte_r <0){ // controllo errore della read
                perror("error to read file backup");
                sem_post(&s1);
                signal_unlock();
                exit(EXIT_FAILURE);
            }
    }
    sem_post(&s1);
    signal_unlock();
    printf("END restore \n");
    for(i=0 ; i<4;i++)
        close(des[i]);

return 0 ;
}

//////////////////////////// FUNZIONI E PROCEDURE MAIN DEL SERVER //////////////////////////////////


void *thread_function(void * arg){
    struct information_connect * user;
    user = (struct information_connect *) arg ;
    signal (SIGINT , signal_handler); // SEGNALE DI USCITA
    char *addr = user->mem_private;
    int fd_sock = user -> des_sock;
    int ret , n;
    char *cmd = malloc(SIZE);
    char c;
    user->tid = gettid(); // inserisco nella struttura l identificatore del thread

    signal(SIGPIPE, sigpipe_handler); // quello che voglo e che il thread specifico capti il segnale di sigpipe cosi che lui stesso andra a chiudersi tramite la logout
    signal(SIGSEGV, sigsegv_handler); // con la segmantation fault

retry:
    if((ret = autenticator( user , 0)) ==-1){ // mando il client a fare il login
redo:   // se torna -1
        memset(addr , 0 , SIZE ); // reset area di memoria
        writer(str9 , fd_sock , 0); //stampa il messaggio di riprova o nuovo utente (se non risesco a scrivere su la socket perche il client ha chiuso la connessione allora) allora effettuo un logout
        ret = reader(addr, fd_sock,1); // legge la risposta dell utente se new , try , logout
        addr = upper(addr); // lo converte in maiuscolo
        if(ret<=0){
             free(cmd);
            printf("the client closed the connection during authentication\n");
            fflush(stdout);
            logout(fd_sock , user );
        }

        if((strcmp(addr , str7))==0){ // caso RETRY rivado a fare il login tramite autenticator
            goto retry;

        }else if((strcmp(addr , str8))==0){// caso NEW faccio registrare il mio nuovo utente
            if((ret =register_new_user(user))==-1){ // nel caso ritorni -1 l utente ha chiuso la connessione e ne faccio il logout dal server
                free(cmd);
                printf("the client closed the connection during registration\n");
                fflush(stdout);
                logout(fd_sock , user );
            }

        }else if((strcmp(addr , str17))==0){ // caso LOGOUT faccio il logout dell utente dal server
            free(cmd);
            logout(fd_sock , user );
        }else {goto redo;} // se mando una stringa qualsiasi che non rientri nelle precedenti allora ristampo il messaggio su cosa deve fare l utente
    }else if(ret ==-2){
            free(cmd);
            printf("the client closed the connection during authentication\n");
            fflush(stdout);
            logout(fd_sock , user );
    }

    while(1){
        writer(legend , fd_sock , 0); // stampa la lista dei comandi che puo fare l utente
        memset(addr , 0 , SIZE );
        memset(cmd , 0 , SIZE );
        ret = reader(cmd, fd_sock,1); // leggo il comando che passa l utente

        if(ret<=0){ // provo a leggere la socket se torna 0 allora significhera che il client ha chiuso la conenssione
            free(cmd);
            printf("the client has logged out\n");
            fflush(stdout);
            logout(fd_sock , user );
        }
        cmd =upper(cmd);
        if((strcmp(cmd , str10))==0){ // LIST
            n= reader_bacheca(addr ,bacheca_fd,fd_sock);  // lista tutti i messaggi in bacheca
            if(n==0){
                writer(str16 , fd_sock , 0); // scrivo che la bacheca e vuota al client
                read(fd_sock , addr , strlen(ack)); // attendo ack di riscontro dal client
            }else if(n==-1){ // se torna -1 allora significa che il client ha chiuso la connessione (la sua socket)
                free(cmd);
                printf("the client closed the connection while reading the bulletin board\n");
                fflush(stdout);
                logout(fd_sock , user );
            }else{
                writer(str25 , fd_sock , 0); // scrivo all utente che e arrivato alla fine della bacheca
                read(fd_sock , addr , strlen(ack)); // attendo la ricezione di un ack dal client
            }


        }else if((strcmp(cmd , str11))==0){ // POST inserisce il messaggio
             if((n = insertMessage(user->login_name,fd_sock , addr))==-1){// fnzione di inseriemnto se torna -1 allora vado in logout
                free(cmd);
                printf("the client closed the connection while posting a new message \n");
                fflush(stdout);
                logout(fd_sock , user );
            }else

                 write(fd_sock,str13 , strlen(str13)); // scrivo che il messaggio e stato inserito correttamente
                 read(fd_sock , addr , strlen(ack)); // riscontro con il client


        }else if((strcmp(cmd , str12))==0){ // REMOVE
            ret = removeMessage(fd_sock  , user, addr); // vado nella remove
            if(ret ==-1){
                writer(str5 , fd_sock , 0); // scrive al client che l oggetto cercato in bacheca non esiste
                read(fd_sock , addr , strlen(ack)); // riscontro con il client

            }else if(ret == -2){
                free(cmd);
                printf("the client closed the connection while it was deleting a message \n");
                fflush(stdout);
                logout(fd_sock , user );
            }else{
                writer(str15 , fd_sock , 0);
                read(fd_sock , addr , strlen(ack)); // riscontro con il client
            }
        }else if((strcmp(cmd , str17))==0){ //LOGOUT
            free(cmd);
            printf("the client has logged out\n");
            fflush(stdout);
            logout(fd_sock , user );

        }else if((strcmp(cmd , str19))==0){ //WHOAMI
            whoami(fd_sock,user);
            read(fd_sock ,addr , strlen(ack)); // vado in attesa dell riscrontro del client dandomi l ok sul continuare a leggere la bacheca

        }else{     // ERRORE DI INSERIMENTO DEL COMANDO
            writer(str14 , fd_sock , 0);
        }
    }
}



void connect_client( int sock_listen ){ // funzione di creazione dell della socket
    int len , ret;
    int sock_con ; // variabile della socket creata
    struct sockaddr_in cliaddr; // refuso dovuto al fatto di voler captare l ip del utente 
    pthread_t tid[MAX_CON];
    long i=0 ;
    int fd ;

    if((login_fd = open(file_2 , O_CREAT|O_RDWR , 06660 )) == -1){ // apertura del file di nomi utente per il login
        perror("error to open USER_LOGIN file ");
        exit(-1);
    }
    if((pwd_fd = open(file_3 , O_CREAT|O_RDWR , 06660 )) == -1){ // apertura del file delle passwrod di login
        perror("error to open PWD_LOGIN file ");
        exit(-1);
    }
     if((bacheca_fd = open(file_4 , O_CREAT|O_RDWR , 06660 )) == -1){ // apertura del file di bacheca
        perror("error to open BACHECA file ");
        exit(-1);
    }
    if((bacheca_key_fd = open(file_5 , O_CREAT|O_RDWR , 06660 )) == -1){ // apertura del file di bacheca con le infromazioni dei messaggi
        perror("error to open KEY_BACHECA file ");
        exit(-1);
    }
    descriptor[0] = login_fd; // salvo descrittori su variabili globali  e anche in un array
    descriptor[1] = pwd_fd;
    descriptor[2] = bacheca_fd;
    descriptor[3] = bacheca_key_fd;

    if(!setjmp (env)){ // la prima volta che viene invocata ritorna 0 ed il server mi dice che ha impostato un punto di ripristino
        printf("punto di ripristino impostato \n"); // messo in questa zona del codice perche voglio che mi vengano ricontrollati i file del server
        fflush(stdout);
    }


    if((fd = open(file_9 , O_RDWR , 06660))<0){ // se manca il file delle chiavi di bacheca significa che i file id backup non li abbaimo quindi vado a farli subito
        printf("missing backup files I create them \n");
        fflush(stdout);
        backup(5);
    }

    else{ // se li abbiamo possiamo procedere al controllo
        close(fd);
        if ((control(1)) == 0){// control 1 -> incrocia 3 file bacheca , key_bacheca , key_bacheca_backup
        printf("The BOARD has been checked successfully\n");
        fflush(stdout);

        }else{
        printf("Corrupted BOARD files restore the last backup \n");
        fflush(stdout);
        restore(4);
        printf("the files have been restored restart the server");
        fflush(stdout);
        exit(0);
        }
        if ((control_usr_pwd()) == 0){// controlla se il file delle password e utenti sia giusto
            printf("check on USERS file successful\n");
            fflush(stdout);
        }else{
            printf("corrupt user files restore the latest backup \n");
            fflush(stdout);
            restore(2);
            printf("the files have been restored restart the server");
            fflush(stdout);
            exit(0);
        }
    }

    while (1){
    start:
        printf("Listening..... \n");
        len = sizeof(cliaddr);
        if((sock_con = accept (sock_listen ,(struct sockaddr*)&cliaddr, &len))== -1){ // chiamata bloccante finche non arriva una connessione
             perror("accept error");
        }
        for(i =0  ;i<MAX_CON ; i++){
            if(bitmask[i] == 0){ // appena trovo un elemento con 0 lo imposto a 1
                client[i].mem_private = (char *)mmap(NULL , SIZE ,  PROT_READ| PROT_WRITE ,MAP_SHARED|MAP_ANONYMOUS,0,0); // area di memoria per uso esclusivo dell utente
                bitmask[i]=1; // imposto il bit a 1
                break; // esco

            }else if(bitmask[i] == -1){
                bitmask[i]=1; // imposto il bit a 1
                break; // esco

            }else
                continue;
        }
        if(i==(MAX_CON-1)){ // se il for arriva alla fine della sua esecuzione e trova tutto occupato 
            close(sock_con); // chiudo il descrittore aperto in fase di accept  
            goto start;  // salto al inizio del while 
        }

        printf("client no. %ld is connected\n", i);
        client[i].i = i ; // indice dell utente
        client[i].des_sock = sock_con; // canale esclusivo di comunicazione 
        client[i].login_name = "NULL";  // nome dell utente che si sta connettendo (sara modificato in fase di autenticazione )
        client[i].login_pwd = "NULL" ; // password di login dell utente (sara modificato in fase di autenticazione )
        create_user_pwd_login();// aggiorna 2 bufferini con password e user (messa qui perche ad ogni client si aggiorna la lista inerente ai bufferini usr e pwd )
        if((ret = pthread_create(&tid[i] , NULL , thread_function , (void *)&client[i]))!=0){ // spawn del thread
            perror("error to create thread");
            exit(EXIT_FAILURE);
        }
    }
}

int main(void){
    int sock_listen ; // variabile di instanziazione della socket di ascolto
    int option = 1;
    max_fault = MAX_FAULT ; // do alla variaibile globale un numero massimo di fault raggiungibili oltre il quale il server termina
    struct sockaddr_in  servaddr; // variabile struttura della socket
    if((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) <0){// creazione della variabile socket
        perror("Error Create Socket ");
        exit(1);
    }
    sem_init(&s1 , 0 , 1);
    sem_init(&s2 , 0 , 1);
    signal(SIGALRM, backup); // chiamo il signale per l intero processo al ricevimento il segnale eseguo wakeup

	alarm(timeout_bck); // richiediamo al sistema di spettare 2 seondi per ricevere il segnale
    tid_main = gettid(); // prelevo l identificatore del thread main
    // imposto i segnali che mi servono
    sigfillset(&set); // blocco tutti i sengali per riattivare solo quelli necessari
    sigdelset(&set , 14); // sigalarm
    sigdelset(&set , 2);  // sigint
    sigdelset(&set , 1); // sighup
    sigdelset(&set , 11);  // sigsegv
    sigdelset(&set , 13); // sigpipe
    sigprocmask(SIG_BLOCK,&set,NULL);

    setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); // campo opzione per riutilizzare subito la socket altrimenti devo aspettare 4 minuti
    memset((void *)&servaddr , 0 , sizeof(servaddr)); //pulizia della struttura sockaddr
    servaddr.sin_family = AF_INET ; // indica il protocollo di comunicazione
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    if ((bind(sock_listen ,(struct sockaddr *) &servaddr, sizeof(servaddr))) == -1) {
        perror("error in bind");
        exit(-1);
    }
    if(listen(sock_listen , BACKLOG) == -1){ // la metto in stato di listen
        perror("listen error");
        exit(-1);
    }

    connect_client(sock_listen); // ne passo il file descriptor della socket
while(1){pause();} // inutile e potrebbe essere tolto in quanto non arriva mai a questa istruzione
}
