/*
 * main.c
 *
 * Created: 02.04.2021
 * Author : Kariantti 
 */ 

#define F_CPU 16000000UL    //16MHz Clock

#include <avr/io.h>
#include <util/delay.h>
#include "lib/LCD/lcd.h"
#include "lib/Keypad/keypad.h"






int
main (void)
{
    uint8_t key;
    KEYPAD_Init();
    
    lcd_init(LCD_DISP_ON);
    lcd_clrscr();

    lcd_puts("Alarm System\n");
    lcd_puts("ACTIVATED");
    _delay_ms(2000);
    lcd_clrscr();

    while (1)
    {
        /* code */
        key = KEYPAD_GetKey();
        lcd_putc(key);
    }
    
}