/*
 * main.c
 * Slave
 * Created: 04.04.2021
 * Author : Kariantti 
 */ 

#define F_CPU 16000000UL // Clock 16MHz
#define BAUD 9600 // BAUD speed for UART

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/setbaud.h>

bool g_b_alarm_active = true;

void uart_putchar(char c, FILE *stream);
char uart_getchar(FILE *stream);

void uart_init();
FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

FILE uart_io = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void 
set_timer_01(void)
{
    TCCR1B = 0; 
    TCNT1 = 0;
    TCCR1A |= (1 << 6);
    OCR1A = 15300;

}
void 
start_alarm_sound(void)
{
    if(g_b_alarm_active)
    {
        TCCR1B |= (1 << CS10);
    }
}
void
stop_alarm_sound(void)
{
    TCCR1B &= ~(1 << CS10);
}

void
SPI_init(void)
{
    // Set MISO output
    DDRB |= (1 << PB4);

    // Enable SPI and SPI interrupt
    SPCR |= (1 << SPE) | (1 << SPIE);
}

int
main (void)
{
    // Init PD7 (PIR Sensor) as output
    DDRD &= ~(1 << PD7);
    DDRB |= (1 << PB5);


    DDRB |= (1 << PB1);

    uart_init();
    SPI_init();

    set_timer_01();
    sei();

    while (1)
    {
        /* code */
        if(PIND & (1 << PD7))
        {
            PORTB |= (1 << PB5);
            start_alarm_sound();
        }
        else
        {
            PORTB &= ~(1 << PB5);
            stop_alarm_sound();
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