#include <msp430fr6989.h>
#define redLED BIT0
#define greenLED BIT7
#define BUTTON_1 BIT1
#define BUTTON_2 BIT2
// -3 = R+++, -2 = R++, -1 = R+, 0 = RG, 1 = G+, 2 = G++, 3 = G+++.
// Button 1 decreases the count; button 2 increases it.
volatile int correction = 0;
void config_ACLK_to32Khz_crystal(void);
void update_flashing(void);
void main(void)
{
    // Stop the watchdog timer.
    WDTCTL = WDTPW | WDTHOLD;
    // Configure the LEDs as GPIO outputs and start with both off.
    P1SEL0 &= ~(redLED | BUTTON_1 | BUTTON_2);
    P1SEL1 &= ~(redLED | BUTTON_1 | BUTTON_2);
    P9SEL0 &= ~greenLED;
    P9SEL1 &= ~greenLED;
    P1DIR |= redLED;
    P9DIR |= greenLED;
    P1OUT &= ~redLED;
    P9OUT &= ~greenLED;
    // Configure the buttons as inputs with pull-up resistors.
    P1DIR &= ~(BUTTON_1 | BUTTON_2);
    P1REN |= BUTTON_1 | BUTTON_2;
    P1OUT |= BUTTON_1 | BUTTON_2;
    // A falling edge indicates that a button was pressed.
    P1IES |= BUTTON_1 | BUTTON_2;
    // Unlock the GPIO pins before clearing any startup interrupt flags.
    PM5CTL0 &= ~LOCKLPM5;
    config_ACLK_to32Khz_crystal();
    P1IFG &= ~(BUTTON_1 | BUTTON_2);
    P1IE |= BUTTON_1 | BUTTON_2;
    // Use Timer_A CCR0 interrupts to flash the LEDs, starting in RG.
    TA0CCTL0 = CCIE;
    update_flashing();
    // Keep ACLK running for the timer while the CPU sleeps.
    for (;;)
    {
        __bis_SR_register(LPM3_bits | GIE);
    }
}
void update_flashing(void)
{
    unsigned int amount;
    // The magnitude determines speed; the sign determines which LED flashes.
    amount = (correction < 0) ? -correction : correction;
    // Stop and reset the timer so the new pattern starts with a full interval.
    TA0CTL = TASSEL_1 | ID_0 | MC_0 | TACLR;
    P1OUT &= ~redLED;
    P9OUT &= ~greenLED;
    // Halve the interval for each step: 16384, 8192, 4096, or 2048 counts.
    TA0CCR0 = (16384u >> amount) - 1;
    TA0CCTL0 &= ~CCIFG;
    // Count from zero to CCR0 repeatedly using the 32,768 Hz ACLK.
    TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;
}
#pragma vector = TIMER0_A0_VECTOR
__interrupt void T0A0_ISR(void)
{
    // CCR0's interrupt flag is cleared automatically when this ISR runs.
    // RG toggles both LEDs together; other states toggle only the needed LED.
    if (correction <= 0)
    {
        P1OUT ^= redLED;
    }
    if (correction >= 0)
    {
        P9OUT ^= greenLED;
    }
}
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void)
{
    unsigned int pressed;
    int previousState;
    // Capture the interrupt flags once; do not poll the button inputs.
    pressed = P1IFG & (BUTTON_1 | BUTTON_2);
    previousState = correction;
    // Move one adjacent state per press, stopping at either end.
    // If both buttons have pending flags, leave the state unchanged.
    if ((pressed == BUTTON_1) && (correction > -3))
    {
        correction--;
    }
    else if ((pressed == BUTTON_2) && (correction < 3))
    {
        correction++;
    }
    if (correction != previousState)
    {
        update_flashing();
    }
    // Wait 20 ms at 1 MHz, then discard flags caused by button bounce.
    // This is the only delay used; all LED flashing uses timer interrupts.
    __delay_cycles(20000);
    P1IFG &= ~(BUTTON_1 | BUTTON_2);
}
void config_ACLK_to32Khz_crystal(void)
{
    // Connect the crystal to PJ.4 (LFXIN) and PJ.5 (LFXOUT).
    PJSEL1 &= ~(BIT4 | BIT5);
    PJSEL0 |= BIT4 | BIT5;
    CSCTL0 = CSKEY;
    // Set MCLK to 1 MHz so the button debounce delay lasts 20 ms.
    CSCTL1 = DCOFSEL_0;
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;
    CSCTL4 &= ~LFXTOFF;
    // Wait for the crystal to stabilize; no button or timer flags are polled.
    do
    {
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;
    } while ((CSCTL5 & LFXTOFFG) != 0);
    CSCTL0_H = 0;
}
