#include <msp430fr6989.h> 
#include <stdio.h>
#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer
#define redLED BIT0
#define greenLED BIT7
#define BUTTON_1 BIT1
#define BUTTON_2 BIT2
volatile unsigned char active_runway = 0;
volatile unsigned int request_1 = 0;
volatile unsigned int request_2 = 0;
void Initialize_UART();
void Initialize_UART_2();
void uart_write_char(unsigned char ch);
unsigned char uart_read_char(void);
void uart_write_number(unsigned long n);
void uart_write_string(char *str);
void config_ACLK_to32Khz_crystal();
void uart_clear_terminal();
void uart_write_menu();
void send_request(int led);


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	config_ACLK_to32Khz_crystal();
	Initialize_UART();
	__enable_interrupt();
	TA0CCR0 = 32767;

	//Initialize LED
	P1DIR |= redLED;
	P1OUT &= ~redLED;

	P9DIR |= greenLED;
	P9OUT &= ~greenLED;

	// Configure the buttons as inputs with pull-up resistors.
    P1DIR &= ~(BUTTON_1 | BUTTON_2);
    P1REN |= BUTTON_1 | BUTTON_2;
    P1OUT |= BUTTON_1 | BUTTON_2;
    // A falling edge indicates that a button was pressed.
    P1IES |= BUTTON_1 | BUTTON_2;
    // Unlock the GPIO pins before clearing any startup interrupt flags.
    PM5CTL0 &= ~LOCKLPM5;
    P1IFG &= ~(BUTTON_1 | BUTTON_2);
    P1IE |= BUTTON_1 | BUTTON_2;

	uart_write_string("\x1B[2J\x1B[H"); //Clear console
	uart_write_menu();
	//Main loop
	while(1)
	{
		// Restore button 1 after its runway has been forfeited
		if ((active_runway != 1) &&
			((P1IE & BUTTON_1) == 0) &&
			(P1IN & BUTTON_1))
		{
			P1IFG &= ~BUTTON_1;
			P1IE |= BUTTON_1;
		}

		// Restore button 2 after its runway has been forfeited
		if ((active_runway != 2) &&
			((P1IE & BUTTON_2) == 0) &&
			(P1IN & BUTTON_2))
		{
			P1IFG &= ~BUTTON_2;
			P1IE |= BUTTON_2;
		}
		char temp = uart_read_char();
		switch (temp)
		{
			case '1':
			request_1 = 1;
			send_request(1);
			break;

			case '3':
			request_2 = 1;
			send_request(3);
			break;

			case '7':
			request_1 = 0;

			if (active_runway == 1)
			{
				TA0CTL = MC_0;       // Stop runway 1's timer
				TA0CCTL0 = 0;        // Disable its timer interrupt
				active_runway = 0;  // Allow another release
			}
			P1OUT &= ~redLED;
			uart_write_string("\x1B[13;1H                ");
			uart_write_string("\x1B[14;1H                ");
			uart_write_string("\x1B[15;1H                ");
			uart_write_string("\x1B[16;1H                ");

			break;

			case '9':
			request_2 = 0;

			if (active_runway == 2)
			{
				TA1CTL = MC_0;       // Stop runway 2's timer
				TA1CCTL0 = 0;        // Disable its timer interrupt
				active_runway = 0;  // Allow another release
			}
			P9OUT &= ~greenLED;
			uart_write_string("\x1B[13;31H                ");
			uart_write_string("\x1B[14;31H                ");
			uart_write_string("\x1B[15;31H                ");
			uart_write_string("\x1B[16;31H                ");

			break;
		}

	}

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

void Initialize_UART_2()
{
	// Configure pins to UART functionality
	P3SEL1 &= ~(BIT4|BIT5);
	P3SEL0 |= (BIT4|BIT5);

	PM5CTL0 &= ~LOCKLPM5;

	// Main configuration register
	UCA1CTLW0 = UCSWRST;

	UCA1CTLW0 |= UCSSEL__ACLK ; // Set clock to 

	// Configure the clock dividers and modulators (and enable oversampling)
	UCA1BRW = 6; // divider
	// Modulators: UCBRF = 8 = 1000 --> UCBRF3 (bit #3)
	// UCBRS = 0x20 = 0010 0000 = UCBRS5 (bit #5)
	// UCA1MCTLW = UCBRF3 | UCBRS5 | UCOS16;
	UCA1MCTLW = 0xEE00;
	// Exit the reset state
	UCA1CTLW0 &= ~UCSWRST;
}

void uart_write_char(unsigned char ch)
{
	while( (FLAGS & TXFLAG) == 0){}

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

void config_ACLK_to32Khz_crystal() 

{ 
    // The 32 KHz crystal uses both PJ.4 (LFXIN) and PJ.5 (LFXOUT).
    PJSEL1 &= ~(BIT4 | BIT5); 
    PJSEL0 |= (BIT4 | BIT5); 
    CSCTL0 = CSKEY; //Unlock cs reg 
    do 
    { 
        CSCTL5 &= ~LFXTOFFG; //local fault flag 
        SFRIFG1 &= ~OFIFG; //Global fault flag 
    }while((CSCTL5 & LFXTOFFG) != 0); 
    CSCTL0_H = 0; 
    return; 
} 

void uart_clear_terminal()
{
	uart_write_string("\x1B[2J\x1B[H"); //Clear Console

	return;
}

void uart_write_menu()
{
	uart_write_string("ORLANDO EXECUTIVE AIRPORT RUNWAY CONTROL");
	uart_write_string("\r\n\r\n\r\n\t\t");
	uart_write_string("Runway 1   Runway 2");
	uart_write_string("\r\n");
	uart_write_string("Request (RQ): \t   1\t      3");
	uart_write_string("\r\n");
	uart_write_string("Forfeit (FF): \t   7\t      9");
	uart_write_string("\r\n");
	uart_write_string("\r\n");
	uart_write_string("\r\n");
	uart_write_string("\r\n");
	uart_write_string("--------\t\t      --------");
	uart_write_string("\r\n");
	uart_write_string("RUNWAY 1\t\t      RUNWAY 2");
	uart_write_string("\r\n");
	uart_write_string("--------\t\t      --------");
	uart_write_string("\r\n");

	return;
}

void send_request(int led)
{
	if(led == 1)
	{
		P1OUT |= redLED;
		uart_write_string("\x1B[13;1H");
		uart_write_string("Requested");

	}
	else if(led == 3)
	{
		P9OUT |= greenLED;
		uart_write_string("\x1B[13;31H");
		uart_write_string("Requested");
	}
	return;
}

#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void)
{
	if((P1IFG & BUTTON_1) != 0){
		P1IFG &= ~BUTTON_1;
		if((active_runway ==0) && request_1)
		{
			//Setup interrupt for flashing led
			P1IE &=  ~BUTTON_1;
			TA0CCTL0 = CCIE;
			TA0CCR0 = 16383;
			TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;
			active_runway = 1;
			request_1 = 0;
			uart_write_string("\x1B[14;1H");
			uart_write_string("In use");	
		}
		else if (active_runway == 1) {
			uart_write_string("\x1b[16;1H**** Inquiry ***");
			P1IE &=  ~BUTTON_1;
		}
		}
	if((P1IFG & BUTTON_2) != 0){
		P1IFG &= ~BUTTON_2;
		if((active_runway ==0) && request_2)
		{
			P1IE &=  ~BUTTON_2;
			TA1CCTL0 = CCIE;
			TA1CCR0 = 16383;
			TA1CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;
			active_runway = 2;
			request_2 = 0;
			uart_write_string("\x1B[14;31H");
			uart_write_string("In use");	
		}
		else if (active_runway == 2) {
			uart_write_string("\x1b[16;31H**** Inquiry ***");
			P1IE &=  ~BUTTON_2;
		}
		}
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void)
{
    P1OUT ^= redLED;
	if (P1IN & BUTTON_1)
	{
        P1IFG &= ~BUTTON_1;
        P1IE |= BUTTON_1;
	}
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer1_A0_ISR(void)
{
    P9OUT ^= greenLED;
	if (P1IN & BUTTON_2)
	{
        P1IFG &= ~BUTTON_2;
        P1IE |= BUTTON_2;
	}
}


