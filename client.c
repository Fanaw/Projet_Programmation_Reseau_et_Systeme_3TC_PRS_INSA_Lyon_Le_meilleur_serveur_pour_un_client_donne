#include "client.h"



int main(int argc, char* argv[]){
    if(argc!=3){
        printf("Deux paramètres nécessaires\n");
        exit(0);
    }
    char* ip_serv=argv[1];
    char* port=argv[2];
    int port_serv=atoi(port);
    int desc;
    if( (desc = socket(PF_INET,SOCK_DGRAM,0)) >0){
    printf("%d\n",desc);

    int reuse = 1;
    int reuseok;
        if( (reuseok = setsockopt(desc,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))) >= 0){
            struct sockaddr_in my_addr;
            memset((char*)&my_addr,0,sizeof(my_addr));
            my_addr.sin_family=AF_INET;
            my_addr.sin_port=htons(port_serv);
            inet_aton(ip_serv,&my_addr.sin_addr);
            int test;
            if((test=connect(desc,(struct sockaddr*)&my_addr,sizeof(my_addr))>=0)){
                char syn[4]="SYN";
                char ack[4]="ACK";

                char buffer[50];  
                ssize_t sendsyn=send(desc,syn,sizeof(syn),0);
                printf("sendsyn renvoie %d\n",(int)sendsyn);
                
                int rcv_synack = recv(desc, buffer, sizeof(buffer), 0);
                printf("%d\n",rcv_synack);
                if( rcv_synack <= 0 ) {
                    printf( "recvfrom() error \n" );
                    return -1;
                }
                printf("reception du syn ack : %s\n",buffer);
                if(strcmp(buffer,"SYN-ACK")==0){
                    ssize_t sendack=send(desc,ack,sizeof(ack),0);
                    printf("sendack renvoie %d\n",(int)sendack);
                    char buffer2[20];
                    int rcv_port = recv(desc, buffer2, sizeof(buffer2), 0);
                    printf("new port :%s\n",buffer2);
                    int port_serv2=atoi(buffer2);
                    int desc2;
                    if( (desc2 = socket(PF_INET,SOCK_DGRAM,0)) >0){
                        int reuseok2;
                        if( (reuseok2 = setsockopt(desc2,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))) >= 0){
                            struct sockaddr_in my_addr2;
                            memset((char*)&my_addr2,0,sizeof(my_addr2));
                            my_addr2.sin_family=AF_INET;
                            my_addr2.sin_port=htons(port_serv2);
                            inet_aton(ip_serv,&my_addr2.sin_addr);
                            int test2;
                            if((test2=connect(desc2,(struct sockaddr*)&my_addr2,sizeof(my_addr2))>=0)){
                                char mess[50]="mabite";
                                ssize_t sendmess=send(desc2,mess,sizeof(mess),0);
                                printf("sendmess renvoie %d\n",(int)sendmess);
                                recoitImage("imrecue.png",desc2);
                                system("xpdf ./imgrecue.png");


                            }else{printf("erreur connect");return -1;}
                        }else{printf("erreur setsockopt2");return -1;}
                    }else{printf("erreur création socket2");return-1;}

                }else{printf("reçu autre chose que ack");return-1;}
            }else{printf("erreur connect1");return -1;}

            
        }else{printf("erreur setsockopt1");return -1;}
    return 1;
    }else{printf("erreur création socket1");return-1;}
}

int recoitImage(char* file, int desc){
    FILE *f = fopen(file,"wb");
    char ptableau [16];
    int compt=0;
    int nombreElements = 15;
    char ack[5]="ACK";
    int rcvLine=16;
    while(rcvLine==16){
        compt++;
        rcvLine=recv(desc,ptableau,sizeof(ptableau),0);
        printf("\nrenvoie du rcvLine : %d\n",rcvLine);
        fwrite(ptableau,sizeof(char),15,f);
        for (int i=0;i<rcvLine;i++){  
                printf("(%d)",ptableau[i]);
        }
        printf("\ncompt :%d\n",compt);
        ack[3]=ptableau[rcvLine-1];
        printf("acquittement :%d\n",(int)ack[3]);
        ssize_t sendack=send(desc,ack,sizeof(ack),0);
    }
    
    return rcvLine;
}