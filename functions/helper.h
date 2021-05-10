#ifndef __HELPER_H
#define __HELPER_H

char printable_char(char);
void print_bytes(void *, int);

char calculate_checksum(void *, int);
int compare_checksum(void *, int, char);

#endif
