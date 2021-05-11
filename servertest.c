#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/mman.h>

#define MAX_CLIENTS 255 
#define PORT 3000 
#define HOST "localhost"
#define SHARED_MEMORY_SIZE 1024
#define BUFFER_SIZE 1024

char* shared_memory = NULL;
int* client_count = NULL;
int* shared_data = NULL;
void get_shared_memory();
void gameloop();
void start_network();
void process_client(int id, int sock);
// comms between server and client
void infchat(int sock)
{
	char buffer[BUFFER_SIZE];
	int n;
	// infinite loop
	for (;;) {
		bzero(buffer, BUFFER_SIZE);
		// copy message from client in buffer
		read(sock, buffer, sizeof(buffer));
		// print buffer with client contents
		printf("From client: %s\t To client : ", buffer);
		bzero(buffer, BUFFER_SIZE);
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
	int pid = 0; int i;
	printf("SERVER started!\n");
	get_shared_memory();
	pid = fork();
	if(pid == 0){
	start_network();
	}else{
	gameloop();
	}
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
