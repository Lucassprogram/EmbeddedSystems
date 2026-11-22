#include <msp430fr6989.h>
#include <stdio.h>

#define FLAGS UCA1IFG
#define RXFLAG UCRXIFG
#define TXFLAG UCTXIFG
#define TXBUFFER UCA1TXBUF
#define RXBUFFER UCA1RXBUF
#define redLED BIT0
#define greenLED BIT7

void config_ACLK_to32Khz_crystal(void);
void Initialize_UART_2(void);
void uart_write_char(unsigned char ch);
unsigned char uart_read_char(void);
void uart_write_number(unsigned long n);
void uart_write_string(char *str);

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer

    config_ACLK_to32Khz_crystal();
    Initialize_UART_2();

    P1DIR |= redLED;
    P1OUT &= ~redLED;
    P9DIR |= greenLED;
    P9OUT &= ~greenLED;

    char *str = "Lucas is cool";

    while (1)
    {
        uart_write_number(123456UL);
        uart_write_char('\r');
        uart_write_char('\n');
        uart_write_string(str);
        uart_write_char('\r');
        uart_write_char('\n');
    }
}

void config_ACLK_to32Khz_crystal(void)
{
    // Configure PJ.4 and PJ.5 for the LaunchPad's 32768 Hz crystal
    PJSEL1 &= ~(BIT4 | BIT5);
    PJSEL0 |= BIT4 | BIT5;
    PM5CTL0 &= ~LOCKLPM5;

    CSCTL0 = CSKEY; // Unlock clock registers
    CSCTL2 = (CSCTL2 & ~SELA_7) | SELA__LFXTCLK;
    CSCTL3 &= ~DIVA_7; // ACLK divider = 1
    CSCTL4 &= ~(LFXTOFF | LFXTBYPASS);

    do
    {
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;
    } while ((CSCTL5 & LFXTOFFG) != 0);

    CSCTL0_H = 0; // Lock clock registers
}

void Initialize_UART_2(void)
{
    P3SEL1 &= ~(BIT4 | BIT5);
    P3SEL0 |= BIT4 | BIT5;
    PM5CTL0 &= ~LOCKLPM5;

    UCA1CTLW0 = UCSWRST;
    UCA1CTLW0 |= UCSSEL__ACLK;

    // Family User's Guide, Table 30-5: 32768 Hz, 4800 baud
    // UCBR = 6, UCBRS = 0xEE, UCBRF unused, UCOS16 = 0
    UCA1BRW = 6;
    UCA1MCTLW = 0xEE00; // UCBRS in bits 15:8; no oversampling

    UCA1CTLW0 &= ~UCSWRST;
}

void uart_write_char(unsigned char ch)
{
    while ((FLAGS & TXFLAG) == 0) {}
    TXBUFFER = ch;
}

unsigned char uart_read_char(void)
{
    if ((FLAGS & RXFLAG) == 0)
    {
        return 0;
    }
    return RXBUFFER;
}

void uart_write_number(unsigned long n)
{
    char digits[100];
    int count = 0;
    do
    {
        digits[count++] = (n % 10) + '0';
        n /= 10;
    } while (n != 0);

    while (count > 0)
    {
        uart_write_char(digits[--count]);
    }
}

void uart_write_string(char *str)
{
    while (*str != '\0')
    {
        uart_write_char(*str++);
    }
}
