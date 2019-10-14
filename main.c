#include <avr/io.h>
#include <util/delay.h>
#include <cpu_speed.h>
#include <graphics.h>
#include <lcd.h>
#include <macros.h>
#include "lcd_model.h"

#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include <stdlib.h>
#define SQRT(x,y) sqrt(x*x + y*y)
// screen size 84 X 48  pixels

//****************************************************//
// Tom & Jerry shapes                                 //
//****************************************************//

uint8_t tom[5] = {
    0b11111,
    0b00100,
    0b00100,
    0b00100,
    0b00100
};

uint8_t jerry[5] = {
    0b00111,
    0b00010,
    0b00010,
    0b11010,
    0b01100
};
uint8_t jerryDirected[5];
uint8_t tomDirected[5];

int jerryX = 0;
int jerryY = 9;

int tomX = LCD_X - 8; // 8 is bitmap size
int tomY = LCD_Y - 9; // 8 is bitmap size

// joystic
int joystick_l;
int joystick_r;
int joystick_d;
int joystick_u;

// Global variables
int currentLevel = 1;
int jerryLives = 5;
int gameScore = 0;

int clock_second=0;
int clock_minute=0;
 
int timerCounter = 0; //counter for the timer
int timerPrescaler = 40; // speed of the counter

// walls
int walls[][4] = {
    {18,15,13,25},
    {25,35,25,45},
    {45,10,60,10},
    {58,25,72,30}
}; 
int numOfWalls = 4;

//****************************************************//
// Starter functions                                  //
//****************************************************//

void setup_draw(void) {
    // Visit each column of output bitmap
    for (int i = 0; i < 5; i++) {
        // Visit each row of output bitmap
        for (int j = 0; j < 5; j++) {
            // Kind of like: cactus_direct[i][j] = cactus_original[j][7-i].
            // Flip about the major diagonal.
            uint8_t jerry_bit_val = BIT_VALUE(jerry[j], (4 - i));
            WRITE_BIT(jerryDirected[i], j, jerry_bit_val);
            uint8_t tom_bit_val = BIT_VALUE(tom[j], (4 - i));
            WRITE_BIT(tomDirected[i], j, tom_bit_val);
        }
    }

}

// 2
// This function setup the device 
void setup( void ) {
    // set The speed
	set_clock_speed(CPU_8MHz);

	//	input from the Left, Right, Up, and Down switches
	//	of the joystick.
    CLEAR_BIT(DDRB, 1); // left
    CLEAR_BIT(DDRB, 7); // down
    CLEAR_BIT(DDRD, 0); // right
    CLEAR_BIT(DDRD, 1); // up
    CLEAR_BIT(DDRB, 0); // centre

    // left and right buttons
    CLEAR_BIT(DDRF, 6); // left
    CLEAR_BIT(DDRF, 5); // right

	//	Initialise the LCD display using the default contrast setting.
    lcd_init(LCD_DEFAULT_CONTRAST);

    LCD_CMD(lcd_set_display_mode, lcd_display_inverse);
}

// 3
// This function displays the start page
void welcomePage() {
    clear_screen();
    draw_string(18, 0, "Abdulaziz", FG_COLOUR );
	draw_string(18, 8, "n10379614", FG_COLOUR );
	draw_string(23, 24, "Jerry's", FG_COLOUR );
	draw_string(18, 32, "Adventure", FG_COLOUR );
	draw_string(10, 40, "SW3 to start", FG_COLOUR );
    show_screen();
    setup_draw(); // adjust jerry
}
// Draw the status bar
void gameHeaderInformations() {
    draw_char(0,0, 'L', FG_COLOUR);
    draw_char(20,0, 'J', FG_COLOUR);
    draw_char(40,0, 'S', FG_COLOUR);

    draw_char(5,0, currentLevel + '0', FG_COLOUR);
    draw_char(25,0, jerryLives + '0', FG_COLOUR);
    draw_char(45,0, gameScore + '0', FG_COLOUR);

    draw_line(0, 8, LCD_X - 1, 8, FG_COLOUR);

    // store time to draw on the screen
    char buffer[32];

    //increase seconds every time counter increases by 1
    if (timerCounter%timerPrescaler == (timerPrescaler-1)){
        clock_second++;
    }
    //once seconds are 59, increase minutes
    if (clock_second > 59) {
        clock_second = 0;
        clock_minute++;
    }
    //fill buffer with characters
    sprintf(buffer, "%2.2d:%2.2d", clock_minute, clock_second);
    draw_string(59,0,buffer,FG_COLOUR);
    timerCounter++;
}

// draw 
void draw_data(int x, int y, uint8_t *shape) {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (BIT_VALUE(shape[j], i) == 1) {
                draw_pixel(x + j,y +i, FG_COLOUR);
            }
        }
    }
}

void drawWalls() {
    for (int i = 0; i < numOfWalls; i++) {
        draw_line(walls[i][0],walls[i][1],walls[i][2],walls[i][3], FG_COLOUR);
    }
}

// void moveTom() {
//     int tempX = rand() % 84;
//     int tempY = (rand() % 38) + 9; 
// }

void process_helper_drawing() {
    clear_screen();
    drawWalls();
    draw_data(jerryX, jerryY, jerryDirected);
    draw_data(tomX, tomY, tomDirected);
    //moveTom();
    gameHeaderInformations();
    show_screen();
}

//****************************************************//
// Starter functions                                  //
//****************************************************//
double CalcDistanceBetween2Points(int x1, int y1, int x2, int y2) {
    return SQRT((x1-x2), (y1-y2));
}

double PointLinesOnLine(int x, int y, int x1, int y1, int x2, int y2, double allowedDistanceDifference) {
    double dist1 = CalcDistanceBetween2Points(x, y, x1, y1);
    double dist2 = CalcDistanceBetween2Points(x, y, x2, y2);
    double dist3 = CalcDistanceBetween2Points(x1, y1, x2, y2);
    
    return abs(dist3 - (dist1 + dist2)) <= allowedDistanceDifference;
}


bool jerryCollisionDetection(int direction) { 
    // direction => left == 0, right == 1, up == 2, down == 3
    for (int k = 0; k < numOfWalls; k++) {
        if (direction == 0) { // left
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(jerryX, jerryY + i, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 1) { // right
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(jerryX + 4, jerryY + i, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 2) { // up
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(jerryX + i, jerryY, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 3) { // down
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(jerryX + i, jerryY + 4, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
    }
    return false;
}
bool tomCollisionDetection(int direction) { 
    // direction => left == 0, right == 1, up == 2, down == 3

    for (int k = 0; k < numOfWalls; k++) {
        if (direction == 0) { // left
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(jerryX, jerryY + i, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 1) { // right
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(jerryX + 4, jerryY + i, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 2) { // up
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(jerryX + i, jerryY, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 3) { // down
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(jerryX + i, jerryY + 4, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
    }
    return false;
}
//****************************************************//
//****************************************************//

// 4
void process() {
    // bool right = true, left = true, up = true, down = true;
    while (1) {
        if (BIT_IS_SET(PINB , 1) && jerryX > 0) { // x-- 
            if (!jerryCollisionDetection(0)) {
                jerryX--;
            }
        }
        if (BIT_IS_SET(PIND , 0) && jerryX < LCD_X - 5) { // x++ 
            if (!jerryCollisionDetection(1)) {
                jerryX++;
            }
        }
        if (BIT_IS_SET(PIND , 1) && jerryY > 9){ // y--
            if (!jerryCollisionDetection(2)) {
                jerryY--;
            }
        }
        if (BIT_IS_SET(PINB , 7) && jerryY < LCD_Y - 5 ) { // y++
            if (!jerryCollisionDetection(3)) {
                jerryY++;
            }
        }
        process_helper_drawing();
    }
}

// 1
int main(void) {
	setup();
    welcomePage();

	for ( ;; ) {
        if (BIT_IS_SET(PINF,5)) {
            
            process();
        }
		_delay_ms(100);
	}

	return 0;
} 