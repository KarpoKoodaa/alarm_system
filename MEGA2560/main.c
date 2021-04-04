/*
 * main.c
 *
 * Created: 02.04.2021
 * Author : Kariantti 
 */ 

#define F_CPU 16000000UL    //16MHz Clock

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>
#include "lib/LCD/lcd.h"
#include "lib/Keypad/keypad.h"

char g_c_pin[5] = "1234";
bool g_b_alarm_active = false;

bool
check_pin(void)
{
    char entered_pin[5]={0};
    uint8_t counter = 0;
    char key;
    bool pin_correct;

    lcd_clrscr();
    lcd_puts("Enter a PIN code");
    lcd_gotoxy(0,1);

    while (counter < 4 && counter > -1)
    {
        key = KEYPAD_GetKey();
        switch (key)
        {
        case '1': 
        case '2':
        case '3':
        case '4':
        case '5': 
        case '6':
        case '7':
        case '8':
        case '9':
        case '0':
            entered_pin[counter] = key;
            lcd_putc(entered_pin[counter]);
            counter++; 
            break;
       case 'B':
            if(counter > 0)
           {
                counter--;
                entered_pin[counter] = " ";
                lcd_gotoxy(counter,1); 
                lcd_putc(entered_pin[counter]);
                lcd_gotoxy(counter,1); 
           }
           else if (counter == 0)
           {
               lcd_gotoxy(counter,1);
               counter = 0;
           }
        default:
            break;
        }
        
    }

    if(strcmp(g_c_pin,entered_pin) == 0)
    {
        pin_correct = true;
    }
    else pin_correct = false;
    _delay_ms(1000);

    return pin_correct;
    
}
bool
show_menu()
{
    char menu_choice = 0;
    lcd_clrscr();
    lcd_puts("MENU");
    _delay_ms(1500);
    lcd_clrscr();
    lcd_puts("1.Activate Alarm\n");
    lcd_puts("2.Change PIN");
    menu_choice = KEYPAD_GetKey();
    
    switch (menu_choice)
    {
    case '1':
        return check_pin();
        
        break;
    case '2':
        lcd_clrscr();
        lcd_puts("Changing PIN\ncode");
        _delay_ms(2000);
        break;
    default:
        show_menu();
        break;
    }

    _delay_ms(5000);
}


int
main (void)
{
    bool pin_status;

    KEYPAD_Init();
    
    lcd_init(LCD_DISP_ON);
    lcd_clrscr();

    lcd_puts("Alarm System\n");
    lcd_puts("DEACTIVATED");
    _delay_ms(2000);
    lcd_clrscr();

    while (1)
    {
        /* code */
        lcd_clrscr();
        lcd_puts("Press a button");
        while (!KEYPAD_GetKey());
        //show_menu();

        pin_status = show_menu();
        lcd_clrscr();
        lcd_puts("Entered PIN:");
        lcd_gotoxy(0,1);
        if(pin_status)
        {
            lcd_puts("PIN CORRECT");
        }
        else lcd_puts("PIN INCORRECT");
        _delay_ms(5000);
    }
    
}