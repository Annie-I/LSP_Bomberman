#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#define MAX 80
#define PORT 3000 
#define SA struct socketaddr


// comms between server and client
void infchat(int sock)
{
	char buffer[MAX];
	int n;
	// infinite loop
	for (;;) {
		bzero(buffer, MAX);
		// copy message from client in buffer
		read(sock, buffer, sizeof(buffer));
		// print buffer with client contents
		printf("From client: %s\t To client : ", buffer);
		bzero(buffer, MAX);
		n = 0;
		// copies server message in buffer
		while ((buffer[n++] = getchar()) != '\n');
		// sends buffer to client
		write (sock, buffer, sizeof(buffer));
		// if "exit" then server exits and communication ends
		if (strncmp("exit", buffer, 4) == 0) {
			printf("Server Exits..\n");
			break;
		}
	}
}

int main()
{
	int sock, conn, len;
	struct sockaddr_in serveraddr, cli;
	// creating socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1){
		printf("Failed to create socket..\n");
		exit(0);
	}else
	printf("Socket successfully created..\n");
	bzero(&serveraddr, sizeof(serveraddr));
	// assign IP, PORT
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORT);
	// binding socket to IP and verification
	if ((bind(sock, (struct socketaddr *)&serveraddr, sizeof(serveraddr))) != 0) {
		printf("Failed to bind socket..\n");
		exit(0);
	}else
	printf("Socket has been successfully binded..\n");
	// server listens and verification
	if ((listen(sock, 5)) != 0){
		printf("Listening has failed..\n");
		exit(0);
	}else
	printf("Server is listening..\n");
	len = sizeof(cli);
	// accepting the data packet from client and verification
	conn = accept(sock, (struct socketaddr *)&cli, &len);
	if (conn < 0){
		printf("Server accept has failed..\n");
		exit(0);
	}else
	printf("Server is accepting the client..\n");
	// function for communication between client and server
	infchat(conn);
	// after communication close the socket
	close(sock);
}
