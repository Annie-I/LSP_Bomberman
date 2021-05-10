#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "../functions/client_functions.h"
#include "../types/packets.h"
#include "../types/objects.h"

#define BUFFER_SIZE 1024
#define PROTOCOL_VERSION 0x00
#define DEBUG 1

typedef struct {
  char name[32];
  char color;
  unsigned int id;
} player_t;

int socket_FD = 0;
char buffer[BUFFER_SIZE];
player_t player;
char *map;
int map_size = 0;
movable_object_data_t *objects;
int object_count = 0;

int main() {
  connect_to_server();
  join_game();

  while(1) {
    char message[BUFFER_SIZE];
    int read_bytes = recv(socket_FD, message, BUFFER_SIZE, 0);

    if (read_bytes > 0) {
      // TODO: Should read entire message looking for packet_start
      // For now assume that only correct messages are received and start is at beginning
      if (message[0] == (char) 0xff && message[1] == (char) 0x00) {
        int message_size = 0;
        memcpy((void *) &message_size, message + 3, sizeof(unsigned int));

        if (!compare_checksum(message, message_size, message[message_size])) {
          // TODO: Retry join packet
          puts("Checksum mismatch, discarding packet");
        } else {
          puts("Checksum matched");
          // Get size without start symbol, packet type, data length
          size_t header_size = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(unsigned int);

          unsigned char type = message[2];

          int packet_data_size = message_size - header_size;
          char packet_data[packet_data_size];
          memcpy((void *) &packet_data, message + header_size, packet_data_size);
          handle_packet(type, packet_data, packet_data_size);
        }
      }
    } else {
      puts("Socket error / connection closed");
      exit_program();
    }
  }

  close(socket_FD);

  return 0;
}

/* Create socket connection */
void connect_to_server() {
  char *ip_address = get_server_ip();
  int port = get_server_port();

  struct sockaddr_in server_address;

  if ((socket_FD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Socket creation error\n");

    exit(1);
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, ip_address, &server_address.sin_addr) <= 0) {
    printf("Invalid or unsupported server address\n");

    exit(1);
  }

  if (connect(socket_FD, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    printf("Connection to server failed\n");

    exit(1);
  }

  printf("Successfully connected to %s on port %i.\n", ip_address, port);
  free(ip_address);
}

char *get_server_ip() {
  int max_address_length = 15;
  char *ip_address = (char*) malloc((max_address_length + 1) * sizeof(char)); // +1 for \0

  #if DEBUG
    strcpy(ip_address, "127.0.0.1");
    return ip_address;
  #endif

  printf("Enter server IP or enter \"0\" for 127.0.0.1: ");
  scanf("%s", ip_address);

  if (strcmp(ip_address, "0") == 0) {
    strcpy(ip_address, "127.0.0.1");
  }

  return ip_address;
}

int get_server_port() {
  int port = 3000;

  #if DEBUG
    return port;
  #endif

  printf("Enter server port or enter \"0\" for 3000: ");
  scanf("%d", &port);

  if (port == 0) {
    port = 3000;
  }

  return port;
}

void exit_program() {
  close(socket_FD);
  exit(1);
}

void get_player_data(char name[32], char *color) {
  #if DEBUG
    strcpy(name, "Player1");
    strcpy(color, "G");

    return;
  #endif

  printf("Enter your name (32 characters, no spaces): ");
  scanf("%s", name);

  printf("Enter your color (1 character): ");
  scanf(" %c", color);
}

void join_game() {
  get_player_data(player.name, &player.color);

  int packet_size = get_player_id_packet(buffer, player.name, player.color);

  #if DEBUG
    puts("Sending ID packet");
  #endif

  send_packet(buffer, packet_size);
}

/* Writes player identification data into buffer and returns its length */
int get_player_id_packet(char *buffer, char name[32], char color) {
  packet_header_t header;
  player_id_data_t body;
  char checksum;

  unsigned int size = sizeof(header.start_symbol) + sizeof(header.type_id) + sizeof(header.data_length)
                    + sizeof(body.protocol_version) +sizeof(body.name) + sizeof(body.color)
                    + sizeof(checksum);

  char packet[size];
  bzero(packet, size);

  // Header
  memcpy((void *) &header.start_symbol, packet_start, sizeof(packet_start));

  header.type_id = (char) PACKET_TYPE_PLAYER_ID;

  int data_length = size - sizeof(checksum);
  memcpy((void *) &header.data_length, (void *) &data_length, sizeof(unsigned int));

  // Body
  body.protocol_version = (char) PROTOCOL_VERSION;

  bzero(body.name, 32);
  strcpy(body.name, name);

  body.color = color;

  int offset = 0;

  memcpy((void *) &packet, (void *) &header.start_symbol, sizeof(header.start_symbol));
  offset += sizeof(header.start_symbol);

  memcpy((void *) &packet + offset, (void *) &header.type_id, sizeof(header.type_id));
  offset += sizeof(header.type_id);

  memcpy((void *) &packet + offset, (void *) &header.data_length, sizeof(header.data_length));
  offset += sizeof(header.data_length);

  memcpy((void *) &packet + offset, (void *) &body.protocol_version, sizeof(body.protocol_version));
  offset += sizeof(body.protocol_version);

  memcpy((void *) &packet + offset, (void *) &body.name, sizeof(body.name));
  offset += sizeof(body.name);

  memcpy((void *) &packet + offset, (void *) &body.color, sizeof(body.color));
  offset += sizeof(body.color);

  checksum = calculate_checksum(packet, size - 1);
  packet[offset] = checksum;

  memcpy((void *) buffer, packet, size);

  return size;
}

int get_ping_response_packet(char *buffer) {
  packet_header_t header;
  char checksum;

  unsigned int size = sizeof(header.start_symbol) + sizeof(header.type_id) + sizeof(header.data_length)
                    + sizeof(checksum);

  char packet[size];
  bzero(packet, size);

  // Header
  memcpy((void *) &header.start_symbol, packet_start, sizeof(packet_start));

  header.type_id = (char) PACKET_TYPE_PING_RESPONSE;

  int data_length = size - sizeof(checksum);
  memcpy((void *) &header.data_length, (void *) &data_length, sizeof(unsigned int));

  int offset = 0;

  memcpy((void *) &packet, (void *) &header.start_symbol, sizeof(header.start_symbol));
  offset += sizeof(header.start_symbol);

  memcpy((void *) &packet + offset, (void *) &header.type_id, sizeof(header.type_id));
  offset += sizeof(header.type_id);

  memcpy((void *) &packet + offset, (void *) &header.data_length, sizeof(header.data_length));
  offset += sizeof(header.data_length);

  checksum = calculate_checksum(packet, size - 1);
  packet[offset] = checksum;

  memcpy((void *) buffer, packet, size);

  return size;
}

void send_packet(char *buffer, int length) {
  send(socket_FD, buffer, length, 0);
}

void handle_packet(unsigned char type, char *packet, int size) {
  #if DEBUG
    printf("Handling packet %02x of size %d\n", type, size);
    print_bytes(packet, size);
  #endif

  switch(type) {
    case PACKET_TYPE_SERVER_ID:
      handle_server_id_packet(packet, size);
      break;

    case PACKET_TYPE_MAP_STATE:
      handle_server_map_packet(packet, size);
      break;

    case PACKET_TYPE_OBJECT_UPDATES:
      handle_server_objects_packet(packet, size);
      break;

    // case PACKET_TYPE_SERVER_CHAT_MESSAGE:
    //   break;

    // case PACKET_TYPE_PLAYER_INFO:
    //   break;

    case PACKET_TYPE_SERVER_PING:
      handle_server_ping_packet();
      break;

    default:
      printf("Packet type %02x handler is not implemented\n", type);
  }
}

void handle_server_id_packet(char *packet, int size) {
  if (packet[0] != (char) PROTOCOL_VERSION) {
    puts("Client and server use different protocols");
    exit_program();
  }

  int id = -1;
  memcpy((void *) &id, (void *) packet + 1, sizeof(int));

  if (id > 0) {
    player.id = id;

    return;
  }

  if (id == 0) {
    puts("Server didn't accept join request");
  } else if (id == -1) {
    puts("Failed to assign id");
  }

  exit_program();
}

void handle_server_map_packet(char *packet, int size) {
  int width = packet[0];
  int height = packet[1];
  int index = 2;

  int x;
  int y;
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      // map[index] = packet[]
      draw_block(packet[index]);

      if (x + 1 == width) {
        printf("\n");
      }

      index++;
    }
  }
}

void handle_server_objects_packet(char *packet, int size) {
  movable_object_data_t object;
  int object_size = sizeof(object.type_id) + sizeof(object.id)
                  + sizeof(object.x) + sizeof(object.y)
                  + sizeof(object.direction) + sizeof(object.status);

  int count = packet[0];
  object_count = count;

  int i;
  int offset = 1;
  for (i = 0; i < count; i++) {
    object.type_id = packet[offset];
    offset += sizeof(object.type_id);

    memcpy((void *) &object.id, packet + offset, sizeof(object.id));
    offset += sizeof(object.id);

    memcpy((void *) &object.x, packet + offset, sizeof(object.x));
    offset += sizeof(object.x);

    memcpy((void *) &object.y, packet + offset, sizeof(object.y));
    offset += sizeof(object.y);

    object.direction = packet[offset];
    offset += sizeof(object.direction);

    object.status = packet[offset];

    // objects[i] = object;
  }

  render_game();
}

void handle_server_ping_packet() {
  int packet_size = get_ping_response_packet(buffer);

  #if DEBUG
    puts("Sending PING RESPONSE packet");
  #endif

  send_packet(buffer, packet_size);
}

void draw_block(char block) {
  switch(block) {
    case BLOCK_TYPE_EMPTY:
      printf(" ");
      break;
    case BLOCK_TYPE_WALL:
      printf("#");
      break;
    case BLOCK_TYPE_BOX:
      printf("o");
      break;
    case BLOCK_TYPE_EXPLOSION:
      printf("@");
      break;
    case BLOCK_TYPE_POWER_UP_1:
      printf("1");
      break;
    case BLOCK_TYPE_POWER_UP_2:
      printf("2");
      break;
    case BLOCK_TYPE_POWER_UP_3:
      printf("3");
      break;
    case BLOCK_TYPE_POWER_UP_4:
      printf("4");
      break;
    default:
      puts("Cannot draw unknown block type");
  }
}

void render_game() {
  puts("Rendering game");
}
