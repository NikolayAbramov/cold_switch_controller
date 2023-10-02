
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
// http://www.nongnu.org/avr-libc/changes-1.8.html:
#define __PROG_TYPES_COMPAT__
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "ip_arp_udp_tcp.h"
#include "websrv_help_functions.h"
#include "cold_sw_drv.h"
#include "enc28j60.h"
#include "net.h"

#include "timeout.h"
#include "drive.h"
#include "web_content.h"
#include "string_functions.h"
#include "scpi.h"
#include "fixed_point.h"
#include "tcpip_srv.h"

static uint8_t buf[BUFFER_SIZE+1];

//Returns 0 if nothing happened, 
//1 if button pressed, 
//2 if held for more than "hold" ticks,
// 3 if held for more than "hold" tics and released 
uint8_t handle_button(volatile uint8_t *port, uint8_t pins, uint8_t button_id, uint32_t hold)
{
	static uint32_t start_time;
	//Active button id
	static uint8_t button=0;
	
	if ( ( ( ~(*port) ) & pins) == pins ){
		if(button == 0){
			button = button_id;
			start_time = cpu_time();
			return(1);
		}			
		if( (button == button_id) && ((cpu_time() - start_time)>=hold ) ){
			start_time = cpu_time();
			return(2);
		}
		return(1);			
	}
	if( (button == button_id) && ((cpu_time() - start_time)>=hold ) ){
		button = 0;
		return(3);
	}		
	return(0);
}

int main(void){
	
		//Main state machine
        enum STATE {standby, do_switch, auto_detect, reset, delay} state, main_state;
		state = standby;
		main_state = standby;
		//do_switch - switch channel chan to new_sw_state
		
		//TCP packet length and pointer
		uint16_t plen;
        uint16_t dat_p;
		
		//Return value, multiple purpose
		uint8_t retval;
		
		//Wait until button release
		uint8_t vait_til_release = 0;
		
		//Disable watchdog as manual says
		wdt_disable();
		
        cli();
		
		sys_timer_conf();
		wait_for_vcc();
        _delay_loop_1(0); // 60us
		enc28j60HardReset();

		//Enable watchdog 
		wdt_enable(WDTO_2S);

		init_tcpip_server();
		// Reset to default button
        BUTTON_DDR &= ~ RST_TO_DEF_MSK;
        BUTTON_PORT|= RST_TO_DEF_MSK; // internal pullup resistor on
		
		drive_init();
		
		sei();

        while(1){
			//Reset watchdog
			wdt_reset();
			
			//Blink every 10 sec
			status_led_blinking(78000, 1500);
			
			//Handle blinks from everything else
			status_led_blink(0);
			
			//Check this piece of crap every 10s
			enc28J60_maintainance( ENC28J60_CHK_PERIOD );
			
			//Reset to default button
			if( (handle_button( &BUTTON_PORT_PIN, RST_TO_DEF_MSK, RST_TO_DEF, RST_TO_DEF_DELAY) == 2 )&&(!vait_til_release) ){
				set_default_ip_mac();
				vait_til_release = 1;
			}				
			if(vait_til_release){
				if (handle_button( &BUTTON_PORT_PIN, RST_TO_DEF_MSK, RST_TO_DEF, 1 ) == 0){
					vait_til_release = 0;
				}					
			}
			
			if((state == do_switch) || (state == reset) || (state == auto_detect) ){
				if( (state == reset) || (state == auto_detect) )
					retval = switch_relays(1);
				else
					retval = switch_relays(0);
					
				switch(retval)
				{
					//disconnected
					case 0: 
					{
						if(state == do_switch)
							new_sw_state = sw_state;
						sw_delay(0,1);
						state = delay;
						break;
					}
					// ok
					case 1:
					{
						sw_delay(0,1);
						state = delay;
						break;
					}
					//nothing to switch
					case 2:
					{
						//if(state==reset)
						//	sw_state = 0;
						if(state == auto_detect){
							enabled_pos = ~sw_state;
							sw_state = 0;
						}									
						state = standby;
						main_state =standby;
						break;
					}
				}	
			}
			
			if(state == delay){
				if( sw_delay(pulse.delay,0) )
					state = main_state;
			}									
					
			// handle ping and wait for a tcp packet
			plen = enc28j60PacketReceive(BUFFER_SIZE, buf);
			/*
			if (plen)
			{
				status_led_blink(1000);
			}
			*/
			buf[BUFFER_SIZE]='\0';
			dat_p = packetloop_arp_icmp_tcp(buf,plen);
			
			if(dat_p==0)
				continue;
			status_led_blink(1000);	
				
			//memcpy_P(buf, PSTR("GET /settings?width=1000&current=156&delay=36 HTTP/1.1"), 54);
			//memcpy_P(buf, PSTR("GET /ip_mac HTTP/1.1"), 20);
			//memcpy_P(buf, PSTR("GET / "), 6);
			
			// If GET				
			if( !strncmp("GET ",(char *)&(buf[dat_p]),4) ){
				//Main page
				if( !strncmp_P( (char *)&(buf[dat_p+4]),PSTR("/ "),2 ) ){
					if( state != standby)
						//lock main page
						plen = print_mainpage(buf, 1);
					else
						plen = print_mainpage(buf, 0);
					goto REPLY;
				}
				//Reset
				if( !strncmp_P( (char *)&(buf[dat_p+4]),PSTR("/reset?"),7 ) ){
					if( state == standby){
						if( prepare_reset((char *)&(buf[dat_p+4])) ){
							state = reset;
							main_state = reset;
						}
					}													
					plen=moved_perm(buf, PSTR("/"));						
					goto REPLY;
				}
				//Switch configuration page/sw_conf
				if( !strncmp_P( (char *)&(buf[dat_p+4]),PSTR("/sw_conf"),8 ) ){
					if( buf[dat_p+4+8] == ' ' ){
						if(main_state == auto_detect)
							plen = print_sw_conf_page(buf, 1,1);
						else{
							if(state == standby)
								plen = print_sw_conf_page(buf, 0,0);
							else
								plen = print_sw_conf_page(buf, 0,1);
						}							
						goto REPLY;						
					}
					if( buf[dat_p+4+8] == '?' ){
						if(state == standby)			
							chk_url_for_command( (char *)&(buf[dat_p+4]), &enabled_pos, 0);
						plen=moved_perm(buf, PSTR("/sw_conf"));
						goto REPLY;
					}
					if( !strncmp_P( (char *)&(buf[dat_p+4+8]),PSTR("/auto_detect"),11 )){
						if(state == standby){
							prepare_auto_detect();
							state = auto_detect;
							main_state = auto_detect;
						}							
						plen=moved_perm(buf, PSTR("/sw_conf"));						
						goto REPLY;
					}
				}
				//Pulse page					
				if( !strncmp_P( (char *)&(buf[dat_p+4]),PSTR("/settings"),9 ) ){
					if( buf[dat_p+4+9] == ' ' ){
						plen = print_settings_page(buf);
						goto REPLY;
					}
					if( buf[dat_p+4+9] == '?' ){
						if( state == standby){						 
							if( !edit_pulse( (char *)&(buf[dat_p+4]) ) )
								set_current();
						}													
						plen=moved_perm(buf, PSTR("/settings"));
						goto REPLY;
					}
				}
				//ip_mac page	
				if( !strncmp_P( (char *)&(buf[dat_p+4]),PSTR("/ip_mac"),7 ) ){
					if( buf[dat_p+4+7] == ' ' ){
						plen = print_ip_mac_page(buf, mymac, myip);
						goto REPLY;
					}
					if( buf[dat_p+4+7] == '?' ){
						if( state == standby){						 
							if( edit_net_sttings( (char *)&(buf[dat_p+4]), mymac, myip) ){
								change_current_ip_mac();
								plen = print_applied_page(buf, mymac, myip);
								www_server_reply(buf,plen, 1);
								//Change mac and ip
								change_ip_mac();
								continue;
							}						
							plen = print_invalid_page(buf);
						}else{
							plen=moved_perm(buf, PSTR("/ip_mac"));
						}
						goto REPLY;
					}
				}		
#ifdef DEBUG				
				//Debug page	
				if( !strncmp_P( (char *)&(buf[dat_p+4]),PSTR("/debug"),6 ) ){
					if( buf[dat_p+4+6] == ' ' ){
						plen = print_debug_page(buf);
						goto REPLY;
					}
				}
#endif											
				//Main page command handling
				if(state == standby){
					if( !strncmp_P( (char *)&(buf[dat_p+4]),PSTR("/?"),2 ) ){
						new_sw_state = sw_state;
						if(chk_url_for_command( (char *)&(buf[dat_p+4]), &new_sw_state, 1) )
							new_sw_state &= enabled_pos;
							if(new_sw_state != sw_state){
								state = do_switch;
								main_state = do_switch;
							}								
					}
				}										
				plen=moved_perm(buf, PSTR("/"));
REPLY:					
				www_server_reply(buf,plen, 1);
				continue;
			}	
			//SCPI
#ifdef SCPI			
			if(to_lower_case((char *)&(buf[dat_p]), '\n', MAX_SCPI_LEN) < MAX_SCPI_LEN){
				if( !strncmp_P((char *)&(buf[dat_p]),PSTR("*idn?\n"),6) ){
					plen = fill_tcp_data_p(buf,0,PSTR("Cold RF remote switch S.N. 1\n"));
					goto SCPI_REPLY;
				}
				
				if( !strncmp_P((char *)&(buf[dat_p]),PSTR("stat"),4) ){
					//query
					if( !(uint8_t)strncmp_P((char *)&(buf[dat_p+4]),PSTR("?\n"),2) ){
						plen = mk_state_response(buf , &sw_state);
						goto SCPI_REPLY;
					}
					//command
					if( buf[dat_p+4] == ' '){
						new_sw_state = sw_state;
						if( parse_state( (char*)&buf[dat_p+5], &new_sw_state ) ){
							new_sw_state &= enabled_pos;
							if(new_sw_state != sw_state){
								state = do_switch;
								main_state = do_switch;
								plen = say_true(buf);
							}else{
								plen = say_disabled(buf);
							}									
							goto SCPI_REPLY;
						}	
					}
				}
				//Set state without switching
				if( !strncmp_P((char *)&(buf[dat_p]),PSTR("set:stat"),8) ){
					//command
					if( buf[dat_p+8] == ' '){
						new_sw_state = sw_state;
						if( parse_state( (char*)&buf[dat_p+9], &new_sw_state ) ){
							new_sw_state &= enabled_pos;
							if(new_sw_state != sw_state){
								sw_state = new_sw_state;
								plen = say_true(buf);
							}
							else
							{
								plen = say_disabled(buf);
							}
							goto SCPI_REPLY;
						}
					}
					
				}						
				plen = say_false(buf);
				goto SCPI_REPLY;															
			}
			continue;		
SCPI_REPLY:																				
			www_server_reply(buf,plen,0);
#endif			
		}
return (0);
}