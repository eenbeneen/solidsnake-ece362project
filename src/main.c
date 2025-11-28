#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/xosc.h"
#include "pico/multicore.h"
#include "matrix.h"
#include "keypad.h"
#include "pico/rand.h"
#include "pico/time.h"  //for timer
#include "hardware/gpio.h"


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
int gameGrid[8][16] = {0};

bool stateGame = false;

repeating_timer_t game_timer;
bool game_tick = false;
int game_timer_interval = 500;

bool growOnNextUpdate = false;

int highScore = 3;

typedef struct SnakePart {
    int xpos;
    int ypos;
    int dir;
    struct SnakePart *next;
} SnakePart;

static SnakePart* head;

void startGameTimer(int speed);
void changeGameSpeed(int speed);
void game_timer_callback(repeating_timer_t *rt);
void die();
void freeSnake();
void rotate(bool goLeft);
int isTouchingItself();
void updateSnake(SnakePart* s);
void init_outputs();
void init_inputs();
void init_keypad();
void grow();
void drawNum(uint8_t number, int x, int y, uint8_t r, uint8_t g, uint8_t b);
void drawFruit(int x, int y);
void drawSnakePart(int x, int y);
void drawWord(int wordsel, int x, int y, uint8_t r, uint8_t g, uint8_t b);
void drawMenu();
void initGame();
void updateGame();
void spawnFruit();
void drawScore(int score, int x, int y, uint8_t r, uint8_t g, uint8_t b);

//Starts a new timer with speed in ms
void startGameTimer(int speed) {
    add_repeating_timer_ms(
        speed,
        game_timer_callback,
        NULL,
        &game_timer
    );
}

void changeGameSpeed(int speed) {
    cancel_repeating_timer(&game_timer);
    startGameTimer(speed);
    game_timer_interval = speed;
}

void game_timer_callback(repeating_timer_t *rt) {
    if (stateGame) {
        game_tick = true;
    }
}

//Call this when the snake dies
void die() {
    freeSnake();
    stateGame = false;
    drawMenu();
}

void freeSnake() {
    SnakePart* cur = head;
    while (cur != NULL) {
        SnakePart* next = cur->next;
        free(cur);
        cur = next;
    }
}

//Rotates snake
//head = snake head pointer
//goLeft = 1 if going left, 0 if going right
void rotate(bool goLeft) {
    head->dir = goLeft ? (head->dir + 1)%4 : (head->dir + 3)%4;
}

//Checks if any parts of a snake have the same position as the head
//head = snake head pointer
int isTouchingItself() {
    //Implement later
    return 0;
}

//Moves snake starting from s
void updateSnake(SnakePart* s) {
    //Update all parts before this one first
    if (s->next) {
        updateSnake(s->next);
        s->next->dir = s->dir;
    } else {
        if (!growOnNextUpdate) {
            gameGrid[s->ypos][s->xpos] = 0;
        }
        else {
            
            grow();
            spawnFruit();
            growOnNextUpdate = false;
        }
    }
    //Move in appropriate direction
    //If snake position is at the edge and
    //you are about to move out stop everything
    switch (s->dir) {
        case SNAKEPART_DIR_RIGHT:
            s->xpos++;
            break;
        case SNAKEPART_DIR_UP:
            s->ypos--;
            break;
        case SNAKEPART_DIR_LEFT:
            s->xpos--;
            break;
        case SNAKEPART_DIR_DOWN:
            s->ypos++;
            break;
    }
    //Check if out of bounds
    if (s->ypos > 7 || s->ypos < 0 || s->xpos > 15 || s->xpos < 0) {
        die();
        return;
    }


    //Set new position in gameGrid
    if (gameGrid[s->ypos][s->xpos] == 2) {
        growOnNextUpdate = true;
    }
    gameGrid[s->ypos][s->xpos] = 1;
    
    
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
void grow() {
    SnakePart* tail = head;
    while (tail->next)
        tail = tail->next;
    SnakePart* newTail = malloc(sizeof(SnakePart));
    newTail->xpos = tail->xpos;
    newTail->ypos = tail->ypos;
    newTail->dir = tail->dir;
    newTail->next = NULL;
    tail->next = newTail;
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

void drawFruit(int x, int y) {
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            matrix_set_pixel(x*4 + 1 + col, y*4 + 1 + row, 1, 0, 0);
        }
    }
}

void drawSnakePart(int x, int y) {
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            matrix_set_pixel(x*4 + 1 + col, y*4 + 1 + row, 0, 1, 0);
        }
    }
}

//Prints a word on the screen based on wordsel value
//0 - PLAY, 1 - SPEED, 2 - SCORE
void drawWord(int wordsel, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (wordsel > 2 || wordsel < 0) return;

    uint8_t play[5][7] = {
        {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}, //P
        {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}, //L
        {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}, //A
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

void drawMenu() {
    matrix_clear();
    drawWord(WORD_PLAY, 5, 2, 0, 0, 1);
    drawWord(WORD_SPEED, 5, 13, 0, 0, 1);
    drawWord(WORD_SCORE, 5, 23, 0, 0, 1);
    drawScore(highScore, 40, 23, 1, 0, 0);
    matrix_refresh_once();
}

//Initialize the starting scene of the game and returns head of snake
void initGame() {
    memset(gameGrid, 0, sizeof(gameGrid));
    //First setup the snake
    head = malloc(sizeof(SnakePart));
    head->xpos = 4;
    head->ypos = 1;
    head->dir = SNAKEPART_DIR_RIGHT;
    SnakePart* part1 = malloc(sizeof(SnakePart));
    part1->xpos = 3;
    part1->ypos = 1;
    part1->dir = SNAKEPART_DIR_RIGHT;
    SnakePart* part2 = malloc(sizeof(SnakePart));
    part2->xpos = 2;
    part2->ypos = 1;
    part2->dir = SNAKEPART_DIR_RIGHT;
    
    head->next = part1;
    part1->next = part2;
    part2->next = NULL;

    gameGrid[1][4] = 1;
    gameGrid[1][3] = 1;
    gameGrid[1][2] = 1;

    spawnFruit();
}

void updateGame() {
    matrix_clear();
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 8; y++) {
            switch(gameGrid[y][x]) {
                case 1:
                    drawSnakePart(x, y);
                    break;
                case 2:
                    drawFruit(x, y);
                    break;
                default:
                    break;
            }
        }
    }

    for (int x = 0; x < 64; x++) {
        if (x == 0 || x == 63) {
            for (int y = 0; y < 32; y++) {
                matrix_set_pixel(x, y, 1, 1, 1);
            }
        }
        else {
            matrix_set_pixel(x, 0, 1, 1, 1);
            matrix_set_pixel(x, 31, 1, 1, 1);
        }
    }
    matrix_refresh_once();
}

void spawnFruit() {
    int emptyCoords[128][2] = {0};
    int index = 0;
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 8; y++) {
            if (gameGrid[y][x] == 0) {
                emptyCoords[index][0] = x;
                emptyCoords[index][1] = y;
                index++;
            }
        }
    }
    int r = get_rand_32() % index;
    gameGrid[emptyCoords[r][1]][emptyCoords[r][0]] = 2;
}

void drawScore(int score, int x, int y, uint8_t r, uint8_t g, uint8_t b) {

    //send score to itself
    uint8_t score2 = matrix_send_score((uint8_t)score);


    int hundreds = (score2 / 100) % 10;
    int tens = (score2 / 10) % 10;
    int ones = score2 % 10;

    // Draw first digit
    drawNum(hundreds, x, y, r, g, b);

    // Draw second digit (7 pixels to the right)
    drawNum(tens, x + 7, y, r, g, b);
    // Draw third digit (14 pixels to the right)
    drawNum(ones, x + 14, y, r, g, b);
}

int main() {

    stdio_init_all();
    keypad_init_pins();
    keypad_init_timer();
    matrix_init();
    startGameTimer(game_timer_interval);
    initGame();
    stateGame = false;
    highScore = 122;
    drawMenu();

    while(1) {

        
        matrix_refresh_once();


        uint16_t ev = key_pop();
        if (ev) {
            unsigned char ch = ev & 0xFF;
            int pressed = (ev >> 8) & 1;
            if (pressed) {
                if (stateGame) {
                    if (ch == '4') rotate(1);
                    else if (ch == '6') rotate(0);
                }
                else {
                    initGame(head);
                    stateGame = true;
                }
            }
        }

        if (game_tick) {
            game_tick = false;
            updateGame();
            updateSnake(head);
        }
        
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