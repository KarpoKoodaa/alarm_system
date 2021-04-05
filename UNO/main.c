/*
 * main.c
 * Slave
 * Created: 04.04.2021
 * Author : Kariantti 
 */ 

#define F_CPU 16000000UL //Clock 16MHz

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/interrupt.h>

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
    TCCR1B |= (1 << CS10);
}
void
stop_alarm_sound(void)
{
    TCCR1B &= ~(1 << CS10);
}

int
main (void)
{
    // Init PD7 (PIR Sensor) as output
    DDRD &= ~(1 << PD7);
    DDRB |= (1 << PB5);


    DDRB |= (1 << PB1);

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