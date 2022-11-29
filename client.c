#ifdef IT
#include  "basic_IT.h"

#else
#include  "basic_ENG.h"
#endif

#define fflush(stdin) while(getchar()!='\n')

// variabili globali
int sockfd;
char *string ;
char *buffercv;
int temp; // descrittore del file che conterrà il testo

// dichiarazione in avanti delle procedure e funzioni
void signal_handler (int sgnumber); // funzione invocata dalla sigint o dalla sigalarm
char *upper( char *msg ); // funzione che converte ogni comando passato nella sua controparte maiuscola
char *writer(char *string , int fd , int opt); // funzione atta a scrivere nella socket

int temp; // descrittore del file temporaneo

void signal_handler (int sgnumber){
	int byte_w;
    if(sgnumber == 2) // gestisce il segnale di sigint
        printf("%s", cli_str0);
    else if(sgnumber == 14) // gestisce il segnale di sigalarm
        printf("%s",cli_str1);
    else if(sgnumber == 11) // gestisce il segnale di segmantation fault
        printf("%s",cli_str3);
    else if(sgnumber == 3) // gestisce il segnale di sigquit
        printf("%s",cli_str4);
    else if(sgnumber == 13) // gestisce il segnale di sigpipe
        printf("%s",cli_str5);
    else
        printf("sengnal %d capture please invoke <kill -l > for details " , sgnumber);

     free(string); // rilascio la stringa dato che ha una malloc implicita nella scanf
     free(buffercv); // rilascio il bufferino visto che ha una malloc
     close(sockfd); //chiusura del descrittore
     sleep(1);
     exit(0); // terminazione del processo
}


void sigsegv_handler (int sgnumber){
    int byte_w;
    if(sgnumber == 2)
        printf("%s", cli_str0);
    else
        printf("%s",cli_str1);

     free(string); // rilascio la stringa dato che ha una malloc implicita nella scanf
     free(buffercv); // rilascio il bufferino visto che ha una malloc
     close(sockfd); //chiusura del descrittore
     sleep(1);
     exit(0); // terminazione del processo
}


void sigpipe_handler (int sgnumber){
    int byte_w;
    if(sgnumber == 2)
        printf("%s", cli_str0);
    else
        printf("%s",cli_str1);

     free(string); // rilascio la stringa dato che ha una malloc implicita nella scanf
     free(buffercv); // rilascio il bufferino visto che ha una malloc
     close(sockfd); //chiusura del descrittore
     sleep(1);
     exit(0); // terminazione del processo
}


char *upper( char *msg ){
    size_t i,len = strlen(msg);
    char *baseid = msg; // do a baseid l indirizzo base di msg
    for(i=0; i<len ; i++){
         (*baseid) = toupper((*baseid));
         baseid++;
    }
    return msg;
}

int writer_text(void){
    int len ,ret;
    char *text = malloc(MAXLINE);
    printf("\n");
    size_t bound = 0 ; // reset per ogni inseriemento testo
    close(temp); // chiudo il file e lo riapro troncato in quanto voglio il mio file pulito ogni volta
    if((temp = open(file_11 , O_TRUNC|O_RDWR,06660)) == -1){ // apro un file temporaneo di appoggio vuot
        perror("error to open temp file ");
        exit(-1);
    }
    while  (1){ // Questo while come condizione di uscita e di passare exit
    riprova:
        memset(text , 0 , MAXLINE); // reset della stringa
        fgets(text , MAXLINE , stdin); // passo la stringa che al massimo preleva 1024 byte dal stdin per ogni riga
        len = strlen(text); // calcolo la lunghezza della stringa


        bound = lseek(temp , 0 , SEEK_CUR); // l indice di posizione del file questo mi aiuta perche voglio evitare che qualcuno immetta un testo enorme in bacheca
        if(strcmp(text , "exit\n") == 0 || bound >= UPPER_BOUND ){ // se passo la stringa exit esco , oppure se arrivo a scrivere in bacheca 4096 byte di testo (evita che un utente saturi la bacheca con un solo messaggio )
            if(bound<1){ // controllo per evitare di passare input vuoti
                printf("inserisci un testo non hai passato nulla\n");
                goto riprova;
            }
            free(text);
            printf("invio testo nella bacheca del server\n");
            return 0 ;  // esco dalla funzione
        }
        if(ret = write(temp , text , len )==-1){
            perror("error to write text ");
            free(text);
            return -1;
        }else{
            continue; // reset della stringa
        }
    }
 }


char *writer(char *string , int fd , int opt){ // questo writer serve a passare oggetti e comandi
    int ret;
    int byte_w;
    if(opt == 2){
         if((byte_w = write(fd , string , strlen(string)))<0){ // scrive nella socket
            perror("error to write socket");
            exit(-1);
        }
        return string;
    }
     redo:
        memset(string , 0 , SIZE_OBJ);
        printf("> ");
        fgets(string , SIZE_OBJ , stdin);

        int len= strlen(string);
        string[len-1]='\0';

        if(string[0]=='\0'){ // evita di passare stringhe vuote cosi non va in segmentation fault
            printf("%s",cli_str2); //messaggio di errore
            goto redo; // torno in redo che pulisce la memeoria
        }
        if((byte_w = write(fd , string , SIZE_OBJ-1 ))<0){ // scrive nella socket
            perror("error to write socket");
            exit(-1);
        }
        if(opt==1)
             string = upper(string);
return string;
}

int main(int argc , char **argv){

    int n , byte_r, byte_w;
    struct sockaddr_in   servaddr ; // variabile struttura della socket
    buffercv = malloc(SIZE);
    string= malloc( SIZE_OBJ);

    int ret,rot;
    signal(SIGALRM, signal_handler); // alarm  (timeout )
    signal(SIGINT, signal_handler); // sigint (CTRL +C)
    signal(SIGSEGV, signal_handler); // segmentation fault (in caso il programma faccia quache operazione che non dovrebbe fare )
    signal(SIGPIPE, signal_handler); // sigpipe (in caso si trovasse il canale di comunicazione chiuso )
    //apro il file che dovra ospitare il testo
    if((temp = open(file_11, O_CREAT|O_TRUNC|O_RDWR,06660)) == -1){ // apro un file temporaneo di appoggio vuot
        perror("error to open temp file ");
        exit(-1);
    }
    if(argc != 2){
        perror("insert SERVER_IP : <IP server>");
        exit(1);
    }
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){// creazione della variabile socket
        perror("Error Create Socket ");
        exit(1);
    }
    memset((void *)&servaddr , 0 , sizeof(servaddr));
    servaddr.sin_family = AF_INET ;
    servaddr.sin_port = htons(SERV_PORT);

    // errore per il passiaggio dell ip in formato non corretot
    if(inet_pton(AF_INET , argv[1], &servaddr.sin_addr) <= 0){
        fprintf(stderr, "errore pass an valid IP %s\n", argv[1]);
        exit(1);
    }

    if(connect(sockfd , (struct sockaddr *)&servaddr , sizeof(servaddr)) == -1){
        perror("connect error");
        exit(1);
    }
   sigset_t set; // variabile locale di una bit mask (signal Mask)
   sigfillset(&set);
   sigdelset(&set , 14); // abilito la sigalarm
   sigdelset(&set , 1); // abilito la sighup -> segnale mandato quando chiudo il terminale (se chiudo il processo alla chiusura della finestra di terminale non ha senso stampare un messaggio )
   sigdelset(&set , 2); // abilito la sigint -> segnale di interruzione (ctrl + c )
   sigdelset(&set , 3); // abilito la sigquit -> uscita da tastiera (ctrl + \)
   sigdelset(&set , 15);  // abilito la sigterm -> segnale di terminazine del processo in caso di shutdouwn del pc
   sigdelset(&set , 11);  // sigsegv // in caso di segmantation fault
   sigdelset(&set , 13); // sigpipe // in caso di chiusura della pipe 
   sigprocmask(SIG_BLOCK,&set,NULL);

  while (1){
start:
     alarm(timeout_con);
     memset(buffercv , 0 , SIZE); // pulisco il bufferino di appoggio
     n = read(sockfd , buffercv , SIZE); // e bloccante finche non c e qualcosa da leggere
     printf("%s", buffercv); // stampa a terminale la stringa che mi viene passata
     if(n<0){
        perror("error in read");
        exit(1);

    }else if((ret = strcmp(str28 ,buffercv))==0){ // quando passo LIST salta l inserimento della scanf  perche al primo giro passo la legenda poi mi lista la bacheca e poi deve ristampare
             memset(string , 0 , SIZE_OBJ); // reset della stringa
             write(sockfd , ack , strlen(ack));
         while(1){
             memset(buffercv , 0 , SIZE); // pulisco il bufferino di appoggio
             n = read(sockfd , buffercv , SIZE); // e bloccante finche non c e qualcosa da leggere
             printf("%s", buffercv); // stampa a terminale la stringa che mi viene passata

             if((strcmp(buffercv,str16))==0){
                 write(sockfd , ack , strlen(ack)); // mando ack su socket
                 goto start;

             }else if((ret = strcmp(buffercv,str25))==0){
                write(sockfd , ack , strlen(ack)); // mando ack su socket
                goto start;
             }else{
                write(sockfd , ack , strlen(ack)); // mando ack su socket
             }
          }
    }else if((strcmp(buffercv , str0 ))==0 ){ // richiesta di inserimento oggetto da parte del server
        writer(string,sockfd,0);  // nella writer passerò l oggetto del messaggio
        memset(string , 0 , SIZE_OBJ);
        memset(buffercv , 0 , SIZE); // pulisco il bufferino di appoggio
        n = read(sockfd , buffercv , SIZE); //leggo il testo di inserimento testo
        if(n<0){
            perror("error in read");
            exit(1);
        }
        printf("%s", buffercv); // stampo il messaggio
        ret = writer_text(); //invoco funzione di inseriemnto testo
        writer(str30,sockfd,2); // qui scrivo al server che sono ho il testo della bacheca pronto per essere inviato

        if (ret){ // se ritorna un valore diverso da 0
            close(temp);
            printf("error in text entry %d\n",ret);
        }
        char *data = malloc(SIZE);
        lseek(temp , 0 , SEEK_SET); // metto a 0 il file temp
        while ((byte_r=read(temp, data , SIZE)) > 0){ //   leggo  SIZE byte
             if((byte_w=write(sockfd, data , strlen(data) ))<0){ // scrivo su socket
                perror("error to WRITE socket in reader_bacheca");
                close(temp);
                free(data);
                return -1;
             }

             memset(data, 0, SIZE); // reset del buffer con tutti 0
             ret=read(sockfd , data , strlen(ack)); // vado in attesa dell riscrontro del server dandomi l ok sul continuare a leggere la bachec
             if(ret<=0){
                close(temp);
                free(data);
                return -1;
            }
        memset(data, 0, SIZE); // reset del buffer con tutti 0
      }
      if(byte_r <0){ // controllo errore della read
        perror("error to READ socket in reader_bacheca");
        exit(-1);
      }

      writer(str31,sockfd,2); // mando la exit al server perche sta aspettando in read
      free(data);

    }else if((strcmp(buffercv , str14 ))==0){ // in caso di comando non trovato
        memset(string , 0 , SIZE_OBJ);

    }else if((strcmp(buffercv , str20 ))==0){ // caso di un utente gia connesso
        memset(string , 0 , SIZE_OBJ);

    }else if((strcmp(buffercv , str21 ))==0){ // caso di un utente esistente
        memset(string , 0 , SIZE_OBJ);

    }else if((strcmp(buffercv , str15 ))==0){ // caso della remove successo
        memset(string , 0 ,SIZE_OBJ);
        write(sockfd , ack , strlen(ack)); // mando ack su socket

    }else if((strcmp(buffercv , str5 ))==0){ // caso della remove fallita
        memset(string , 0 , SIZE_OBJ);
        write(sockfd , ack , strlen(ack)); // mando ack su socket

    }else if((strcmp(buffercv , str13 ))==0){ //messaggio inserito
        memset(string , 0 , SIZE_OBJ);
        write(sockfd , ack , strlen(ack)); // mando ack su socket

    }else if((strcmp(buffercv , str29 ))==0){ // caso della whoami
        // legge username e password dell utente
        memset(string , 0 , SIZE_OBJ); // pulisco il comando
        write(sockfd , ack , strlen(ack)); // mando ack su socket
        memset(buffercv , 0 , SIZE); // pulisco il bufferino di appoggio
        n = read(sockfd , buffercv , SIZE); // e bloccante finche non c e qualcosa da leggere
        printf("%s", buffercv); // stampa a terminale la stringa che mi viene passata
        write(sockfd , ack , strlen(ack)); // mando ack su socket

    }else if((strcmp(buffercv , str18 ))==0){ // caso del LOGOUT
            free(string); // rilascio la stringa dato che ha una malloc implicita nella scanf
            free(buffercv); // rilascio il bufferino visto che ha una malloc
            close(sockfd); //chiusura del descrittore
             exit(0); // terminazione del processo
    }else {
        memset(buffercv , 0 , SIZE);
        string = writer(string, sockfd , 1);

    }
  }
}
