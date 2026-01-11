#ifndef COMMON_H
#define COMMON_H

//MVP nastavenia sveta
#define GRID_W 40
#define GRID_H 40
#define MAX_H 200
#define MAX_W 200

//Sieť
#define DEFAULT_PORT 5555
#define MAX_NAME_LEN 32
#define MAX_PLAYERS 8

//Herné smery
typedef enum {
  DIR_NONE = -1,
  DIR_UP = 0,
  DIR_DOWN = 1,
  DIR_LEFT = 2,
  DIR_RIGHT = 3
} direction_t;

#endif
