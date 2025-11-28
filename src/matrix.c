#include <string.h>
#include "pico/stdlib.h"
#include "matrix.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "hub75_shift.pio.h"

#ifndef MATRIX_COMPAT_SHIFT_BUG
#define MATRIX_COMPAT_SHIFT_BUG 1
#endif

static uint8_t framebuffer[MATRIX_HEIGHT][MATRIX_WIDTH];

static PIO  g_pio = pio0;
static uint g_sm  = 0;
static uint g_off = 0;

void matrix_init_score() {
    spi_init(spi0, 1000000);  // 1 MHz, plenty fast

    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI); // SCK
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI); // MISO

    // Chip select pin
    gpio_init(PIN_SPI_CS);
    gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
    gpio_put(PIN_SPI_CS, 1);
}

uint8_t matrix_send_score(uint8_t score) {
    uint8_t rx;

    gpio_put(PIN_SPI_CS, 0);                 // select
    spi_write_read_blocking(spi0, &score, &rx, 1);
    gpio_put(PIN_SPI_CS, 1);                 // deselect

    return rx; // whatever came back
}

static inline void set_row_address(int row_pair) {
    gpio_put(PIN_A, (row_pair >> 0) & 1);
    gpio_put(PIN_B, (row_pair >> 1) & 1);
    gpio_put(PIN_C, (row_pair >> 2) & 1);
    gpio_put(PIN_D, (row_pair >> 3) & 1);
}

static inline void pulse_lat(void) {
    gpio_put(PIN_LAT, 1);
    sleep_us(10);
    gpio_put(PIN_LAT, 0);
}

static inline uint8_t pack6(uint8_t top_col, uint8_t bot_col) {
#if MATRIX_COMPAT_SHIFT_BUG
    // gpio_put(PIN_R1, (top_col >> 0) ? 1 : 0);
    // gpio_put(PIN_G1, (top_col >> 1) ? 1 : 0);
    // gpio_put(PIN_B1, (top_col >> 2) ? 1 : 0);
    uint8_t r1 = (top_col >> 0) ? 1 : 0;
    uint8_t g1 = (top_col >> 1) ? 1 : 0;
    uint8_t b1 = (top_col >> 2) ? 1 : 0;

    uint8_t r2 = (bot_col >> 0) ? 1 : 0;
    uint8_t g2 = (bot_col >> 1) ? 1 : 0;
    uint8_t b2 = (bot_col >> 2) ? 1 : 0;
#else
    uint8_t r1 = (top_col & 0x1) ? 1 : 0;
    uint8_t g1 = (top_col & 0x2) ? 1 : 0;
    uint8_t b1 = (top_col & 0x4) ? 1 : 0;

    uint8_t r2 = (bot_col & 0x1) ? 1 : 0;
    uint8_t g2 = (bot_col & 0x2) ? 1 : 0;
    uint8_t b2 = (bot_col & 0x4) ? 1 : 0;
#endif

    return (r1 << 0) | (g1 << 1) | (b1 << 2) | (r2 << 3) | (g2 << 4) | (b2 << 5);
}

static inline void bitstream_write(uint32_t *dst, uint32_t bit_index, uint32_t value, uint32_t nbits) {
    uint32_t wi = bit_index >> 5;
    uint32_t sh = bit_index & 31;
    if (sh + nbits <= 32) {
        dst[wi] |= (value << sh);
    } else {
        uint32_t lo = 32 - sh;
        dst[wi]     |= (value & ((1u << lo) - 1u)) << sh;
        dst[wi + 1] |= (value >> lo);
    }
}

void matrix_init(void) {
    const uint8_t ctrl_pins[] = { PIN_A, PIN_B, PIN_C, PIN_D, PIN_LAT, PIN_OE };
    for (int i = 0; i < (int)(sizeof(ctrl_pins) / sizeof(ctrl_pins[0])); i++) {
        gpio_init(ctrl_pins[i]);
        gpio_set_dir(ctrl_pins[i], GPIO_OUT);
        gpio_put(ctrl_pins[i], 0);
    }

    gpio_put(PIN_OE, 1);

    // Load PIO program
    g_off = pio_add_program(g_pio, &hub75_shift_program);
    g_sm  = 0;

    pio_sm_set_enabled(g_pio, g_sm, false);
    pio_sm_clear_fifos(g_pio, g_sm);
    pio_sm_restart(g_pio, g_sm);

    pio_sm_config c = hub75_shift_program_get_default_config(g_off);

    // Data pins are contiguous: 10..15
    sm_config_set_out_pins(&c, PIN_R1, 6);
    sm_config_set_sideset_pins(&c, PIN_CLK);

    // pull threshold = 6 so every `out pins,6` consumes exactly one word’s low 6 bits
    sm_config_set_out_shift(&c, true, false, 32);  // shift_right=true, autopull=false


    // Deeper TX FIFO
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    // was 8.0f
    sm_config_set_clkdiv(&c, 32.0f);


    // Init PIO pins / directions
    for (int i = 0; i < 6; i++) pio_gpio_init(g_pio, PIN_R1 + i);
    pio_gpio_init(g_pio, PIN_CLK);

    pio_sm_set_consecutive_pindirs(g_pio, g_sm, PIN_R1, 6, true);
    pio_sm_set_consecutive_pindirs(g_pio, g_sm, PIN_CLK, 1, true);

    pio_sm_init(g_pio, g_sm, g_off, &c);
    pio_sm_set_enabled(g_pio, g_sm, true);

    matrix_init_score();

    matrix_clear();
}

void matrix_clear(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
    matrix_refresh_once();
    gpio_put(PIN_OE, 0);
}

void matrix_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
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

        // Clear "done" flag for this row
        pio_interrupt_clear(g_pio, 0);

        // Push exactly 64 pixels worth of 6-bit data.
        // IMPORTANT: this requires sm_config_set_out_shift(... autopull, threshold=6)
        // so each OUT pins,6 consumes one pushed word (low 6 bits).
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            uint8_t top_col = framebuffer[top_y][x];
            uint8_t bot_col = framebuffer[bot_y][x];
            uint8_t w6 = pack6(top_col, bot_col);   // [R1 G1 B1 R2 G2 B2] in bits 0..5
            pio_sm_put_blocking(g_pio, g_sm, (uint32_t)w6);
        }

        // Wait until PIO signals "finished shifting this row"
        while (!pio_interrupt_get(g_pio, 0)) {
            tight_loop_contents();
        }
        pio_interrupt_clear(g_pio, 0);

        // Latch the shifted data into outputs
        pulse_lat();

        // Select which row-pair is active
        set_row_address(row_pair);

        // Enable LEDs for a short time (controls brightness)
        gpio_put(PIN_OE, 0);
        sleep_us(50);   // same as your original
        gpio_put(PIN_OE, 1);
    }
}




/*
#include <string.h>
#include "pico/stdlib.h"
#include "matrix.h"

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
    sleep_us(1);
    gpio_put(PIN_CLK, 0);
}

static inline void pulse_lat(void) {
    gpio_put(PIN_LAT, 1);
    sleep_us(10);
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
*/