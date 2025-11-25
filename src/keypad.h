#ifndef KEYPAD_H
#define KEYPAD_H

#include "pico/stdlib.h"

void keypad_drive_column();
void keypad_isr();
void keypad_init_pins();
void keypad_init_timer();
uint8_t keypad_read_rows();
void key_push(uint16_t key_event);
uint16_t key_pop();

#endif