/*
 * This is an exercise work of Introduction to Embedded Systems course. 
 * The alarm system is build using Arduino MEGA 2560 (ATmega2560) and Arduino UNO (ATmega328p)
 *
 * main.c
 *
 * Master device
 * 
 * Created: 02.04.2021
 * 
 * Author : Kariantti Laitala
 */ 

#define F_CPU 16000000UL    // 16MHz Clock
#define BAUD 9600           // BAUD speed for UART

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// Libraries for LCD and Keypad
//
#include "lib/LCD/lcd.h"
#include "lib/Keypad/keypad.h"

// Parameters to handle SPI communcations
//
#define ACTIVATE 5          // Activate alarm
#define DEACTIVATE 2        // Deactivate alarm 
#define CHECK 3             // Check data SPI connection
#define OK 3               // Connection ok

// Parameters for PIN status
//
#define OK 1        // PIN correct
#define CHANGED 2   //PIN changed
#define NOK 3       // PIN incorrect

// Parameter to define MAX size of PIN code array
//
#define MAX_SIZE 4          // Max size of char PIN

// Global variables
//
char g_c_pin[5] = "1234\0";                     // Global Variable for PIN code
bool g_b_alarm_active = false;                  // Global Variable for alarm status
uint8_t g_i_timeout = 0;                        // Global Variable for timeout counter
volatile bool g_b_timeout = false;              // Global Variable for timeout
volatile bool g_b_connection_status = false;    // Global Variable for SPI connection status

// Declarations of all functions
// 
void init_timeout_counter();
void get_pin_code(char * entered_pin_code);
bool change_pin_code();
uint8_t show_menu();
void SPI_init(void);
void transmit_byte(uint8_t data);
uint8_t SPI_master_tx_rx(uint8_t data);
void SPI_connection_check();
void read_pin_eeprom(void);
void write_pin_eeprom(char * new_pin_code);
void init_timeout_counter(void);
void disable_timer_counter(void);
void go_standby_mode(void);

/*! 
 * @brief Asks user to input PIN code and checks if correct.
 * If alarm is armed, will show "Alarm active" on LCDa
 * @return The entered PIN boolean
 */
bool check_pin(void)
{
    char entered_pin[5] = {0, 0, 0, 0, 0};

    // Start timeout counter to timeout if no input
    init_timeout_counter();
    
    if(g_b_alarm_active)
    {
        lcd_clrscr();
        lcd_puts("Alarm Active");
        _delay_ms(1000);
        disable_timer_counter();
    }
    lcd_clrscr();
    lcd_puts("PIN + '#'");
    lcd_gotoxy(0,1);
    _delay_ms(500);

    // Ask PIN code from user
    get_pin_code(entered_pin);

    // Disable timeout
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

/*!
 * @brief Receives PIN code as user input
 * @param[in] entered_pin_code A pointer to array that is used for user PIN code
 * 
 */
void get_pin_code(char * entered_pin_code)
{
    uint8_t counter = 0;
    g_b_timeout = false; 

   while ((entered_pin_code[4] != '#') && (g_b_timeout != true))
    {
        if(counter < 5)
        {
            char key = KEYPAD_GetKey();
            switch (key)
            {
                // Accepts 0-9 digits or '#' from keypad
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

                case 'B': // (B)ackspace erases a digit 

                    // Clears timer as input received
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
                       // Keeps the "cursor" at the beginning of the LCD screen 

                       lcd_gotoxy(counter, 1);
                       _delay_ms(200);
                    }
                    break;
    
                default:
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

    // Remove # from the end and NULL
    entered_pin_code[4]= '\0';
}

/*!
 * @brief Changes the PIN code that is stored to EEPROM
 * @return Status of the change
 */
bool change_pin_code()
{
    char entered_pin_code[5] = {0, 0, 0, 0, 0};

    lcd_clrscr();
    lcd_puts("Old PIN+'#':\n");
    get_pin_code(entered_pin_code);

    bool pin_correct = false;
    
    if (strcmp(g_c_pin,entered_pin_code) == 0)
    {
        pin_correct = true;
        char new_pin_code_1[5] = {0, 0, 0, 0, 0};

        lcd_clrscr();
        lcd_puts("New PIN+'#':\n");

        // Request new PIN code from user
        get_pin_code(new_pin_code_1);
        _delay_ms(200);

        char new_pin_code_2[5] = {0, 0, 0, 0, 0};

        lcd_clrscr();
        lcd_puts("Confirm PIN+'#':\n");
        get_pin_code(new_pin_code_2);

        // Compare that PIN codes match
        if(strcmp(new_pin_code_1, new_pin_code_2) == 0)
        {
            // Write new PIN to memory
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

/*! 
 * @brief Displays the MENU for the user. User can arm/dearm the alarm or change PIN
 * @return Status of PIN change, 1 = OK, 2 = CHANGED & 3 = NOK
 */
uint8_t show_menu()
{
   bool pin_action = false;

   lcd_clrscr();
   lcd_puts("MENU");
   _delay_ms(1500);
   lcd_clrscr();
   if(g_b_alarm_active)
   {
       pin_action = check_pin();
       if(pin_action)
       {
           return 1;
       }
       return 3;
   }
   else
   {
        lcd_puts("1.Activate\n");
        lcd_puts("2.Change PIN");
   }
    
    char menu_choice = 0;
    
    while ((menu_choice != '1') && (menu_choice != '2'))
    {

        menu_choice = KEYPAD_GetKey();
    
        switch (menu_choice)
        {
            case '1':
                pin_action = check_pin();
                if (pin_action)
                {
                    return OK;
                }
                else return NOK;
        
            break;
            
            case '2':
                _delay_ms(200);
                lcd_clrscr();
                lcd_puts("Changing PIN\ncode");
                pin_action = change_pin_code();

                if(pin_action)
                {
                    return CHANGED;
                }
                else return NOK;
            break;

            default:
            break;
        }
    }
    return NOK;
}

/*!
 * @brief Initializes SPI connection and set device as Master
 */
void SPI_init(void)
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

/*!
 * @brief Transmits one (1) byte to Slave over SPI
 * @param[in] data The data that is sent over to Slave
 */
void transmit_byte(uint8_t data)
{
    // Load data into register
    SPDR = data;

    // Wait until transmission is complete
    while(!(SPSR & (1 << SPIF)));
}

/*!
 * @brief Transmits and Receives data over the SPI
 * @param[in] Data that is sent over the SPI
 * @return The received data
 */
uint8_t SPI_master_tx_rx(uint8_t data)
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

/*!
 * @brief Check status of SPI connection between Master and Slave. 
 * The status is stored to Global Variable g_b_connection_status
 *
 */
void SPI_connection_check()
{
    uint8_t status = SPI_master_tx_rx(CHECK);
    if (status != OK)
    {
        g_b_connection_status = false;
    }
    else g_b_connection_status = true;

}

/*!
 * @brief Reads the PIN code that is stored in EEPROM
 *
 */
void read_pin_eeprom(void)
{
    for (uint8_t address_index = 0; address_index < MAX_SIZE; address_index++)
    {
        while(EECR & (1 << EEPE))
        {
            // Wait until previous write done
        }

        // Set index as EEPROM address
        EEAR = address_index;

        // Enable read from EEPROM
        EECR |= (1 << EERE);

        // Read data from EEPROM to global pin variable
        g_c_pin[address_index] = EEDR;

    }
}

/*!
 * @brief Stores new PIN code to EEPROM and reads the PIN code from EEPROM
 * @param[in] new_pin_code Pointer to new PIN code inserted by the user
 */
void write_pin_eeprom(char * new_pin_code)
{
    for (uint8_t address_index = 0; address_index < MAX_SIZE; address_index++)
    {

        while (EECR & (1 << EEPE))
        {
            // Wait until previous write done
        }

        // Set index as EEPROM address 
        EEAR = address_index;

        // Add data to be written to EEPROM data register
        EEDR = new_pin_code[address_index];

        // Enable master programming
        EECR |= (1 << EEMPE);

        // Enable EEPROM programming
        EECR |= (1 << EEPE); 
    }

    // Read the new PIN code to Global variable
    read_pin_eeprom();
}

/*!
 * @brief Initializes the counter that is used for input timeout
 *
 */
void init_timeout_counter(void)
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

/*!
 * @brief Disables the counter that is used for input timeout
 *
 */
void disable_timer_counter(void)
{

    // Reset global timeout variable
    g_i_timeout = 0;

    // Reset TCCR1B counter
    TCCR1B = 0;

    // Disable global interrupts
    cli();
}

/*!
 * @brief Interrupt Service Routine to wake up the device from sleep. Disables the PCINT interrupts so Keypad works normally after wake up
 *
 */
ISR (PCINT2_vect)
{
    // Disable interrupts PCINT2 interrupts
    PCMSK2 &= ~(00000000);
    PCIFR &= ~(1 << PCIF2);

    // Disable sleep mode
    SMCR &= ~(1 << SE);
    _delay_ms(50);
}

/*!
 * @brief Interrupt Service Routine to timeout if no user input received.
 * It takes two cycles to timeout, i.e. around 8 secods
 *
 */
ISR (TIMER1_OVF_vect)
{
   g_i_timeout++;

    if(g_i_timeout == 2)
    {
        lcd_clrscr();
        lcd_puts("Timeout, no input\nreceived");
        g_b_timeout = true;
        g_i_timeout = 0;
        cli();
        _delay_ms(1000);
    }
}

/*!
 * @brief Enables the Standby mode for the device. Enables the interrups for PCINT16.
 *
 */
void go_standby_mode(void)
{
    lcd_clrscr();
    lcd_puts("Press '1'\nto wake up");

    // Check that there is no unexpected interrupt
    PCIFR |= (1 << PCIF2);

    // Enable Pin Change Interrupt 2
    PCICR |= (1 << PCIE2);

    // Enable Pin change mask register 2 interrupts for PCINT16 pin
    PCMSK2 |= (1 << PCINT16);

    // Enable global interrutps
    sei();

    // Enable Sleep mode
    SMCR |= (1 << SM1);
    _delay_ms(100);

    // Set Sleep enable bit
    SMCR |= (1 << SE);

    // Goto to sleep
    sleep_cpu();

}

int main (void)
{
    // Init Keypad
    KEYPAD_Init();
    
    // Delay 1 second to get the SPI to be enabled
    // Init SPI connection
    SPI_init();

    // Read the PIN code from EEPROM
    read_pin_eeprom();

    // Init LCD
    lcd_init(LCD_DISP_ON);
    lcd_clrscr();

    lcd_puts("Alarm System\n");
    lcd_puts("DEACTIVATED");
    _delay_ms(2000);
    lcd_clrscr();

    // Check that SPI connection is working
    SPI_connection_check();
    lcd_clrscr();
    while (!g_b_connection_status)
    {
        lcd_gotoxy(0,0);
        lcd_puts("SPI ERROR\nPress UNO reset");
        SPI_connection_check();
    }

    while (1)
    {
        uint8_t pin_status = 0;
        lcd_clrscr();

        // Show options to go menu or standby mode        
        lcd_puts("'#' for menu\n'*' for standby");
        char pressed_key = KEYPAD_GetKey();
        
        while (pressed_key != '*' && pressed_key != '#')
        {
            pressed_key = KEYPAD_GetKey();
        }

        if (pressed_key == '#')
        {
            // Go to MENU
            pin_status = show_menu();
            
            lcd_clrscr();
            lcd_puts("Alarm System");
            lcd_gotoxy(0, 1);
            
            if(pin_status == OK)
            {
                lcd_puts("PIN CORRECT");
                if(!g_b_alarm_active)
                {
                    SPI_master_tx_rx(ACTIVATE);
                    lcd_clrscr();
                    lcd_puts("Alarm Armed");
                    _delay_ms(1500);

                }
                else SPI_master_tx_rx(DEACTIVATE);
                g_b_alarm_active = !g_b_alarm_active;
                _delay_ms(1500);
            }
            else if(pin_status == CHANGED)
            {
              
                lcd_puts("PIN CHANGED");
                _delay_ms(1500);
            }
            else if(pin_status == NOK)
            {
                lcd_puts("PIN INCORRECT");
                _delay_ms(1500);
            }
            else
            {
                lcd_clrscr();
                lcd_puts("Unexcpected\nErro");
                _delay_ms(1500);
            }
        }
        else if(pressed_key == '*')
        {
            // Will go to standby mode
            _delay_ms(100);
            lcd_clrscr();
            
            lcd_puts("Standby");
            _delay_ms(1000);
            go_standby_mode();
        }
        else 
        {
            lcd_clrscr();
            lcd_puts("Unexpected\nERROR");
            _delay_ms(1500);
        }
        
    }
}


