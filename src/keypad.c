#include "pico/stdlib.h"
#include <hardware/gpio.h>
#include <stdio.h>
#include "keypad.h"

// Global column variable
int col = -1;

// Global key state
static bool state[16]; // Are keys pressed/released

// Keymap for the keypad
const char keymap[17] = "DCBA#9630852*741";

// Key event queue
#define KEY_QUEUE_SIZE 64
static uint16_t key_queue[KEY_QUEUE_SIZE];
static int queue_head = 0;
static int queue_tail = 0;

// Push a key event to the queue
void key_push(uint16_t key_event) {
    key_queue[queue_tail] = key_event;
    queue_tail = (queue_tail + 1) % KEY_QUEUE_SIZE;
    if (queue_tail == queue_head) {
        // Queue overflow - discard oldest
        queue_head = (queue_head + 1) % KEY_QUEUE_SIZE;
    }
}

// Pop a key event from the queue (returns 0 if empty)
uint16_t key_pop() {
    if (queue_head == queue_tail) {
        return 0;
    }
    uint16_t key = key_queue[queue_head];
    queue_head = (queue_head + 1) % KEY_QUEUE_SIZE;
    return key;
}


/********************************************************* */
// Implement the functions below.

void keypad_init_pins() {
    //Set GP6-9 to output
    for (int gpio = 6; gpio <= 9; gpio++) {
        gpio_init(gpio);
        gpio_set_function(gpio, GPIO_FUNC_SIO);
        gpio_set_dir(gpio, GPIO_OUT);
        gpio_put(gpio, 0);
    }
    //Set GP2-5 to input
    for (int gpio = 2; gpio <= 5; gpio++) {
        gpio_init(gpio);
        gpio_set_function(gpio, GPIO_FUNC_SIO);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }
}

void keypad_init_timer() {
    hw_set_bits(&timer_hw->inte, (1u << 0) | (1u << 1));
    
    irq_set_exclusive_handler(timer_hardware_alarm_get_irq_num(timer_hw, 0), keypad_drive_column);
    irq_set_exclusive_handler(timer_hardware_alarm_get_irq_num(timer_hw, 1), keypad_isr);
    irq_set_enabled(timer_hardware_alarm_get_irq_num(timer_hw, 0), true);
    irq_set_enabled(timer_hardware_alarm_get_irq_num(timer_hw, 1), true);
                     
    timer_hw->alarm[0] = timer_hw->timerawl + 1000000;
    timer_hw->alarm[1] = timer_hw->timerawl + 1100000;


}

void keypad_drive_column() {
    // fill in
    timer_hw->intr = (1u << 0);
    if (col >= 0) {
        gpio_put(col+6, 0);
    }
    col = (col+1)%4;
    sio_hw->gpio_togl = 1u << (6 + col);
    
    timer_hw->alarm[0] = timer_hw->timerawl + 25000;
}

uint8_t keypad_read_rows() {
    return (sio_hw->gpio_in >> 2) & 0xF;
}

void keypad_isr() {
    // fill in
    timer_hw->intr = (1u << 1);

    if (col < 0) {
        timer_hw->alarm[1] = timer_hw->timerawl + 25000;
        return;
    }

    uint8_t rowvals = keypad_read_rows();


    for (int row = 0; row < 4; row++) {
        int ind = col*4 + row;
        if (rowvals & (1u << row)) {
            if (!state[ind]) {
                state[ind] = true;
                key_push(((uint16_t)1u << 8) | (uint16_t)(uint8_t)keymap[ind]);
            }
        }
        else if (state[ind]) {
            state[ind] = false;
            key_push(((uint16_t)0u << 8) | (uint16_t)(uint8_t)keymap[ind]);
        }
    }
    
    timer_hw->alarm[1] = timer_hw->timerawl + 25000;
    
}