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

#include <stdlib.h>
// screen size 84 X 48  pixels
uint8_t jerry[8] = {
    0b00001111,
    0b00000110,
    0b00000110,
    0b00000110,
    0b00000110,
    0b11100110,
    0b11000110,
    0b01111000,
};
uint8_t jerryDirected[8];

int jerryX = 23;
int jerryY = 23;

// joystic
int joystick_l;
int joystick_r;
int joystick_d;
int joystick_u;

// Global variables
int currentLevel = 1;
int jerryLives = 5;
int gameScore = 0;

// int clock_millisecond=0;
int clock_second=0;
int clock_minute=0;
 
int timerCounter = 0; //counter for the timer
int timerPrescaler = 40; // speed of the counter

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
            uint8_t bit_val = BIT_VALUE(jerry[j], (7 - i));
            WRITE_BIT(jerryDirected[i], j, bit_val);
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
        clock_second=0;
        clock_minute++;
    }
    //fill buffer with characters
    sprintf(buffer, "%2.2d:%2.2d", clock_minute, clock_second);
    draw_string(59,0,buffer,FG_COLOUR);
    timerCounter++;
}

// draw 
void draw_data(int x, int y) {
    LCD_CMD(lcd_set_function, lcd_instr_basic | lcd_addr_horizontal);
    LCD_CMD(lcd_set_x_addr, x);
    LCD_CMD(lcd_set_y_addr, y/8);

    for (int i = 0; i < 8; i++) {
        unit8_t reg = (1 << directedJerry[i]);
        for (int j = 0; j < 8: j++) {
            unit8_t mask = (1 << j);
            if (((reg & mask) >> j) == 1) {
                draw_pixel
            }
        }
    }

    // for (int i = 0; i < 8; i++) {
    //     //lcd_write(LCD_D, jerryDirected[i]);

    //     //LCD_DATA(jerryDirected[i]);

    //     //LCD_DATA(screen_buffer[bank * LCD_X + col]);

    //     // uint8_t mask = (1 << jerryDirected[i]);
    //     // LCD_DATA(screen_buffer[(y/8) * LCD_X + x] ^ mask);
        
    // }
}


// 4
void process() {
    while (1) {
        clear_screen();
        draw_data(jerryX, jerryY);
        gameHeaderInformations();

        // test 
        int num = jerryY;
        char snum[10];
        // convert 123 to string [buf]
        itoa(num, snum, 10);    
        draw_string(0, 10, snum, FG_COLOUR);
        /////////

        draw_char(5, 30, 'x', FG_COLOUR);
        draw_char(10, 31, 'x', FG_COLOUR);
        draw_char(15, 32, 'x', FG_COLOUR);
        draw_char(20, 33, 'x', FG_COLOUR);
        
        if (BIT_IS_SET(PINB , 1)) { // x-- 
            jerryX--;
        }
        if (BIT_IS_SET(PIND , 0)) { // x++ 
            jerryX++;
        }

        if (BIT_IS_SET(PIND , 1)){ // y--
            jerryY--;
        }
        if (BIT_IS_SET(PINB , 7)) { // y++
            jerryY++;
        }

        show_screen();
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