#include <ctype.h>
#include <stdio.h>
#include <string.h>

char printable_char(char c) {
  if (isprint(c) != 0) {
    return c;
  }

  return ' ';
}

void print_bytes(void *packet, int count) {
  if (count > 999) {
    return;
  }

  printf("[No] [C] [HEX] [DEC] [ BINARY ]\n");
  printf("================================\n");

  char *p = (char *) packet;
  int i;

  for (i = 0; i < count; i++) {
    printf("%3d | %c | %02x | %3d | %c%c%c%c%c%c%c%c\n", i, printable_char(p[i]), p[i], p[i],
      p[i] & 0x80 ? '1' : '0',
      p[i] & 0x40 ? '1' : '0',
      p[i] & 0x20 ? '1' : '0',
      p[i] & 0x10 ? '1' : '0',
      p[i] & 0x08 ? '1' : '0',
      p[i] & 0x04 ? '1' : '0',
      p[i] & 0x02 ? '1' : '0',
      p[i] & 0x01 ? '1' : '0'
    );
  }
}

/*
  Packet and packet length WITHOUT checksum
  Returns calculated checksum for passed packet
 */
char calculate_checksum(void *packet, int length) {
  int i;
  char *p = (char *)packet;
  char checksum = p[0];

  for (i = 1; i < length; i++) {
    checksum ^= p[i];
  }

  return checksum;
}

/* Compares calculated checksum vs one in packet and returns
 *  1 if they match
 *  else return 0
 */
int compare_checksum(void *packet, int length, char sent_checksum) {
  char calculated_checksum = calculate_checksum(packet, length);

  if (calculated_checksum == sent_checksum) {
    return 1;
  }

  return 0;
}
