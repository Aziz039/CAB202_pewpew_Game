/*
To be done:
# Add super power

# in void keyboardInputs(void):
	* viii.The number of cheese which has been consumed in the current room,
		add variable count cheese that reset each level
	* ix. A true or false value for whether Jerry is in super-mode,
		bool for super power
		
# pause mode
	* pause tom
		
# input walls for next level more than 4

# wall movement fix bugs
*/
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
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "cab202_adc.h"

#include <string.h>
#include <lcd_model.h>
#include "lcd.h"
#include "usb_serial.h"


// command to get serial port
// [System.IO.Ports.SerialPort]::getportnames()
// then open putty then choose serial 
// put port number and speed to 115200
//
// to send file  " cat room2.txt | plink -serial COM7 "

#define SQRT(x,y) sqrt(x*x + y*y)
// screen size 84 X 48  pixels

//****************************************************//
// character shapes                                   //
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

uint8_t cheese[5] = {
    0b01111,
    0b11000,
    0b10000,
    0b11000,
    0b01111
};

uint8_t traps[5] = {
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b01110
};

uint8_t door[5] = {
    0b11111,
    0b10001,
    0b11001,
    0b10001,
    0b11111
};

// adjusted shapes
uint8_t jerryDirected[5];
uint8_t tomDirected[5];
uint8_t cheeseDirected[5];
uint8_t trapsDirected[5];
uint8_t doorDirected[5];

//****************************************************//
//****************************************************//


//****************************************************//
// Global variables                                   //
//****************************************************//
// To & Jerry current location in the game
int jerryX;
int jerryY;
int tomX;
int tomY;
// direction => left == 0, right == 1, up == 2, down == 3
int tomeSideDirection;
// timers
int clock_second = 0; // seconds counter
int clock_minute = 0; // minutes counter
int timerCounter = 0; //counter for the game timer
int timerPrescaler = 40; // speed of the game counter
int jerrySpeedPrescaler = 3; // speed of Jerry counter
int jerryTimerCounter = 0; //counter for Jerry timer
int tomSpeedPrescaler = 5; // speed of Tom counter
int tomTimerCounter = 0; //counter for Tom timer
// cheese, traps and fireworks
int cheeseLocation[5][2];
int trapsLocation[5][2];
int maxNumOfCheese = 5;
int numOfCheeseOnGame = 0;
bool drawCheeseTimer = false;
int maxNumOfTraps = 5;
int numOfTrapsOnGame = 0;
bool drawTrapsTimer = false;
int fireworksLocation[20][2];
int numOfFireworks = 0;
int maxOfFireworks = 20;
// door variables
int doorX;
int doorY;
int winningScore = 5;
bool doorSpawned = false;
// joystic
int joystick_l;
int joystick_r;
int joystick_d;
int joystick_u;
// headers
int currentLevel = 1;
int jerryLives = 5;
int gameScore = 0;
bool gamePaused = false;
// helper variable to wall prescaller
int wallTimerPrescaler = 2;
bool drawWallCheck = false; 
int offsetPixels = 3;
/* struct to store level informations */
struct profile {
	int jerryInitialX;
    int jerryInitialY;
    int tomInitialX;
    int tomInitialY;
    int walls[6][4];
    int numOfWalls;
};
const int maxNumOfLevels = 10;
struct profile levels[10]; // max 10 levels
bool levelExist[10];
bool gameLoadded = false;
//****************************************************//
//****************************************************//

//****************************************************//
// Function Declarations                              //
//****************************************************//
// This function setup the device 
void setup( void );
// adjust the bitmap
void setup_draw(void);
// This function displays the start game page
void welcomePage();
// This function displays the end game page
void endPage();
// This function displays the bye page
void byePage();
// This function displays the enter file page
void enterNextLevelFilePage();
// Draw the status bar
void gameHeaderInformations();
// draw shapes
void draw_data(int x, int y, uint8_t *shape);
// draw walls
void drawWalls();
// draw cheese
void drawCheese();
// draw Traps
void drawTraps();
// draw fireworks
void drawFireworks();
// move walls
void moveWalls();
//helper function for walls cross edges
void wallsCrossScreenEdge();
// check if location overlap Characters (Tom & Jerry)
bool overlapWithCharacter(int x, int y);
// check if Jerry ate a cheese
void didJerryAteTheCheese();
// check if Jerry was trapped
void didJerryTraped();
// get inputs from adc
void adc_update(int left_adc, int right_adc);
// helper function tp call all drawing functions
void process_helper_drawing();
// helper function to check collisions
double CalcDistanceBetween2Points(int x1, int y1, int x2, int y2);
double PointLinesOnLine(int x, int y, int x1, int y1, int x2, int y2, double allowedDistanceDifference);
// check Characters collision with walls
bool characterWallCollision(int direction, int x, int y);
// update Tom location 
void moveTom();
// check if Jerry overlapped with Tom
void didTomCatchJerry();
// check characters spawned position
bool checkSpawnedPosition(int x, int y);
// Check if character overlap with any wall
bool overlapWithWalls(int x, int y);
// check if Jerry enter the door for next level
bool didJerryEnterTheDoor();
// draw door to go to next level
void openDoor();
void goToNextLevel();
// function to restart game variables
void restartGame();
// check if fireworks overlap with walls
bool checkFireworksPosition(int x,int y);
// Jerry shoot fireworks
void jerryShootFireworks();
// Go to next level
void nextLevel();
// get data from USB
void get_usb(void);
void usb_serial_send(char * message);
void usb_serial_read_string(char * message);
// take inputs from keyboard
void keyboardInputs(void);
// process function to run during game in a loop
void process();
// iniate first lavel
struct profile firstLevel();
// draw int
void draw_int(uint8_t x, uint8_t y, int value, colour_t colour);
//****************************************************//
//****************************************************//




//****************************************************//
// main function                                      //
//****************************************************//
//****************************************************//
//****************************************************//
int main(int argc, char *argv[]) {
    // setupFiles(argv, argc);
	setup(); // setup device and input
    welcomePage();
    for ( ;; ) {
        if (BIT_IS_SET(PINF,5)) {
            while (BIT_IS_SET(PINF,5)) {
            }
            process();
        }
		_delay_ms(100);
	}
	return 0;
} 

//****************************************************//
// setup functions                                    //
//****************************************************//
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

    // Enable LEDS 0 and 1
	SET_BIT(DDRB, 2);
	SET_BIT(DDRB, 3);

	//	Initialise the LCD display using the default contrast setting.
    lcd_init(LCD_DEFAULT_CONTRAST);
    LCD_CMD(lcd_set_display_mode, lcd_display_inverse);

    // iniate adc
    adc_init();

    //setup USB connection
	usb_init();
    while ( !usb_configured() ) {
		// Block until USB is ready.
	}
    
    srand(time(NULL)); // for random variables

    setup_draw(); // adjust characters

    // iniate level and postioins
    levels[0] = firstLevel();
    jerryX = levels[0].jerryInitialX;
    jerryY = levels[0].jerryInitialY;
    tomX = levels[0].tomInitialX;
    tomY = levels[0].tomInitialY;
    for (int i = 0; i < maxNumOfLevels; i++) {
        levelExist[i] = false;
    }
    levelExist[0] = true;
    tomeSideDirection = rand() % 4; // Tom first direction
}

// This function displays the start page
void welcomePage() {
    clear_screen();
    draw_string(18, 0, "Abdulaziz", FG_COLOUR );
	draw_string(18, 8, "n10379614", FG_COLOUR );
	draw_string(23, 24, "Jerry's", FG_COLOUR );
	draw_string(18, 32, "Adventure", FG_COLOUR );
	draw_string(10, 40, "SW3 to start", FG_COLOUR );
    show_screen();
}

// This function displays the end game page
void endPage() {
    clear_screen();
    draw_string(18, 16, "Game Over!", FG_COLOUR );
	draw_string(18, 32, "Press SW3", FG_COLOUR );
	draw_string(10, 40, "to restart..", FG_COLOUR );
    show_screen();
}

// This function displays the end game page
void byePage() {
    clear_screen();
    draw_string(30, 16, "Bye!", FG_COLOUR );
    show_screen();
}

// This function displays the enter file page
void enterNextLevelFilePage() {
    clear_screen();
    draw_string(10, 0, "Enter a next", FG_COLOUR );
	draw_string(10, 8, "level file..", FG_COLOUR );
    draw_string(30, 16, "or", FG_COLOUR );
    draw_string(10, 32, "SW3 to rstart..", FG_COLOUR );
	draw_string(10, 40, "SW2 to exit..", FG_COLOUR );
    show_screen();
}

// This function adjust the bitmaps 
void setup_draw(void) {
    // Visit each column of output bitmap
    for (int i = 0; i < 5; i++) {
        // Visit each row of output bitmap
        for (int j = 0; j < 5; j++) {
            // Flip about the major diagonal.
            uint8_t jerry_bit_val = BIT_VALUE(jerry[j], (4 - i));
            WRITE_BIT(jerryDirected[i], j, jerry_bit_val);
            uint8_t tom_bit_val = BIT_VALUE(tom[j], (4 - i));
            WRITE_BIT(tomDirected[i], j, tom_bit_val);
            uint8_t cheese_bit_val = BIT_VALUE(cheese[j], (4 - i));
            WRITE_BIT(cheeseDirected[i], j, cheese_bit_val);
            uint8_t traps_bit_val = BIT_VALUE(traps[j], (4 - i));
            WRITE_BIT(trapsDirected[i], j, traps_bit_val);
            uint8_t door_bit_val = BIT_VALUE(door[j], (4 - i));
            WRITE_BIT(doorDirected[i], j, door_bit_val);
        }
    }

}
struct profile firstLevel() {
	struct profile tmpLevel;
    tmpLevel.jerryInitialX = 0;
    tmpLevel.jerryInitialY = 9;
    tmpLevel.tomInitialX = LCD_X - 8;
    tmpLevel.tomInitialY = LCD_Y - 9;
    tmpLevel.numOfWalls = 4;
    int walls[4][4] = {
    {18,15,13,25},
    {25,35,25,45},
    {45,10,60,10},
    {58,25,72,30}
    }; 
    for (int i = 0; i < tmpLevel.numOfWalls; i++) {
        for (int j = 0; j < 4; j++) {
            tmpLevel.walls[i][j] = walls[i][j];
        }
    }
	return tmpLevel;
}
//****************************************************//
//****************************************************//


//****************************************************//
// process functions                                  //
//****************************************************//
void process() {
    while (1) {
        // handle keyboard inputs
        keyboardInputs();
        // pause game
        if (BIT_IS_SET(PINF,5)) { // right button 
            while(BIT_IS_SET(PINF,5)) {
            }
            gamePaused = !gamePaused; // toggle gamePaused
        }
        // go to next level
        if (BIT_IS_SET(PINF,6)) { // left button 
            while(BIT_IS_SET(PINF,6)) {
            }
            goToNextLevel();
            gameLoadded = false;
        }

        // joystick movement
        if (jerryTimerCounter%jerrySpeedPrescaler == (jerrySpeedPrescaler-1)) {
            if (BIT_IS_SET(PINB , 1) && jerryX > 0) { // x-- 
                if (!characterWallCollision(0, jerryX, jerryY)) {
                    jerryX--;
                }
            }
            if (BIT_IS_SET(PIND , 0) && jerryX < LCD_X - 5) { // x++ 
                if (!characterWallCollision(1, jerryX, jerryY)) {
                    jerryX++;
                }
            }
            if (BIT_IS_SET(PIND , 1) && jerryY > 9){ // y--
                if (!characterWallCollision(2, jerryX, jerryY)) {
                    jerryY--;
                }
            }
            if (BIT_IS_SET(PINB , 7) && jerryY < LCD_Y - 5 ) { // y++
                if (!characterWallCollision(3, jerryX, jerryY)) {
                    jerryY++;
                }
            }
        } // end of joystick

        // shoot fireworks
        if (BIT_IS_SET(PINB, 0)) { // centre
            while(BIT_IS_SET(PINB,0)) {
            }
            jerryShootFireworks();
        }

        // check if jerry has lives remaining
        if (jerryLives <= 0) {
            endPage();
            while (1) {
                if (BIT_IS_SET(PINF,5)) { // right button 
                    while(BIT_IS_SET(PINF,5)) {
                    }
                    restartGame();
                    break;
                } 
                if (BIT_IS_SET(PINF,6)) { // left button 
                    while(BIT_IS_SET(PINF,6)) {
                    }
                    clear_screen();
                    byePage();
                    show_screen();
                    exit(0);
                }
            }
        }
        didTomCatchJerry();
        process_helper_drawing(); // done
    }
}

// helper function for process to call drawing functions
void process_helper_drawing() {
    clear_screen();

    //read both potentiometers
    // 1024 / 4 = 256 => {0,1,2,3}
	int left_adc = adc_read(0) / 256; 
	int right_adc = adc_read(1) / 256;
    adc_update(left_adc, right_adc);

    // draw game assets
    drawWalls();
    drawCheese();
    drawTraps();
    drawFireworks();

    // game functionality
    didJerryAteTheCheese();
    didJerryTraped();

    // move Tom
    moveTom();

    // check if door should be opened 
    openDoor();

    // draw characters
    draw_data(jerryX, jerryY, jerryDirected);
    draw_data(tomX, tomY, tomDirected);

    // status bar
    gameHeaderInformations(); 
    show_screen();
}

// Draw the status bar
void gameHeaderInformations() {
    draw_char(0,0, 'L', FG_COLOUR); // level
    draw_char(20,0, 'J', FG_COLOUR); // Jerry lives
    draw_char(40,0, 'S', FG_COLOUR); // game score

    draw_int(5,0, currentLevel, FG_COLOUR);
    draw_int(25,0, jerryLives, FG_COLOUR);
    draw_int(45,0, gameScore, FG_COLOUR);

    draw_line(0, 8, LCD_X - 1, 8, FG_COLOUR);

    // store time to draw on the screen
    char buffer[32];
    if (!gamePaused) {
        //increase seconds every time counter increases by 1
        if (timerCounter%timerPrescaler == (timerPrescaler-1)){
            clock_second++;
        }
        //once seconds are 59, increase minutes
        if (clock_second > 59) {
            clock_second = 0;
            clock_minute++;
        }
        timerCounter++;
    }
    //fill buffer with characters
    sprintf(buffer, "%2.2d:%2.2d", clock_minute, clock_second);
    draw_string(59,0,buffer,FG_COLOUR);
    tomTimerCounter++; // increase Tom timer
    jerryTimerCounter++; // increase Tom timer
}
//****************************************************//
//****************************************************//




//****************************************************//
// Drawing Functions                                  //
//****************************************************//
// draw bitmap
void draw_data(int x, int y, uint8_t *shape) {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (BIT_VALUE(shape[j], i) == 1) {
                draw_pixel(x + j,y +i, FG_COLOUR);
            }
        }
    }
}
// draw int
void draw_int(uint8_t x, uint8_t y, int value, colour_t colour) {
	char buffer[20];
	snprintf(buffer, sizeof(buffer), "%d", value);
	draw_string(x, y, buffer, colour);
}

// draw walls
void drawWalls() {
    if ((clock_second + 1) % wallTimerPrescaler == 0 ) {
        drawWallCheck = true;
    }
    if (clock_second % wallTimerPrescaler == 0 && drawWallCheck) {
        moveWalls();
        drawWallCheck = false;
    }
    for (int i = 0; i < levels[currentLevel - 1].numOfWalls; i++) {
        int x1 = levels[currentLevel - 1].walls[i][0];
        int y1 = levels[currentLevel - 1].walls[i][1];
        int x2 = levels[currentLevel - 1].walls[i][2];
        int y2 = levels[currentLevel - 1].walls[i][3];
        draw_line(x1, y1, x2, y2, FG_COLOUR);
    }
}

// draw cheese
void drawCheese() {
    if ((clock_second + 1) % 2 == 0) {
        drawCheeseTimer = true;
    }
    if (clock_second % 2 == 0 && drawCheeseTimer) {
        if (numOfCheeseOnGame < maxNumOfCheese) { // check if number of cheese less than maximum
            int x = rand() % 84; // create random x variable for cheese location
            int y = (rand() % 40) + 8; // create randomm y variable for cheese location
            if (checkSpawnedPosition(x, y)) { // check if cheese overlapp any other object
                cheeseLocation[numOfCheeseOnGame][0] = x;
                cheeseLocation[numOfCheeseOnGame][1] = y;
                numOfCheeseOnGame++;   
                drawCheeseTimer = false;
            }
        }
    }
    for (int i = 0; i < numOfCheeseOnGame; i++) {
        draw_data(cheeseLocation[i][0], cheeseLocation[i][1], cheeseDirected);
    }
}

// draw traps
void drawTraps() {
    if ((clock_second + 1) % 3 == 0) {
        drawTrapsTimer = true;
    }
    if (clock_second % 3 == 0 && drawTrapsTimer) {
        if (numOfTrapsOnGame < maxNumOfTraps) { // check if number of cheese less than maximum
            int x;
            int y;
            if (tomeSideDirection == 0) {
                x = tomX + 5; 
                y = tomY; 
            } else if (tomeSideDirection == 1) {
                x = tomX - 5; 
                y = tomY; 
            } else if (tomeSideDirection == 2) {
                x = tomX; 
                y = tomY + 5; 
            } else {
                x = tomX; 
                y = tomY - 5; 
            }
            if (checkSpawnedPosition(x, y)) { // check if cheese overlapp any other object
                trapsLocation[numOfTrapsOnGame][0] = x;
                trapsLocation[numOfTrapsOnGame][1] = y;
                numOfTrapsOnGame++;   
                drawTrapsTimer = false;
            }
        }
    }
    for (int i = 0; i < numOfTrapsOnGame; i++) {
        draw_data(trapsLocation[i][0], trapsLocation[i][1], trapsDirected);
    }
}

// draw fireworks
void drawFireworks() {
    for (int i = 0; i < numOfFireworks; i++) {
        if (fireworksLocation[i][0] < tomX) {
            fireworksLocation[i][0]++;
        } else if (fireworksLocation[i][0] > tomX) {
            fireworksLocation[i][0]--;
        } else {
            if (fireworksLocation[i][1] < tomY) {
                fireworksLocation[i][1]++;
            } else if (fireworksLocation[i][1] > tomY) {
                fireworksLocation[i][1]--;
            } else {
                // fireworks cathced Jerry
                tomX = LCD_X - 8; // 8 is bitmap size
                tomY = LCD_Y - 9; // 8 is bitmap size
                fireworksLocation[i][0] = fireworksLocation[numOfFireworks - 1][0];
                fireworksLocation[i][1] = fireworksLocation[numOfFireworks - 1][1];
                numOfFireworks--;
            }
        }
        wallsCrossScreenEdge();
        // check collision with walls
        if (!checkFireworksPosition(fireworksLocation[i][0], fireworksLocation[i][1])) {
            draw_pixel(fireworksLocation[i][0], fireworksLocation[i][1], FG_COLOUR);
        } else {
            fireworksLocation[i][0] = fireworksLocation[numOfFireworks - 1][0];
            fireworksLocation[i][1] = fireworksLocation[numOfFireworks - 1][1];
            numOfFireworks--;
        }
    }
}
//****************************************************//
//****************************************************//




//****************************************************//
// Drawing helpers                                    //
//****************************************************//
// move walls perpendicularly
void moveWalls() {
    for (int i = 0; i < levels[currentLevel - 1].numOfWalls; i++) {
        int x1 = levels[currentLevel - 1].walls[i][0];
        int y1 = levels[currentLevel - 1].walls[i][1];
        int x2 = levels[currentLevel - 1].walls[i][2];
        int y2 = levels[currentLevel - 1].walls[i][3];
        
        double L = sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
        int x1p = x1 + offsetPixels * (y2-y1) / L;
        int x2p = x2 + offsetPixels * (y2-y1) / L;
        int y1p = y1 + offsetPixels * (x1-x2) / L;
        int y2p = y2 + offsetPixels * (x1-x2) / L;
        levels[currentLevel - 1].walls[i][0] = x1p;
        levels[currentLevel - 1].walls[i][1] = y1p;
        levels[currentLevel - 1].walls[i][2] = x2p;
        levels[currentLevel - 1].walls[i][3] = y2p;
    }
}

// draw line on the opposite edge of screeen
void wallsCrossScreenEdge() {
    for (int i = 0; i < levels[currentLevel - 1].numOfWalls; i++) {
        int x1 = levels[currentLevel - 1].walls[i][0];
        int y1 = levels[currentLevel - 1].walls[i][1];
        int x2 = levels[currentLevel - 1].walls[i][2];
        int y2 = levels[currentLevel - 1].walls[i][3];

        // y = mx + b
        double m = (y2 - y1) / (x2 - x1);
        double b = y1 - (m * x1);

        if (y1 == y2) {// ------- going up
            if (y1 <= 8) {
                levels[currentLevel - 1].walls[i][1] = LCD_Y - 1;
                levels[currentLevel - 1].walls[i][3] = LCD_Y - 1;
            }
        } else if (x1 == x2) { // | going right
            if (x1 > (LCD_X - 1)) {
                levels[currentLevel - 1].walls[i][0] = 0;
                levels[currentLevel - 1].walls[i][2] = 0;
            }
        } else if (x1 > x2) { //   /  going right down
            if (x1 > LCD_X - 1) {
                int x3 = LCD_X - 1;
                int y3 = (m * x3) + b;
                int xDiff = x1 - x3;
                if (x2 > LCD_X - 1) {
                    levels[currentLevel - 1].walls[i][0] = xDiff; 
                    levels[currentLevel - 1].walls[i][1] = y1;
                    levels[currentLevel - 1].walls[i][2] = 1;
                    levels[currentLevel - 1].walls[i][3] = y2;
                } else {
                    // d = sqrt( (x2-x1)^2 + (y2-y1)^2 )
                    draw_line(0, y3, xDiff, y1, FG_COLOUR);
                }
            }
            if (y2 > LCD_Y - 1) {
                if (y1 > LCD_Y - 1) {
                    levels[currentLevel - 1].walls[i][1] = 9;
                    levels[currentLevel - 1].walls[i][3] = (y2 - y1) + 9;
                } else {
                    int y3 = LCD_Y - 1;
                    int x3 = (y3 - b) / m;
                    draw_line(x3, 9, x2, (y2 - y3), FG_COLOUR);
                }
            }

        } else if (x1 < x2) { //   \ going right up
            // double slope = (y2 - y1) / (x2 - x1);
            if (x2 > LCD_X - 1) {
                if (x1 > LCD_X - 1) {
                    // reset
                    levels[currentLevel - 1].walls[i][0] = 0;
                    levels[currentLevel - 1].walls[i][2] = x2 - x1;
                } else {
                    int x3 = LCD_X - 1;
                    int y3 = (m * x3) + b;
                    int y4 = y3;
                    int x5 = x2 - x3;
                    int y5 = y2;
                    draw_line(1, y4, x5, y5, FG_COLOUR);
                }
            }
            if (y1 < 9) {
                levels[currentLevel - 1].walls[i][1] = (LCD_Y - 1) - (y2 - y1);
                levels[currentLevel - 1].walls[i][3] = LCD_Y - 1;
            } else {
                int y3 = 8;
                int x3 = (y3 - b) / m;
                int x4 = x3;
                int y4 = LCD_Y - 1;
                int x5 = x1;
                int y5 = (LCD_Y - 1) - (y3 - y1);
                draw_line(x5, y5, x4, y4, FG_COLOUR);
            }
        }
    }
}

// check characters spawned position
bool checkSpawnedPosition(int x, int y) {
    // check if cheese overlapp any other object
    if (x >= 0 && x < LCD_X - 4 && y > 8 && y < LCD_Y - 4) {
        if (!overlapWithWalls(x, y)) { // check if overlap with wall
            //if (!overlapWithCharacter(x,y)) { // check if overlap with character
                return true;
            //}
        }
    }
    return false;
}

// Check if character overlap with any wall
bool overlapWithWalls(int x, int y) {
    for (int k = 0; k < levels[currentLevel - 1].numOfWalls; k++) {
        if (PointLinesOnLine(x, y, levels[currentLevel - 1].walls[k][0],
                levels[currentLevel - 1].walls[k][1],levels[currentLevel - 1].walls[k][2],
                levels[currentLevel - 1].walls[k][3], 10e-5) == 1 ||
        PointLinesOnLine(x + 4, y + 4, levels[currentLevel - 1].walls[k][0],
                levels[currentLevel - 1].walls[k][1],levels[currentLevel - 1].walls[k][2],
                levels[currentLevel - 1].walls[k][3], 10e-5) == 1 ||
        PointLinesOnLine(x, y + 4, levels[currentLevel - 1].walls[k][0],
                levels[currentLevel - 1].walls[k][1],levels[currentLevel - 1].walls[k][2],
                levels[currentLevel - 1].walls[k][3], 10e-5) == 1 ||
        PointLinesOnLine(x + 4, y, levels[currentLevel - 1].walls[k][0],
                levels[currentLevel - 1].walls[k][1],levels[currentLevel - 1].walls[k][2],
                levels[currentLevel - 1].walls[k][3], 10e-5) == 1) {
            return true;
        }
    } 
    return false;
}
//****************************************************//
//****************************************************//




//****************************************************//
// Game functions                                     //
//****************************************************//
void restartGame() {
    levels[currentLevel - 1] = firstLevel();
    // gane variables
    currentLevel = 1;
    jerryLives = 5;
    gameScore = 0;

    jerryX = levels[currentLevel - 1].jerryInitialX;
    jerryY = levels[currentLevel - 1].jerryInitialY;

    tomX = levels[currentLevel - 1].tomInitialX; // 8 is bitmap size
    tomY = levels[currentLevel - 1].tomInitialY; // 8 is bitmap size

    winningScore = 5;
    doorSpawned = false;
    
    // reset timers
    clock_second= 0;
    clock_minute= 0;
    timerCounter = 0; //counter for the timer
    tomSpeedPrescaler = 5; // speed of Tom counter
    tomTimerCounter = 0; //counter for Tom timer

    gamePaused = false;

    numOfCheeseOnGame = 0;
    drawCheeseTimer = false;

    numOfTrapsOnGame = 0;
    drawTrapsTimer = false;

    numOfFireworks = 0;
}

// shoot fireworks
void jerryShootFireworks() {
    if (gameScore >= 3) {
        if (numOfFireworks < maxOfFireworks) {
            int x, y;
            if (!checkFireworksPosition(jerryX + 5, jerryY + 2) && jerryX < LCD_X - 2) { // shoot to the right
                x = jerryX + 5;
                y = jerryY + 2;
            } else if (!checkFireworksPosition(jerryX - 1, jerryY + 2) && jerryX > 1) { // shoot to the left
                x = jerryX - 1;
                y = jerryY + 2;
            } else if (!checkFireworksPosition(jerryX + 2, jerryY - 1) && jerryY > 9) { // shoot up
                x = jerryX + 2;
                y = jerryY - 1;
            } else { // shoot down
                x = jerryX + 2;
                y = jerryY + 5;
            }
            // add firework to array 
            fireworksLocation[numOfFireworks][0] = x;
            fireworksLocation[numOfFireworks][1] = y;
            numOfFireworks++;
        }
    }
}

// Check if Jerry ate a cheese
void didJerryAteTheCheese() {
    for (int i = 0; i < numOfCheeseOnGame; i++) {
        if (PointLinesOnLine(cheeseLocation[i][0], cheeseLocation[i][1], jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1 ||
        PointLinesOnLine(cheeseLocation[i][0] + 4, cheeseLocation[i][1] + 4, jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1 ||
        PointLinesOnLine(cheeseLocation[i][0], cheeseLocation[i][1] + 4, jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1 ||
        PointLinesOnLine(cheeseLocation[i][0] + 4, cheeseLocation[i][1], jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1
        ) {
            cheeseLocation[i][0] = cheeseLocation[numOfCheeseOnGame - 1][0];
            cheeseLocation[i][1] = cheeseLocation[numOfCheeseOnGame - 1][1];
            numOfCheeseOnGame--;
            gameScore++;
        }
    }
}

// check if Jerry was trapped
void didJerryTraped() {
    for (int i = 0; i < numOfTrapsOnGame; i++) {
        if (PointLinesOnLine(trapsLocation[i][0], trapsLocation[i][1], jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1 ||
        PointLinesOnLine(trapsLocation[i][0] + 4, trapsLocation[i][1] + 4, jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1 ||
        PointLinesOnLine(trapsLocation[i][0], trapsLocation[i][1] + 4, jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1 ||
        PointLinesOnLine(trapsLocation[i][0] + 4, trapsLocation[i][1], jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1) {
            trapsLocation[i][0] = trapsLocation[numOfTrapsOnGame - 1][0];
            trapsLocation[i][1] = trapsLocation[numOfTrapsOnGame - 1][1];
            numOfTrapsOnGame--;
            jerryLives--;
        }
    }
}

// Check if Jerry overlapped with Tom
void didTomCatchJerry() {
    if (PointLinesOnLine(jerryX, jerryY, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1 ||
        PointLinesOnLine(jerryX + 4, jerryY + 4, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1 ||
        PointLinesOnLine(jerryX, jerryY + 4, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1 ||
        PointLinesOnLine(jerryX + 4, jerryY, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1
        ) {
        jerryLives--;
        jerryX = levels[currentLevel - 1].jerryInitialX;
        jerryY = levels[currentLevel - 1].jerryInitialY;
        tomX = levels[currentLevel - 1].tomInitialX;
        tomY = levels[currentLevel - 1].tomInitialY;
    }
}

// check if door should be open
void openDoor() {
    if (gameScore >= winningScore) {
        if (checkSpawnedPosition(doorX, doorY)) {
            draw_data(doorX, doorY, doorDirected);
            doorSpawned = true;
        } else if (!checkSpawnedPosition(doorX, doorY) && !doorSpawned) {
            doorX = rand() % (LCD_X - 5);
            doorY = (rand() % (LCD_Y - 11)) + 8;
        }
    }
    if (didJerryEnterTheDoor() && doorSpawned) {
        goToNextLevel();
    }
}

// check if Jery entered the door
bool didJerryEnterTheDoor() {
    if (PointLinesOnLine(jerryX, jerryY, doorX, doorY ,doorX + 4 , doorY + 4, 10e-5) == 1 ||
    PointLinesOnLine(jerryX + 4, jerryY + 4, doorX, doorY ,doorX + 4 , doorY + 4, 10e-5) == 1 ||
    PointLinesOnLine(jerryX, jerryY + 4, doorX, doorY ,doorX + 4 , doorY + 4, 10e-5) == 1 ||
    PointLinesOnLine(jerryX + 4, jerryY, doorX, doorY ,doorX + 4 , doorY + 4, 10e-5) == 1
    ) {
        return true;
    }
    return false;
}
//****************************************************//
//****************************************************//

//****************************************************//
// Collision functions                                //
//****************************************************//

// Character collision with walls
bool characterWallCollision(int direction, int x, int y) {
    // direction => left == 0, right == 1, up == 2, down == 3
    for (int k = 0; k < levels[currentLevel - 1].numOfWalls; k++) {
        if (direction == 0) { // left
            for (int i = 0; i < 5; i++) { 
                if (PointLinesOnLine(x, y + i, levels[currentLevel - 1].walls[k][0],
                levels[currentLevel - 1].walls[k][1],levels[currentLevel - 1].walls[k][2],
                levels[currentLevel - 1].walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 1) { // right
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(x + 4, y + i, levels[currentLevel - 1].walls[k][0],
                levels[currentLevel - 1].walls[k][1],levels[currentLevel - 1].walls[k][2],
                levels[currentLevel - 1].walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 2) { // up
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(x + i, y, levels[currentLevel - 1].walls[k][0],
                levels[currentLevel - 1].walls[k][1],levels[currentLevel - 1].walls[k][2],
                levels[currentLevel - 1].walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 3) { // down
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(x + i, y + 4, levels[currentLevel - 1].walls[k][0],
                levels[currentLevel - 1].walls[k][1],levels[currentLevel - 1].walls[k][2],
                levels[currentLevel - 1].walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
    }
    return false;
}

// check fireworks collision with walls
bool checkFireworksPosition(int x,int y) {
    for (int k = 0; k < levels[currentLevel - 1].numOfWalls; k++) {
        if (PointLinesOnLine(x, y, levels[currentLevel - 1].walls[k][0],
                levels[currentLevel - 1].walls[k][1],levels[currentLevel - 1].walls[k][2],
                levels[currentLevel - 1].walls[k][3], 10e-5) == 1) {
            return true;
        }
    }
    return false;
}
//****************************************************//
//****************************************************//

//****************************************************//
// Helper collision functions                         //
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
//****************************************************//
//****************************************************//


//****************************************************//
// Control functions                                  //
//****************************************************//

// move Tom
void moveTom() {
    // sideDirection => left == 0, right == 1, up == 2, down == 3
    if (tomTimerCounter%tomSpeedPrescaler == (tomSpeedPrescaler-1)) {
        
        if (tomeSideDirection == 0) { // left
            if (!characterWallCollision(0, tomX, tomY) && tomX > 0) {
                tomX--;
            }
            else {
                tomeSideDirection = (rand() % 3) + 1;
                tomSpeedPrescaler = (rand() % 3) + 4;
            }
        } else if (tomeSideDirection == 1) { // right
            if (!characterWallCollision(1, tomX, tomY) && tomX < LCD_X - 5) {
                tomX++;
            }
            else {
                tomeSideDirection = (rand() % 3); 
                tomSpeedPrescaler = (rand() % 3) + 4;
                if (tomeSideDirection == 1) {
                    tomeSideDirection = 3;
                }
            }
        } else if (tomeSideDirection == 2) { // up
            if (!characterWallCollision(2, tomX, tomY) && tomY > 9 ) {
                tomY--;
            }
            else {
                tomeSideDirection = (rand() % 3) + 1; 
                tomSpeedPrescaler = (rand() % 3) + 4;
                if (tomeSideDirection == 2) {
                    tomeSideDirection = 3;
                }
            }
        } else if (tomeSideDirection == 3) { // down
            if (!characterWallCollision(3, tomX, tomY) && tomY < LCD_Y - 5) {
                tomY++;
            }
            else {
                tomeSideDirection = (rand() % 3); 
                tomSpeedPrescaler = (rand() % 3) + 4;
            }
        }
    }
    
}

// adc control function
void adc_update(int left_adc, int right_adc) {
    if (right_adc == 0) {
        // change to wall prescaller
        // degree = -10;
        wallTimerPrescaler = 2;
        offsetPixels = -3;
    } else if (right_adc == 1) {
        // degree = 0;
        wallTimerPrescaler = 4;
        offsetPixels = -3;
    } else if (right_adc == 2) {
        // degree = 10;
        wallTimerPrescaler = 4;
        offsetPixels = 3;
    } else if (right_adc == 3) {
        // degree = 20;
        wallTimerPrescaler = 2; 
        offsetPixels = 3;
    }
    if (left_adc == 0) {
        jerrySpeedPrescaler = 4;
        tomSpeedPrescaler = 7;
    } else if (left_adc == 1) {
        jerrySpeedPrescaler = 3;
        tomSpeedPrescaler = 6;
    } else if (left_adc == 2) {
        jerrySpeedPrescaler = 2;
        tomSpeedPrescaler = 5;
    } else if (left_adc == 3) {
        jerrySpeedPrescaler = 1;
        tomSpeedPrescaler = 4;
    }
    
}

// take inputs from keyboard
void keyboardInputs(void) {
    if (usb_serial_available()) {
        char tx_buffer[32];
        int c = usb_serial_getchar(); //read usb port
        /*
        i. The keys ‘w’, ‘a’, ‘s’, and ‘d’ can be used to
            move Jerry up, left, down and right,respectively,
        ii. The ‘p’ key toggles pause mode,
        iii. The ‘f’ key fires a firework (if available), and
        iv. The ‘l’ key changes the level.
        v. The ‘i’ key request information to be sent to computer: 
            i. A timestamp,
            ii. The current level number,
            iii. Jerry’s lives,
            iv. The score,
            v. The number of fireworks on screen,
            vi. The number of mousetraps on screen,
            vii. The number of cheese on screen,
            viii.The number of cheese which has been consumed in the current room,
            ix. A true or false value for whether Jerry is in super-mode,
            x. A true or false value for whether the game is paused.
        */
        if (c =='w') { // up
            snprintf( tx_buffer, sizeof(tx_buffer), "received '%c'\r\n", c );
            usb_serial_send( tx_buffer );  
            if (!characterWallCollision(2, jerryX, jerryY) && jerryY > 9) {
                    jerryY--;
            } 
        }
        if (c =='s') { // down
            snprintf( tx_buffer, sizeof(tx_buffer), "received '%c'\r\n", c );
            usb_serial_send( tx_buffer );
            if (!characterWallCollision(3, jerryX, jerryY) && jerryY < LCD_Y - 5) {
                    jerryY++;
            }  
        }
        if (c =='a') { // left
            snprintf( tx_buffer, sizeof(tx_buffer), "received '%c'\r\n", c );
            usb_serial_send( tx_buffer );
            if (!characterWallCollision(0, jerryX, jerryY) && jerryX > 0) {
                jerryX--;
            }  
        }
        if (c =='d') { // right
            snprintf( tx_buffer, sizeof(tx_buffer), "received '%c'\r\n", c );
            usb_serial_send( tx_buffer );   
            if (!characterWallCollision(1, jerryX, jerryY) && jerryX < LCD_X - 5) {
                jerryX++;
            }
        }
        if (c =='p') { // pause game
            snprintf( tx_buffer, sizeof(tx_buffer), "received '%c'\r\n", c );
            usb_serial_send( tx_buffer );  
            if (!characterWallCollision(2, jerryX, jerryY) && jerryY > 9) {
                    jerryY--;
            } 
        }  
        if (c =='f') { // fires a firework
            snprintf( tx_buffer, sizeof(tx_buffer), "received '%c'\r\n", c );
            usb_serial_send( tx_buffer );
            if (!characterWallCollision(3, jerryX, jerryY) && jerryY < LCD_Y - 5) {
                    jerryY++;
            }  
        }
        if (c =='l') { // next level
            snprintf( tx_buffer, sizeof(tx_buffer), "received '%c'\r\n", c );
            usb_serial_send( tx_buffer );
            if (!characterWallCollision(0, jerryX, jerryY) && jerryX > 0) {
                jerryX--;
            }  
        }
        if (c =='i') { // information request
            // i. A timestamp,
            char timestamp_buffer[32];
            snprintf( timestamp_buffer, sizeof(timestamp_buffer), "Timestamp '%2.2d:%2.2d' \r\n", clock_minute, clock_second );
            usb_serial_send( timestamp_buffer );
            // ii. The current level number,
            char currentLevel_buffer[32];
            snprintf( currentLevel_buffer, sizeof(currentLevel_buffer), "current level '%d'\r\n", currentLevel );
            usb_serial_send( currentLevel_buffer );
            // iii. Jerry’s lives,
            char JerryLives_buffer[32];
            snprintf( JerryLives_buffer, sizeof(JerryLives_buffer), "Jerry lives '%d'\r\n", jerryLives );
            usb_serial_send( JerryLives_buffer );
            // iv. The score,
            char gameScore_buffer[32];
            snprintf( gameScore_buffer, sizeof(gameScore_buffer), "game score '%d'\r\n", gameScore );
            usb_serial_send( gameScore_buffer );
            // v. The number of fireworks on screen,
            char numOfFireworks_buffer[32];
            snprintf( numOfFireworks_buffer, sizeof(numOfFireworks_buffer), "fireworks on screen '%d'\r\n", numOfFireworks );
            usb_serial_send( numOfFireworks_buffer );
            // vi. The number of mousetraps on screen,
            char nmousetraps_buffer[32];
            snprintf( nmousetraps_buffer, sizeof(nmousetraps_buffer), "nmousetraps '%d'\r\n", numOfTrapsOnGame );
            usb_serial_send( nmousetraps_buffer );
            // vii. The number of cheese on screen,
            char cheese_buffer[32];
            snprintf( cheese_buffer, sizeof(cheese_buffer), "cheese '%d'\r\n", numOfCheeseOnGame );
            usb_serial_send( cheese_buffer );
            // viii need modification
            // viii.The number of cheese which has been consumed in the current room,
            char cheeseConsumed_buffer[32];
            snprintf( cheeseConsumed_buffer, sizeof(cheeseConsumed_buffer), "cheese consumed '%d'\r\n", gameScore );
            usb_serial_send( cheeseConsumed_buffer );
            // ix need modification
            // ix. A true or false value for whether Jerry is in super-mode,
            char isSuper_buffer[32];
            if (gamePaused) {
                snprintf( isSuper_buffer, sizeof(isSuper_buffer), "Jerry super 'True' \r\n");
            } else {
                snprintf( isSuper_buffer, sizeof(isSuper_buffer), "Jerry super 'False' \r\n");
            }
            usb_serial_send( isSuper_buffer );
            // x. A true or false value for whether the game is paused.
            char gameStatus_buffer[32];
            if (gamePaused) {
                snprintf( gameStatus_buffer, sizeof(gameStatus_buffer), "game paused 'True' \r\n");
            } else {
                snprintf( gameStatus_buffer, sizeof(gameStatus_buffer), "game paused 'False' \r\n");
            }
            usb_serial_send( gameStatus_buffer );
        }
    }
}
//****************************************************//
//****************************************************//




//****************************************************//
// File handler functions                             //
//****************************************************//

// Check if there is next level or exit or restart game
void goToNextLevel() {
    currentLevel++;
    while (!gameLoadded) {
        enterNextLevelFilePage();
        // sw3 => restart
        if (BIT_IS_SET(PINF,5)) { // right button 
            while(BIT_IS_SET(PINF,5)) {
            }
            restartGame();
            break;
        } 
        // sw2 => exitGame
        if (BIT_IS_SET(PINF,6)) { // left button 
            while(BIT_IS_SET(PINF,6)) {
            }
            clear_screen();
            byePage();
            show_screen();
            exit(0);
        }

        if (levelExist[currentLevel - 1]) {
            // there is a next level
            // starting game
            // delay
            clear_screen();
            draw_string(18, 8,"Loading game..", FG_COLOUR);
            show_screen();
            _delay_ms(300);
            gameLoadded = true;
            break;

        } else {
            // there is no next level
            get_usb();
            if (levels[currentLevel - 1].numOfWalls > 3) {
                levelExist[currentLevel - 1] = true;
            }
        }
    }
}

void get_usb(void) {

    if (usb_serial_available()) {
        char tx_buffer[32];
        
        int c = usb_serial_getchar(); //read usb port

        if (c =='T'){ //
            usb_serial_read_string(tx_buffer);
            usb_serial_send( tx_buffer );
            sscanf( tx_buffer, "%d %d", &levels[currentLevel - 1].tomInitialX,
                    &levels[currentLevel - 1].tomInitialY);
            
        }
        if (c =='J'){ //
            usb_serial_read_string(tx_buffer);
            usb_serial_send( tx_buffer );
            sscanf( tx_buffer, "%d %d", &levels[currentLevel - 1].jerryInitialX
            , &levels[currentLevel - 1].jerryInitialY);      
        }  
        //things to check here. Variable wall_num should be less than MAX_WALLS 
        if (c =='W') {
            char walls[64];
            usb_serial_read_string(walls);
            usb_serial_send( walls );
            int wallCounter = levels[currentLevel - 1].numOfWalls; 
            sscanf( walls, "%d %d %d %d", &levels[currentLevel - 1].walls[wallCounter][0],
            &levels[currentLevel - 1].walls[wallCounter][1], 
            &levels[currentLevel - 1].walls[wallCounter][2],
            &levels[currentLevel - 1].walls[wallCounter][3]);
            levels[currentLevel - 1].numOfWalls++;
        }
    }
}

void usb_serial_send(char * message) {
	// Cast to avoid "error: pointer targets in passing argument 1 
	//	of 'usb_serial_write' differ in signedness"
	usb_serial_write((uint8_t *) message, strlen(message));
}

void usb_serial_read_string(char * message) {
    int c = 0;
    int buffer_count= 0;   
    while (c != '\n'){
        c = usb_serial_getchar();
        message[buffer_count] = c;
        buffer_count++;
    }
}
//****************************************************//
//****************************************************//