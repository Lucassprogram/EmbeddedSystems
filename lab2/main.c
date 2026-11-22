#include <msp430fr6989.h>

#define RED_LED   BIT0
#define GREEN_LED BIT7
#define BUTTON_1  BIT1
#define BUTTON_2  BIT2

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    P1DIR |= RED_LED;
    P9DIR |= GREEN_LED;
    P1OUT &= ~RED_LED;
    P9OUT &= ~GREEN_LED;

    P1DIR &= ~(BUTTON_1 | BUTTON_2);
    P1REN |= BUTTON_1 | BUTTON_2;
    P1OUT |= BUTTON_1 | BUTTON_2;

    for (;;)
    {
        // The pull-ups make a pressed button read as zero. 
        if ((P1IN & BUTTON_2) == 0 && (P1IN & BUTTON_1) != 0 &&
            (P1OUT & RED_LED) == 0)
            P9OUT |= GREEN_LED;
        else if ((P1IN & BUTTON_2) != 0)
            P9OUT &= ~GREEN_LED;

        if ((P1IN & BUTTON_1) == 0 && (P1IN & BUTTON_2) != 0 &&
            (P9OUT & GREEN_LED) == 0)
            P1OUT |= RED_LED;
        else if ((P1IN & BUTTON_1) != 0)
            P1OUT &= ~RED_LED;
    }
} 
