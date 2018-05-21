#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

//Initialisation variable globale
int desc_connect;
int option = 1; 
int desc_client;
int port_client=4999;


void sigfun(int sig);
void closing(int desc1,int desc2);
void ajoutSeq (int seq, char* messEnvoye);
int envoiImage(char* file,int desc,  struct sockaddr_in my_addr, int cli_len);
int slowStart(char* file,int desc,  struct sockaddr_in my_addr, int cli_len);
int getMax(int a,int b);
int ackToInt(char* ack);
