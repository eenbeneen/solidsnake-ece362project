#include <stdio.h>
#include <stdlib.h>

#define X_MAX 31
#define Y_MAX 31
#define X_MIN 0
#define Y_MIN 0
#define SNAKEPART_DIR_RIGHT 0
#define SNAKEPART_DIR_UP 1
#define SNAKEPART_DIR_LEFT 2
#define SNAKEPART_DIR_DOWN 3


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

/*  Keypad functions. Could use the ones from lab
void init_outputs() {
    // fill in
    *intialize gpio output for each pin
}

void init_inputs() {
    // fill in
    *intialize gpio input for each pin
}

void init_keypad() {
    // fill in
    
}
*/

//Adds one part to the tail of the snake
//head = snake head pointer
void grow(SnakePart* head) {
    SnakePart* tail = head;
    while (tail->next != NULL)
        tail = tail->next;
    SnakePart newTail = {tail->xpos, tail->ypos, tail->dir, NULL};
    tail->next = &newTail;
}

int main() {

    /*  Intialize keypad
    init_outputs();
    init_inputs();
    init_keypad();
    */
    
    return 0;
}