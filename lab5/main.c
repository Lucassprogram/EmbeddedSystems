#include <msp430fr6989.h>
#define BUTTON_1 BIT1
#define BUTTON_2 BIT2
#define BUTTONS (BUTTON_1 | BUTTON_2)
#define DEBOUNCE_SAMPLES 3
#define LONG_PRESS_TICKS 100
#define REPEAT_TICKS 10

const unsigned char LCD_Shapes[10] = {
    0xFC, 0x60, 0xDB, 0xF3, 0x67, 0xB7, 0xBF, 0xE4, 0xFF, 0xF7
};
volatile unsigned long elapsed_seconds = 0;
volatile unsigned char running = 0;
static unsigned char released_ticks = 0;
static unsigned char button_down[2] = {0, 0};
static unsigned char samples[2] = {0, 0};

static unsigned int s1_hold = 0;
static unsigned char s1_consumed = 0;
static unsigned char repeat_ticks = 0;
static unsigned char rewind_mode = 0;

void Initialize_LCD(void);
void config_ACLK_to32Khz_crystal(void);
void update_display(void);
void adjust_time(unsigned char backwards);


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    P1DIR &= ~BUTTONS;
    P1REN |= BUTTONS;
    P1OUT |= BUTTONS;
    P1IES |= BUTTONS;
    P1IE &= ~BUTTONS;
    PM5CTL0 &= ~LOCKLPM5;
    config_ACLK_to32Khz_crystal();
    Initialize_LCD();
    update_display();

    // Advance the clock once per second.
    TA0CCR0 = 32767;
    TA0CCTL0 = CCIE;
    TA0CTL = TASSEL_1 | MC_1 | TACLR;
    // Button sampling runs only while a press/debounce is in progress.
    TA1CCR0 = 327;
    TA1CCTL0 = CCIE;
    TA1CTL = TASSEL_1 | MC_0 | TACLR;
    P1IFG &= ~BUTTONS;
    P1IE |= BUTTONS;
    for (;;) {
        __bis_SR_register(LPM3_bits | GIE);
        __no_operation();
    }
}

void update_display(void)
{
    unsigned int hours = elapsed_seconds / 3600UL;
    unsigned int minutes = (elapsed_seconds / 60UL) % 60;
    unsigned int seconds = elapsed_seconds % 60;
    LCDM10 = LCD_Shapes[hours / 10];
    LCDM6 = LCD_Shapes[hours % 10];
    LCDM4 = LCD_Shapes[minutes / 10];
    LCDM19 = LCD_Shapes[minutes % 10];
    LCDM15 = LCD_Shapes[seconds / 10];
    LCDM8 = LCD_Shapes[seconds % 10];
    // Hour/minute colon, minute/second decimal point, TMR and !.
    LCDM7 = (!running || (elapsed_seconds % 2 == 0)) ? BIT2 : 0;
    LCDM20 = BIT0;
    LCDM3 = running ? BIT3 : BIT0;
}

void adjust_time(unsigned char backwards)
{
    if (backwards) {
        elapsed_seconds = (elapsed_seconds + 86400UL - 60UL) % 86400UL;
    } else {
        elapsed_seconds = (elapsed_seconds + 60UL) % 86400UL;
    }
    update_display();
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Time_ISR(void)
{
    if (running) {
        elapsed_seconds = (elapsed_seconds + 1UL) % 86400UL;
        update_display();
    }
}

#pragma vector = PORT1_VECTOR
__interrupt void Buttons_ISR(void)
{
    P1IE &= ~BUTTONS;
    P1IFG &= ~BUTTONS;
    released_ticks = 0;
    TA1CTL = TASSEL_1 | MC_1 | TACLR;
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void Debounce_ISR(void)
{
    unsigned char i;
    unsigned char raw = (~P1IN) & BUTTONS;
    unsigned char pressed_events = 0;
    unsigned char released_events = 0;

    // Three matching samples accept a press or release.
    for (i = 0; i < 2; i++) {
        unsigned char mask = (i == 0) ? BUTTON_1 : BUTTON_2;
        unsigned char pressed = (raw & mask) != 0;
        if (pressed == button_down[i]) {
            samples[i] = 0;
        } else if (++samples[i] >= DEBOUNCE_SAMPLES) {
            button_down[i] = pressed;
            samples[i] = 0;
            if (pressed) {
                pressed_events |= mask;
            } else {
                released_events |= mask;
            }
        }
    }
    if (pressed_events & BUTTON_1) {
        s1_hold = 0;
        s1_consumed = 0;
    }
    if (button_down[1]) {
        unsigned char backwards = button_down[0] || (raw & BUTTON_1);
        if (backwards) {
            s1_consumed = 1; // The chord must not toggle/reset the timer.
        }
        if ((pressed_events & BUTTON_2) || backwards != rewind_mode) {
            adjust_time(backwards);
            repeat_ticks = 0;
        } else if (++repeat_ticks >= REPEAT_TICKS) {
            repeat_ticks = 0;
            adjust_time(backwards);
        }
        rewind_mode = backwards;
    } else {
        repeat_ticks = 0;
        rewind_mode = 0;
    }

    if (button_down[0] && !s1_consumed && !button_down[1]) {
        if (++s1_hold >= LONG_PRESS_TICKS) {
            elapsed_seconds = 0;
            running = 0;
            s1_consumed = 1;
            TA0CTL |= TACLR;
            TA0CCTL0 &= ~CCIFG;
            update_display();
        }
    }
    if ((released_events & BUTTON_1) && !s1_consumed) {
        running ^= 1;
        if (running) {
            TA0CTL |= TACLR;
            TA0CCTL0 &= ~CCIFG;
        }
        update_display();
    }
    // Return to button interrupts once both buttons are released.
    if (raw || button_down[0] || button_down[1]) {
        released_ticks = 0;
    } else if (++released_ticks >= DEBOUNCE_SAMPLES) {
        TA1CTL = TASSEL_1 | MC_0 | TACLR;
        P1IFG &= ~BUTTONS;
        P1IE |= BUTTONS;
    }
}

void config_ACLK_to32Khz_crystal(void)
{
    PJSEL1 &= ~(BIT4 | BIT5);
    PJSEL0 |= BIT4 | BIT5;
    CSCTL0_H = CSKEY >> 8;
    CSCTL4 &= ~LFXTOFF;
    CSCTL2 = (CSCTL2 & ~SELA_7) | SELA__LFXTCLK;
    do {
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);
    CSCTL0_H = 0;
}

void Initialize_LCD(void)
{
	PJSEL0 = BIT4 | BIT5;

	LCDCPCTL0 = 0xFFD0;
	LCDCPCTL1 = 0xF83F;
	LCDCPCTL2 = 0x00F8;

	//Configure LFXT 32kHz crystal
	CSCTL0_H = CSKEY >> 8;
	CSCTL4 &= ~LFXTOFF;
	do {
		CSCTL5 &= ~LFXTOFFG;
		SFRIFG1 &= ~OFIFG;
	}while (SFRIFG1 & OFIFG);
	CSCTL0_H = 0;

	// Initialize LCD_C
	// ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
	LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;
	// VLCD generated internally,
	// V2-V4 generated internally, v5 to ground
	// Set VLCD voltage to 2.60v
	// Enable charge pump and select internal reference for it
	LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;
	LCDCCPCTL = LCDCPCLKSYNC; // Clock synchronization enabled

	LCDCMEMCTL = LCDCLRM;

	//Turn LCD on 
	LCDCCTL0 |= LCDON;
}

