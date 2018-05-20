#include "serveur_client1.h"


void sigfun(int sig)
{
    printf("Fermeture du serveur bye\n");
    close(desc);
    exit(0);
	(void) signal(SIGINT, SIG_DFL);
}

int main(int argc, char* argv[]){
    clock_t temps=clock();
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
            
            while(1){
                signal(SIGINT, sigfun);
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
                    
                    printf("Envoi du synack avec nouveau port privé\n");
                    numport++;
                    char syn_ack[20] = "SYN-ACK";
                    char port[4];
                    sprintf(port, "%d", numport);
                    strcat(syn_ack, port);
                    ssize_t sendsyn = sendto(desc,syn_ack,strlen(syn_ack),0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
                    int rcv_ack = recvfrom( desc, buffer, sizeof(buffer), 0,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
                    if( rcv_ack <= 0 ) {
                        printf( "recvfrom() error \n" );
                        return -1;
                    }
                    printf("%s\n",buffer); //on reçoit l'acquittement que le nouveau port à bien été reçu
                    if(strcmp(buffer,"ACK") == 0){
                        printf("Ouverture deuxième socket\n");
                        int desc2;
                        if( (desc2 = socket(PF_INET,SOCK_DGRAM,0)) > 0){ //on ouvre une deuxième socket privée
                            int reuseok2;
                            printf("desc2=%d\n",desc2);
                            if( (reuseok2 = setsockopt(desc2,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))) >= 0){

                                //Struct sockaddr2 privé
                                struct sockaddr_in my_addr2;
                                memset((char*)&my_addr2,0,sizeof(my_addr2));
                                my_addr2.sin_family = AF_INET;
                                my_addr2.sin_port = htons(numport);
                                my_addr2.sin_addr.s_addr = htonl(INADDR_ANY);

                                //Bind socket2
                                bind(desc2,(struct sockaddr*)&my_addr2,sizeof(my_addr2));
                                printf("Bind2\n");
                                int pid = fork();
                                if(pid == 0){

                                    printf("je suis dans le fils\n");
                                    int rcv_mess = recvfrom(desc2, buffer, sizeof(buffer), 0,(struct sockaddr*)&my_addr2.sin_addr, (unsigned*)&cli_len); //on reçoit le nom de fichier à envoyer
                                    if( rcv_mess <= 0 ) {
                                        printf( "recvfrom() error \n" );
                                        return -1;
                                    }
                                    printf("reçu nom fichier : %s\n",buffer);

                                    slowStart(buffer,desc2,my_addr2,cli_len);
                                    close(desc2);
                                    exit(0);

                                }
                            }else{printf("erreur setsockopt 2");return -1;}
                        } else{printf("erreur création socket 2");return-1;}
                        close(desc2);
                    }else{printf("mauvaise réception de l'ACK");return-1;}
                        
                }else{printf("mauvaise réception du SYN");return-1;}
            }       
        }else{printf("erreur setsockopt 1");return -1;}

        close(desc);
        clock_t temps2=clock();
        printf("\n diff de temp : %lf\n",(double)(temps2-temps)/CLOCKS_PER_SEC);
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
    int nbElements = 1493;
    char buffer[nbElements];
    int nbelemrecu=nbElements;
    int seq = 0;
    char ackRecu[10];
    char messEnvoye[6+nbElements];
    fd_set input_set;
    struct timeval timeout;
    printf("on commence");
    
    while(nbelemrecu==nbElements){
        seq++;
        ajoutSeq(seq,messEnvoye);
        printf("mess=%s\n",messEnvoye);
        nbelemrecu = fread( buffer , sizeof(char) , nbElements , f);
        /*for(int i=0;i<nbelemrecu;i++){ //copie du buffer dans le message à envoyer
            messEnvoye[i+6]=buffer[i];  //à améliorer car perte de temps mais j'arrivais pas avec strcat
        }*/
        memcpy(messEnvoye+6,buffer,nbelemrecu);
        ssize_t sendLine=sendto(desc,messEnvoye,nbelemrecu+6,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
        printf("renvoie du sendLine : %d\n", (int)sendLine);
        
        timeout.tv_sec = 0; 
        timeout.tv_usec = 2000;
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
    int nbElements = 1493;
    char buffer[nbElements];
    int nbelemrecu=nbElements;
    int seq = 0;
    char ackRecu[10];
    char messEnvoye[6+nbElements];
    clock_t temps=0;
    int max=0;
    int numSeq;
    int cwnd=1;
    int ssthresh=130;
    int compt;

    
    while(nbelemrecu==nbElements){ //tant que ce n'est pas la fin du fichier
        compt=0;
        if (cwnd==0) cwnd++;
        while(compt<cwnd){ //on envoie cwnd paquets
            seq++;
            ajoutSeq(seq,messEnvoye); //on garde le numero de la dernière sequence
            //printf("mess=%s\n",messEnvoye);
            nbelemrecu = fread( buffer , sizeof(char) , nbElements , f);
            memcpy(messEnvoye+6,buffer,nbelemrecu);
            ssize_t sendLine=sendto(desc,messEnvoye,nbelemrecu+6,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
            compt++;
        }

        temps=clock()*1000000/CLOCKS_PER_SEC; //on lance un timer
        while(((clock()*1000000/CLOCKS_PER_SEC)-temps)<(700*cwnd) && max<seq){ //tant qu'on est pas en timeout et qu'on à pas reçu le plus grand ack
            int rcv_ack = recvfrom(desc, ackRecu, sizeof(ackRecu),MSG_DONTWAIT,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
            int numAck=ackToInt(ackRecu);
            max=getMax(numAck,max);
        }

        if(max==seq){ //si on à recu l'ack du dernier paquet envoyé
            if(cwnd>=ssthresh) { //si on est en congestion avoidance
                cwnd+=5;
            } else {
                cwnd*=2; //on double la fenetre
            }
        } else if (max>seq){ //si on recoit un ack du groupe de message d'avant
            int diffAck=seq-max;
            seq-=diffAck;
            fseek(f, -nbelemrecu*diffAck, SEEK_CUR); //on actualise le curseur dans le fichier
            if(cwnd>ssthresh) { //si on est en congestion avoidance
                cwnd+=5;
            } else {
                cwnd*=2; //on double la fenetre
            }
        }else {
            int nbAckNonRecu=seq-max;
            seq-=nbAckNonRecu;
            fseek(f, -nbelemrecu*nbAckNonRecu, SEEK_CUR); //on actualise le curseur dans le fichier
            ssthresh=cwnd/1.7; //on fixe le threshold pr congestion avoidance
            cwnd/=2; //on divise par 2
        } printf("cwnd=%d\n",cwnd);
        //printf("\n");
    }
    ssize_t sendLine=sendto(desc,"FIN",4,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
    fclose(f);
    return nbelemrecu;

    
}
