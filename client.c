#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <libgen.h>
#include <sys/select.h>
#include <time.h>



#include "send_packet.h"

void error(int ret, char* msg){

	if (ret ==-1){
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

#define DATASIZE 1522
#define BUFSIZE 1522
#define PACKET_HEADER_LENGTH 12
#define APP_HEADER_LENGTH 8 
#define STDIN_FD 0
#define SLIDING_WINDOW 7
#define THRESHOLD 7
#define PORT 5678


struct Node
{
	char data[DATASIZE]; 
	struct Node *next; 
};

struct Packet{
	uint32_t packet_size;
	uint8_t seq_no;
	uint8_t ack_no;
	uint8_t flag;
	uint8_t unused;
	uint32_t payload_length;
	uint32_t uniq_no;
	uint32_t filename_length;
	uint8_t filename[150];
	char picturedata[1522];
};


// Definere funksjoner 
struct Node* add_linked_list_item(struct Node **head_ref,char *indata, int indata_size);
void display(struct Node **head_ref);
void display_seq_no(struct Node **head_ref);
struct Node* get_start(struct Node **head_ref);
struct Node* read_files(struct Node **head_ref);
void read_filenames(const char * tekstfile);
struct Node * delete_acked_packet(struct Node **head_ref, char ack_no);
void delete_list(struct Node **head_ref);
void do_before_exit();
uint8_t* pack_uint8(uint8_t* dest, uint8_t src);
uint8_t* pack_uint32be(uint8_t* dest, uint32_t src);
uint8_t* build_packet(
   uint32_t packet_length,
   uint8_t seq,
   uint8_t ack,
   uint8_t flag,
   uint8_t unused,
   uint32_t body_length,
   uint8_t* body
);
uint8_t* build_payload(
   uint32_t uniq_no,
   uint32_t filename_length,
   uint8_t* filename,
   uint8_t* picturedata
);
struct Packet *unpack(char* buffer);

struct Node *picturedata; 
struct Node *filenames;
struct Node *Packets;
struct Node *NotACKedPackets;

void validate_arguments(int argc,char const *argv[]){
 		if (argc<5){

		printf("use command: ./client IP-address serverport textfile.txt drop_percentage \n");
		exit(EXIT_FAILURE);

		if(fopen(argv[3],"r")==NULL){
			perror("Argumentfil finnes ikke");
			exit(EXIT_FAILURE);
		}
	}
}


int main(int argc, char const *argv[]){

validate_arguments(argc, argv);

int dp = atoi(argv[4]);
float drop_percentage = dp/(float)100; 
printf("drop percentage: %4.2f \n", drop_percentage);

set_loss_probability(drop_percentage);

//Definerer og instasierer
char buf[BUFSIZE];
memset(buf,'\0',BUFSIZE-1);
char * filename;
char * payload;
char * packet;
uint32_t uniq_no=5000;
uint8_t seq_no =0x00;



read_filenames(argv[3]); //Lager en lenket-liste av filnavn
//display(&filenames);
read_files(&filenames);  //Lager en lenket- liste med bildedata
//display(&picturedata);

FILE* fd= fopen("packetinfo.txt", "wb");

//hente ut informasjon fra filenames og picturedata.
struct Node * start;
start = filenames->next;

struct Node * picturedata_start;
picturedata_start = picturedata->next;

//Lage liste over pakker med nyttelast



while(start!=NULL|picturedata_start!=NULL){

	filename= basename(start->data);
	memcpy(buf,picturedata_start->data,strlen(picturedata_start->data));
    payload = build_payload(uniq_no,
						    strlen(filename)+1,
						    filename,
						    buf);

	packet = build_packet(PACKET_HEADER_LENGTH+APP_HEADER_LENGTH+strlen(filename)+strlen(buf)+1,
						  seq_no,
						  0x0,
						  0x01,
						  0x7f,
						  APP_HEADER_LENGTH+strlen(filename)+strlen(buf)+1,
						  payload);


	add_linked_list_item(&Packets,packet,
		                 PACKET_HEADER_LENGTH+APP_HEADER_LENGTH+strlen(filename)+strlen(buf)+1); 
	fwrite(packet,
		   PACKET_HEADER_LENGTH+APP_HEADER_LENGTH+strlen(filename)+strlen(buf)+1,
		   1,
		   fd);


	start=start->next;
	picturedata_start=picturedata_start->next;
	uniq_no++;
	seq_no++;
	memset(buf,'\0',BUFSIZE-1);
	free(payload);
	free(packet);

    };


/* Åpner en forbindelse til serveren og sender
bildene til den*/
int so,rc;


struct in_addr ipadresse;
struct sockaddr_in destaddr, myaddr;

fd_set set;
time_t last_ack, curr_time;
struct timeval tv;



/* setter lokalhost som IPv4 adresse*/

inet_pton(AF_INET,argv[1],&ipadresse);

/* skal sende med IPv4 på PORT 1234 lokalhost ipadresse */

myaddr.sin_family = AF_INET;
myaddr.sin_port = htons(PORT);
myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

destaddr.sin_family = AF_INET;
destaddr.sin_port = htons(atoi(argv[2]));
destaddr.sin_addr = ipadresse;

/*  Vil ha en socket som sender datagrammer med IPv4 */

so = socket(AF_INET,SOCK_DGRAM,0);
error(so,"socket");

rc = bind(so, (struct sockaddr *)&myaddr, sizeof(struct sockaddr_in));
    error(rc, "bind");

//Initialiserer fd set
 
FD_ZERO(&set);
FD_SET(so, &set);

/* initialiserer timeval structen */
tv.tv_sec = 5;
tv.tv_usec = 0;

/* start timer */
last_ack = time(NULL); 

//Finner første pakke
struct Node * packet_list;
packet_list = Packets->next;

struct Node * NotACKed_list;
//Finner størrelsen til pakken

int packet_size;
int tmp_packet_size;
int count=0;


while(packet_list!=NULL){

	memcpy(&tmp_packet_size,packet_list->data,sizeof(int));
	packet_size=ntohl(tmp_packet_size);

	memcpy(buf, packet_list->data, packet_size);

	while (count<SLIDING_WINDOW){

		memcpy(&tmp_packet_size,packet_list->data,sizeof(int));
		packet_size=ntohl(tmp_packet_size);

		memcpy(buf, packet_list->data, packet_size);

		rc = send_packet(so,
				buf,
				packet_size,
				0,
				(struct sockaddr *)&destaddr,
				sizeof(struct sockaddr_in)	
		  			);
		error(rc,"sendto");
		printf("Sent %d bytes! Sequence no: %x \n",rc,buf[4]);

		add_linked_list_item(&NotACKedPackets,packet_list->data,packet_size);
		count++;
		display_seq_no(&NotACKedPackets);
		packet_list=packet_list->next;
	};
	

	//venter på svar 
	rc = select(FD_SETSIZE, &set, NULL, NULL, &tv);
    error(rc, "select");

            /* Vi har mottatt en melding */
        if (rc && FD_ISSET(so, &set)) {

            rc=recvfrom(so,buf, BUFSIZE-1,0,NULL,NULL);
		    error(rc,"recvfrom");

			struct Packet *serversvar = unpack(buf);

			buf[rc]='\0';
			printf("Received %d bytes: \n Ack_no: %x \n ",rc, serversvar->ack_no);

			free(serversvar);
			

            if (buf[6]==0x02) {
                printf("Received ACK\n");
                NotACKedPackets=delete_acked_packet(&NotACKedPackets,buf[5]);
                count--;
                last_ack = time(NULL);

                continue;
            }
        }
        /* select har timet ut */
        else  {
        	printf("Timeout! Sending packets in window again. \n");
        		
				NotACKed_list = NotACKedPackets->next;
				
				
            	while (NotACKed_list!=NULL){

	            	memcpy(&tmp_packet_size,NotACKed_list->data,sizeof(int));
					packet_size=ntohl(tmp_packet_size);

					memcpy(buf, NotACKed_list->data, packet_size);
	            	
	            	rc = send_packet(so,
									buf,
									packet_size,
									0,
									(struct sockaddr *)&destaddr,
									sizeof(struct sockaddr_in)	
			  						);
					error(rc,"sendto");

					printf("Sent %d bytes! Sequence no: %x \n",rc,buf[4]);

					NotACKed_list=NotACKed_list->next;

			};
			free(NotACKed_list);
            /* reset structene til select */
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            FD_SET(so, &set);
        }
    }


char * termination_packet = build_packet(PACKET_HEADER_LENGTH,
						  0x30,
						  0x0,
						  0x04,
						  0x7f,
						  0,
						  "\0");

rc = send_packet(so,
				termination_packet,
				PACKET_HEADER_LENGTH,
				0,
				(struct sockaddr *)&destaddr,
				sizeof(struct sockaddr_in)	
		  			);
	error(rc,"sendto");

free(termination_packet);

//Lukker socket 
close(so);
//Frigjøre minne 

//Lukke åpne filer
fclose(fd);
//fclose(fd2);
delete_list(&Packets);
delete_list(&NotACKedPackets);
do_before_exit();


	return EXIT_SUCCESS;
}