#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

double a=0.1;
int envoiImage(char* file,int desc,  struct sockaddr_in my_addr, int cli_len);