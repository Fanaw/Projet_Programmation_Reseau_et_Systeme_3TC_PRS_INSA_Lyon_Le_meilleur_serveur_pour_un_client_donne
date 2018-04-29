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

                            slowStart(buffer,desc2,my_addr2,cli_len);
                            


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

int getMax(int a,int b){
	if(a>b){
		return a;
	}else{
		return b;
	}
}

void ajoutSeq (int seq, char* messEnvoye){
        if(seq<10)  sprintf(messEnvoye, "00000%d", seq);
        if(10<=seq && seq <100) sprintf(messEnvoye, "0000%d", seq);
        if(100<=seq && seq <1000) sprintf(messEnvoye, "000%d", seq);
        if(1000<=seq && seq <10000) sprintf(messEnvoye, "00%d", seq);
        if(10000<=seq && seq <100000) sprintf(messEnvoye, "0%d", seq);
        if(100000<=seq && seq <1000000) sprintf(messEnvoye, "%d", seq);
}

int ackToInt(char* ack){
	char numAck[6]="";
	int i;
	for(i=0;i<6;i++){
		numAck[i]=ack[i+3];
	}
	return atoi(numAck);
}

int envoiImage(char* file,int desc,  struct sockaddr_in my_addr, int cli_len){
    FILE *f = fopen(file,"rb");
    int nbElements = 1000;
    char buffer[nbElements];
    int nbelemrecu=nbElements;
    int seq = 0;
    char ackRecu[10];
    char messEnvoye[6+nbElements];
    fd_set input_set;
    struct timeval timeout;

    
    while(nbelemrecu==nbElements){
        seq++;
        ajoutSeq(seq,messEnvoye);
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
            seq--;
        } else {
        int rcv_ack = recvfrom(desc, ackRecu, sizeof(ackRecu), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);     
        printf("ackRecu reçu : %s\n",ackRecu);
        }
        printf("\n");

    }
    ssize_t sendLine=sendto(desc,"FIN",4,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
    return nbelemrecu;
    
}


int slowStart(char* file,int desc,  struct sockaddr_in my_addr, int cli_len){
    FILE *f = fopen(file,"rb");
    int nbElements = 1000;
    char buffer[nbElements];
    int nbelemrecu=nbElements;
    int seq = 0;
    char ackRecu[10];
    char messEnvoye[6+nbElements];
    clock_t temps=0;
    int max=0;
    int numSeq;
    int cwnd=1;
    int ssthresh=100;
    int compt;

    
    while(nbelemrecu==nbElements){ //tant que ce n'est pas la fin du fichier
        compt=0;
        while(compt<cwnd){ //on envoie cwnd paquets
            seq++;
            ajoutSeq(seq,messEnvoye); //on garde le numero de la dernière sequence
            printf("mess=%s\n",messEnvoye);
            nbelemrecu = fread( buffer , sizeof(char) , nbElements , f);
            for(int i=0;i<nbelemrecu;i++){ //copie du buffer dans le message à envoyer
                messEnvoye[i+6]=buffer[i];  //à améliorer ? un epu primaire non ?
            }
        
            ssize_t sendLine=sendto(desc,messEnvoye,nbelemrecu+6,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
            compt++;
            printf("Message %d sur %d envoyé\n",compt,cwnd);
        }

        temps=clock()/CLOCKS_PER_SEC; //on lance un timer
        while((clock()/CLOCKS_PER_SEC-temps)<2 && max<seq){ //tant qu'on est pas en timeout et qu'on à pas reçu le plus grand ack
            int rcv_ack = recvfrom(desc, ackRecu, sizeof(ackRecu),MSG_DONTWAIT,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
            
            //implémenter fast recovery

            int numAck=ackToInt(ackRecu);
            max=getMax(numAck,max);
        }

        if(max==seq){ //si on à recu l'ack du dernier paquet envoyé
            printf("on est bon\n");
            if(cwnd>ssthresh) { //si on est en congestion avoidance
                cwnd+=1;
                printf("on est au dessus du seuil donc +1\n");
            } else {
                cwnd*=2; //on double la fenetre
                printf("on double !\n");
            }
            printf("cwnd=%d\n",cwnd);
        } else if (max>seq){ //si on recoit un ack du groupe de message d'avant
            int diffAck=seq-max;
            printf("ack supérieur : %d\n",diffAck);
            seq-=diffAck;
            fseek(f, -nbelemrecu*diffAck, SEEK_CUR); //on actualise le curseur dans le fichier
            if(cwnd>ssthresh) { //si on est en congestion avoidance
                cwnd+=1;
                printf("on est au dessus du seuil donc +1\n");
            } else {
                cwnd*=2; //on double la fenetre
                printf("on double !\n");
            }
        }else {
            int nbAckNonRecu=seq-max;
            printf("nb ack non recu : %d\n",nbAckNonRecu);
            seq-=nbAckNonRecu;
            fseek(f, -nbelemrecu*nbAckNonRecu, SEEK_CUR); //on actualise le curseur dans le fichier
            ssthresh=cwnd/2; //on fixe le threshold pr congestion avoidance
            printf("actualisation du threshold : %d\n",ssthresh);
            cwnd=1; //on repare de 1
        } printf("\n");
    }
    ssize_t sendLine=sendto(desc,"FIN",4,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
    return nbelemrecu;
    
}
