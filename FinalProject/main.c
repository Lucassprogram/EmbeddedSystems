#include <msp430fr6989.h> 
#include "GrLib/grlib/grlib.h"
#include "LcdDriver/lcd_driver.h"
void Initialize_ADC();
void Initialize_Buzzer(void);
void Buzzer_Set(unsigned char on);

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	//Initialize Functions
	Initialize_ADC();
	Initialize_Buzzer();

	//Initailize Variables
	// int SPI_Locations[9] = [0,1,2,3,4,5,6,7,8];
	// char ADC_Direction[4] = ['U','D','L','R'];
	PM5CTL0 &= ~LOCKLPM5;  // Enable GPIO pins

	return 0;
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

void Buzzer_Set(unsigned char on)
{
    if (on)
        P2SEL0 |= BIT7;        // Route TB0.6 PWM to the buzzer
    else
        P2SEL0 &= ~BIT7;       // GPIO low silences the buzzer
}
