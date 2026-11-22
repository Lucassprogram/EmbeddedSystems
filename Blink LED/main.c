#include <msp430fr6989.h>
#include <stdint.h>
#define redLED BIT0
#define greenLED BIT7

void main(void)
{

    volatile uint32_t i;
    volatile uint32_t j;

//    volatile unsigned int i;
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    P1DIR |= redLED;
    P1OUT &= ~redLED;
    P9DIR |= greenLED;
    P9OUT &= ~redLED;

    //Pattern #2
    for(;;)
    {
        P9OUT = 0;
        P1OUT = 0;
        //Initial pattern
        //Blink alternate
        for(i = 0; i < 10; i++)
        {
            P9OUT ^= greenLED;
            __delay_cycles(150000);
            P9OUT = 0;
            P1OUT ^= redLED;
            __delay_cycles(150000);
            P1OUT = 0;
        }
        //Speed up
        for(i = 0; i < 5; i++)
        {
            P9OUT ^= greenLED;
            __delay_cycles(100000);
            P9OUT = 0;
            P1OUT ^= redLED;
            __delay_cycles(100000);
            P1OUT = 0;
        }
        for(i = 0; i < 5; i++)
        {
            P9OUT ^= greenLED;
            __delay_cycles(50000);
            P9OUT = 0;
            P1OUT ^= redLED;
            __delay_cycles(50000);
            P1OUT = 0;
        }
        for(i = 0; i < 10; i++)
        {
            P9OUT ^= greenLED;
            __delay_cycles(5000);
            P9OUT = 0;
            P1OUT ^= redLED;
            __delay_cycles(5000);
            P1OUT = 0;
        }
        //Flash both
        for(i = 0; i < 10; i++)
        {
            P9OUT ^= greenLED;
            P1OUT ^= redLED;
            __delay_cycles(300000);
        }
    }


    //Pattern #1
    for(;;)
    {
        //Create intial pattern
        for(i = 0; i < 10; i++)
        {
            P9OUT ^= greenLED;
            __delay_cycles(50000);

        }
        P9OUT = 0;
        __delay_cycles(100000);
        for(i = 0; i < 10; i++)
        {
            P1OUT ^= redLED;
            __delay_cycles(50000);

        }
        P1OUT = 0;
        __delay_cycles(100000);
    }

    //Regular flashing
    for(;;)
    {
//        for(i = 0; i < 20000; i++)
//        {}
        P9OUT ^= greenLED;

    //    for(i = 0; i < 1200000; i++)
    //    {}
        P1OUT ^= redLED;
        __delay_cycles(3000000);

    }

}
