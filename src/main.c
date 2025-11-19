#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/xosc.h"
#include "pico/multicore.h"
#include "matrix.h"



#define X_MAX 31
#define Y_MAX 31
#define X_MIN 0
#define Y_MIN 0
#define SNAKEPART_DIR_RIGHT 0
#define SNAKEPART_DIR_UP 1
#define SNAKEPART_DIR_LEFT 2
#define SNAKEPART_DIR_DOWN 3

const uint8_t pixelNums[10][7] = {
    {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, //0
    {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}, //1
    {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}, //2
    {0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110}, //3
    {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}, //4
    {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}, //5
    {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}, //6
    {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}, //7
    {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}, //8
    {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100}  //9
};

const char keymap[16] = "DCBA#9630852*741";
char key = '\0';
int col = 0;

typedef struct SnakePart {
    int xpos;
    int ypos;
    int dir;
    struct SnakePart *next;
} SnakePart;

typedef struct Food {
    int xpos;
    int ypos;
} Food;

//Call this when the snake dies
void die() {
    //Add restart later
    exit(0);
}

//Rotates snake
//head = snake head pointer
//goLeft = 0 if going left, 1 if going right
void rotate(SnakePart* head, int goLeft) {
    head->dir = goLeft ? (head->dir + 1)%4 : (head->dir - 1)%4;
}

//Checks if any parts of a snake have the same position as the head
//head = snake head pointer
int isTouchingItself(SnakePart* head) {
    //Implement later
    return 0;
}

//Moves snake starting from part s
void move(SnakePart* s) {
    //Move in appropriate direction
    switch (s->dir) {
        case SNAKEPART_DIR_RIGHT:
            s->xpos++;
            break;
        case SNAKEPART_DIR_UP:
            s->ypos++;
            break;
        case SNAKEPART_DIR_LEFT:
            s->xpos--;
            break;
        case SNAKEPART_DIR_DOWN:
            s->ypos--;
            break;
    }
    
}

/*
void init_outputs() {
    // fill in
    for(int i = 22; i <26; i++ ) {
        uint32_t mask = 1u << (i);
        sio_hw->gpio_oe_set = mask;
        hw_write_masked(&pads_bank0_hw->io[i],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
        );
        sio_hw->gpio_clr = mask;
        io_bank0_hw->io[i].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
        hw_clear_bits(&pads_bank0_hw->io[i], PADS_BANK0_GPIO0_ISO_BITS);
    }
}

void init_inputs() {
    // fill in

   
        uint32_t mask = 1u << (21);
        sio_hw->gpio_oe_clr = mask;
        hw_write_masked(&pads_bank0_hw->io[21],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
                );


        io_bank0_hw->io[21].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;

        hw_clear_bits(&pads_bank0_hw->io[21], PADS_BANK0_GPIO0_ISO_BITS);

        mask = 1u << (26);
        sio_hw->gpio_oe_clr = mask;
        hw_write_masked(&pads_bank0_hw->io[26],
                   PADS_BANK0_GPIO0_IE_BITS,
                   PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
                );

        io_bank0_hw->io[26].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;

        hw_clear_bits(&pads_bank0_hw->io[26], PADS_BANK0_GPIO0_ISO_BITS);
    

}

void init_keypad() {
    // fill in

    for (int pin = 2; pin <= 5; pin++) {
        uint32_t mask = 1u << pin;
        sio_hw->gpio_oe_clr = mask;

        hw_write_masked(&pads_bank0_hw->io[pin],
                        PADS_BANK0_GPIO0_IE_BITS,
                        PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);

        io_bank0_hw->io[pin].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;

        hw_clear_bits(&pads_bank0_hw->io[pin], PADS_BANK0_GPIO0_ISO_BITS);
    }

    for (int pin = 6; pin <= 9; pin++) {
        uint32_t mask = 1u << pin;
        sio_hw->gpio_oe_set = mask;

        hw_write_masked(&pads_bank0_hw->io[pin],
                        PADS_BANK0_GPIO0_IE_BITS,
                        PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);

        sio_hw->gpio_clr = mask;

        io_bank0_hw->io[pin].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;

        hw_clear_bits(&pads_bank0_hw->io[pin], PADS_BANK0_GPIO0_ISO_BITS);
    }

}*/

//Adds one part to the tail of the snake
//head = snake head pointer
void grow(SnakePart* head) {
    SnakePart* tail = head;
    while (tail->next != NULL)
        tail = tail->next;
    SnakePart newTail = {tail->xpos, tail->ypos, tail->dir, NULL};
    tail->next = &newTail;
}

void dispMessage() {
    for (int i = 0; i < MATRIX_WIDTH; i++) {
        printf("setting pixel");
        matrix_set_pixel(i, 0, 0, 1, 0);
    }
    while (1) {
        matrix_refresh_once();
    }
}

int main() {
    //stdio_init_all();
    
    // init_outputs();
    // init_inputs();
    // init_keypad();
    //printf("Hello");
    //matrix_init();
    //dispMessage();

    stdio_init_all();

    matrix_init();

    // Draw a simple pattern: top row green, middle row red, bottom row blue
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        matrix_set_pixel(x, 0, 1, 1, 1);          // row 0 green
        matrix_set_pixel(x, 15, 1, 0, 0);         // row 15 red
        matrix_set_pixel(x, 31, 0, 0, 1);         // row 31 blue
    }

    while (1) {
        matrix_refresh_once();
        // No sleeps here: we want to refresh as fast as possible
    }
    
    //push
    return 0;
}