#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

// All funtions after main should be initialized here
void game();
void evolve();
void shiftOutRow(int r);
void shiftOutCol(int c);
void turnOnCol(int c);
void turnOnRow(int r);
void setDataBit(int bitIndex, int value);
void shiftOut();
void displayBoard();
void displayColumn(int column);
void shiftOutX();
void shiftOutY();
void setSPIDataOnes();

char inchar(void);
void outchar(char x);

//  Variable declarations  	   			 		  			 		       
int tenthsec = 0;               // One-tenth second flag
int leftpb = 0;                 // left pushbutton flag
int rghtpb = 0;                 // right pushbutton flag
int previousLeftButton    = 0;  // previous left push button state
int previousRightButton = 0;    // previous right push button state
int gameStarted = 0;            // toggle for starting the game                        
int rticnt = 0;                 // RTICNT (variable)
int prevpb = 0;                 // previous state of pushbuttons (variable)
int evolveFlag = 0;             
int displayBoardFlag = 0;
int interuptFlag = 0;
int TICKS_BETWEEN_EVOLUTIONS = 500;
int ticksSinceLastEvolution = 0;

int h = 27;                     // height of the board
int w = 21;                     // width of the board
char board[27][21];              // array representing conway's game of life board
char tempBoard[27][21];          // temperary game board for board evolution
int tickCounter = 0;            // timer variable
char SPIData[4];                // the data we will put on to the registers, LSB first
char yRegIndex;

/***********************************************************************
Initializations
***********************************************************************/
void  initializations(void) {
  // Set the PLL speed (bus clock = 24 MHz)
  CLKSEL = CLKSEL & 0x80; // disengage PLL from system
  PLLCTL = PLLCTL | 0x40; // turn on PLL
  SYNR = 0x02;            // set PLL multiplier
  REFDV = 0;              // set PLL divider
  while (!(CRGFLG & 0x08)){}
  CLKSEL = CLKSEL | 0x80; // engage PLL

  // Disable watchdog timer (COPCTL register)
  COPCTL = 0x40;          //COP off - RTI and COP stopped in BDM-mode
                    
  //;  < add TIM initialization code here >
  TSCR1 = 0x80;           //enable TC7
  TSCR2 = 0x0c;           //set TIM prescale factor to 16 and enable counter reset after OC7

  TIOS = 0x80;            //set TIM TC7 for Output Compare
  TIE = 0x80;             //enable TIM TC7 interrupt
  TC7 = 15000;            //set up TIM TC7 to generate 0.1 ms interrupt rate
  
  // Initialize asynchronous serial port (SCI) for 9600 baud, no interrupts
  SCIBDH =  0x00;         //set baud rate to 9600
  SCIBDL =  0x9C;         //24,000,000 / 16 / 156 = 9600 (approx)  
  SCICR1 =  0x00;         
  SCICR2 =  0x0C;         //initialize SCI for program-driven operation
  
  // Initialize the SPI to 6.25 MHz
  // TODO: Make sure is LSB first
  SPICR1 = 0b01011000;
  SPICR2 = 0;
  SPIBR = 1;
  
  //set up leds for output
  DDRT_DDRT0 = 1;
  DDRT_DDRT1 = 1;

  //  Initialize Port AD pins 7 and 6 for use as digital inputs
  DDRAD = 0; 		          //program port AD for input mode
  ATDDIEN = 0xC0;         //program PAD7 and PAD6 pins as digital inputs

  //  Add additional port pin initializations here  (e.g., Other DDRs, Ports) 
  // Initialize RTI for 2.048 ms interrupt rate  
  RTICTL = 0x1F;
  CRGINT = 0x80;
}

/* 
 * GAME FUNCTIONS
 */
void evolve() {
  int y;
  for (y = 0; y < h; y++) {
    int x;
    for (x = 0; x < w; x++) {
      int n = 0;
      int y1;
      for (y1 = y - 1; y1 <= y + 1; y1++) {
        int x1;
				for (x1 = x - 1; x1 <= x + 1; x1++) {
					if (board[(y1 + h) % h][(x1 + w) % w]) {
						n++;
					}
				}
			}
			
			if (board[y][x]) {
			  n--;
			}

      tempBoard[y][x] = (n == 3 || (n == 2 && board[y][x]));
    }
  }

  for (y = 0; y < h; y++) {
    int x;
    for (x = 0; x < w; x++) {
      board[y][x] = tempBoard[y][x];
    }
  }
}

void game() {
  int x;
  for (x = 0; x < w; x++) {
    int y;
    for (y = 0; y < h; y++) {
      board[y][x] = tickCounter % 2;
    }
  }
}

/*
 * SHIFTING/DISPLAY FUNCTIONS
 */
void turnOnRow(int r) {
  int x;
  for (x = 0; x < w; x++) {
    // turn on this LED
    board[r][x] = 1;
  }
}

void turnOnCol(int c) {
  // iterate through all 21 columns
  int x;
  for (x = 0; x < w; x++) {
    // turn on this LED
    board[c][x] = 1;
  }
}

void setDataBit(int bitIndex, int value) { 
  int desiredCharIndex = bitIndex / 8;
  int innerIndex = bitIndex % 8;
  char mask = 1 << innerIndex;
  if (value == 1) {
    SPIData[desiredCharIndex] |= mask;
  } else if (value == 0) {
    mask = ~mask;
    SPIData[desiredCharIndex] &= mask;
  }
}
 
void shiftOut() {
  int i;
  for (i=0; i < 4; i++ ) {
    // read the SPTEF bit, continue if bit is 1
    while(!SPISR_SPTEF);
    // write data to SPI data register
    SPIDR = SPIData[i];
  }
}

void displayBoard() {
  int i,x,y;
  for (i=0; i < w; i++) {
    displayColumn(i);
    
    // For testing, arbitrary delay
    x = 10000;
    while (x-->0) {
      y = 10000;
      while (y-->0){
      }
    }
    // End arbitrary delay
  }
}

void displayColumn(int column) {
  /* Set up X register */
  setSPIDataOnes();
  setDataBit(31 - column, 0);
  shiftOutX();
  
  /* Set up Y register */
  // yRegIndex had to be declared globally
  for (yRegIndex=0; yRegIndex < h; yRegIndex++) {
    setDataBit(31 - yRegIndex, board[column][yRegIndex]);   
  }
  shiftOutY();
}

void shiftOutX() {
  PTT_PTT7 = 1; // Left side is X register
  shiftOut();
}

void shiftOutY() {
  PTT_PTT7 = 0; // Right side is Y register
  shiftOut();
}

void setSPIDataOnes() {
  int i;
  for (i=0; i < 4; i++){
    SPIData[i] = 1; 
  }
}
	 		  			 		  		
void main(void) {
  DisableInterrupts;
	initializations(); 		  			 		  		
	EnableInterrupts;

  for(;;) {
    // check to see if user wants to start the game (presses left push button)
    if (leftpb == 1) {
      leftpb = 0;      
      // stop counter 
      tickCounter = 0;
      // initialize game if it hasn't been started before...
      if (gameStarted == 0) {
        // make sure flag for starting game is set to 0
        gameStarted = 1;
        //outchar('s'); // DEBUG
        game();
      }
    }
    
    // check to see if the user would like to reset the game (presses right push button)
    if (rghtpb == 1) {
      rghtpb = 0;
      // reset the game
      //outchar('r'); // DEBUG
      gameStarted = 0;
    }
    
    if (evolveFlag) {
      evolveFlag = 0;
      evolve(); 
    }
    
    if (displayBoardFlag) {
      displayBoardFlag = 0;
      displayBoard();  
    }
    
    _FEED_COP(); /* feeds the watchdog timer */
  } /* loop forever */
  /* make sure that you never leave main */
}

interrupt 7 void RTI_ISR(void)
{
  // set CRGFLG bit
  CRGFLG = CRGFLG | 0x80;
 
  if (previousLeftButton == 1) {
    if (PORTAD0_PTAD7 == 0) {
      leftpb = 1;
    }
  }
  previousLeftButton = PORTAD0_PTAD7;

  if (previousRightButton == 1) {
    if (PORTAD0_PTAD6 == 0) {
      rghtpb = 1;
    }
  }
  previousRightButton = PORTAD0_PTAD6;
}

interrupt 15 void TIM_ISR( void)
{
  // set TFLG1 bit
  TFLG1 = TFLG1 | 0x80; 
  interuptFlag = 1;
  tickCounter++;  	
  
  if (gameStarted == 1) {
    displayBoardFlag = 1;
    if (ticksSinceLastEvolution >= TICKS_BETWEEN_EVOLUTIONS) {
      ticksSinceLastEvolution = 0;
      evolveFlag = 1;  
    }
  }
}

char  inchar(void) {
  /* receives character from the terminal channel */
        while (!(SCISR1 & 0x20)); /* wait for input */
    return SCIDRL;
}
void outchar(char ch) {
  /* sends a character to the terminal channel */
    while (!(SCISR1 & 0x80));  /* wait for output buffer empty */
    SCIDRL = ch;
}