#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <sys/select.h>
#include <unistd.h>
#include "header.h"

fd_set input_descriptors, all_descriptors;

int main(int argc,char *argv[])
{
	int client_descriptor, server_port, max_descriptors=0;
	char *server_ip, *username;
	struct sockaddr_in server_sockaddr;
	struct hostent *host;	
	int select_value;	
	char *input, input_buffer[1024], receive_buffer[1024];
	int input_bytes;
	
	//Creating a socket for the client.
	client_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	if(client_descriptor == -1)
	{
		printf("ERROR: Could not create socket\n");
		return -1;
	}	
	printf("INFO: Created socket\n");	
	
	//Storing server details the command line arguments
	username = argv[1];
	server_ip = argv[2];
	server_port = atoi(argv[3]);
	
	//Initializing the server_sockaddr structure
	bzero(&server_sockaddr, sizeof(server_sockaddr));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(server_port);
	host = gethostbyname(server_ip);
	memcpy(&server_sockaddr.sin_addr.s_addr, host->h_addr, host->h_length); 
	
	//Connect to the server
	if(connect(client_descriptor,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr)) == -1)
	{
		printf("ERROR: Cannot connect to the server\n");
		exit(1);
	}
	printf("INFO: Connected to the server\n");
	
	//Initializing the fd_set variables
	FD_ZERO(&input_descriptors); 
	FD_ZERO(&all_descriptors);
	FD_SET(client_descriptor, &all_descriptors);
	FD_SET(STDIN_FILENO,&all_descriptors);
	max_descriptors = client_descriptor;
	
	//Initiate the JOIN operation with the server
	printf("INFO: Initiating JOIN\n");
	struct sbcpmessage *join_message = (struct sbcpmessage*)malloc(sizeof(struct sbcpmessage));
	create_join_sbcp_message(&join_message, username);
	printf("INFO: Sending join request for user: %s\n", join_message->attributes[0].payload);
	write(client_descriptor,(void *)&(*join_message),sizeof(*join_message));
	
	while(1)
	{
		input_descriptors = all_descriptors;
		/* Wait for a input read or write from server.
		If select succeeds, it returns the number of ready socket descriptors.*/
		select_value = select(max_descriptors+1, &input_descriptors, NULL, NULL, NULL);
		if(select_value == -1)
		{
			printf("ERROR: Select failed for input descriptors. \n");
			return -1;
		}	
		if(select_value == 0)
		{
			printf("INFO: All descriptors are idle. \n");
		}
		//If input is received from the client console, then creates a SEND message to be conveyed to the server.
		if(FD_ISSET(STDIN_FILENO,&input_descriptors))
		{                                
			bzero(input_buffer,1024);
			//Read the input entered in the client
            input_bytes = read(STDIN_FILENO, input_buffer, sizeof(input_buffer));  			                      
			struct sbcpmessage *output_message = (struct sbcpmessage*)malloc(sizeof(struct sbcpmessage));			
			create_send_sbcp_message(&output_message, username, input_buffer);   			
			write(client_descriptor,(void *) &(*output_message),sizeof((*output_message)));	    	  
        }
		//Is it write event from server?
        if(FD_ISSET(client_descriptor,&input_descriptors))
        {			
			bzero(receive_buffer,1024);
			input = receive_buffer;
						
			//Read the message from server
			struct sbcpmessage *incoming_message = (struct sbcpmessage*)malloc(sizeof(struct sbcpmessage));
			input_bytes = read(client_descriptor,(struct sbcpmessage *) &(*incoming_message),sizeof(*incoming_message));			
			
			//If number of bytes read is less than or equal to zero, the server has disconnected. Force exit the client.			
            if(input_bytes <= 0)
            {
				printf("\n Terminating because the server has disconnected. \n");
				exit(1);
			}   	
			//If the received message if of the type NAK.
			if(incoming_message->type == 5)
			{
				printf("%s\n",incoming_message->attributes[0].payload);
				printf("\n Terminating. \n");
				exit(1);
			}	
			//If the received message if of the type ACK.
			if(incoming_message->type == 7)
			{				
				printf("Received ACK from server for the username is %s\n",incoming_message->attributes[0].payload);	
				printf("Client Count: %s\n", incoming_message->attributes[1].payload);
				printf("Other users in session: %s\n\n", incoming_message->attributes[2].payload);
			}	
			//If the received message if of the type FWD or ONLINE or OFFLINE.
			if(incoming_message->type == 3 || incoming_message->type == 6 || incoming_message->type == 8)
			{
				printf("%s\n",incoming_message->attributes[0].payload);											
			}
        }
	}
	close(client_descriptor);
}