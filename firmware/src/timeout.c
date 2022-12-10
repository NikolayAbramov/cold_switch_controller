/*
 * Created: 23.05.2016 13:51:33
 *  Author: Nikolay
 */ 
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>

volatile uint32_t timer_base_count=0;
//Timer
ISR(TIMER1_OVF_vect)
{
	timer_base_count += 0xFFFF;
}

void sys_timer_conf()
{
	//Run 16bit timer1
	//clock = cpu_clock/1024
	//Allow overflow interrupt
	TCCR1B |= (1<<CS12) | (1<<CS10);
	TIMSK1 |= 1<<TOIE1;
}	

uint32_t cpu_time()
{
	return( timer_base_count + TCNT1);
}

uint32_t elapsed_time(uint32_t start_time)
{
	uint32_t time;
	uint32_t delta;
	
	time = cpu_time();
	if (start_time > time)
		delta = (0xFFFFFFFF-start_time)+time;
	else
		delta = time-start_time;
	
	return(delta);	
}