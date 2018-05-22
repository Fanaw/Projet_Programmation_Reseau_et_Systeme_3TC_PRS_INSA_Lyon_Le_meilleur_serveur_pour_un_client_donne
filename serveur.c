#include "serveur.h"

#define TAILLE_PAQUET 1494
#define ELEM_RESTANT tailleFichier-nbPaquet*TAILLE_PAQUET


//______________Début_du_programme________________\\ 

int main(int argc, char* argv[]){
    if(argc != 2){
        exit(0);
    }

    //Initialisation Port et Descripteur
    int port = atoi(argv[1]);
    

    
    //Initialisation pour Socket
    struct sockaddr_in sa_connect;
    memset((char*)&sa_connect,0,sizeof(sa_connect));
    sa_connect.sin_family = AF_INET;
    sa_connect.sin_port = htons(port);
    sa_connect.sin_addr.s_addr = htonl(INADDR_ANY);


    //Création Socket
    if ( (desc_connect = socket(PF_INET,SOCK_DGRAM,0)) > 0 ){

        //Bind Socket Connect
        if(bind(desc_connect,(struct sockaddr*)&sa_connect,sizeof(sa_connect)) == 0){
        }else{
            closing(desc_client,desc_connect);
            exit(0);
        }


        //While SYN ACK (connect)
        while(1){
            //Détournement Ctrl-C
            signal (SIGINT, sigfun);

            //SYN Client
            char buffer[50];
            int cli_len = sizeof(sa_connect.sin_addr);
            if( recvfrom( desc_connect, buffer, sizeof(buffer), 0,(struct sockaddr*)&sa_connect.sin_addr, (unsigned*)&cli_len) <= 0){
                exit(0);
            }

            //Si SYN reçu on continu
            if(strcmp(buffer,"SYN") == 0){
                port_client++;
                char syn_ack[20] = "SYN-ACK";
                char port[4];
                sprintf(port, "%d", port_client);
                strcat(syn_ack, port);
                sendto(desc_connect,syn_ack,strlen(syn_ack),0,(struct sockaddr*)&sa_connect.sin_addr, cli_len);
                if(recvfrom( desc_connect, buffer, sizeof(buffer), 0,(struct sockaddr*)&sa_connect.sin_addr, (unsigned*)&cli_len) <= 0){

                }
                //Si ACK reçu on continu
                if(strcmp(buffer,"ACK") == 0){
                }
                //Création Socket Client
                if( (desc_client = socket(PF_INET,SOCK_DGRAM,0)) > 0){
                    
                    if(setsockopt(desc_client,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option)) >= 0){

                        //Bind Socket Client
                        struct sockaddr_in sa_client;
                        memset((char*)&sa_client,0,sizeof(sa_client));
                        sa_client.sin_family = AF_INET;
                        sa_client.sin_port = htons(port_client);
                        sa_client.sin_addr.s_addr = htonl(INADDR_ANY);

                        bind(desc_client,(struct sockaddr*)&sa_client,sizeof(sa_client));

                        //Création processus fils
                        int pid = fork();
                        if(pid == 0){

                            //Réception du nom de fichier
                            if( recvfrom(desc_client, buffer, sizeof(buffer), 0,(struct sockaddr*)&sa_client.sin_addr, (unsigned*)&cli_len) <= 0){
                            }

                            //Envoi du fichier
                            slowStart(buffer,desc_client,sa_client,cli_len);

                            closing(desc_client,desc_connect);

                        }



                       

                    }

                }else{
                    closing(desc_client,desc_connect);
                    exit(0);
                }

    
            }

        }





    }else{
        closing(desc_client,desc_connect);
        exit(0);
    }


   


}

//______________Fin_du_Programme________________\\ 


//Fonction détournement signal Ctrl-C
void sigfun(int sig){
    
    closing(desc_client,desc_connect);
    exit(0);
    (void) signal(SIGINT, SIG_DFL);
}

void closing(int desc1, int desc2){
    close(desc1);
    close(desc2);
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


int getMax(int a,int b){
	if(a>b){
		return a;
	}else{
		return b;
	}
}

int slowStart(char* file,int desc,  struct sockaddr_in my_addr, int cli_len){
    char buffer[TAILLE_PAQUET];
    int nbelemrecu=TAILLE_PAQUET;
    int seq = 0;
    int nbPaquet=0;
    char ackRecu[10];
    char messEnvoye[6+TAILLE_PAQUET];
    clock_t temps=0;
    int max=0;
    int numSeq;
    int cwnd=20;
    int ssthresh=130;
    int compt;
    
    FILE *f = fopen(file,"rb");
    fseek (f, 0, SEEK_END);   // non-portable
    int tailleFichier=ftell (f);        
    char *bufferfile = malloc(tailleFichier);
    fseek(f, 0, SEEK_SET);
    int lect=fread(bufferfile,1,tailleFichier,f);   
    int fin=0;
    int seqmax=1+tailleFichier/TAILLE_PAQUET;
    
    while(nbelemrecu==TAILLE_PAQUET){ //tant que ce n'est pas la fin du fichier
        compt=0;
        if (cwnd==0) cwnd++;
        while(compt<cwnd){ //on envoie cwnd paquets
            seq++;
            ajoutSeq(seq,messEnvoye); //on garde le numero de la dernière sequence
            if(ELEM_RESTANT>TAILLE_PAQUET){
                memcpy(buffer,bufferfile+TAILLE_PAQUET*nbPaquet,TAILLE_PAQUET);
                nbelemrecu=TAILLE_PAQUET;
                memcpy(messEnvoye+6,buffer,nbelemrecu);
                nbPaquet++;
                ssize_t sendLine=sendto(desc,messEnvoye,nbelemrecu+6,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
            }else{
                memcpy(buffer,bufferfile+TAILLE_PAQUET*nbPaquet,ELEM_RESTANT);
                nbelemrecu=ELEM_RESTANT;
                fin=1;
                memcpy(messEnvoye+6,buffer,ELEM_RESTANT);
                nbPaquet++;
                compt=cwnd;
                ssize_t sendLine=sendto(desc,messEnvoye,nbelemrecu+6,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
            }
            compt++;
        }

        temps=clock()*1000000/CLOCKS_PER_SEC; //on lance un timer
        while(((clock()*1000000/CLOCKS_PER_SEC)-temps)<(200*cwnd) && max<seq){ //tant qu'on est pas en timeout et qu'on à pas reçu le plus grand ack
            
            int rcv_ack = recvfrom(desc, ackRecu, sizeof(ackRecu),MSG_DONTWAIT,(struct sockaddr*)&my_addr.sin_addr, (unsigned*)&cli_len);
            int numAck=ackToInt(ackRecu);
            max=getMax(numAck,max);
        }
        if(fin==1 && max<seq){
            nbelemrecu=TAILLE_PAQUET;
            fin=0;
        }
        if(max==seqmax){
            nbelemrecu=ELEM_RESTANT;
        }
        if(max==seq){ //si on à recu l'ack du dernier paquet envoyé
            if(cwnd>=ssthresh) { //si on est en congestion avoidance
                //cwnd+=5;
            } else {
                //cwnd*=2; //on double la fenetre
            }
        } else if (max>seq){ //si on recoit un ack du groupe de message d'avant
            int diffAck=seq-max;
            seq-=diffAck;
            nbPaquet-=diffAck;
            if(cwnd>ssthresh) { //si on est en congestion avoidance
                //cwnd+=5;
            } else {
                //cwnd*=2; //on double la fenetre
            }
        }else {
            int nbAckNonRecu=seq-max;
            seq-=nbAckNonRecu;
            nbPaquet-=nbAckNonRecu;
            ssthresh=cwnd/1.7; //on fixe le threshold pr congestion avoidance
            //cwnd/=2; //on divise par 2
        }
    }
    ssize_t sendLine=sendto(desc,"FIN",4,0,(struct sockaddr*)&my_addr.sin_addr, cli_len);
    fclose(f);
    return nbelemrecu;

    
}
