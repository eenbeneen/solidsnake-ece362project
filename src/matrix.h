#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>


#define PIN_R1   10
#define PIN_G1   11
#define PIN_B1   12
#define PIN_R2   13
#define PIN_G2   14
#define PIN_B2   15

#define PIN_A    16
#define PIN_B    17
#define PIN_C    18
#define PIN_D    19

#define PIN_CLK  20
#define PIN_LAT  21
#define PIN_OE   26

#define MATRIX_WIDTH   64
#define MATRIX_HEIGHT  32

#define PIN_SPI_SCK 27
#define PIN_SPI_MOSI 28
#define PIN_SPI_MISO 29
#define PIN_SPI_CS 30


uint8_t matrix_send_score(uint8_t score);
void matrix_init(void);
void matrix_clear(void);
void matrix_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void matrix_refresh_once(void);

#endif
