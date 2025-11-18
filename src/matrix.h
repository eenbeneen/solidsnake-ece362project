#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>

#define MATRIX_WIDTH   64
#define MATRIX_HEIGHT  32

void matrix_init(void);
void matrix_clear(void);
void matrix_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void matrix_refresh_once(void);

#endif
