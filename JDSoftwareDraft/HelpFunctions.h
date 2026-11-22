#ifndef HELP_FUNCTIONS_H
#define HELP_FUNCTIONS_H

#include <msp430.h>
#include <stdio.h>

void Setup_LCD()
{
    LCD_Initialize();
    // Store message in a character array.
    char Data[] = "Junior Design ";
    // LCD_Initialize() already homes and clears the display.
    LCD_String(Data);
}

// Move the cursor without the interrupt-based delay in LCD_Cursor_Postion().
// Junior_Design.h must be included before this helper.
void Set_LCD_Cursor_NoTimer(char location)
{
    unsigned char command = (unsigned char)(0x80 | location);
    unsigned int nibble;

    nibble = (command & 0xF0) + LCD_BACKLITE;
    LCD_I2C_Send(nibble);
    LCD_I2C_Send(nibble + LCD_E);
    LCD_I2C_Send(nibble);
    LCD_I2C_Send(LCD_BACKLITE);

    nibble = ((command << 4) & 0xF0) + LCD_BACKLITE;
    LCD_I2C_Send(nibble);
    LCD_I2C_Send(nibble + LCD_E);
    LCD_I2C_Send(nibble);
    LCD_I2C_Send(LCD_BACKLITE);
    Delay(DELAY_1ms);
}

void Send_LCD(int location, char* data)
{
    Set_LCD_Cursor_NoTimer((char)location);
    LCD_String(data);
}

char* Read_Compute_Distance()
{
    static char Data2[32];
    unsigned long ticks;
    unsigned long tenths_in;
    unsigned long echo_start = 0;
    unsigned long echo_stop = 0;
    unsigned int sample;
    unsigned int saw_rise = 0;
    unsigned int saw_fall = 0;

    if (P2IN & ECHO_INTERUPT) {
        sprintf(Data2, "P2.0 HIGH IDLE  ");
        return Data2;
    }

    Ultrasonic_Trigger();
    // Poll the echo pin so a missed Port 2 interrupt does not lose a reading.
    for (sample = 0; sample < 10000; ++sample) {
        if (P2IN & ECHO_INTERUPT) {
            if (!saw_rise) {
                echo_start = Delay_Count;
                saw_rise = 1;
            }
        } else if (saw_rise) {
            echo_stop = Delay_Count;
            saw_fall = 1;
            break;
        }
        Delay(10);
    }

    if (!saw_rise) {
        sprintf(Data2, "NO SIGNAL P2.0  ");
        return Data2;
    }
    if (!saw_fall) {
        sprintf(Data2, "ECHO STAYS HIGH ");
        return Data2;
    }

    if (Trigger1 == 0 && State_1 == 0) {
        ticks = Ultrasonic_Echo_Read();
    } else {
        ticks = echo_stop - echo_start;
    }
    tenths_in = ticks * 66929UL / 100000UL;

    sprintf(Data2,"%dmm %d.%din",
        (int)(ticks * 17UL / 10UL),
        (int)(tenths_in / 10UL),
        (int)(tenths_in %10UL)
    );
    return Data2;
}

#endif // HELP_FUNCTIONS_H
