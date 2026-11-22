#include <msp430.h> 

void uart_write_char(unsigned char ch);
unsigned char uart_read_char(void);
void uart_write_number(unsigned long n);
void uart_write_string(char *str);
void Initialize_ADC();
void Initialize_UART();
void Initialize_Buzzer(void);
void Buzzer_Set(unsigned char on);
void Display_Platform(const unsigned char height[4], unsigned char selected,
                      unsigned char adjust_mode, unsigned char delta);
void Print_Corner(unsigned char value, unsigned char selected);

#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer
#define redLED BIT0
#define greenLED BIT7
/**
 * main.c
 */
int main(void)
{
    unsigned char height[4] = {127, 127, 127, 127};
    unsigned char selected = 0;  // 0=top-left, 1=top-right, 2=bottom-left, 3=bottom-right
    unsigned char adjust_mode = 0;
    unsigned char was_pressed = 0;
    unsigned char adjustment_armed = 0;
    unsigned char display_tick = 0;

    WDTCTL = WDTPW | WDTHOLD;
    Initialize_ADC();
    Initialize_UART();

    P1DIR |= BIT0;              // LaunchPad red LED
    P1OUT &= ~BIT0;

    P3SEL0 &= ~BIT2;            // Joystick pushbutton on P3.2
    P3SEL1 &= ~BIT2;
    P3DIR &= ~BIT2;
    P3REN |= BIT2;              // Pull up: pressed reads 0
    P3OUT |= BIT2;

    Initialize_Buzzer();
    uart_write_string("\x1b[2J\x1b[H");  // Clear the terminal once

    while (1)
    {
        unsigned int x, y;
        unsigned char pressed, i, minimum, maximum, delta;

        ADC12CTL0 |= ADC12SC;
        while (ADC12CTL1 & ADC12BUSY) { }
        x = ADC12MEM0;
        y = ADC12MEM1;

        pressed = ((P3IN & BIT2) == 0);

        if (pressed && !was_pressed)
        {
            adjust_mode = !adjust_mode;  // Button switches between cursor and height control
            adjustment_armed = 0;
            display_tick = 0;
        }
        was_pressed = pressed;

        if (!adjust_mode)
        {
            // Horizontal chooses left/right; vertical chooses top/bottom.
            if (x < 1200) selected &= ~1;
            else if (x > 2900) selected |= 1;
            if (y < 1200) selected &= ~2;
            else if (y > 2900) selected |= 2;
        }
        else
        {
            // Return to center before adjusting, so selection does not change height.
            if (y > 1600 && y < 2500) adjustment_armed = 1;
            if (adjustment_armed && !pressed)
            {
                if (y > 2900 && height[selected] < 255)
                    height[selected]++;
                else if (y < 1200 && height[selected] > 0)
                    height[selected]--;
            }
        }

        minimum = maximum = height[0];
        for (i = 1; i < 4; i++)
        {
            if (height[i] < minimum) minimum = height[i];
            if (height[i] > maximum) maximum = height[i];
        }
        delta = maximum - minimum;

        Buzzer_Set(delta > 10);

        // Redraw the same terminal screen about every half second.
        if (display_tick == 0)
            Display_Platform(height, selected, adjust_mode, delta);
        display_tick++;
        if (display_tick >= 5) display_tick = 0;

        P1OUT ^= BIT0;
        __delay_cycles(100000);  // About 0.1 s at 1 MHz
    }
}

void Print_Corner(unsigned char value, unsigned char selected)
{
    uart_write_char(selected ? '*' : ' ');
    uart_write_char('0' + value / 100);
    uart_write_char('0' + (value / 10) % 10);
    uart_write_char('0' + value % 10);
}

void Display_Platform(const unsigned char height[4], unsigned char selected,
                      unsigned char adjust_mode, unsigned char delta)
{
    uart_write_string("\x1b[H");  // Move cursor to top-left; overwrite the last frame
    uart_write_string("*** PLATFORM BALANCING CONTROL ***\r\n\r\n");
    uart_write_string("        TOP\r\n");
    uart_write_string("    .................\r\n");
    uart_write_string("    |");
    Print_Corner(height[0], selected == 0);
    uart_write_string("      ");
    Print_Corner(height[1], selected == 1);
    uart_write_string(" |\r\n");
    uart_write_string("   L|               |R\r\n");
    uart_write_string("   E|               |I\r\n");
    if (delta > 10)
        uart_write_string("   F|     DANGER    |G\r\n");
    else
        uart_write_string("   F|    BALANCED   |G\r\n");
    uart_write_string("   T|               |H\r\n");
    uart_write_string("    |               |T\r\n");
    uart_write_string("    |               | \r\n");
    uart_write_string("    |");
    Print_Corner(height[2], selected == 2);
    uart_write_string("      ");
    Print_Corner(height[3], selected == 3);
    uart_write_string(" |\r\n");
    uart_write_string("    .................\r\n");
    uart_write_string("        BOTTOM\r\n\r\n");
    uart_write_char(adjust_mode ? ' ' : '*');
    uart_write_string("MOVE CURSOR     MAX DELTA\r\n");
    uart_write_char(adjust_mode ? '*' : ' ');
    uart_write_string("ADJUST VALUE       ");
    uart_write_number(delta);
    uart_write_string("mm   ");
}

void Initialize_Buzzer(void)
{
    // BoosterPack buzzer is on J4.40 = P2.7/TB0.6.
    P2OUT &= ~BIT7;
    P2DIR |= BIT7;
    P2SEL1 &= ~BIT7;
    P2SEL0 &= ~BIT7;            // GPIO low while alarm is off

    TB0CCR0 = 999;             // 1 kHz tone with 1 MHz SMCLK
    TB0CCR6 = 500;             // Approximately 50% duty cycle
    TB0CCTL6 = OUTMOD_7;
    TB0CTL = TBSSEL__SMCLK | MC__UP | TBCLR;
}

void Buzzer_Set(unsigned char on)
{
    if (on)
        P2SEL0 |= BIT7;        // Route TB0.6 PWM to the buzzer
    else
        P2SEL0 &= ~BIT7;       // GPIO low silences the buzzer
}

void Initialize_ADC()
{
	// Configure the pins to analog functionality
	// X-axis: A10/P9.2, for A10 (P9DIR=x, P9SEL1=1, P9SEL0=1)
	P9DIR  &= ~BIT2;
	P9SEL1 |= BIT2;
	P9SEL0 |= BIT2;
	// BoosterPack vertical joystick: J3.26 -> P8.7/A4
	P8DIR  &= ~BIT7;
	P8SEL1 |= BIT7;
	P8SEL0 |= BIT7;
	// Turn off ENC (Enable Conversion) bit while modifying the configuration
	ADC12CTL0 &= ~ADC12ENC;
	//*************** ADC12CTL0 ***************
	// Set ADC12SHT0 (select the number of cycles that you computed)
	ADC12CTL0 = ADC12SHT0_3 | ADC12ON | ADC12MSC;
	//*************** ADC12CTL1 ***************
	// Set ADC12SHS (select ADC12SC bit as the trigger)
	// Set ADC12SHP bit
	// Set ADC12DIV (select the divider you determined)
	// Set ADC12SSEL (select MODOSC)
	ADC12CTL1 = ADC12SHS_0 | ADC12SHP | ADC12DIV_0 | ADC12SSEL_0 | ADC12CONSEQ_1;
	//*************** ADC12CTL2 ***************
	// Set ADC12RES (select 12-bit resolution)
	// Set ADC12DF (select unsigned binary format)
	ADC12CTL2 = ADC12RES_2;
	//*************** ADC12CTL3 ***************
	// Start the sequence at ADC12MEM0
    ADC12CTL3 = ADC12CSTARTADD_0;
	//*************** ADC12MCTL0 ***************
	// Set ADC12VRSEL (select VR+=AVCC, VR-=AVSS)
	// Set ADC12INCH (select channel A10)
	// Turn on ENC (Enable Conversion) bit at the end of the configuration
	ADC12MCTL0 = ADC12VRSEL_0 | ADC12INCH_10;
    ADC12MCTL1 = ADC12VRSEL_0 | ADC12INCH_4 | ADC12EOS;
	ADC12CTL0 |= ADC12ENC;
	return;
}


void Initialize_UART()
{
    // Configure pins to UART functionality
    P3SEL1 &= ~(BIT4|BIT5);
    P3SEL0 |= (BIT4|BIT5);
    PM5CTL0 &= ~LOCKLPM5;
    // Main configuration register
    UCA1CTLW0 = UCSWRST;
    UCA1CTLW0 |= UCSSEL_2; // Set clock to SMCLK
    // Configure the clock dividers and modulators (and enable oversampling)
    UCA1BRW = 6; // divider
    // Modulators: UCBRF = 8 = 1000 --> UCBRF3 (bit #3)
    // UCBRS = 0x20 = 0010 0000 = UCBRS5 (bit #5)
    UCA1MCTLW = UCBRF3 | UCBRS5 | UCOS16;
    // Exit the reset state
    UCA1CTLW0 &= ~UCSWRST;
}
void uart_write_char(unsigned char ch)
{
    while((FLAGS & TXFLAG) == 0){}
    TXBUFFER = ch;
    return;
}
unsigned char uart_read_char(void)
{
    unsigned char temp;
    if((FLAGS & RXFLAG) == 0)
    {
        return 0;
    }
    temp = RXBUFFER;
    return temp;
}
void uart_write_number(unsigned long n)
{
    char digits[100];
    int count = 0;
    do{
        digits[count++] = (n % 10) + '0';
        n = n / 10;
    }while(n != 0);
    while (count > 0)
    {
        uart_write_char(digits[--count]);
    }
}
void uart_write_string(char *str)
{
    while(*str != '\0')
    {
        uart_write_char(*str++);
    }
}
