#ifndef __OBJECT_TYPES_H
#define __OBJECT_TYPES_H

typedef enum {
  BLOCK_TYPE_EMPTY = 0x00,
  BLOCK_TYPE_WALL = 0x01,
  BLOCK_TYPE_BOX = 0x02,
  BLOCK_TYPE_EXPLOSION = 0x03,
  BLOCK_TYPE_POWER_UP_1 = 0x04,
  BLOCK_TYPE_POWER_UP_2 = 0x05,
  BLOCK_TYPE_POWER_UP_3 = 0x06,
  BLOCK_TYPE_POWER_UP_4 = 0x07
} block_types_e;

typedef enum {
  MOVABLE_OBJECT_PLAYER = 0x00,
  MOVABLE_OBJECT_BOMB = 0x01
} movable_object_types_e;

#endif
