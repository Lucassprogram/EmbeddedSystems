#include <msp430.h>
#include <stdio.h>
#include "Junior_Design.h"
#include "HelpFunctions.h"

int main(void)

{

  WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
  // Go initialize the MSP430G2553 hardware to
  // setup I2C and the timers
  // Set the MCLk to 8 MHz and setup GPIO
  // for the ultrasonic sensor
  Init_HW();
  Setup_LCD();
  // Go initialize the LCD display
  // Mosfet / Blink
  P2DIR |= BIT3;
  P2OUT &= ~BIT3;
  while (1)
  {
    Duty = (Duty + 10) % 100;
    Send_LCD(0x40, Read_Compute_Distance());
    P2OUT ^= BIT3; //Blink Led
    Delay(DELAY_1s / 2);
  }
}












