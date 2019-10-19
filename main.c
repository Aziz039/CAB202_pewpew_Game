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

uint8_t jerryDirected[5];
uint8_t tomDirected[5];
uint8_t cheeseDirected[5];
uint8_t trapsDirected[5];
uint8_t doorDirected[5];

int jerryX = 0;
int jerryY = 9;

int tomX = LCD_X - 8; // 8 is bitmap size
int tomY = LCD_Y - 9; // 8 is bitmap size
int tomeSideDirection;

int cheeseLocation[5][2];
int trapsLocation[5][2];

int doorX;
int doorY;
int winningScore = 5;
bool doorSpawned = false;

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

int jerrySpeedPrescaler = 3; // speed of Jerry counter
int jerryTimerCounter = 0; //counter for Jerry timer

int tomSpeedPrescaler = 5; // speed of Tom counter
int tomTimerCounter = 0; //counter for Tom timer

bool gamePaused = false;

int maxNumOfCheese = 5;
int numOfCheeseOnGame = 0;
bool drawCheeseTimer = false;

int maxNumOfTraps = 5;
int numOfTrapsOnGame = 0;
bool drawTrapsTimer = false;

int fireworksLocation[20][2];
int numOfFireworks = 0;
int maxOfFireworks = 20;

// walls
int walls[][4] = {
    {18,15,13,25},
    {25,35,25,45},
    {45,10,60,10},
    {58,25,72,30}
}; 
int numOfWalls = 4;
double degree = 10;

void moveTom();
double CalcDistanceBetween2Points(int x1, int y1, int x2, int y2);
double PointLinesOnLine(int x, int y, int x1, int y1, int x2, int y2, double allowedDistanceDifference);
void openDoor();
bool checkFireworksPosition(int x,int y);
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
            uint8_t cheese_bit_val = BIT_VALUE(cheese[j], (4 - i));
            WRITE_BIT(cheeseDirected[i], j, cheese_bit_val);
            uint8_t traps_bit_val = BIT_VALUE(traps[j], (4 - i));
            WRITE_BIT(trapsDirected[i], j, traps_bit_val);
            uint8_t door_bit_val = BIT_VALUE(door[j], (4 - i));
            WRITE_BIT(doorDirected[i], j, door_bit_val);
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

    // Enable LEDS 0 and 1
	SET_BIT(DDRB, 2);
	SET_BIT(DDRB, 3);

	//	Initialise the LCD display using the default contrast setting.
    lcd_init(LCD_DEFAULT_CONTRAST);

    LCD_CMD(lcd_set_display_mode, lcd_display_inverse);

    adc_init();

    srand(time(NULL)); 
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
    setup_draw(); // adjust characters
}

void endPage() {
    clear_screen();
    draw_string(18, 16, "Game Over!", FG_COLOUR );
	draw_string(18, 32, "Press SW3", FG_COLOUR );
	draw_string(10, 40, "to restart..", FG_COLOUR );
    show_screen();
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
    jerryTimerCounter++;
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
bool rotateWall = false;
int counterForDrawingWalls;
void drawWalls() {
    for (int i = 0; i < numOfWalls; i++) {

        if ((clock_second + 1) % 2 == 0) {
            rotateWall = true;
        }
        if (clock_second % 2 == 0 && rotateWall) {
            int x1 = walls[i][0];
            int y1 = walls[i][1];
            int x2 = walls[i][2];
            int y2 = walls[i][3];
            double r = degree * 0.01745;
            int xx2 = (int) (x1 + ((x2-x1) * cos(r) - (y2-y1) * sin(r)));
            int xy2 = (int) (y1 + ((x2-x1) * sin(r) + (y2-y1) * cos(r)));
            walls[i][2] = xx2;
            walls[i][3] = xy2;
            counterForDrawingWalls++;
            if (counterForDrawingWalls >= numOfWalls) {
                rotateWall = false;
                counterForDrawingWalls = 0;
            }
        }
        draw_line(walls[i][0],walls[i][1],walls[i][2],walls[i][3], FG_COLOUR);
    }
}

bool overlapWithWalls(int x, int y) {
    // Check if cheese overlap with all walls
    for (int i = 0; i < numOfWalls; i++) {
        if (PointLinesOnLine(x, y, walls[i][0],walls[i][1],walls[i][2],walls[i][3], 10e-5) == 1 ||
        PointLinesOnLine(x + 4, y + 4, walls[i][0],walls[i][1],walls[i][2],walls[i][3], 10e-5) == 1 ||
        PointLinesOnLine(x, y + 4, walls[i][0],walls[i][1],walls[i][2],walls[i][3], 10e-5) == 1 ||
        PointLinesOnLine(x + 4, y, walls[i][0],walls[i][1],walls[i][2],walls[i][3], 10e-5) == 1
        ) {
            return true;
        }
    } 
    return false;
}

bool overlapWithCharacter(int x, int y) {
    // check if cheese overlap Tom
    if (PointLinesOnLine(x, y, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1 ||
        PointLinesOnLine(x + 4, y + 4, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1 ||
        PointLinesOnLine(x, y + 4, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1 ||
        PointLinesOnLine(x + 4, y, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1
        ) {
            return true;
    }

    // check if cheese overlap Jerry
    if (PointLinesOnLine(x, y, jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1 ||
        PointLinesOnLine(x + 4, y + 4, jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1 ||
        PointLinesOnLine(x, y + 4, jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1 ||
        PointLinesOnLine(x + 4, y, jerryX, jerryY ,jerryX + 4 , jerryY + 4, 10e-5) == 1
        ) {
            return true;
    }
    return false;
}

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

void adc_update(int left_adc, int right_adc) {
    if (right_adc == 0) {
        degree = -10;
    } else if (right_adc == 1) {
        degree = 0;
    } else if (right_adc == 2) {
        degree = 10;
    } else if (right_adc == 3) {
        degree = 20;
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

void process_helper_drawing() {
    //read both potentiometers
	int left_adc = adc_read(0) / 256; // 1024 / 4 = 256 => {0,1,2,3}
	int right_adc = adc_read(1) / 256;
    clear_screen();
    draw_char(10, 20, left_adc + '0', FG_COLOUR);
    adc_update(left_adc, right_adc);
    drawWalls();
    didJerryAteTheCheese();
    didJerryTraped();
    drawCheese();
    drawTraps();
    drawFireworks();
    openDoor();
    draw_data(jerryX, jerryY, jerryDirected);
    draw_data(tomX, tomY, tomDirected);
    moveTom();
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

bool characterWallCollision(int direction, int x, int y) {
    // direction => left == 0, right == 1, up == 2, down == 3
    for (int k = 0; k < numOfWalls; k++) {
        if (direction == 0) { // left
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(x, y + i, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 1) { // right
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(x + 4, y + i, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 2) { // up
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(x + i, y, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
        if (direction == 3) { // down
            for (int i = 0; i < 5; i++) {
                if (PointLinesOnLine(x + i, y + 4, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
                    return true;
                }
            }
        }
    }
    return false;
}

//****************************************************//
//****************************************************// 

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

void didTomCatchJerry() {
    if (PointLinesOnLine(jerryX, jerryY, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1 ||
        PointLinesOnLine(jerryX + 4, jerryY + 4, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1 ||
        PointLinesOnLine(jerryX, jerryY + 4, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1 ||
        PointLinesOnLine(jerryX + 4, jerryY, tomX, tomY ,tomX + 4 , tomY + 4, 10e-5) == 1
        ) {
        jerryLives--;
        jerryX = 0;
        jerryY = 9;
        tomX = LCD_X - 8;
        tomY = LCD_Y - 9;
    }
}

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
        currentLevel++;
        doorSpawned = false;
        gameScore = 0;
    }
}

void restartGame() {
    jerryX = 0;
    jerryY = 9;

    tomX = LCD_X - 8; // 8 is bitmap size
    tomY = LCD_Y - 9; // 8 is bitmap size

    winningScore = 5;
    doorSpawned = false;

    // Global variables
    currentLevel = 1;
    jerryLives = 5;
    gameScore = 0;

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
}

// check if fireworks overlap with walls
bool checkFireworksPosition(int x,int y) {
    for (int k = 0; k < numOfWalls; k++) {
        if (PointLinesOnLine(x, y, walls[k][0],walls[k][1],walls[k][2],walls[k][3], 10e-5) == 1) {
            return true;
        }
    }
    return false;
}

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

// 4
void process() {
    // bool right = true, left = true, up = true, down = true;
    while (1) {
        if (BIT_IS_SET(PINF,5)) { // right button 
            while(BIT_IS_SET(PINF,5)) {
            }
            gamePaused = !gamePaused; // toggle gamePaused
        }
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
        }
        if (BIT_IS_SET(PINB, 0)) { // centre
            while(BIT_IS_SET(PINB,0)) {
            }
            jerryShootFireworks();
        }
        if (jerryLives <= 0) {
            endPage();
            while (1) {
                if (BIT_IS_SET(PINF,5)) { // right button 
                    restartGame();
                    break;
                } 
            }
        }
        // openDoor();
        didTomCatchJerry();
        process_helper_drawing();
    }
}

// 1
int main(void) {
	setup(); // setup device and input
    welcomePage(); // show f
    tomeSideDirection = rand() % 4; // Tom first direction
    doorX = rand() % (LCD_X - 5);
    doorY = (rand() % (LCD_Y - 11)) + 8;
	for ( ;; ) {
        if (BIT_IS_SET(PINF,5)) {
            while (BIT_IS_SET(PINF,5)) {
            }
            // CLEAR_BIT(PINF,5);
            process();
        }
		_delay_ms(100);
	}

	return 0;
} 