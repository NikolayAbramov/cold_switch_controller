/*
 * IncFile1.h
 *
 * Created: 22.04.2016 16:12:38
 *  Author: Nikolay
 */ 
// User defined limits
#define MAX_WIDTH 500//ms
#define MAX_DELAY 7000//ms
#define MAX_CURRENT 900//mA

//Relay driver
#define DRV_PORT PORTC
#define SDI 0
#define SC 2
#define LC 1

//Current control
#define PWM OCR2B
//Current mA to 8bit PWM value fixed point conversion
#define PWM_EXP 11
//PWM_CONST = 2^PWM_EXP*256.0*RSENS/(VCC*1000)
////RSENS - current sens resistor, Ohm
//PWM = I*PWM_CONST/2^PWM_EXP
//I in mA
#define PWM_CONST 419
//Maximal possible current in mA
//#define I_MAX (uint16_t) (65536/PWM_CONST)
//100ms
#define LOW_VCC_TIMEOUT 800
#define CHAN_MASK() UINT16_MAX >> (16 - NPOS)

typedef struct{
uint16_t width;
uint16_t delay;
uint16_t current;
//Natural units
uint16_t phys_width; //ms
uint16_t phys_delay; //ms
uint16_t phys_current; //mA
	
} pulse_str;

extern pulse_str pulse;
//Relay state
extern uint16_t sw_state;
extern uint16_t new_sw_state;
extern uint16_t enabled_pos;

void drive_init();
extern void set_current();

void load_data();
void save_data();

uint8_t do_pulse(uint8_t, uint8_t, uint8_t);

extern uint8_t switch_relays(uint8_t);
extern uint8_t sw_delay(uint16_t, uint8_t);

extern void modify_sw_state(uint16_t*, uint8_t, uint8_t, uint8_t, uint8_t);
extern void chan_off(uint16_t*, uint8_t);

extern void wait_for_vcc();

extern void status_led_on();
extern uint8_t status_led_is_on();
extern void status_led_off();
extern void status_led_blinking ( uint32_t, uint32_t );
extern void status_led_blink ( uint32_t );