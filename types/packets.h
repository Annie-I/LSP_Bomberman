#ifndef __PACKET_TYPES_H
#define __PACKET_TYPES_H

char packet_start[2] = { 0xff, 0x00 };

typedef enum {
  PACKET_TYPE_PLAYER_ID = 0x00,
  PACKET_TYPE_PLAYER_INPUT = 0x01,
  PACKET_TYPE_PLAYER_MESSAGE = 0x02,
  PACKET_TYPE_PING_RESPONSE = 0x03,
  PACKET_TYPE_SERVER_ID = 0x80,
  PACKET_TYPE_MAP_STATE = 0x81,
  PACKET_TYPE_OBJECT_UPDATES = 0x82,
  PACKET_TYPE_SERVER_CHAT_MESSAGE = 0x83,
  PACKET_TYPE_PLAYER_INFO = 0x84,
  PACKET_TYPE_SERVER_PING = 0x85
} packet_type_e;

typedef enum {
  DIR_UP = 0,
  DIR_RIGHT = 1,
  DIR_DOWN = 2,
  DIR_LEFT = 3
} direction_e;

typedef struct {
  unsigned short start_symbol;
  unsigned char type_id;
  unsigned int data_length;
} packet_header_t;

typedef struct {
  unsigned char protocol_version;
  char name[32];
  char color;
} player_id_data_t;

typedef struct {
  char x_movement;
  char y_movement;
  unsigned char is_placing_bomb;
} player_input_data_t;

typedef struct {
  char message[256];
} player_message_t;

typedef struct {
  unsigned char protocol_version;
  unsigned int is_accepted;
} server_id_t;

typedef struct {
  unsigned char x_size;
  unsigned char y_size;
  // unsigned char block_id; // multiply by x_size and y_size
} map_state_t;

typedef struct {
  unsigned char count;
} movable_object_t;

typedef struct {
  unsigned char type_id;
  unsigned int id;
  float x;
  float y;
  char direction;
  unsigned char status;
} movable_object_data_t;

typedef struct {
  unsigned char message_type;
  char message[256];
} server_chat_message_t;

typedef struct {
  unsigned int player_count;
} player_info_t;

typedef struct {
  unsigned char id;
  char name[32];
  char color;
  unsigned int points;
  unsigned char lives;
} player_info_data_t;

#endif
