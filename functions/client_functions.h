#ifndef __CLIENT_FUNCTIONS_H
#define __CLIENT_FUNCTIONS_H

#include "helper.h"

void connect_to_server();
char *get_server_ip();
int get_server_port();

void join_game();
void get_player_data(char[32], char *);

void handle_packet(unsigned char, char *, int);
void handle_server_id_packet(char *, int);
void handle_server_map_packet(char *, int);
void handle_server_objects_packet(char *, int);
void handle_server_ping_packet();

int get_player_id_packet();
int get_ping_response_packet();
void send_player_input_packet();
void send_packet(int);
void draw_block(char);
void draw_object(char);
void render_game();
unsigned char find_object_in_coords(int, int);

void create_game_thread();
void *game_thread_handler();
void handle_pressed_button(int);

void exit_program();
#endif
