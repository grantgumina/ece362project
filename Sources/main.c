#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

// All funtions after main should be initialized here
void game();
void evolve();
// Note: inchar and outchar can be used for debugging purposes

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
int interuptFlag = 0;

int h = 10;
int w = 10;
int board[30][30];
int tempBoard[30][30];
int tickCounter = 0;

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
  //  Add RTI/interrupt initializations here

  // build the game board
}
	 		  
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
      //board[y][x] = rand() < RAND_MAX / 10 ? 1 : 0;
    }
  }
}
	 		  			 		  		
void main(void) {
	initializations(); 		  			 		  		
	EnableInterrupts;

  for(;;) {
    // check if left button has been pressed (game started)
    if (leftpb == 1) {
      leftpb = 0;      
      // stop counter 
      tickCounter = 0;
      // initialize game if it hasn't been started before...
      if (gameStarted == 0) {
        // make sure flag for starting game is set to 0
        gameStarted = 1;
        game();
      }
    }
    // check if right button has been pressed (game reset)
    if (rghtpb == 2) {
      // reset the game
      gameStarted = 0;
    }
    
    // display game board
    // evolve game board
    // delay animation
    
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