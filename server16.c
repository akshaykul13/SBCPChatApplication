#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <sys/select.h>
#include "header.h"

int checkIfUserNameIsValid(char username[16], struct clientdetails client_list[]);
void process_incoming_message(struct sbcpmessage incoming_message, struct clientdetails client_list[], int client_descriptor, int max_clients);
void process_offline_client(struct sbcpmessage incoming_message, struct clientdetails client_list[], int client_descriptor);

/*fd_set to store the socket descriptors of the active connections with the server. */
fd_set client_descriptors, all_descriptors;
int current_number_of_clients;

int main(int argc,char *argv[])
{
	int sock_descriptor, new_client_descriptor, server_port, max_clients, max_descriptors, i, j;
	char *server_ip;
	struct sockaddr_in server_sockaddr, client_sockaddr;
	int len_sockaddr_in;
	struct hostent *host;	
	int select_value;	
	current_number_of_clients = 0;	
	
	//Creating a socket for the server.
	sock_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_descriptor == -1)
	{
		printf("ERROR: Cannot create socket\n");
		return -1;
	}	
	printf("INFO: Created socket\n");	
	
	//Storing server details the command line arguments
	server_ip = argv[1];
	server_port = atoi(argv[2]);
	max_clients = atoi(argv[3]);
	
	//Initializing the server_sockaddr structure
	bzero(&server_sockaddr, sizeof(server_sockaddr));
	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(server_port);
	host = gethostbyname(server_ip);
	memcpy(&server_sockaddr.sin_addr.s_addr, host->h_addr, host->h_length); 
	len_sockaddr_in = sizeof(client_sockaddr);
	//Structure to store the clients which have connected to the server
	struct clientdetails client_list[max_clients];
	
	//Binding the created socket to the IP address and port number provided
	if (bind(sock_descriptor, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) == -1)
	{
		printf("ERROR: Unable to bind the socket to IP = %s and Port = %d\n", server_ip, server_port);
		return -1;
	}
	printf("INFO: Completed bind\n");
	
	//Listening on the socket with the maximum clients provided
	if(listen(sock_descriptor, max_clients) == -1)
	{
		printf("ERROR: Unable to listen because of exceeding queue limit\n");
		return -1;
	}
	printf("INFO: Started Listen\n");
		
	//Initializing the fd_set variables
	FD_ZERO(&client_descriptors); 
	FD_ZERO(&all_descriptors);
	FD_SET(sock_descriptor, &all_descriptors);
	max_descriptors = sock_descriptor;
	
	while(1)
	{
		client_descriptors = all_descriptors;
		/*If select succeeds, it returns the number of ready socket descriptors.*/
		select_value = select(max_descriptors+1, &client_descriptors, NULL, NULL, NULL);
		if(select_value == -1)
		{
			printf("ERROR: Select failed for input descriptors.\n");
			return -1;
		}		
		
		for(i=0; i<= max_descriptors; i++)
		{	
			//Check which descriptor is set.
			if(FD_ISSET(i, &client_descriptors))
			{
				//If the server socket descriptor is set, then that means a client is trying to establish a new connection.
				if(i == sock_descriptor)
				{
					//Accept the new connection coming to the server's socket descriptor.
					new_client_descriptor = accept(sock_descriptor, (struct sockaddr *)&client_sockaddr, (socklen_t *)&len_sockaddr_in);
					if(new_client_descriptor == -1)
					{
						printf("ERROR: Unable to connect to the client.\n");
					}
					else
					{							
						//Set the client descriptor in the fd_set of all available descriptors.
						FD_SET(new_client_descriptor, &all_descriptors);
				        if(new_client_descriptor >= max_descriptors)
						{
							max_descriptors = new_client_descriptor;
						}
						printf("INFO: Established connection with the client.\n");
					}
				}
				//Some client is sending a message to the server using the client's descriptor.
				else
				{
					struct sbcpmessage incoming_message;
					//Read the message being sent by the client.
					int numofbytes = read(i,(struct sbcpmessage *) &incoming_message,sizeof(incoming_message));
					if(numofbytes > 0)
					{										
						//Process the received message based on the type of message being sent.
						process_incoming_message(incoming_message, client_list, i, max_clients); 
					}
					else
					{
						//Handle resources when a client disconnects from the server.
						process_offline_client(incoming_message, client_list, i);
					}
				}
			}
		}
	}
	close(sock_descriptor);
}


/*
 * process_incoming_message : This function is used by the server to process the SBCP messages being sent by the client based on the type and 
 *							  perform appropriate actions for each type.
 *
 * INPUTS : 
 *		struct sbcpmessage incoming_message - the incoming message from the client.
 *		struct clientdetails client_list[] - the list of the clients currently connected to the server.
 *		int client_descriptor - socket descriptor of the client which has sent the SBCP message.
 *		int max_clients - the maximum number of clients that the server can handle.
 */
void process_incoming_message(struct sbcpmessage incoming_message, struct clientdetails client_list[], int client_descriptor, int max_clients)
{
	char *reason;
	struct sbcpmessage *output_message;
	char concat_string[512], online_string[512], message_string[512];	
	int j;
	
	switch(incoming_message.type)
	{
		//When the client sends a JOIN message
		case JOIN:
			//Checks if the username provided by the client is not already in use and whether the server has reached maximum connections.
			if(checkIfUserNameIsValid(incoming_message.attributes[0].payload, client_list) && current_number_of_clients < max_clients)
			{											
				struct clientdetails client;
				//Creating a string of all the clients currently in session.
				for(j=0; j<current_number_of_clients; j++)
				{
					strcat(concat_string,client_list[j].username);
					strcat(concat_string,"\t");
				}
				strcpy(client.username,incoming_message.attributes[0].payload);
				client.socket_desc = client_descriptor;
				client_list[current_number_of_clients] = client;
				current_number_of_clients++;
				//Create an ACK message to be sent to the client indicating a successful JOIN along with client count and other users in the session.
				output_message = (struct sbcpmessage*)malloc(sizeof(struct sbcpmessage));																
				create_ack_sbcp_message(&output_message, concat_string, incoming_message.attributes[0].payload, current_number_of_clients);
				printf("INFO: Sending ACK response for user: %s\n", incoming_message.attributes[0].payload);
				write(client_descriptor, (void *)&(*output_message), sizeof(*output_message));					
				printf("INFO: %s has joined.\n", incoming_message.attributes[0].payload);
				//Create a FWD message to be sent to all the clients indicating that this particular client is ONLINE.
				strcpy(online_string, incoming_message.attributes[0].payload);
				strcat(online_string," is Online.");				
				create_online_sbcp_message(&output_message, online_string);
				for(j=0; j<current_number_of_clients; j++)
				{				
					if(client_list[j].socket_desc != client_descriptor)
					{
						if(write(client_list[j].socket_desc, (void *)&(*output_message), sizeof(*output_message)) == -1)
						{
							printf("ERROR: Write error on broadcasting ONLINE message.\n");
						}
					}
				}
			}	
			//If the username check or the maximum client count check fails.
			else
			{	
				//Determine why the JOIN has failed along with appropriate reason.
				if(current_number_of_clients >= max_clients)
				{
					reason = MAX_CLIENTS_REACHED;
				}
				else
				{
					reason = INVALID_USERNAME;										
				}
				output_message = (struct sbcpmessage*)malloc(sizeof(struct sbcpmessage));				
				create_nak_sbcp_message(&output_message, reason);
				printf("INFO: Sending NAK response for user: %s\n", incoming_message.attributes[0].payload);
				write(client_descriptor, (void *)&(*output_message), sizeof(*output_message));														
				FD_CLR(client_descriptor, &all_descriptors);							
			}
			break;
		//When the client sends a message to the server through the SEND command.
		case SEND:						
			strcpy(message_string, incoming_message.attributes[1].payload);		
			strcpy(concat_string, incoming_message.attributes[0].payload);		
			strcat(concat_string, ": ");
			strcat(concat_string, message_string);
			output_message = (struct sbcpmessage*)malloc(sizeof(struct sbcpmessage));				
			create_fwd_sbcp_message(&output_message, concat_string);
			printf("INFO: Sending broadcast for user: %s\n", incoming_message.attributes[0].payload);
			for(j=0; j<current_number_of_clients; j++)
			{							
				if(client_list[j].socket_desc != client_descriptor)
				{
					if(write(client_list[j].socket_desc, (void *)&(*output_message), sizeof(*output_message)) == -1)
					{
						printf("ERROR: Write error on broadcasting]n");
					}
				}
			}		
			break;
	}
}

/*
 * checkIfUserNameIsValid : This function is used by the server to detect whether the provided username is valid or not.
 *
 * INPUTS : 
 *		char username[16] - the username provided by the client.
 *		struct clientdetails client_list[] - the list of the clients currently connected to the server. 
 */
int checkIfUserNameIsValid(char username[16], struct clientdetails client_list[])
{
	int i;
	for(i=0; i<current_number_of_clients; i++)
	{		
		if(strcmp(client_list[i].username,username) == 0)
		{
			printf("INFO: Duplicate Username: %s\n", username);
			return 0;
		}
	}
	return 1;
}	

/*
 * process_offline_client : This function is used by the server to cleanup resources when a client disconnects and sends the OFFLINE message to all clients.
 *
 * INPUTS : 
 *		struct sbcpmessage incoming_message - the incoming message from the client.
 *		struct clientdetails client_list[] - the list of the clients currently connected to the server. 
 *		int client_descriptor - socket descriptor of the client which has sent the SBCP message.
 */
void process_offline_client(struct sbcpmessage incoming_message, struct clientdetails client_list[], int client_descriptor)
{
	int index=-1, i, j;
	char username[16], concat_string[512];
	struct sbcpmessage *output_message;
	
	//Finds the username of the client which has disconnected.
	for(i=0; i<current_number_of_clients; i++)
	{
		if(client_list[i].socket_desc == client_descriptor)
		{
			index = i;
			strcpy(username, client_list[i].username);
			break;
		}
	}
	//Clears the socket descriptor of the client from the fd_set and sends the FWD message to all clients indicating that the user is OFFLINE.
	if(index != -1)
	{
		printf("INFO: %s has disconnected.\n", username);
		FD_CLR(client_descriptor, &all_descriptors);
		for(i=index; i<current_number_of_clients-1; i++)
		{
			client_list[i] = client_list[i+1];
		}
		//client_list[current_number_of_clients] = NULL;
		current_number_of_clients--;
		strcpy(concat_string, username);		
		strcat(concat_string, " is Offline.\n");
		output_message = (struct sbcpmessage*)malloc(sizeof(struct sbcpmessage));				
		create_offline_sbcp_message(&output_message, concat_string);
		printf("INFO: Sending offline broadcast for user: %s\n", username);
		for(j=0; j<current_number_of_clients; j++)
		{								
			if(write(client_list[j].socket_desc, (void *)&(*output_message), sizeof(*output_message)) == -1)
			{
				printf("ERROR: Write error on broadcasting]n");
			}
		}	
	}
	else
	{
		printf("ERROR: Could not find client username in client list.\n");
	}	
}