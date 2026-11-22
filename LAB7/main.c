#include <msp430fr6989.h>
#include <stdio.h>
#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer
#define redLED BIT0
#define greenLED BIT7
void Initialize_UART();
void uart_write_char(unsigned char ch);
unsigned char uart_read_char(void);
void uart_write_number(unsigned long n);
void uart_write_string(char *str);
void initialize_I2C(void);
int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int * data);
int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data);

volatile unsigned char clock_seconds = 0;
volatile unsigned char clock_minutes = 0;
volatile unsigned char clock_hours = 12;
volatile unsigned char minute_due = 0;

void initialize_timer(void);
void initialize_S2(void);
void set_time_from_uart(void);
void log_lux(void);
void uart_write_two_digits(unsigned char value);
/**
* main.c
*/
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
    unsigned char light_sens_address = 0x44;
    unsigned int lux_config = 0x7604;

    Initialize_UART();
    initialize_I2C();
    initialize_S2();
    initialize_timer();
    uart_write_string("\x1B[2J\x1B[H"); // Clear console
    i2c_write_word(light_sens_address, 0x01, lux_config);

    uart_write_string("*** Lux Logger ***\r\n");
    log_lux();

    while (1)
    {
        if ((P3IN & BIT1) == 0) // BoosterPack S2 is active low on P3.1
        {
            __delay_cycles(20000); // Debounce for about 20 ms
            if ((P3IN & BIT1) == 0)
            {
                while ((P3IN & BIT1) == 0) {}
                set_time_from_uart();
                log_lux();
            }
        }

        if (minute_due)
        {
            __disable_interrupt();
            minute_due = 0;
            __enable_interrupt();
            log_lux();
        }
    }
}

void initialize_S2(void)
{
    P3DIR &= ~BIT1;
    P3REN |= BIT1;
    P3OUT |= BIT1; // Enable pull-up; button press reads low
}

void initialize_timer(void)
{
    TA0CCR0 = 32767; // ACLK at 32768 Hz gives one interrupt per second
    TA0CCTL0 = CCIE;
    TA0CTL = TASSEL__ACLK | MC__UP | TACLR;
    __enable_interrupt();
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer_A0_isr(void)
{
    clock_seconds++;
    if (clock_seconds >= 60)
    {
        clock_seconds = 0;
        clock_minutes++;
        if (clock_minutes >= 60)
        {
            clock_minutes = 0;
            clock_hours++;
            if (clock_hours > 12)
            {
                clock_hours = 1;
            }
        }
        minute_due = 1;
    }
}

void uart_write_two_digits(unsigned char value)
{
    uart_write_char((value / 10) + '0');
    uart_write_char((value % 10) + '0');
}

void set_time_from_uart(void)
{
    char digits[5];
    unsigned char count;
    unsigned char ch;
    unsigned char new_hours;
    unsigned char new_minutes;

    while (1)
    {
        count = 0;
        uart_write_string("Enter the time...(3 or 4 digits then hit Enter)\r\n");
        while (1)
        {
            while ((FLAGS & RXFLAG) == 0) {}
            ch = RXBUFFER;

            if (ch == '\r' || ch == '\n')
            {
                if (count == 0)
                {
                    continue; // Ignore the extra line ending some terminals send
                }
                break;
            }

            if (ch >= '0' && ch <= '9' && count < 4)
            {
                digits[count++] = ch; // Do not echo typed digits
            }
        }

        if (count == 3)
        {
            new_hours = digits[0] - '0';
            new_minutes = (digits[1] - '0') * 10 + (digits[2] - '0');
        }
        else if (count == 4)
        {
            new_hours = (digits[0] - '0') * 10 + (digits[1] - '0');
            new_minutes = (digits[2] - '0') * 10 + (digits[3] - '0');
        }
        else
        {
            new_hours = 0;
            new_minutes = 60;
        }

        if (new_hours >= 1 && new_hours <= 12 && new_minutes < 60)
        {
            __disable_interrupt();
            clock_hours = new_hours;
            clock_minutes = new_minutes;
            clock_seconds = 0;
            minute_due = 0;
            __enable_interrupt();

            uart_write_string("Time is set to ");
            uart_write_number(clock_hours);
            uart_write_char(':');
            uart_write_two_digits(clock_minutes);
            uart_write_string("\r\n");
            return;
        }

        uart_write_string("Invalid time. Try again.\r\n");
    }
}

void log_lux(void)
{
    static unsigned char first_reading = 1;
    static unsigned int low_limit = 0;
    static unsigned int high_limit = 0;
    unsigned int raw_result;
    unsigned int lux;
    unsigned char hours;
    unsigned char minutes;
    unsigned char event = 0;
    unsigned long lux_x100;

    i2c_read_word(0x44, 0x00, &raw_result);
    lux_x100 = (unsigned long)raw_result * 128;
    lux = (lux_x100 + 50) / 100; // Round to nearest whole lux

    if (first_reading)
    {
        first_reading = 0;
        low_limit = (lux > 10) ? lux - 10 : 0;
        high_limit = lux + 10;
    }
    else if (lux >= high_limit)
    {
        event = 1;
        low_limit = (lux > 10) ? lux - 10 : 0;
        high_limit = lux + 10;
    }
    else if (lux <= low_limit)
    {
        event = 2;
        low_limit = (lux > 10) ? lux - 10 : 0;
        high_limit = lux + 10;
    }

    __disable_interrupt();
    hours = clock_hours;
    minutes = clock_minutes;
    __enable_interrupt();

    uart_write_number(hours);
    uart_write_char(':');
    uart_write_two_digits(minutes);
    uart_write_char(' ');
    uart_write_number(lux);
    uart_write_string(" lux");
    if (event == 1)
    {
        uart_write_string(" <Up>");
    }
    else if (event == 2)
    {
        uart_write_string(" <Down>");
    }
    uart_write_string("\r\n");
}
void initialize_I2C(void)
{
    P4SEL1 |= (BIT1|BIT0);
    P4SEL0 &= ~(BIT1|BIT0);
    //Enter reset state and set all field in this register to zero
    UCB1CTLW0 = UCSWRST;
    // Fields that should be nonzero are changed below
    // (Master Mode: UCMST) (I2C mode: UCMODE_3) (Synchronous mode: UCSYNC)
    // (UCSSEL 1:ACLK, 2,3:SMCLK)
    UCB1CTLW0 |= UCMST | UCMODE_3 | UCSYNC | UCSSEL_3;
    // Clock frequency: SMCLK/8 = 1 MHz/8 = 125 KHz
    UCB1BRW = 8;
    // Chip Data Sheet p. 53 (Should be 400 KHz max)
    // Exit the reset mode at the end of the configuration
    UCB1CTLW0 &= ~UCSWRST;
}
////////////////////////////////////////////////////////////////////
// Read a word (2 bytes) from I2C (address, register)
int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int * data) {
    unsigned char byte1=0, byte2=0; // Intialize to ensure successful reading
    UCB1I2CSA = i2c_address; // Set address
    UCB1IFG &= ~UCTXIFG0;
    // Transmit a byte (the internal register address)
    UCB1CTLW0 |= UCTR;
    UCB1CTLW0 |= UCTXSTT;
    while((UCB1IFG & UCTXIFG0)==0) {} // Wait for flag to raise
    UCB1TXBUF = i2c_reg; // Write in the TX buffer
    while((UCB1IFG & UCTXIFG0)==0) {} // Buffer copied to shift register; Tx in progress; set Stop bit
    // Repeated Start
    UCB1CTLW0 &= ~UCTR;
    UCB1CTLW0 |= UCTXSTT;
    // Read the first byte
    while((UCB1IFG & UCRXIFG0)==0) {} // Wait for flag to raise
    byte1 = UCB1RXBUF;
    // Assert the Stop signal bit before receiving the last byte
    UCB1CTLW0 |= UCTXSTP;
    // Read the second byte
    while((UCB1IFG & UCRXIFG0)==0) {} // Wait for flag to raise
    byte2 = UCB1RXBUF;
    while((UCB1CTLW0 & UCTXSTP)!=0) {}
    while((UCB1STATW & UCBBUSY)!=0) {}
    *data = (byte1 << 8) | (byte2 & (unsigned int)0x00FF);
    return 0;
}
////////////////////////////////////////////////////////////////////
// Write a word (2 bytes) to I2C (address, register)
int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data) {
    unsigned char byte1, byte2;
    UCB1I2CSA = i2c_address; // Set I2C address
    byte1 = (data >> 8) & 0xFF; // MSByte
    byte2 = data & 0xFF; // LSByte
    UCB1IFG &= ~UCTXIFG0;
    // Write 3 bytes
    UCB1CTLW0 |= (UCTR | UCTXSTT);
    while( (UCB1IFG & UCTXIFG0) == 0) {}
    UCB1TXBUF = i2c_reg;
    while( (UCB1IFG & UCTXIFG0) == 0) {}
    UCB1TXBUF = byte1;
    while( (UCB1IFG & UCTXIFG0) == 0) {}
    UCB1TXBUF = byte2;
    while( (UCB1IFG & UCTXIFG0) == 0) {}
    UCB1CTLW0 |= UCTXSTP;
    while( (UCB1CTLW0 & UCTXSTP) != 0 ) {}
    while((UCB1STATW & UCBBUSY)!=0) {}
    return 0;
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
