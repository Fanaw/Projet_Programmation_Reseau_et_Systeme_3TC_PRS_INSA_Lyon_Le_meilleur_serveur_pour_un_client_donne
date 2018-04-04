#include "serveur.h"


int main(int argc, char* argv[]){
    if(argc != 2){
        printf("Un paramètre nécessaire\n");
        exit(0);
    }

    //port serveur et descripteur
    char* port = argv[1];
    int port_serv = atoi(port);
    int desc;


    //Socket
    if( (desc = socket(PF_INET,SOCK_DGRAM,0)) > 0){
        printf("descripteur : %d\n",desc);

        int reuse = 1;
        int reuseok;
        if( (reuseok = setsockopt(desc,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))) >= 0){

            //Struct sockaddr pour connexion
            struct sockaddr_in my_addr;
            memset((char*)&my_addr,0,sizeof(my_addr));
            my_addr.sin_family = AF_INET;
            my_addr.sin_port = htons(port_serv);
            my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

            //bind
            printf("return bind : %d\n",bind(desc,(struct sockaddr*)&my_addr,sizeof(my_addr)));

            //SYN client
            char buffer[50];
            int cli_len = sizeof(my_addr.sin_addr);
            printf("En attente du SYN\n");
            int rcv_syn = recvfrom( desc, buffer, sizeof(buffer), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
            printf("SYN reçu\n");
            if( rcv_syn <= 0 ) {
                //error SYN
                printf( "recvfrom() error \n" );
                return -1;
            }
            printf("%s\n",buffer);
            if(strcmp(buffer,"SYN") == 0){
                printf("Envoie du synack\n");
                char syn_ack[8] = "SYN-ACK";
                ssize_t sendsyn = sendto(desc,syn_ack,strlen(syn_ack),0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
                int rcv_ack = recvfrom( desc, buffer, sizeof(buffer), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
                if( rcv_ack <= 0 ) {
                    printf( "recvfrom() error \n" );
                    return -1;
                }
                printf("%s\n",buffer);
                if(strcmp(buffer,"ACK") == 0){
                    int desc2;
                    if( (desc2 = socket(PF_INET,SOCK_DGRAM,0)) > 0){
                        int reuseok2;
                        if( (reuseok2 = setsockopt(desc2,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))) >= 0){

                            //Struct sockaddr2 privé
                            struct sockaddr_in my_addr2;
                            memset((char*)&my_addr2,0,sizeof(my_addr2));
                            my_addr2.sin_family = AF_INET;
                            my_addr2.sin_port = htons(4009);
                            my_addr2.sin_addr.s_addr = htonl(INADDR_ANY);

                            //Bind socket2
                            bind(desc2,(struct sockaddr*)&my_addr2,sizeof(my_addr2));


                            char new_port[5] = "4009";
                            ssize_t sendsyn=sendto(desc,new_port,strlen(new_port),0,(struct sockaddr*)&my_addr.sin_addr, cli_len);


                            int rcv_mess = recvfrom(desc2, buffer, sizeof(buffer), 0,(struct sockaddr*)&my_addr2.sin_addr, (unsigned*)&cli_len);
                            if( rcv_mess <= 0 ) {
                                printf( "recvfrom() error \n" );
                                return -1;
                            }
                            printf("reçu socket 2 %s\n",buffer);


//CONNEXION GO !!!!!


                            envoiImage("imgtest.png", desc2,my_addr2,cli_len);
                            


//END !!!!!

                        }else{printf("erreur setsockopt 2");return -1;}
                    } else{printf("erreur création socket 2");return-1;}
                    
                }
            }

        }else{printf("erreur setsockopt 1");return -1;}

        close(desc);
        return 1;
    }else{printf("erreur création socket 1");return-1;}
}

int sendToI(int i, int desc,  struct sockaddr_in my_addr, int cli_len, FILE *f){
    char * ptableau;
    int nombreElements = 15;
    ptableau = malloc((nombreElements+1) * sizeof(char));
    for(int k=1;k<=i;k++){
        int nbelemrecu = fread( ptableau , sizeof(char) , nombreElements , f);
        ptableau[nbelemrecu]= k;
        ssize_t sendLine=sendto(desc , ptableau , nbelemrecu+1 ,0, (struct sockaddr*)&my_addr.sin_addr, cli_len);
        printf("renvoie du sendto n°%d: %d\n",k, (int)sendLine);
    }
    return 1;
}

int recvFromI(int i,int desc,  struct sockaddr_in my_addr, int cli_len, FILE *f){
    char ACK[5];
    struct timeval timeout;
    timeout.tv_sec=2;
    timeout.tv_usec=0;
    for(int k=1;k<=i;k++){
        select(desc+1,NULL,(fd_set*) desc,NULL, &timeout);
        int rcv_ack = recvfrom(desc, ACK, sizeof(ACK), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);        
    }
    return i;
    
}
int slowStart(int desc,  struct sockaddr_in my_addr, int cli_len,FILE *f){
    int mss=0;
    clock_t start_t,end_t,total_t;
    while(1){
        start_t = clock();
        sendToI(mss,desc,my_addr,cli_len,f);
        mss=recvFromI(mss,desc,my_addr,cli_len,f);
        end_t=clock();
        total_t=(double)(end_t-start_t)/mss;
        int rtt = rtt+a*(double)total_t;
    }
    return mss;
}
int envoiImage(char* file,int desc,  struct sockaddr_in my_addr, int cli_len){

    FILE *f = fopen(file,"rb");
    char * ptableau;
    int nombreElements = 15;
    ptableau = malloc((nombreElements+1) * sizeof(char));
    //Ne pas oublier de tester le retour de malloc
    int nbelemrecu=nombreElements;
    int val = 0;
    char ACK[5];
    while(nbelemrecu==nombreElements){
        val++;
        nbelemrecu = fread( ptableau , sizeof(char) , nombreElements , f);
        ptableau[nbelemrecu]= val;
        ssize_t sendLine=sendto(desc,ptableau,nbelemrecu+1,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
        printf("renvoie du sendLine : %d\n", (int)sendLine);
        for (int i=0;i<nbelemrecu+1;i++){  
            printf("(%d)",ptableau[i]);
        }
        printf("\nnum seq %d\n",(int)ptableau[nbelemrecu]);
        printf("val %d\n",val);
        int rcv_ack = recvfrom(desc, ACK, sizeof(ACK), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
        printf("ack reçu n° %d\n",(int)ACK[3]);


    }
    
    return nbelemrecu;
    
}

