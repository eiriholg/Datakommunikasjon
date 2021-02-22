#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>



#define DATASIZE 1522
#define BUFSIZE 1522
#define PACKET_HEADER_LENGTH 12
#define APP_HEADER_LENGTH 8 


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

struct Node *picturedata; 
struct Node *filenames;


//Funksjonsdeklarasjon for å unngå implisit function declaration
void do_before_exit();
//Funksjon som skal lage linked list

struct Node* add_linked_list_item(struct Node **head_ref, char *indata, int indata_size){

	//Lager nye nodepekere 
	struct Node *new_node, *node; 
	//Allokerer plass til en struct
	new_node= (struct Node*)malloc(sizeof(struct Node));

	memset(new_node->data, '\0', DATASIZE); //Nullstiller minnet. 
	memcpy(new_node->data, indata, indata_size);
	new_node -> next = NULL; 

	//Sjekker om listen er tom
	if ((*head_ref) == NULL){

		//Initialiserer start
		(*head_ref) = (struct Node*)malloc(sizeof(struct Node));
		(*head_ref) -> next = new_node;
	}else{

		//Legger til noden etter den forrige ved å loope over hele listen

		node = (*head_ref);
		while(node->next != NULL){

			node = node->next;
		}
		node ->next = new_node;
	} 

	return (*head_ref);
};

void display(struct Node **head_ref) 
{ 
	if ((*head_ref)==NULL){

		printf("The list is empty \n");
		return;
	}

    struct Node* node; 
    node = (*head_ref); 
    node = node->next; 
    while (node != NULL) { 
    	printf("%p ->", node);
        printf("%s \n", node->data);

        node = node->next; 
    } 
    printf("\n"); 
    return ; 
} 
void display_seq_no(struct Node **head_ref) 
{ 
  if ((*head_ref)==NULL){

    printf("The list is empty \n");
    return;
  }

    struct Node* node; 
    node = (*head_ref); 
    node = node->next; 

    printf("Not ACKed package list: ");
    while (node != NULL) { 
      
        printf("%x ", node->data[4]);

        node = node->next; 
    } 
    printf("\n"); 
    return ; 
} 
 
void read_filenames(const char * textfile) 
{ 
	
	char buffer[BUFSIZE];
	memset(buffer,'\0',BUFSIZE);

	FILE *f = fopen(textfile,"r");
		if (f==NULL){ //Sjekker om filen eksisterer
	            perror("fopen ");
	            exit(EXIT_FAILURE);
	            };

	while(fgets(buffer,BUFSIZE-1,f)!=NULL){
			strtok(buffer,"\n");
			add_linked_list_item(&filenames,buffer,strlen(buffer));

		}

	fclose(f);
	     
    
    return; 
}
struct Node* read_files(struct Node **head_ref) 
{ 
	if ((*head_ref)==NULL){

		printf("The list is empty \n");
		return NULL;
	}
	char buffer[BUFSIZE];
	memset(buffer,'\0',BUFSIZE);
    struct Node* node; 
    node = (*head_ref); 
    node = node->next; 
    while (node != NULL) { 
    	 
        FILE * stream = fopen(node->data,"r"); // Mangler newline
			if (stream==NULL){ //Sjekker om filen eksisterer
            perror("Error: ");
            do_before_exit();
            return NULL;
        };

		fread(&buffer,sizeof(char),BUFSIZE-1,stream);
		add_linked_list_item(&picturedata, buffer,strlen(buffer));


		fclose(stream);
        node = node->next; 
    }
    return (*head_ref); 
}

struct Node* get_start(struct Node **head_ref)
{ 
	if ((*head_ref)==NULL){

		printf("The list is empty \n");
		return NULL;
	}

    struct Node* node; 
    node = (*head_ref); 
    node = node->next; 

    return node; 
 }


struct Node * delete_acked_packet(struct Node **head_ref, char ack_no){

	//lagre head 
	struct Node* temp = (*head_ref), *prev;
	temp = temp->next; //Go to the first node.

 
 

	if(temp != NULL && (temp->data[4] == ack_no)){ 

		(*head_ref)->next = temp->next;   // Changed head 
        printf("Deleting package \n");
        free(temp);               // free old head 
        return (*head_ref); 
	}

	while (temp != NULL && (temp->data[4] != ack_no))
    { 
        prev = temp; 
        temp = temp->next; 
    } 
     // If key was not present in linked list 
    if (temp == NULL) 
    	{
    		printf("Didn't find the package \n");
    	return (*head_ref);
    } 

    // Unlink the node from linked list 
    prev->next = temp->next; 
    printf("Deleting package \n");
    free(temp);  // Free memory 
    return (*head_ref); 
}


void delete_list(struct Node **head_ref){
	struct Node* current = *head_ref;
	struct Node* next;

	while (current != NULL){

		next = current -> next; //Nytt head
		free(current);
		current =next;
	
	}

  *head_ref=NULL;
  }


void do_before_exit(){

	delete_list(&filenames);
	delete_list(&picturedata);
	
}

uint8_t* pack_uint8(uint8_t* dest, uint8_t src) {
   *(dest++) = src;
   return dest;
}

uint8_t* pack_uint32be(uint8_t* dest, uint32_t src) {
   *(dest++) = src >> 24;
   *(dest++) = (src >> 16) & 0xFF;
   *(dest++) = (src >> 8) & 0xFF;
   *(dest++) = src & 0xFF;
   return dest;
}

uint8_t* build_packet(
   uint32_t packet_length,
   uint8_t seq,
   uint8_t ack,
   uint8_t flag,
   uint8_t unused,
   uint32_t body_length,
   uint8_t* body
) {
   uint8_t* packet = malloc(sizeof(uint8_t)*PACKET_HEADER_LENGTH+body_length);
   memset(packet, 0 , PACKET_HEADER_LENGTH+body_length);
   uint8_t* p = packet;
   p = pack_uint32be(p,packet_length);
   p = pack_uint8(p, seq);
   p = pack_uint8(p, ack);
   p = pack_uint8(p, flag);
   p = pack_uint8(p, unused);
   p = pack_uint32be(p, body_length);
   memcpy(p, body, body_length);
   return packet;
}

uint8_t* build_payload(
   uint32_t uniq_no,
   uint32_t filename_length,
   uint8_t* filename,
   uint8_t* picturedata
) {
   uint8_t* payload = malloc(
   							sizeof(uint8_t)*APP_HEADER_LENGTH+
   							strlen(filename)+
   							strlen(picturedata)+1); //+1 for to null-byte

   memset(payload, 0 , sizeof(uint8_t)*APP_HEADER_LENGTH+
   						strlen(filename)+
   						strlen(picturedata)+1); //+1 for to null-byte
   //Setter inn dataene 
   uint8_t* p = payload;
   p = pack_uint32be(p,uniq_no);
   p = pack_uint32be(p, filename_length);
   memcpy(p, filename, filename_length);
   p=p+filename_length-1; //Skriver kun inn filnavn, ikke null-byte
   memcpy(p,"\0",1); // Skriver inn null-byte
   p++;
   memcpy(p, picturedata, strlen(picturedata));
  
   return payload;
}

struct Packet *unpack(char* buffer){

  struct Packet *pakke = (struct Packet*)malloc(sizeof(struct Packet));
  

  //Lagrer pakkestørrelse.
  int tmp_packet_size;
  memcpy(&tmp_packet_size,buffer,sizeof(int));
  pakke->packet_size=ntohl(tmp_packet_size);

  //Lagrer sekvensnummer
  memcpy(&pakke->seq_no,buffer+sizeof(int),sizeof(char));

  //Lagrer ack-nummer
  memcpy(&pakke->ack_no,buffer+sizeof(int)+sizeof(char),sizeof(char));

  //Lagrer flagg
  memcpy(&pakke->flag,buffer+sizeof(int)+2*sizeof(char),sizeof(char));

  //lagrer unused 
  memcpy(&pakke->unused,buffer+sizeof(int)+3*sizeof(char),sizeof(char));
  
  if (pakke->flag == 0x01){ // Hvis pakken inneholder payload
    int tmp_payload_length;
    memcpy(&tmp_payload_length,buffer+sizeof(int)+4*sizeof(char),sizeof(int));
    pakke->payload_length= ntohl(tmp_payload_length);

    int tmp_uniq_no;
    memcpy(&tmp_uniq_no,buffer+2*sizeof(int)+4*sizeof(char),sizeof(int));
    pakke->uniq_no= ntohl(tmp_uniq_no);

    int tmp_filename_length;
    memcpy(&tmp_filename_length,buffer+3*sizeof(int)+4*sizeof(char),sizeof(int));
    pakke->filename_length= ntohl(tmp_filename_length);

    memset(pakke->filename,0,150);
    memset(pakke->picturedata,0,1522);

    memcpy(pakke->filename,buffer+4*sizeof(int)+4*sizeof(char),pakke->filename_length);
    pakke->filename[pakke->filename_length]=0;
    memcpy(pakke->picturedata,
      buffer+4*sizeof(int)+4*sizeof(char)+pakke->filename_length,
      pakke->payload_length-(2*sizeof(int)+pakke->filename_length));
  }
  printf("Unpack complete! \n");
  
  return pakke;
};