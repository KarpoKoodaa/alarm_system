/*
 * main.c
 * Master
 * Created: 02.04.2021
 * Author : Kariantti 
 */ 

#define F_CPU 16000000UL    // 16MHz Clock
#define BAUD 9600           // BAUD speed for UART
#define ACTIVATE 1          // Activate alarm
#define DEACTIVATE 2        // Deactivate alarm 
#define CHECK 3             // Check data SPI connection
#define MAX_SIZE 4          // Max size of char PIN


#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "lib/LCD/lcd.h"
#include "lib/Keypad/keypad.h"

char g_c_pin[5] = "1234\0";
bool g_b_alarm_active = false;
uint8_t g_i_timeout = 0;
volatile bool g_b_timeout = false;
volatile bool g_b_connection_status = false;

void uart_putchar(char c, FILE *stream);
char uart_getchar(FILE *stream);

void uart_init();
FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

FILE uart_io = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void init_timeout_counter();

bool
check_pin(void)
{
    char entered_pin[5] = {0, 0, 0, 0, 0};
    uint8_t counter = 0;

    init_timeout_counter();
    
    if(g_b_alarm_active)
    {
        lcd_clrscr();
        lcd_puts("Alarm Active");
        _delay_ms(1000);
        disable_timer_counter();
    }
    lcd_clrscr();
    lcd_puts("Enter a PIN code");
    lcd_gotoxy(0,1);
    _delay_ms(500);

    get_pin_code(entered_pin);
    disable_timer_counter();

    bool pin_correct = false;
    
    if (strcmp(g_c_pin,entered_pin) == 0)
    {
        pin_correct = true;
    }
    else
    {
        pin_correct = false;
    } 
    _delay_ms(1000);

    return pin_correct;
}

void
get_pin_code(char * entered_pin_code)
{
    uint8_t counter = 0;
    //lcd_clrscr();
    //init_timeout_counter();
    g_b_timeout = false; 

   while ((entered_pin_code[4] != '#') && (g_b_timeout != true))
    {
        if(counter < 5)
        {
            char key = KEYPAD_GetKey();
            switch (key)
            {
                // Takes 0-9 digits from keypad

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
                case '#':
                    entered_pin_code[counter] = key;
                    lcd_putc(entered_pin_code[counter]);
                    counter++;
                    g_i_timeout = 0; 
                    _delay_ms(200);
                break;

                case 'B':
                  // Erases a digit 

                    g_i_timeout = 0;
                    TCCR1A = 0;

                    if (counter != 0)
                    {
                        --counter;
                        entered_pin_code[counter] = 32; // Enter empty char
                        lcd_gotoxy(counter, 1); 
                        lcd_putc(entered_pin_code[counter]);
                        lcd_gotoxy(counter, 1);
                        _delay_ms(200); 
                    }
                    else if (counter == 0)
                    {
                       // Blocks that no negative numbers on counter

                       lcd_gotoxy(counter, 1);
                       //counter = 0;
                       _delay_ms(200);
                    }
                    else 
                    {

                    }
                    break;
    
                default:
                    //g_i_timeout = 0;
                    break;
            }
        }
        else
        {
            char key = KEYPAD_GetKey();
            
            if(key == 'B')
            {
                --counter;
                entered_pin_code[counter] = 32; // Enter empty char
                lcd_gotoxy(counter, 1); 
                lcd_putc(entered_pin_code[counter]);
                lcd_gotoxy(counter, 1);
                _delay_ms(200);  
            }
        }
        
    }

    // Remove # from the end
    entered_pin_code[4]= '\0';
}

bool
change_pin_code()
{
    char entered_pin_code[5] = {0, 0, 0, 0, 0};

    lcd_clrscr();
    lcd_puts("Enter PIN Code:\n");
    get_pin_code(entered_pin_code);

    bool pin_correct = false;
    
    if (strcmp(g_c_pin,entered_pin_code) == 0)
    {
        pin_correct = true;
        char new_pin_code_1[5] = {0, 0, 0, 0, 0};

        lcd_clrscr();
        lcd_puts("Enter new PIN:\n");

        get_pin_code(new_pin_code_1);
        _delay_ms(200);

        char new_pin_code_2[5] = {0, 0, 0, 0, 0};

        lcd_clrscr();
        lcd_puts("Enter PIN again:\n");
        get_pin_code(new_pin_code_2);

        if(strcmp(new_pin_code_1, new_pin_code_2) == 0)
        {
            //strcpy(g_c_pin,new_pin_code_2);
            write_pin_eeprom(new_pin_code_2);
            lcd_clrscr();
            lcd_puts("PIN CHANGED\n");
            _delay_ms(2000);
        }
    }
    else
    {
        pin_correct = false;
    } 

    return pin_correct;
}

bool
show_menu()
{
    
    lcd_clrscr();
    lcd_puts("MENU");
    _delay_ms(1500);
    lcd_clrscr();
    if(g_b_alarm_active)
    {
        return check_pin();
    }
    else
    {
        lcd_puts("1.Activate\n");
        lcd_puts("2.Change PIN");
    }
    
    char menu_choice = 0;
    
    do
    {
        menu_choice = KEYPAD_GetKey();
    
        switch (menu_choice)
        {
            case '1':
                return check_pin();
        
                break;
            case '2':
                _delay_ms(200);
                // return change_pin_code();
                 lcd_clrscr();
                lcd_puts("Changing PIN\ncode");
                return change_pin_code();
                //_delay_ms(1000);
                break;
            default:
                //show_menu();
                break;
        }
    } while(menu_choice == 'z');

    _delay_ms(5000);
}

void
SPI_init(void)
{
    // Set MOSI, SS and SCK ouput
    DDRB |= (1 << PB2) | (1 << PB1) | (1 << PB0);

    // Set MISO input
    DDRB &= ~(1 << PB3);

    // Set as Master and clock to fosc/128
    SPCR = (1 << MSTR) | (1 << SPR1) | (1 << SPR0);

    // Wait until all set
    _delay_ms(10);
    
    // Enable SPI
    SPCR |= (1 << SPE);

}

void
transmit_byte(uint8_t data)
{
    // Load data into register
    SPDR = data;

    // Wait until transmission is complete
    while(!(SPSR & (1 << SPIF)));
}

uint8_t
SPI_master_tx_rx(uint8_t data)
{

    uint8_t received_data = 0;
    // Set SS Low
    PORTB &= ~(1 << PB0);

    // Load transmit data into register
    SPDR = data;

    // Wait until transmission to complete
    while(!(SPSR & (1 << SPIF)));
    received_data = SPDR;

    // Set SS High
    PORTB |= (1 << PB0);

    return received_data;
}

void SPI_connection_check()
{
    uint8_t status = SPI_master_tx_rx(CHECK);
    if (status != 3)
    {
        g_b_connection_status = false;
    }
    else g_b_connection_status = true;

}

void read_pin_eeprom(void)
{
    for (uint8_t address_index = 0; address_index < MAX_SIZE; address_index++)
    {
        while(EECR & (1 << EEPE))
        {
            // Wait until previous write done
        }

        EEAR = address_index;
        EECR |= (1 << EERE);
        g_c_pin[address_index] = EEDR;

    }
}

void write_pin_eeprom(char * new_pin_code)
{
    for (uint8_t address_index = 0; address_index < MAX_SIZE; address_index++)
    {

        while (EECR & (1 << EEPE))
        {
            // Wait until previous write done
        }

        EEAR = address_index;
        EEDR = new_pin_code[address_index];

        // Enable master programming
        EECR |= (1 << EEMPE);

        // Enable EEPROM programming
        EECR |= (1 << EEPE); 
    }

    // Read the new PIN code to Global variable
    read_pin_eeprom();
}

void
init_timeout_counter(void)
{
    g_i_timeout = 0;

    // Normal mode
    TCCR1A = 0;

    // Start timer with 1024 prescaler
    TCCR1B = (1 << CS12) | (1 << CS10);

    // Enable overflow interrupt
    TIMSK1 = (1 << TOIE1);
    // Enable interrupts
    sei();

}

void disable_timer_counter(void)
{
    g_i_timeout = 0;
    TCCR1B = 0;
    cli();
}

ISR (PCINT2_vect)
{
    printf("PCINT2 vector\n");
    // Disable interrupts
    PCMSK2 &= 0;
    SMCR &= ~(1 << SE);
    
}

ISR
(TIMER1_OVF_vect)
{
   g_i_timeout++;

    if(g_i_timeout == 2)
    {
        lcd_clrscr();
        lcd_puts("Timeout!");
        g_b_timeout = true;
        g_i_timeout = 0;
        cli();
        _delay_ms(1000);
    }
}

void go_standby_mode(void)
{
    lcd_clrscr();
    lcd_puts("Standby mode");

    sei();

    // Enable Pin change mask register 2 interrupts
    PCMSK2 = 1;

    // Enable Sleep mode
    SMCR |= (1 << SM1);
    _delay_ms(100);

    // Set Sleep enable bit
    SMCR |= (1 << SE);

    sleep_cpu();

}

int
main (void)
{
    KEYPAD_Init();
    uart_init();
    SPI_init();
    read_pin_eeprom();

    lcd_init(LCD_DISP_ON);
    lcd_clrscr();

    lcd_puts("Alarm System\n");
    lcd_puts("DEACTIVATED");
    _delay_ms(2000);
    lcd_clrscr();

    SPI_connection_check();
    lcd_clrscr();
    while (!g_b_connection_status)
    {
        lcd_gotoxy(0,0);
        lcd_puts("SPI CONNECTION\nERROR");
        SPI_connection_check();
    }

    while (1)
    {
        bool pin_status = false;
        /* code */
        lcd_clrscr();
        
        lcd_puts("'*' for menu\n'#' for standby");
        char pressed_key = KEYPAD_GetKey();
        while (pressed_key != '*' && pressed_key != '#')
        {
            pressed_key = KEYPAD_GetKey();
        }

        if (pressed_key == '*')
        {
            // MENU
            pin_status = show_menu();
            lcd_clrscr();
            lcd_puts("Entered PIN:");
            lcd_gotoxy(0, 1);
            if(pin_status)
            {
                lcd_puts("PIN CORRECT");
                if(!g_b_alarm_active)
                {
                    SPI_master_tx_rx(ACTIVATE);

                }
                else SPI_master_tx_rx(DEACTIVATE);
                g_b_alarm_active = !g_b_alarm_active;
            }
            else lcd_puts("PIN INCORRECT");
            _delay_ms(2000);
        }
        else
        {
            // Will go to standby mode
            // TODO standby mode
            lcd_clrscr();
            lcd_puts("STANDBY!!!");
            go_standby_mode();
            _delay_ms(2000);
        }
        
        
        // lcd_puts("Press a button");
        // while (!KEYPAD_GetKey());
        //show_menu();

        // pin_status = show_menu();
        // lcd_clrscr();
        // lcd_puts("Entered PIN:");
        // lcd_gotoxy(0, 1);
        // if(pin_status)
        // {
        //     lcd_puts("PIN CORRECT");
        //     if(!g_b_alarm_active)
        //     {
        //         SPI_master_tx_rx(ACTIVATE);
                
        //     }
        //     else SPI_master_tx_rx(DEACTIVATE);
        //     g_b_alarm_active = !g_b_alarm_active;
        // }
        // else lcd_puts("PIN INCORRECT");
        // _delay_ms(5000);
    }
}


// UART FOR TROUBLESHOOTING

void
uart_init()
{
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

    
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);

    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
     
    stdout = &uart_output;
    stdin = &uart_input;
}

void
uart_putchar(char c, FILE *stream)
{
    if(c == '\n')
    {
        uart_putchar('\r', stream);
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

char
uart_getchar(FILE *stream)
{
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}
