
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

#define WORD_PLAY 0
#define WORD_SPEED 1
#define WORD_SCORE 2

const uint8_t pixelNums[10][7] = {
    {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}, //0
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

//divide matrix into 8x16 grid for snake game
//define values: 0 = empty, 1 = snake, 2 = food
#define GRID_EMPTY = 0
#define GRID_SNAKE = 1
#define GRID_FOOD = 2
uint8_t gameGrid[8][16] = {0};

const char keymap[16] = "DCBA#9630852*741";
char key = '\0';
int col = 0;
bool stateGame = false;

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

}

//Adds one part to the tail of the snake
//head = snake head pointer
void grow(SnakePart* head) {
    SnakePart* tail = head;
    while (tail->next != NULL)
        tail = tail->next;
    SnakePart newTail = {tail->xpos, tail->ypos, tail->dir, NULL};
    tail->next = &newTail;
}

//x, y = top-left corner of number
void drawNum(uint8_t number, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (number > 9)
        return;

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (pixelNums[number][row] & (1 << (4 - col))) {
                matrix_set_pixel(x + col, y + row, r, g, b);
            } else {
                matrix_set_pixel(x + col, y + row, 0, 0, 0);
            }
        }
    }
}

void drawFood(Food food) {
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            matrix_set_pixel(food.xpos + col, food.ypos + row, 1, 0, 0);
        }
    }
}

void drawSnakePart(SnakePart part) {
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            matrix_set_pixel(part.xpos + col, part.ypos + row, 0, 1, 0);
        }
    }
}

void drawSnake(SnakePart* head) {
    SnakePart* current = head;
    while (current != NULL) {
        drawSnakePart(*current);
        current = current->next;
    }
}

//Prints a word on the screen based on wordsel value
//0 - PLAY, 1 - SPEED, 2 - SCORE
void drawWord(int wordsel, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (wordsel > 2 || wordsel < 1) return;

    uint8_t play[5][7] = {
        {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}, //P
        {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}, //L
        {0b01110, 0b10001, 0b10000, 0b11111, 0b10001, 0b10001, 0b10001}, //A
        {0b10001, 0b01010, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100}, //Y
        {0,0,0,0,0,0,0} //empty
    };
    uint8_t speed[5][7] = {
        {0b01110, 0b10001, 0b10000, 0b01110, 0b00001, 0b10001, 0b01110}, //S
        {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}, //P
        {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}, //E
        {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}, //E
        {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110}, //D
    };
    uint8_t score[5][7] = {
        {0b01110, 0b10001, 0b10000, 0b01110, 0b00001, 0b10001, 0b01110}, //S
        {0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110}, //C
        {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, //O
        {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b10001}, //R
        {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}, //E
    };

    uint8_t (*letters)[7] = 
        (wordsel == WORD_PLAY) ? play :
        (wordsel == WORD_SPEED) ? speed :
        score;
    int currX = x;
    for (int i = 0; i < 5; i++) {
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (letters[i][row] & (1 << (4 - col))) {
                    matrix_set_pixel(currX + col, y + row, r, g, b);
                } else {
                    matrix_set_pixel(currX + col, y + row, 0, 0, 0);
                }
            }
        }
        currX += 7;
    }
}

int main() {

    stdio_init_all();

    matrix_init();
    // Draw numbers 0-9 in top row
    for (int i = 0; i < 10; i++) {
        drawNum(i, i * 6, 0, 1, 0, 0);
    }

    //draw food at 10, 0
    Food food = {5, 20};
    drawFood(food);

    //draw a snake at 20, 20
    SnakePart head = {20, 20, SNAKEPART_DIR_RIGHT, NULL};
    SnakePart tail = {18, 20, SNAKEPART_DIR_RIGHT, NULL};
    head.next = &tail;
    drawSnake(&head);

    //draw word at 40, 20
    drawWord(WORD_SCORE, 30, 20, 1, 0, 0);


    while (1) {
        matrix_refresh_once();
        // No sleeps here: we want to refresh as fast as possible
    }
    
    //push
    return 0;
}

// test_matrix.c – standalone diagnostic, ignore matrix.c / matrix.h
/*
#include "pico/stdlib.h"

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

static inline void set_row_address(int row_pair) {
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
    sleep_us(50);
    gpio_put(PIN_LAT, 0);
}

int main() {
    stdio_init_all();

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

    // Enable output by default (OE active low)
    gpio_put(PIN_OE, 0);

    while (1) {
        // SCAN ALL ROW PAIRS, IGNORING FRAMEBUFFER COMPLETELY
        for (int row_pair = 0; row_pair < 16; row_pair++) {
            // Select this row pair
            set_row_address(row_pair);

            // Shift 64 GREEN pixels for both top and bottom
            for (int x = 0; x < 64; x++) {
                // PURE GREEN TEST: ONLY G1/G2 HIGH
                gpio_put(PIN_R1, 0);
                gpio_put(PIN_G1, 1);
                gpio_put(PIN_B1, 0);

                gpio_put(PIN_R2, 0);
                gpio_put(PIN_G2, 1);
                gpio_put(PIN_B2, 0);
                pulse_clk();
            }

            pulse_lat();          // latch this row-pair
            gpio_put(PIN_OE, 0);
            sleep_us(50);        // keep it visible a bit
            gpio_put(PIN_OE, 1);
        }
    }
}
*/