#ECEN 602
#Homework - 1 
#Group - 16
@Author - Akshay Kulkarni(124003323) and Sangeeta Panigrahy(522007493)

This project contains the server and client of a simple chat service. The server and client use the SBCP message format to communicate. 
The server provides a single chat room which can only handle a finite number of clients.

Features of this chat application :
1. A single server creates a chat room where in a number of clients can join the session with a username.
2. When a client joins the session with a username, the server accepts the connection if the number of connections does not exceed the maximum number of connections the server can handle.
3. Once the server accepts the connection, the server sends an acknowledgement to the client along with the client count including the user and the list of clients excluding the user.
4. In case of the username being already in use or the the number of clients already reaching the maximum connections the server sends out a negative acknowledgement to the client.
4. The client displays the acknowledgement or negative acknowledgement to the user and terminates the session.
5. In case the server disconnects all the clients also disconnect from the server.
6. Once a client joins/disconnects the session, the server sends a message to all the clients to indicate that the particular user is online/offline.
7. When a client is in session, the user can type in the console to chat with other users. This message will be sent to the server, which in turn broadcasts the message to all the users in the session except the sender.
8. This message will be displayed in the console of all the clients.
9. A connection is refused to the client when:
	a. The username provided by the client is already in use.
	b. The client count reaches the maximum number of clients the server can handle.
	c. The server disconnects from the session.
	In all these cases the client session terminates displaying a proper message.
	
Running the application:
1. Locate the folder of the project in the terminal and type make.
2. Run the server first by typing ./server16 server_ip server_port max_clients
3. Then connect clients by typing ./client16 username server_ip server_port
4. Multiple clients can be connected using command 3.
5. A client can communicate to other clients just by typing the message directly in the terminal.
6. To disconnect the client or server type Ctrl+C or Command+C.
7. To clean the binaries type make clean in the terminal.

The application consists of the following files:
1. server16.c
2. client16.c
3. header.h
4. makefile
5. README.txt

Flow Diagram:

			Server											Client
									
															send JOIN(username)
			if username is already in use					
				send NAK(reason)
			else if max client count is reached
				send NAK(reason)
			else 
				send ACK(client count, client list)
				send ONLINE broadcast to all clients
															if ACK/NAK is received then it displays the message
															send SEND(message) if the user chats on the console
															if ONLINE broadcast is received then it displays that
															the user is online.
			broadcast the message through FWD(message)
															if FWD is received, display the message in the console
			if server disconnects												
															client also disconnects displaying proper reason
															
															if client disconnects
			server sends a broadcast to all clients
			with the user OFFLINE message
															if OFFLINE broadcast is received then the client displays that 
															the user is offline.