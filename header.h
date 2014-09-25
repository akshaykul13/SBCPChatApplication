#include <stdio.h>
#include <stdlib.h>

#define INVALID_USERNAME "The provided username already exists."
#define MAX_CLIENTS_REACHED "The server has reached the maximum number of clients it can handle"

#define JOIN 	2
#define FWD 	3
#define SEND 	4
#define NAK		5
#define OFFLINE 6
#define ACK		7
#define ONLINE	8
#define IDLE	9

/* The structure to hold the SBCP attributes of the SBCP messages. */
struct sbcpattributes{
  int type :16;
  int payloadlength :16;
  char payload[512];
};

/* The structure to hold the SBCP message. */
struct sbcpmessage{
  int version :9;
  char type :7;
  int length :16;
  struct sbcpattributes attributes[4];
};

/* The structure to store the details of the clients which have connected to the server. */
struct clientdetails{
  char username[16];
  int socket_desc;
};

/* This method creates the JOIN SBCP message to be sent by the client to the server. */
void create_join_sbcp_message(struct sbcpmessage **sbcp_message, char username[16])
{	
	(*sbcp_message)->version = 3;
	(*sbcp_message)->length = 1;
	(*sbcp_message)->type = 2;
	(*sbcp_message)->attributes[0].type = 2;
	(*sbcp_message)->attributes[0].payloadlength = strlen(username);
	strcpy((*sbcp_message)->attributes[0].payload, username);					
}

/* This method creates the NAK message in case the username is already present or the client count exceeds the maximum clients the server can handle. */
void create_nak_sbcp_message(struct sbcpmessage **sbcp_message, char *reason)
{
	(*sbcp_message)->version = 3;					 
	(*sbcp_message)->type = 5;
	(*sbcp_message)->length = 1;
    (*sbcp_message)->attributes[0].type = 1;
	(*sbcp_message)->attributes[0].payloadlength = strlen(reason);
	strcpy((*sbcp_message)->attributes[0].payload,reason);	
}

/* This method creates the ACK message to be sent by the server to the client in the case of a successful JOIN. */
void create_ack_sbcp_message(struct sbcpmessage **sbcp_message, char *string, char username[16], int client_count)
{
	(*sbcp_message)->version = 3;					 
	(*sbcp_message)->type = 7;
	(*sbcp_message)->length = 3;
    (*sbcp_message)->attributes[0].type = 2;
	(*sbcp_message)->attributes[0].payloadlength = strlen(username);
	strcpy((*sbcp_message)->attributes[0].payload,username);	
	(*sbcp_message)->attributes[1].type = 3;
	char str[15];
	sprintf(str, "%d", client_count);		
	(*sbcp_message)->attributes[1].payloadlength = strlen(str);
	strcpy((*sbcp_message)->attributes[1].payload,str);	
	(*sbcp_message)->attributes[2].type = 4;
	(*sbcp_message)->attributes[2].payloadlength = strlen(string);
	strcpy((*sbcp_message)->attributes[2].payload,string);	
}

/* This method creates the SEND message which the client sends to the server. */ 
void create_send_sbcp_message(struct sbcpmessage **sbcp_message, char username[16], char input_buffer[512])
{
	(*sbcp_message)->version = 3;
	(*sbcp_message)->type = 4;
	(*sbcp_message)->length = 2;
	(*sbcp_message)->attributes[0].type = 2;
	(*sbcp_message)->attributes[0].payloadlength = strlen(username);
	strcpy((*sbcp_message)->attributes[0].payload,username);
	(*sbcp_message)->attributes[1].type = 4;
	(*sbcp_message)->attributes[1].payloadlength = strlen(input_buffer);
	strcpy((*sbcp_message)->attributes[1].payload,input_buffer);	
}

/* This method creates the ONLINE message which the server sends when a new client joins the session. */
void create_online_sbcp_message(struct sbcpmessage **sbcp_message, char message[512])
{
	(*sbcp_message)->version = 3;
	(*sbcp_message)->type = 8;
	(*sbcp_message)->length = 1;
	(*sbcp_message)->attributes[0].type = 4;
	(*sbcp_message)->attributes[0].payloadlength = strlen(message);
	strcpy((*sbcp_message)->attributes[0].payload,message);
}

/* This method creates the FWD message which the server sends to all clients when it receives the SEND message. */
void create_fwd_sbcp_message(struct sbcpmessage **sbcp_message, char message[512])
{
	(*sbcp_message)->version = 3;
	(*sbcp_message)->type = 3;
	(*sbcp_message)->length = 1;
	(*sbcp_message)->attributes[0].type = 4;
	(*sbcp_message)->attributes[0].payloadlength = strlen(message);
	strcpy((*sbcp_message)->attributes[0].payload,message);
}

/* This method creates the OFFLINE message which the server sends to all clients when a client disconnects from the session. */
void create_offline_sbcp_message(struct sbcpmessage **sbcp_message, char message[512])
{
	(*sbcp_message)->version = 3;
	(*sbcp_message)->type = 6;
	(*sbcp_message)->length = 1;
	(*sbcp_message)->attributes[0].type = 4;
	(*sbcp_message)->attributes[0].payloadlength = strlen(message);
	strcpy((*sbcp_message)->attributes[0].payload,message);
}