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

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <stdio.h>

// Parameters to handle SPI communication
//
#define ACTIVATE 5          // Activate Alarm
#define DEACTIVATE 2        // Deactivate
#define CHECK 3             // Connection check
#define EMPTY 255           // Empty data 
#define ACK 1               // ACK

// Global Variables for SPI communication
//
bool g_b_alarm_active = false;  // Global Variable for alarm status
int g_fail_counter = 0;         // SPI fail counter

/*!
 * @brief Initialize the PWM for Buzzer sound
 * 
 */
void set_timer_01(void)
{
    // Set up 16-bit timer
    TCCR1B = 0;     
    TCNT1 = 0; 

    // Set compare mode to toggle     
    TCCR1A |= (1 << 6); 

    // 523Hz sound without prescaler (Sound can be higher, but testing purpose keeped low)
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
    // Initialize SPI interface
    SPI_init();

    // Init PD7 (PIR Sensor) as output
    DDRD &= ~(1 << PD7);

    // Init PB1 as buzzer
    DDRB |= (1 << PB1);

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
