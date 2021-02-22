#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "pgmread.h"

#define BUFSIZE 2048
#define DATASIZE 1522
#define PACKET_HEADER_LENGTH 12
#define APP_HEADER_LENGTH 8 

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


void error(int ret, char* msg){

	if (ret ==-1){
		perror(msg);
		exit(EXIT_FAILURE);
	}
}
void validate_arguments(int argc,char const *argv[]){
 		if (argc<4){

		printf("use command: ./server port mappenavn filnavn \n");
		exit(EXIT_FAILURE);

		if(opendir(argv[2])==NULL){
			perror("Mappenavn finnes ikke");
			exit(EXIT_FAILURE);
		}
		if(fopen(argv[3],"r")==NULL){
			perror("Utskriftsfil finnes ikke");
			exit(EXIT_FAILURE);
		}
	}
}
void compare_buffers(const char *argv[],struct Packet* packet,FILE * fp){

DIR * dp = opendir(argv[2]);

struct dirent *entry;
char picture_buffer[BUFSIZE];
char picturedata[BUFSIZE];
char fileplacement[250];
memcpy(picturedata,
	   packet->picturedata,
	   packet->payload_length-(2*sizeof(int)+packet->filename_length));
entry = readdir(dp);
struct Image* received_picture;
struct Image* stored_picture;
int match = 0;
 
//Lage et Image av mottat buffer 
received_picture = Image_create(picturedata);

   while ( entry ) {
        if( entry->d_type == DT_REG ){

            //printf("Comparing with: %s\n", entry->d_name);
            //finner fileplassering
            memcpy(fileplacement,argv[2],strlen(argv[2]));
            memcpy(fileplacement+strlen(argv[2]),"/",1);
            memcpy(fileplacement+strlen(argv[2])+1,entry->d_name,entry->d_reclen);
         
            //Lese inn bildedata lagret under filnavn entry->d_name
			FILE * f = fopen(fileplacement,"r");
				if(f==NULL){

					perror("fopen");
					exit(EXIT_FAILURE);
				}

			int fd=fileno(f);
			struct stat st;
			fstat(fd,&st);
			int filesize= st.st_size;


			fread(&picture_buffer,sizeof(char),filesize,f);
            
            //Lage et Image av lagret bildedata og mottatt bildedata
			stored_picture 	 = Image_create(picture_buffer);
			

             //Sammenlikne disse  med Image_compare
			if (Image_compare(stored_picture,received_picture)){
            	//skrive til fil om det det er en match eller ikke
				
				fputs(packet->filename,fp);
				fputs(" ",fp);
				fputs(entry->d_name,fp);
				fputs("\n",fp);
				
				printf("Saved match! \n");
	           	match=1;
	            };



           	Image_free(stored_picture);
			fclose(f);
        } 
        entry = readdir(dp);

    }

if (match==0){
	fputs(packet->filename,fp);
	fputs(" ",fp);
	fputs("UNKNOWN",fp);
	fputs("\n",fp);
				
	printf("Saved UNKNOWN! \n");
	};

Image_free(received_picture);
closedir(dp);

printf("Buffer compare complete! \n");
};




// Definere funksjoner som ligger i filfunctions.c
struct Node* add_linked_list_item(struct Node **head_ref,char *indata);
void display(struct Node **head_ref);
void display_seq_no(struct Node **head_ref);
struct Node* get_start(struct Node **head_ref);
struct Node* read_files(struct Node **head_ref);
void read_filenames(const char * tekstfile);
struct Node* delete_acked_packet(struct Node **head_ref, char ack_no);
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

int main(int argc, char const *argv[])
{
validate_arguments(argc, argv);

//Deklarerer og initierer variable og datastrukter
int port= atoi(argv[1]);
int so,rc;
char buf[BUFSIZE];
char s[BUFSIZE];
struct in_addr ipadresse;
struct sockaddr_in adresse;
struct sockaddr_storage their_addr;
socklen_t addr_len = sizeof(their_addr);
uint8_t expecting_seq_no=0x00;

//Nyttige funksjoner
uint8_t respond(struct Packet *pakke, uint8_t expecting_seq_no,char const *argv[],FILE *fp){

	if(pakke->seq_no==expecting_seq_no){

		//Sender ack på pakken
		uint8_t ack =  pakke->seq_no;

		uint8_t *ack_packet = build_packet(PACKET_HEADER_LENGTH,
											  0x0,
											  ack,
											  0x02,
											  0x7f,
											  0,
											  "\0");

		rc= sendto(so,ack_packet,PACKET_HEADER_LENGTH,0,(struct sockaddr *)&their_addr,addr_len);
		free(ack_packet);

		switch (pakke->flag){

			case 0x01:
					compare_buffers(argv,pakke,fp);
					expecting_seq_no++;
					break;
			case 0x04:
					free(pakke);
					//Lukker diverse ting
					fclose(fp);
					close(so);
					break;
			case 0x02:
					break;
			default:
					printf("Unknown flag given. ");
					break;
		};

	}else{
		printf("Not correct sequence number! ");

	};

		return expecting_seq_no;
};

uint8_t validate_packet(char * buffer,uint8_t expecting_seq_no,char const *argv[],FILE *fp){

	struct Packet * pakke= unpack(buf);

	expecting_seq_no =respond(pakke,expecting_seq_no,argv,fp);

	free(pakke);

return expecting_seq_no;
};


//Åpner en fil å skrive til
FILE * fp = fopen(argv[3],"w");
/* Lytte til en ipv4-adresse på lokalhost */

inet_pton(AF_INET,"127.0.0.1",&ipadresse);

/* Lytte til en IPv4-adresse på PORT 1234 på lokalhost */

adresse.sin_family = AF_INET;
adresse.sin_port = htons(port);
adresse.sin_addr = ipadresse;

/*  Vil ha en socket som tar imot datagrammer fra en IPv4 */

so = socket(AF_INET,SOCK_DGRAM,0);
error(so,"socket");

/* Socketen skal lytte på addressen vi har satt (lokalhost, port 1234)  */
rc = bind(so,(struct sockaddr *)&adresse, sizeof(struct sockaddr_in));
error(rc,"bind");



//Klar til å lytte etter pakker.

/* Vent til det kommer noe, deretter lagres alt opp til BUFSIZE-1 i buffer*/
while (1){

	rc=recvfrom(so, buf, BUFSIZE-1, 0,(struct sockaddr *)&their_addr, &addr_len);
	error(rc,"recvfrom");

	printf("received %d bytes sequence no: %x \n ",rc,buf[4]);
	
	if(buf[6]==0x04){ //Hvis det mottas en termineringspakke
		break;
	};

	expecting_seq_no= validate_packet(buf,expecting_seq_no,argv,fp);

	printf("Expecting sequence number: %x \n",expecting_seq_no);

};





//Lukker diverse ting
fclose(fp);
close(so);

	return EXIT_SUCCESS;
}