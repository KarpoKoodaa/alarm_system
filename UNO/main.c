/*
 * This is an exercise work of Introduction to Embedded Systems course.
 * The alarm system is build using Arduino MEGA 2560 (ATmega2560) and Arduino UNO (ATmega328p)
 *
 * main.c
 * 
 * Slave device
 * 
 * Created: 04.04.2021
 * 
 * Author : Kariantti Laitala
 */ 

#define F_CPU 16000000UL    // Clock 16MHz
#define BAUD 9600           // BAUD speed for UART

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/setbaud.h>

// Parameters to handle SPI communication
//
#define ACTIVATE 1          // Activate Alarm
#define DEACTIVATE 2        // Deactivate
#define CHECK 3             // Connection check
#define EMPTY 255           // Empty data 
#define ACK 1               // ACK

// Global Variables for SPI communication
//
bool g_b_alarm_active = false;  // Global Variable for alarm status
int g_fail_counter = 0;         // SPI fail counter


// This one needs some clean up
//
void uart_putchar(char c, FILE *stream);
char uart_getchar(FILE *stream);

void uart_init();
FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

FILE uart_io = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

/*!
 * @brief Initialize the PWM for Buzzer sound
 * TODO: Comments into code
 */
void set_timer_01(void)
{
    TCCR1B = 0; 
    TCNT1 = 0;
    TCCR1A |= (1 << 6);
    OCR1A = 15300;

}

/*!
 * @brief Activates the buzzer sound 
 *
 */
void start_alarm_sound(void)
{
    if(g_b_alarm_active)
    {
        TCCR1B |= (1 << CS10);
    }
}

/*!
 * @brief Stops the alarm sound if alarm is deactivated
 *
 */
void stop_alarm_sound(void)
{
    TCCR1B &= ~(1 << CS10);
}

/*!
 * @brief Initializes the SPI connection and sets device as Slave
 *
 */
void SPI_init(void)
{
    // Set MISO output
    DDRB |= (1 << PB4);

    // Enable SPI and SPI interrupt
    SPCR |= (1 << SPE) | (1 << SPIE);
}

/*!
 * @brief Transmits and receives data over the SPI
 * @param[in] data Data that is sent over SPI to Master
 * 
 */
void SPI_slave_tx_rx(uint8_t data)
{
    // Add data to SPI register
    SPDR = data;
}

/*!
 * @brief Interrupt Service Routine to receive the data and sets response message to SPDR register
 *
 */
ISR (SPI_STC_vect)
{
    uint8_t received_data = SPDR;

    if (received_data == ACTIVATE)
    {
        // Arm the alarm system
        g_b_alarm_active = true;
        printf("Alarm activated, %d\n", g_b_alarm_active);
        SPI_slave_tx_rx(ACK);
    }
    else if (received_data == DEACTIVATE)
    {
        // Dearm the alarm system
        g_b_alarm_active = false;

        // Deactivate the PIR sensor
        PORTB &= ~(1 << PB5);

        // Stop the alarm sound
        stop_alarm_sound();
        printf("Alarm deactivated, %d\n", g_b_alarm_active);

        // Send ACK to Master
        SPI_slave_tx_rx(ACK);
    }
    else if ((received_data == CHECK) || (received_data == EMPTY))
    {
        // Send ACK to Master if check is requested
        SPI_slave_tx_rx(ACK);
        
    }
    else
    {
       // Increase Fail counter if incorrect data received
       // Is used for debugging if needed
        g_fail_counter++;
    }
   
}

int main (void)
{
    // Init PD7 (PIR Sensor) as output
    DDRD &= ~(1 << PD7);

    // Why this??
  //  DDRB |= (1 << PB5);

    // Init PB1 as buzzer
    DDRB |= (1 << PB1);

    //uart_init();

    // Initialize SPI interface
    SPI_init();

	// Start PWM to make the buzzer sound if alarm activates
    set_timer_01();

    sei();

    while (1)
    {
        // Activate Buzzer if PIR sensor active
        if(PIND & (1 << PD7))
        {
            PORTB |= (1 << PB5);
            start_alarm_sound();
        }
        else
        {
            // Clear the PIR sensor
            PORTB &= ~(1 << PB5);
        }
    }
    
}


// UART for Troubleshooting

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
    if(c == '\n'){
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
