#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <ncurses.h>

#include "../functions/client_functions.h"
#include "../types/packets.h"
#include "../types/objects.h"

#define BUFFER_SIZE 1024
#define PROTOCOL_VERSION 0x00
#define DEBUG 0

typedef enum {
  CHARACTER_EMPTY = 32,
  CHARACTER_WALL = 35,
  CHARACTER_BOX = 111,
  CHARACTER_EXPLOSION = 64,
  CHARACTER_POWER_UP_1 = 49,
  CHARACTER_POWER_UP_2 = 50,
  CHARACTER_POWER_UP_3 = 51,
  CHARACTER_POWER_UP_4 = 52,
  CHARACTER_PLAYER = 36,
  CHARACTER_BOMB = 33
} character_e;

typedef enum {
  BUTTON_UP = 119, // w
  BUTTON_RIGHT = 100, // d
  BUTTON_DOWN = 115, // s
  BUTTON_LEFT = 97, // a
  BUTTON_PLACE_BOMB = 32 // spacebar
} button_e;

typedef struct {
  char name[32];
  char color;
  unsigned int id;
  int direction;
  int is_placing_bomb;
} player_t;

typedef struct {
  unsigned char width;
  unsigned char height;
  unsigned char *blocks;
} map_t;

int socket_FD = 0;
char buffer[BUFFER_SIZE];
player_t player;
map_t map;
movable_object_data_t *objects;
int object_count = 0;

int main() {
  player.id = 0;
  player.direction = 0;
  player.is_placing_bomb = 0;
  map.width = 0;
  map.height = 0;
  map.blocks = NULL;
  objects = NULL;

  connect_to_server();
  join_game();
  create_game_thread();

  initscr();
  cbreak();
  noecho();
  clear();
  curs_set(0);
  nonl();
  intrflush(stdscr, 0);
  keypad(stdscr, 1);

  char button = getch();
  int escape_button = 27;

  // Escape button
  while (button != escape_button) {
    handle_pressed_button(button);

    button = getch();
  }

  endwin();
  free(map.blocks);
  free(objects);
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

    exit_program();
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, ip_address, &server_address.sin_addr) <= 0) {
    printf("Invalid or unsupported server address\n");

    exit_program();
  }

  if (connect(socket_FD, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    printf("Connection to server failed\n");

    exit_program();
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
  free(map.blocks);
  free(objects);
  endwin();
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

void send_player_input_packet() {
  packet_header_t header;
  player_input_data_t body;
  char checksum;

  unsigned int size = sizeof(header.start_symbol) + sizeof(header.type_id) + sizeof(header.data_length)
                    + sizeof(body.x_movement) +sizeof(body.y_movement) + sizeof(body.is_placing_bomb)
                    + sizeof(checksum);

  char packet[size];
  bzero(packet, size);

  // Header
  memcpy((void *) &header.start_symbol, packet_start, sizeof(packet_start));

  header.type_id = (char) PACKET_TYPE_PLAYER_INPUT;

  int data_length = size - sizeof(checksum);
  memcpy((void *) &header.data_length, (void *) &data_length, sizeof(unsigned int));

  // Body
  body.x_movement = 0;
  body.y_movement = 0;

  if (player.direction == DIR_UP) {
    body.y_movement = (char) 10;

  } else if (player.direction == DIR_RIGHT) {
    body.x_movement = (char) 20;
  } else if (player.direction == DIR_DOWN) {
    body.y_movement = (char) -45;
  } else if (player.direction == DIR_LEFT) {
    body.x_movement = (char) -95;
  }

  body.is_placing_bomb = player.is_placing_bomb;

  int offset = 0;

  memcpy((void *) &packet, (void *) &header.start_symbol, sizeof(header.start_symbol));
  offset += sizeof(header.start_symbol);

  memcpy((void *) &packet + offset, (void *) &header.type_id, sizeof(header.type_id));
  offset += sizeof(header.type_id);

  memcpy((void *) &packet + offset, (void *) &header.data_length, sizeof(header.data_length));
  offset += sizeof(header.data_length);

  memcpy((void *) &packet + offset, (void *) &body.x_movement, sizeof(body.x_movement));
  offset += sizeof(body.x_movement);

  memcpy((void *) &packet + offset, (void *) &body.y_movement, sizeof(body.y_movement));
  offset += sizeof(body.y_movement);

  memcpy((void *) &packet + offset, (void *) &body.is_placing_bomb, sizeof(body.is_placing_bomb));
  offset += sizeof(body.is_placing_bomb);

  checksum = calculate_checksum(packet, size - 1);
  packet[offset] = checksum;

  memcpy((void *) buffer, packet, size);

  send_packet(buffer, size);
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
    #if DEBUG
      puts("Successfully joined server game");
    #endif

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

  if (width != map.width || height != map.height) {
    free(map.blocks);

    map.width = width;
    map.height = height;
    map.blocks = (unsigned char *) malloc ((width * height)*sizeof(unsigned char));
  }

  memcpy((void *) map.blocks, packet + index, width * height);

  render_game();
}

void handle_server_objects_packet(char *packet, int size) {
  free(objects);

  int count = packet[0];
  object_count = count;

  objects = (movable_object_data_t *) malloc (sizeof(movable_object_data_t) * object_count);

  movable_object_data_t obj;
  int offset = 1;
  int i;
  for (i = 0; i < count; i++) {
    obj.type_id = packet[offset];
    offset += sizeof(obj.type_id);

    memcpy((void *) &obj.id, packet + offset, sizeof(obj.id));
    offset += sizeof(obj.id);

    memcpy((void *) &obj.x, packet + offset, sizeof(obj.x));
    offset += sizeof(obj.x);

    memcpy((void *) &obj.y, packet + offset, sizeof(obj.y));
    offset += sizeof(obj.y);

    obj.direction = packet[offset];
    offset += sizeof(obj.direction);

    obj.status = packet[offset];
    offset += sizeof(obj.status);

    objects[i] = obj;
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
      addch(CHARACTER_EMPTY);
      break;
    case BLOCK_TYPE_WALL:
      addch(CHARACTER_WALL);
      break;
    case BLOCK_TYPE_BOX:
      addch(CHARACTER_BOX);
      break;
    case BLOCK_TYPE_EXPLOSION:
      addch(CHARACTER_EXPLOSION);
      break;
    case BLOCK_TYPE_POWER_UP_1:
      addch(CHARACTER_POWER_UP_1);
      break;
    case BLOCK_TYPE_POWER_UP_2:
      addch(CHARACTER_POWER_UP_2);
      break;
    case BLOCK_TYPE_POWER_UP_3:
      addch(CHARACTER_POWER_UP_3);
      break;
    case BLOCK_TYPE_POWER_UP_4:
      addch(CHARACTER_POWER_UP_4);
      break;
    default:
      puts("Cannot draw unknown block type");
  }
}

void draw_object(char object_type) {
  switch(object_type) {
    case MOVABLE_OBJECT_PLAYER:
      // TODO: Get the actual color of given player
      addch(CHARACTER_PLAYER);
      break;
    case MOVABLE_OBJECT_BOMB:
      addch(CHARACTER_BOMB);
      break;
    default:
      puts("Cannot draw unknown object type");
  }
}

void render_game() {
  #if DEBUG
    printf("Rendering game with %d objects\n", object_count);
  #endif

  erase();

  int x;
  int y;
  int index = 0;
  for (y = 1; y <= map.height; y++) {
    for (x = 1; x <= map.width; x++) {
      unsigned char obj_type_id = find_object_in_coords(x, y);

      if (obj_type_id == 0xff) {
        draw_block(map.blocks[index]);
      } else {
        draw_object(obj_type_id);
      }

      if (x == map.width) {
        addch(10);
      }

      index++;
    }
  }

  refresh();
}

unsigned char find_object_in_coords(int x, int y) {
  if (object_count == 0) {
    return 0xff;
  }

  int i;
  for (i = 0; i < object_count; i++) {
    int obj_x = (int) roundf(objects[i].x);
    int obj_y = (int) roundf(objects[i].y);

    if (x == obj_x && y == obj_y) {
      return objects[i].type_id;
    }
  }

  return 0xff;
}

void create_game_thread() {
  pthread_t input_thread;

  if (pthread_create(&input_thread, NULL , game_thread_handler, NULL) < 0) {
    puts("ERROR: Could not create the input thread");
    exit_program();
  }
}

void *game_thread_handler() {
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
          #if DEBUG
            puts("Checksum mismatch, discarding packet");
          #endif
        } else {
          #if DEBUG
            puts("Checksum matched");
          #endif
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

  return NULL;
}

void handle_pressed_button(int button) {
  int case_difference = 32;
  // erase();
  // addch(CHARACTER_BOMB);
  // addch(CHARACTER_BOMB);
  // addch(CHARACTER_BOMB);
  // addch(CHARACTER_BOMB);
  // addch(CHARACTER_BOMB);
  // refresh();


  if (button == BUTTON_UP || button == KEY_UP || button == BUTTON_UP - case_difference) {
    player.direction = DIR_UP;
  } else if (button == BUTTON_RIGHT || button == KEY_RIGHT || button == BUTTON_RIGHT - case_difference) {
    player.direction = DIR_RIGHT;
  } else if (button == BUTTON_DOWN || button == KEY_DOWN || button == BUTTON_DOWN - case_difference) {
    player.direction = DIR_DOWN;
  } else if (button == BUTTON_LEFT || button == KEY_LEFT || button == BUTTON_LEFT - case_difference) {
    player.direction = DIR_LEFT;
  } else if (button == BUTTON_PLACE_BOMB) {
    player.is_placing_bomb = 1;
  } else {
    return;
  }

  send_player_input_packet();
  player.is_placing_bomb = 0;
}
