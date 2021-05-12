#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
 int socket_desc, client_sock, c, read_size;
 struct sockaddr_in server, client;
 char client_message[2000];
 socket_desc=socket(AF_INET, SOCK_STREAM, 0);
 if (socket_desc == -1)
 {
 printf("Could not create socket");
 }
 puts("Socket created");
 server.sin_family = AF_INET;
 server.sin_addr.s_addr = INADDR_ANY;
 server.sin_port=htons(3000);
 
 if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server))<0)
 {
 perror("bind failed. Error");
 return 1;
 }
 puts("bind done");
 
 listen(socket_desc, 3);
 
 puts("Waiting for incoming connections");
 c=sizeof(struct sockaddr_in);
 
 client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
 if(client_sock<0)
 {
 perror("accept failed");
 return 1;
 }
 puts("connection accepted");
 
 while ((read_size=recv(client_sock, client_message, 2000, 0)) > 0)
 {
 write(client_sock, client_message, strlen(client_message));
 }
 if(read_size==0)
 {
 puts("client disconnected");
 fflush(stdout);
 }
 else if(read_size == -1)
 {
 perror("recv failed");
 }
 return 0;
}

void *connection_handler(void *socket_desc)
{
int sock = *(int*)socket_desc;
int read_size;
char *message, client_message[2000];

message="Hello\n";
write(sock, message, strlen(message));

message="Write something to server";
write(sock, message, strlen(message));

while((read_size=recv(sock, client_message, 2000, 0))>0)
{
write(sock, client_message, strlen(client_message));
}

if(read_size==0)
{
puts("Client disconnected");
fflush(stdout);
}
else if(read_size==-1)
{
perror("recv failed");
}
free(socket_desc);
return 0;
}

