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
// uint8_t tom[8] = {
//     0b11111111,
//     0b11111111,
//     0b00011000,
//     0b00011000,
//     0b00011000,
//     0b00011000,
//     0b00011000,
//     0b00011000,
// };

// uint8_t jerry[8] = {
//     0b00001111,
//     0b00000110,
//     0b00000110,
//     0b00000110,
//     0b00000110,
//     0b11100110,
//     0b11000110,
//     0b01111000,
// };
uint8_t tom[8] = {
    0b11111110,
    0b00010000,
    0b00010000,
    0b00010000,
    0b00010000,
    0b00010000,
    0b00010000,
    0b00010000,
};

uint8_t jerry[8] = {
    0b00011111,
    0b00000100,
    0b00000100,
    0b00000100,
    0b00000100,
    0b11100100,
    0b01000100,
    0b01111000,
};
uint8_t jerryDirected[8];
uint8_t tomDirected[8];

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
    for (int i = 0; i < 8; i++) {
        // Visit each row of output bitmap
        for (int j = 0; j < 8; j++) {
            // Kind of like: cactus_direct[i][j] = cactus_original[j][7-i].
            // Flip about the major diagonal.
            uint8_t jerry_bit_val = BIT_VALUE(jerry[j], (7 - i));
            WRITE_BIT(jerryDirected[i], j, jerry_bit_val);
            uint8_t tom_bit_val = BIT_VALUE(tom[j], (7 - i));
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
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
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

void process_helper_drawing() {
    clear_screen();
    drawWalls();
    draw_data(jerryX, jerryY, jerryDirected);
    draw_data(tomX, tomY, tomDirected);
    gameHeaderInformations();
    show_screen();
}

double CalcDistanceBetween2Points(int x1, int y1, int x2, int y2) {
    return SQRT((x1-x2), (y1-y2));
}

double PointLinesOnLine(int x, int y, int x1, int y1, int x2, int y2, double allowedDistanceDifference) {
    double dist1 = CalcDistanceBetween2Points(x, y, x1, y1);
    double dist2 = CalcDistanceBetween2Points(x, y, x2, y2);
    double dist3 = CalcDistanceBetween2Points(x1, y1, x2, y2);
    
    return abs(dist3 - (dist1 + dist2)) <= allowedDistanceDifference;
}


bool tomCollisionDetection() {
    //double IsCol = PointLinesOnLine (xc, yc, x1+dx1, y1+dy1, x2+dx1, y2+dy1, 10e-5);
    for (int i = 0; i < numOfWalls; i++) {
        for (int i = 0; i < 8; i++) {
            // Visit each row of output bitmap
            for (int j = 0; j < 8; j++) {
                // Flip about the major diagonal.
                uint8_t bit_val = BIT_VALUE(tom[j], (7 - i));
                if (bit_val == 1) {
                    double IsCol = PointLinesOnLine (tomX + j, tomY + i, walls[i][0],walls[i][1],walls[i][2],walls[i][3], 10e-5);
                    if (IsCol == 1) {
                        return true;
                    }
                }
            }
        }  
    }
    return false;
}

bool jerryCollisionDetection() {
    //double IsCol = PointLinesOnLine (xc, yc, x1+dx1, y1+dy1, x2+dx1, y2+dy1, 10e-5);
    for (int k = 0; k < numOfWalls; k++) {
        for (int i = 0; i < 8; i++) {
            // Visit each row of output bitmap
            for (int j = 0; j < 8; j++) {
                // Flip about the major diagonal.
                uint8_t bit_val = BIT_VALUE(jerry[j], (7 - i));
                if (bit_val == 1) {
                    double IsCol = PointLinesOnLine(jerryX + j, jerryY + i, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5);
                    // // test 
                    // char output[50];
                    // snprintf(output, 50, "%f", IsCol);
                    // draw_string(50,10,output, FG_COLOUR);
                    if (IsCol == 1) {
                        return true;
                    }
                }
            }
        }  
    }
    return false;
}

void collisionDetection() {

}
// 4
void process() {
    bool right = true, left = true, up = true, down = true;
    while (1) {
        if (BIT_IS_SET(PINB , 1) && jerryX > 0) { // x-- 
            if (!jerryCollisionDetection()) {
                jerryX--;
                right = true;
            } else if (right == false) {
                jerryX--;
                right = true;
            }
            else {
                left = false;
            }
            
        }
        if (BIT_IS_SET(PIND , 0) && jerryX < LCD_X - 8) { // x++ 
            if (!jerryCollisionDetection()) {
                jerryX++;
                left = true;
            } else if (left == false) {
                jerryX++;
                left = true;
            }
            else {
                right = false;
            }
        }
        if (BIT_IS_SET(PIND , 1) && jerryY > 9){ // y--
            if (!jerryCollisionDetection()) {
                jerryY--;
                down = true;
            } else if (down == false) {
                jerryY--;
                down = true;
            }
            else {
                up = false;
            }
        }
        if (BIT_IS_SET(PINB , 7) && jerryY < LCD_Y - 8 ) { // y++
            if (!jerryCollisionDetection()) {
                jerryY++;
                up = true;
            } else if (up == false) {
                jerryX++;
                up = true;
            }
            else {
                down = false;
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