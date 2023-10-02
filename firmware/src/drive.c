/*
 * CProgram1.c
 *
 * Created: 22.04.2016 16:11:34
 *  Author: Nikolay
 */ 
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "drive.h"
#include "cold_sw_drv.h"
#include "timeout.h"

pulse_str stored_pulse EEMEM = {78,0,41, 10,0,200};
uint16_t stored_sw_state EEMEM = 0;	
uint16_t stored_enabled_pos EEMEM = 0xFFFF;
//93.5 ma
uint8_t	current_offset EEMEM = 19;

pulse_str pulse;
uint16_t sw_state;
uint16_t new_sw_state;
uint16_t enabled_pos;

uint8_t pulse_OK;
	
ISR(TIMER1_COMPA_vect)
{
	//Finish pulse
	DRV_PORT |= 1<<LC;
	DRV_PORT &= ~(1<<LC);
	//Disable interrupt
	TIMSK1 &= ~(1<<OCIE1A);
}

ISR(INT0_vect)
{
	//Low VCC signal from MAX809
	save_data();
	wait_for_vcc();
}	

ISR(PCINT2_vect)
{
	//Current comparator signal
	pulse_OK = 1;
}

void drive_init()
{
	//Relay drive
	//PC0 - SDI
	//PC1 - LC
	//PC2 - SC
	//PC3 - LED
	DDRC = 0b00001111;
	//PWM output OC2B for current limit
	DDRD |= 1<<PORTD3;
	TCCR2A |= (1<<WGM20) | (1<<WGM21);
	TCCR2A |= 1<<COM2B1;
	PWM = 0;
	TCCR2B |= 1<<CS20;
	//Low VCC interrupt on falling edge
	EICRA |= (1<<ISC01);
	EIMSK |= 1<<INT0;
	//Current comparator interrupt
	PCMSK2 |= 1<<PCINT20;
	
	load_data();
	set_current();
}	

void set_current()
{
	uint8_t offset;
	
	offset = eeprom_read_byte( (const uint8_t*)&current_offset );
	PWM = pulse.current + offset;
}	
	
void load_data()
{
	eeprom_read_block( (void*)&pulse, (const void*)&stored_pulse, sizeof(pulse_str) );
	sw_state = eeprom_read_word( (const uint16_t*)&stored_sw_state );
	enabled_pos = eeprom_read_word( (const uint16_t*)&stored_enabled_pos );
}

void save_data()
{
	eeprom_update_block( (const void*)&pulse, (void*)&stored_pulse, sizeof(pulse_str) );
	eeprom_update_word( &stored_sw_state, sw_state);
	eeprom_update_word( &stored_enabled_pos, enabled_pos);
}

uint8_t do_pulse(uint8_t chan, uint8_t pos, uint8_t act)
{
	uint8_t begin, offset, i;

	begin = chan*NPOS*2;
//	end = begin + NPOS*2;
	offset = begin+pos*2;

	if(act)
		DRV_PORT &= ~(1<<SDI);
	else
		DRV_PORT |= 1<<SDI;
	
	DRV_PORT |= 1<<SC;
	DRV_PORT &= ~(1<<SC);	
		
	for(i=NCHAN*NPOS*2; i>0; i--)
	{
		if( ( ( (i-1) ==offset) && act ) || ( (i-1) == offset+1 ) )
			DRV_PORT |= 1<<SDI;
		else	
			DRV_PORT &= ~(1<<SDI);
		
		DRV_PORT |= 1<<SC;
		DRV_PORT &= ~(1<<SC);							
	}
	
	//Enable current comparator interrupt
	pulse_OK = 0;
	PCIFR |= 1<<PCIF2;
	PCICR |= 1<<PCIE2;
	
	//Load timer1
	OCR1A = TCNT1 + pulse.width;
	//Start pulse
	DRV_PORT |= 1<<LC;
	DRV_PORT &= ~(1<<LC);
	
	//Preload data to stop pulse
	for(i=0;i<=NPOS*2*NCHAN;i++)
	{
		DRV_PORT &= ~(1<<SDI);
		DRV_PORT |= 1<<SC;
		DRV_PORT &= ~(1<<SC);										
	}
	//Allow interrupt
	TIFR1 = 1<<OCF1A;
	TIMSK1 |= 1<<OCIE1A;
	//Wait
	while(TIMSK1 & (1<<OCIE1A)){
		}
	//Disable current comparator interrupt
	PCICR &= ~(1<<PCIE2);
	return(pulse_OK);
}	

uint8_t switch_relays(uint8_t skip)
{
	//Performs physical switching of relays
	//Changes switch state from sw_state to new_sw_state
	//
	//skip == 1 - force disconntcted coils to state 0 in new_state. 
	//This way we can reset all of then and find ones which are actually connected.  
	//
	//skip == 0 - left disconntcted coils in new_state as they are in sw_state
	//
	//return value:
	//0 coil disconnected
	//1 OK
	//2 nothing to switch sw_state == new_sw_state
	uint8_t i,j, offset;
	
	for(i=0;i<NCHAN;i++)
	{
		for(j=0;j<NPOS;j++)
		{
			offset = j+i*NPOS;
			//ON					 		
			if( ( new_sw_state & 1<<offset ) && !( sw_state & 1<<offset ) ){
				if( do_pulse(i, j, 1) ){
					sw_state |= 1<<offset;
					return(1);
				}else{
					return(0);
				}						
			}				
			//OFF
			if( ( sw_state & 1<<offset ) && !( new_sw_state & 1<<offset ) ){
				if( do_pulse(i, j, 0) ){
					sw_state &= ~(1<<offset);
					return(1);
				}else{
					//for reset
					if(skip)
						new_sw_state |= 1<<offset;
					return(0);
				}						
			}
		}
	}
	return(2);		
}

uint8_t sw_delay(uint16_t delay, uint8_t act)
{
	//act = 1  - set start time
	//act = 0 - wait 
	static uint32_t start_time;
	
	if(act > 0){
		start_time = cpu_time();
		return(1);
	}
	
	if( elapsed_time(start_time) >= delay)
		return(1);
	
	return(0);
}

//Modify target switch state according to chan pos and act
//Typically *target points to new_sw_state in main()
//Initially new_sw_state=sw_state which is the current switch state
//If release if 1 then previously connected positions will be set 0 in new_sw_state
void modify_sw_state(uint16_t *target, uint8_t chan, uint8_t pos, uint8_t act, uint8_t release)
{
	uint8_t offset;
	//Channel mask	
	uint16_t mask; 
	mask = CHAN_MASK();
	
	offset = pos+NPOS*chan;
	
	if( act ){
		*target |= (1 << offset);
		if(release){
			mask = ~(mask << (NPOS*chan));
			mask |= (1 << offset);		 
			*target &= mask;
		}			
	}else{		
		*target &= ~(1 << offset );
	}		
}

void chan_off(uint16_t *target, uint8_t chan)
{
	//Channel mask	
	uint16_t mask; 
	mask = CHAN_MASK();
	mask = ~(mask << (NPOS*chan));
	*target &= mask;
}

void wait_for_vcc()
{
	uint32_t start_time;
	start_time = cpu_time();
		while(1)
		{
			if( elapsed_time(start_time)<LOW_VCC_TIMEOUT){
				if(PIND & 1<<PIND2)
					break;
				else
					start_time = cpu_time();
			}				
		}
}

void status_led_on(void)
{
	PORTC |= 1<<PORTC3;
}

uint8_t status_led_is_on(void)
{
	if ( PINC & (1<<PORTC3) )
	{
		return 1;
	}
	return 0;
}

void status_led_off(void)
{
	PORTC &= ~(1<<PORTC3);
}

void status_led_blinking(uint32_t period, uint32_t duration)
{
	static uint32_t led_start_time = 0;
	static uint8_t led_is_on = 0;
	
	if( led_is_on )
	{
		if (cpu_time() - led_start_time >  duration)
		{
			status_led_off();
			led_is_on = 0;
		}
	}
	 
	if (cpu_time() - led_start_time >  period)
	{
		status_led_on();
		led_start_time = cpu_time();
		led_is_on = 1;
	}
}

//Single blink 
void status_led_blink(uint32_t delay)
{
	static uint32_t led_start_time = 0 ;
	static uint32_t captured_delay = 0 ;
	static uint8_t led_is_on = 0;
	
	if ( delay )
	{
		status_led_on();
		led_start_time = cpu_time();
		captured_delay = delay;
		led_is_on = 1;
	} 
	else
	{
		if ( cpu_time() - led_start_time >  captured_delay )
		{
			if ( led_is_on )
			{
				status_led_off();
				led_is_on = 0;
			}
		}
	}
}