/* vim: set sw=8 ts=8 si et: */
#ifndef F_CPU
#define F_CPU 8000000UL
#else
#warning "F_CPU is already defined"
#endif

#ifndef ALIBC_OLD
#include <util/delay.h>
#else
#include <avr/delay.h>
#endif

#define PRESC 1024 //System timer prescaler
//#define TICKS(seconds) (uint16_t)(seconds*F_CPU/PRESC)

//ms to tics fixed point conversion
#define TICS_EXP 4
//TICS_CONST = 2^TICS_EXP*1e-3*F_CPU/PRESC)
//PRESC - system timer prescaler
//TICS  = ms*TICS_CONST/2^TICS_EXP
#define TICS_CONST 125

#define MAX_MS (uint16_t) (65536/TICS_CONST)
//System timer


extern void sys_timer_conf();
extern uint32_t cpu_time();
extern uint32_t elapsed_time(uint32_t);
