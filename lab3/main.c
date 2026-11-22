#include <msp430fr6989.h> 
#define redLED BIT0 
#define greenLED BIT7
#define BUTTON_1  BIT1 
#define BUTTON_2  BIT2
  
// void config_ACLK_to32Khz_crystal(); 
  
// void flashLED(unsigned int clocks, unsigned int ledPin); 
  
// unsigned int get_Clocks(unsigned int button);

// void main(void) 
// { 
//     unsigned int clocks;

//     // Stop the Watchdog timer 
//     WDTCTL = WDTPW | WDTHOLD; 
//     PM5CTL0 &= ~LOCKLPM5; 
//     // Unlock the GPIO pins 
//     P1DIR |= redLED; 
//     P9DIR |= greenLED;
//     // Configure the LEDs as output 
//     P1OUT &= ~redLED; 
//     P9OUT &= ~greenLED;
//     // Configure buttons
//     P1DIR &= ~(BUTTON_1 | BUTTON_2);
//     P1REN |= (BUTTON_1 | BUTTON_2);
//     P1OUT |= (BUTTON_1 | BUTTON_2);
//     // Configure ACLK to the 32 KHz crystal (function call) 
//     config_ACLK_to32Khz_crystal(); 
//     // Select ACLK and divide by 1, but leave the timer stopped (MC_0).
//     // This saves power while the program is waiting for a pulse.
//     TA0CTL = TASSEL_1 | ID_0 | MC_0 | TACLR;
//     TA0CTL &= ~TAIFG; 
//     //Main loop 
//     for(;;) 
//     { 
// 		clocks = get_Clocks(BUTTON_1);

//         if(clocks == 0)
//         {
//             // Zero means the 16-bit timer overflowed, so show an error.
//             P9OUT |= greenLED;

//             // Stay in the error state until S2 is physically pressed.
//             while((P1IN & BUTTON_2) != 0){}
//             P9OUT &= ~greenLED;

//             // Wait for S2 to be released before accepting another pulse.
//             while((P1IN & BUTTON_2) == 0){}
//         }
//         else
//         {
//             // Replay the pulse for the same number of ACLK cycles.
//             flashLED(clocks, redLED);
//         }
//     } 
// } 

// void config_ACLK_to32Khz_crystal() 

// { 
//     // The 32 KHz crystal uses both PJ.4 (LFXIN) and PJ.5 (LFXOUT).
//     PJSEL1 &= ~(BIT4 | BIT5); 
//     PJSEL0 |= (BIT4 | BIT5); 
//     CSCTL0 = CSKEY; //Unlock cs reg 
//     do 
//     { 
//         CSCTL5 &= ~LFXTOFFG; //local fault flag 
//         SFRIFG1 &= ~OFIFG; //Global fault flag 
//     }while((CSCTL5 & LFXTOFFG) != 0); 
//     CSCTL0_H = 0; 
//     return; 
// } 

  

// void flashLED(unsigned int clocks, unsigned int ledPin) 
// {
//     // Keep the timer stopped while setting the replay duration.
//     TA0CTL = TASSEL_1 | ID_0 | MC_0 | TACLR;
//     TA0CCR0 = clocks; 
//     TA0CCTL0 &= ~CCIFG;

//     // Turn the output pulse on, then start Timer_A in up mode.
//     P1OUT |= ledPin;
//     TA0CTL |= MC_1;

//     // CCIFG is set when the timer reaches TA0CCR0. This uses the timer
//     // instead of a software delay loop and supports up to 65,535 cycles.
//     while((TA0CCTL0 & CCIFG) == 0){}

//     P1OUT &= ~ledPin;
//     // Stop and clear the timer when replay is finished.
//     TA0CTL = TASSEL_1 | ID_0 | MC_0 | TACLR;
//     TA0CCTL0 &= ~CCIFG;
// }

// unsigned int get_Clocks(unsigned int button)
// {
//     unsigned int clocks;

//     // Wait for the active-low input pulse. Timer_A stays stopped here.
//     while((P1IN & button) != 0){}

//     // The pulse has started, so clear the timer and run it continuously.
//     // Continuous mode allows the full 16-bit range to be measured.
//     TA0CTL = TASSEL_1 | ID_0 | MC_2 | TACLR;
//     TA0CTL &= ~TAIFG;

//     // Measure while S1 is held down (the input remains low).
//     while((P1IN & button) == 0)
//     {
//         // TAIFG means the timer passed 65,535 and the pulse is too long.
//         if((TA0CTL & TAIFG) != 0)
//         {
//             TA0CTL = TASSEL_1 | ID_0 | MC_0 | TACLR;
//             return 0;
//         }
//     }

//     // Check once more in case release and overflow happened together.
//     if((TA0CTL & TAIFG) != 0)
//     {
//         TA0CTL = TASSEL_1 | ID_0 | MC_0 | TACLR;
//         return 0;
//     }

//     clocks = TA0R;
//     // Stop Timer_A after sampling to save power.
//     TA0CTL = TASSEL_1 | ID_0 | MC_0 | TACLR;
//     return clocks;
// }

void main(void) 

{ 

    // Stop the Watchdog timer 

    WDTCTL = WDTPW | WDTHOLD; 

 

    PM5CTL0 &= ~LOCKLPM5; 

 

    // Unlock the GPIO pins 

    P1DIR |= redLED; 

    P9DIR |= greenLED; 

 

    // Configure the LEDs as output 

    P1OUT &= ~redLED; 

    P9OUT &= ~greenLED; 

 

    // Configure ACLK to the 32 KHz crystal (function call) 

    config_ACLK_to32KHz_crystal(); 

 

    // Set timer period 

    TA0CCR0 = 327; 

 

    // Configure Timer_A 

    TA0CTL |= TASSEL_1 | ID_0 | MC_1 | TACLR; 

 

    // Use ACLK, divide by 1, continuous mode, clear TA 

    TA0CTL &= ~TAIFG; 

 

    // Main loop 

    for (;;) 

    { 

        while ((TA0CTL & TAIFG) != TAIFG) 

        { 

        } 

 

        P1OUT ^= redLED; 

        TA0CTL &= ~TAIFG; 

    } 

} 