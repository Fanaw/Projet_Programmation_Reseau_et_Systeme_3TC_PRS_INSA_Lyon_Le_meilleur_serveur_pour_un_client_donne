#include "serveur.h"


int main(int argc, char* argv[]){
    if(argc != 2){
        printf("Numéro de port nécessaire !\n");
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
            if(strcmp(buffer,"SYN") == 0){ //Si on a bien reçu SYN on continue 
                printf("Envoie du synack avec nouveau port privé\n");
                char syn_ack[12] = "SYN-ACK5009";
                ssize_t sendsyn = sendto(desc,syn_ack,strlen(syn_ack),0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
                int rcv_ack = recvfrom( desc, buffer, sizeof(buffer), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
                if( rcv_ack <= 0 ) {
                    printf( "recvfrom() error \n" );
                    return -1;
                }
                printf("%s\n",buffer); //on reçoit l'acquittement que le nouveau port à bien été reçu
                if(strcmp(buffer,"ACK") == 0){
                    int desc2;
                    if( (desc2 = socket(PF_INET,SOCK_DGRAM,0)) > 0){ //on ouvre une deuxième socket privée
                        int reuseok2;
                        if( (reuseok2 = setsockopt(desc2,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))) >= 0){

                            //Struct sockaddr2 privé
                            struct sockaddr_in my_addr2;
                            memset((char*)&my_addr2,0,sizeof(my_addr2));
                            my_addr2.sin_family = AF_INET;
                            my_addr2.sin_port = htons(5009);
                            my_addr2.sin_addr.s_addr = htonl(INADDR_ANY);

                            //Bind socket2
                            bind(desc2,(struct sockaddr*)&my_addr2,sizeof(my_addr2));

                            printf("Bind2\n");
                            int rcv_mess = recvfrom(desc2, buffer, sizeof(buffer), 0,(struct sockaddr*)&my_addr2.sin_addr, (unsigned*)&cli_len); //on reçoit le nom de fichier à envoyer
                            if( rcv_mess <= 0 ) {
                                printf( "recvfrom() error \n" );
                                return -1;
                            }
                            printf("reçu nom fichier : %s\n",buffer);

                            envoiImage2(buffer,desc2,my_addr2,cli_len);
                            


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

/*"int sendToI(int i, int desc,  struct sockaddr_in my_addr, int cli_len, FILE *f){
    char * ptableau;
    int nbElements = 15;
    ptableau = malloc((nbElements+1) * sizeof(char));
    for(int k=1;k<=i;k++){
        int nbelemrecu = fread( ptableau , sizeof(char) , nbElements , f);
        ptableau[nbelemrecu]= k;
        ssize_t sendLine=sendto(desc , ptableau , nbelemrecu+1 ,0, (struct sockaddr*)&my_addr.sin_addr, cli_len);
        printf("renvoie du sendto n°%d: %d\n",k, (int)sendLine);
    }
    return 1;
}*/

/*int recvFromI(int i,int desc,  struct sockaddr_in my_addr, int cli_len, FILE *f){
    char ACK[5];
    struct timeval timeout;
    timeout.tv_sec=2;
    timeout.tv_usec=0;
    for(int k=1;k<=i;k++){
        select(desc+1,NULL,(fd_set*) desc,NULL, &timeout);
        int rcv_ack = recvfrom(desc, ACK, sizeof(ACK), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);        
    }
    return i;
    
}*/

/*int slowStart(int desc,  struct sockaddr_in my_addr, int cli_len,FILE *f){
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
}*/
void ajoutSeq (int compt, char* messEnvoye){
        if(compt<10)  sprintf(messEnvoye, "00000%d", compt);
        if(10<=compt && compt <100) sprintf(messEnvoye, "0000%d", compt);
        if(100<=compt && compt <1000) sprintf(messEnvoye, "000%d", compt);
        if(1000<=compt && compt <10000) sprintf(messEnvoye, "00%d", compt);
        if(10000<=compt && compt <100000) sprintf(messEnvoye, "0%d", compt);
        if(100000<=compt && compt <1000000) sprintf(messEnvoye, "%d", compt);
}

int envoiImage(char* file,int desc,  struct sockaddr_in my_addr, int cli_len){
    FILE *f = fopen(file,"rb");
    int nbElements = 1000;
    char buffer[nbElements];
    int nbelemrecu=nbElements;
    int compt = 0;
    char ackRecu[10];
    char messEnvoye[6+nbElements];
    fd_set input_set;
    struct timeval timeout;

    
    while(nbelemrecu==nbElements){
        compt++;
        ajoutSeq(compt,messEnvoye);
        printf("mess=%s\n",messEnvoye);
        nbelemrecu = fread( buffer , sizeof(char) , nbElements , f);
        for(int i=0;i<nbelemrecu;i++){ //copie du buffer dans le message à envoyer
            messEnvoye[i+6]=buffer[i];  //à améliorer car perte de temps mais j'arrivais pas avec strcat
        }
        ssize_t sendLine=sendto(desc,messEnvoye,nbelemrecu+6,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
        printf("renvoie du sendLine : %d\n", (int)sendLine);
        
        timeout.tv_sec = 1; 
        timeout.tv_usec = 0;
        FD_SET(desc,&input_set); 
        int res_select=select(desc+1,&input_set,NULL,NULL,&timeout);
        printf("res selct=%d\n",res_select);
        if(res_select==0){
            int res_fseek=fseek(f, -nbelemrecu, SEEK_CUR);
            compt--;
        } else {
        int rcv_ack = recvfrom(desc, ackRecu, sizeof(ackRecu), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);     
        printf("ackRecu reçu : %s\n",ackRecu);
        }
        printf("\n");

    }
    ssize_t sendLine=sendto(desc,"FIN",4,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
    return nbelemrecu;
    
}

int envoiImage2(char* file,int desc,  struct sockaddr_in my_addr, int cli_len){
    FILE *f = fopen(file,"rb");
    int nbElements = 1000;
    char buffer[nbElements];
    char buffer2[nbElements];
    int nbelemrecu=nbElements;
    int nbelemrecu2=nbElements;
    int compt = 0;
    char ackRecu[10];
    char ackRecu2[10];
    char messEnvoye[6+nbElements];
    char messEnvoye2[6+nbElements];
    fd_set input_set;
    struct timeval timeout;

    
    while(nbelemrecu==nbElements && nbelemrecu2==nbElements){
        compt++;
        ajoutSeq(compt,messEnvoye);
        compt++;
        ajoutSeq(compt,messEnvoye2);
        printf("mess=%s\n",messEnvoye);
        printf("mess2=%s\n",messEnvoye2);
        nbelemrecu = fread( buffer , sizeof(char) , nbElements , f);
        nbelemrecu2 = fread( buffer2 , sizeof(char) , nbElements , f);
        for(int i=0;i<nbelemrecu;i++){ //copie du buffer dans le message à envoyer
            messEnvoye[i+6]=buffer[i];  //à améliorer car perte de temps mais j'arrivais pas avec strcat
        }
        for(int i=0;i<nbelemrecu2;i++){ //copie du buffer dans le message à envoyer
            messEnvoye2[i+6]=buffer2[i];  //à améliorer car perte de temps mais j'arrivais pas avec strcat
        }
        ssize_t sendLine=sendto(desc,messEnvoye,nbelemrecu+6,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
        ssize_t sendLine2=sendto(desc,messEnvoye2,nbelemrecu2+6,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
        printf("renvoie du sendLine : %d\n", (int)sendLine);

        timeout.tv_sec = 1; 
        timeout.tv_usec = 0;
        FD_SET(desc,&input_set);
        int res_select=select(desc+1,&input_set,NULL,NULL,&timeout);
        printf("res selct=%d\n",res_select);
        if(res_select==0){ //si on ne recoit aucun ack
            int res_fseek=fseek(f, -nbelemrecu*2, SEEK_CUR); //on recule 2 fois en arrière
            compt-=2;
        } else { //si recu qqc
            int rcv_ack = recvfrom(desc, ackRecu, sizeof(ackRecu), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
            printf("ackRecu reçu : %s\n",ackRecu);

            char ackMesspart1 [10]="ACK";
            char ackMesspart2 [6];
            ajoutSeq(compt-1,ackMesspart2);
            strcat(ackMesspart1,ackMesspart2);
            printf("ack normal=%s\n",ackMesspart1);

            if(strcmp(ackRecu,ackMesspart1) == 0){ //si recu ack1 on voit si reception ack2
                printf("ack 1 recu on voit pour 2\n");
                timeout.tv_sec = 1; 
                timeout.tv_usec = 0;
                FD_SET(desc,&input_set);
                int res_select=select(desc+1,&input_set,NULL,NULL,&timeout);
                printf("res selct=%d\n",res_select);

                if(res_select==0){ //si on n'a pas recu ack2
                    printf("ack 2 pas recu \n");
                    int res_fseek=fseek(f, -nbelemrecu, SEEK_CUR); //on recule 1 fois
                    compt--;
                } else { //si on recoit ack2
                    int rcv_ack2 = recvfrom(desc, ackRecu2, sizeof(ackRecu2), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
                } 
            } //pas de else car si on recoit ack 2 c'est qu'on à recu le 1 aussi
            //FAUX ICI LE CLIENT 1 PEUT FAIRE DES ACK DISCONTINUS !! (donc à revoir)
            printf("\n");

        }

    }
    ssize_t sendLine=sendto(desc,"FIN",4,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
    return nbelemrecu;
    
}


