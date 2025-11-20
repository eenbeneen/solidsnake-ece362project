#include <string.h>
#include "pico/stdlib.h"
#include "matrix.h"

// Your wiring:
// gpio 25 is R1
// gpio 26 is G1
// gpio 27 is B1
// gpio 28 is R2
// gpio 29 is G2
// gpio 30 is B2
// gpio 31 is HA (A)
// gpio 32 is HB (B)
// gpio 33 is HC (C)
// gpio 34 is HD (D)
// gpio 35 is CLK
// gpio 36 is LAT
// gpio 37 is OE

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

// Simple 3-bit color: bit0=R, bit1=G, bit2=B (1 bit per channel)
static uint8_t framebuffer[MATRIX_HEIGHT][MATRIX_WIDTH];

static inline void set_row_address(int row_pair) {
    // row_pair is 0..15 for 32-pixel-tall, 1:16 scan
    gpio_put(PIN_A, (row_pair >> 0) & 1);
    gpio_put(PIN_B, (row_pair >> 1) & 1);
    gpio_put(PIN_C, (row_pair >> 2) & 1);
    gpio_put(PIN_D, (row_pair >> 3) & 1);
}

static inline void pulse_clk(void) {
    gpio_put(PIN_CLK, 1);
    gpio_put(PIN_CLK, 0);
}

static inline void pulse_lat(void) {
    gpio_put(PIN_LAT, 1);
    gpio_put(PIN_LAT, 0);
}

void matrix_init(void) {
    const uint8_t pins[] = {
        PIN_R1, PIN_G1, PIN_B1,
        PIN_R2, PIN_G2, PIN_B2,
        PIN_A, PIN_B, PIN_C, PIN_D,
        PIN_CLK, PIN_LAT, PIN_OE
    };

    for (int i = 0; i < (int)(sizeof(pins) / sizeof(pins[0])); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
        gpio_put(pins[i], 0);
    }

    // LEDs off to start
    gpio_put(PIN_OE, 1);

    matrix_clear();

}

void matrix_clear(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
    matrix_refresh_once();
    gpio_put(PIN_OE, 0);
}

void matrix_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) {
        return;
    }

    uint8_t val = (r ? 1 : 0) | (g ? 0b10 : 0) | (b ? 0b100 : 0);

    framebuffer[y][x] = val;
}

void matrix_refresh_once(void) {
    // For 64x32, 1:16 scan: 16 row-pairs (top+bottom)
    for (int row_pair = 0; row_pair < 16; row_pair++) {
        int top_y = row_pair;
        int bot_y = row_pair + 16;

        // Turn off LEDs while we shift data
        gpio_put(PIN_OE, 1);


        // Shift one row-pair (64 columns)
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            uint8_t top_col = framebuffer[top_y][x];
            uint8_t bot_col = framebuffer[bot_y][x];

            // Top half
            gpio_put(PIN_R1, (top_col >> 0) ? 1 : 0);
            gpio_put(PIN_G1, (top_col >> 1) ? 1 : 0);
            gpio_put(PIN_B1, (top_col >> 2) ? 1 : 0);

            // Bottom half
            gpio_put(PIN_R2, (bot_col >> 0) ? 1 : 0);
            gpio_put(PIN_G2, (bot_col >> 1) ? 1 : 0);
            gpio_put(PIN_B2, (bot_col >> 2) ? 1 : 0);

            pulse_clk();
        }

        // Latch the shifted data into outputs
        pulse_lat();

        
        set_row_address(row_pair);

        // Enable LEDs for a short time (this controls overall brightness)
        gpio_put(PIN_OE, 0);
        sleep_us(50);   // tweak this for brightness / flicker
        gpio_put(PIN_OE, 1);
    }
}
