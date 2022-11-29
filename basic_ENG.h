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

#define SERV_PORT	6000
#define BACKLOG		3
#define MAXLINE		1024
#define PAGE_SIZE   4096
#define N_PAGE      1
#define SIZE        N_PAGE*PAGE_SIZE
#define SIZE_NAME   100
#define SIZE_OBJ    20
#define MAX_CON     100
#define MAX_FAULT   3
#define timeout_bck    120// ogni 60 secondi il thread main si fa il backup dei file aggiornati
#define timeout_con    360// Tempo di sessione del client se non fa nulla entro questo tempo chiudo la connessione

char *ack="ACK"; // inserirlo per gestire la latenza di stampa della bacheca fra client e server

 //  insertMessage(char *name ,  int fd,  char *msg)
char *str0 = "enter the subject of the message:";
char *str1 = "enter the message text:";
char *str2 = "invalid character in passed object try again:";
char *str6 = "you have inserted an object that is too large, try again";
char *str24= "you have entered a text that is too large in the bulletin board try again";


//int  removeMessage(int  fd , char *msg , struct information_connect  *host){
char *str3 = "enter the subject of the message you want to delete:";
char *str4 = "failed authentication";
char *str5 = "remove failed check that the object exists or is yours \n";

//  void *thread_function(void * arg)
char *str7  = "RETRY";
char *str8  = "NEW";
char *str9  = "RETRY -> to retry, NEW-> to create a new user, LOGOUT -> to exit the server";
char *str10 = "LIST";
char *str11 = "POST";
char *str12 = "REMOVE";
char *str13 = "message entered correctly";
char *str14 = "No command Found \n";
char *str15 = "remove performed successfully \n";
char *str16 = "the content of the board is empty\n\n";
char *str17 = "LOGOUT";
char *str18 = "Logout successful\n";
char *str19 = "WHOAMI";
char *str20 = "user already logged in\n"; // si trova nell autenticator
char *str21 = "username not available \n"; // si trova in register_new_user
char *str22 = "If you are a new user enter any character from the keyboard and press enter. \nif you are already registered enter your username. \nusername: "; // si trova in autenticator
char *str23 = "password: "; // si trova in autenticator
char *str25 = "end of bulletin board";


char *legend = "\nThe allowed actions are:                                        cmd\n1) Read all your messages on the electronic bulletin board ->   LIST\n2) Deposit a new message on the bulletin board             ->   POST\n3) Remove your message from the bulletin board             ->   REMOVE\n4) To log out                                              ->   LOGOUT\n5) Print your account details                              ->   WHOAMI\n";


//int register_new_user(struct information_connect  *host)
char *str26 = "enter your username: ";
char *str27 = "enter your password: ";


char *str28 = "I'm on the bulletin board\n\n";
char *str29 = "your credentials are\n";

char *str30 = "ready";


char *file_2 = "user_login.txt"; // descriptor [0]
char *file_3 = "pwd_login.txt"; // descriotir [1];
char *file_4 = "bacheca.txt"; // descriptor[2];
char *file_5 = "key_bacheca.txt" ; // descriptor[3];
char *file_6 = "user_login_backup.txt"; // des[0]
char *file_7 = "pwd_login_backup.txt"; // des[1]
char *file_8 = "bacheca_backup.txt"; // des[2]
char *file_9 = "key_bacheca_backup.txt" ; // des[3]
char *file_10 = "temporary.txt";
char *file_11 = "text.txt";

// client

char *cli_str0 = "Session stopped by client \n";
char *cli_str1 = "Timeout Session\n";
char *cli_str2 = "Invalid input retry\n";
char *cli_str3 = "Segmantation Fault \n";
char *cli_str4 = "signal SIGQUIT received \n";
char *cli_str5 = "Server comunication closed \n";






