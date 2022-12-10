/*
 * Created: 27.04.2016 15:27:20
 *  Author: Nikolay
 */ 
#include <avr/io.h>
#include <stdlib.h>

//Rounds a fixed point number
uint32_t round_fix(uint32_t val, uint8_t exp)
{
	if (exp!=0){
		if( (val & ~(0xFFFFFFFF<<exp)) > (1<<(exp-1)) )
			val = (val>>exp)+1;
		else
			val = val>>exp;
	}			
	return(val); 
}