
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
// http://www.nongnu.org/avr-libc/changes-1.8.html:
#define __PROG_TYPES_COMPAT__
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
//#include <avr/wdt.h>
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


uint8_t defaultmac[6] EEMEM = DEFAULTMAC;
uint8_t defaultip[4] EEMEM = DEFAULTIP;

uint8_t currentip[4] EEMEM = DEFAULTIP;
uint8_t currentmac[6] EEMEM = DEFAULTMAC;

uint8_t myip[4];
uint8_t mymac[6];	

static uint8_t buf[BUFFER_SIZE+1];

void change_ip_mac(void)
{
	enc28J60WriteMAC (mymac);
	init_udp_or_www_server(mymac,myip);
}

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
		
		//TCP packet lenght and pointer
		uint16_t plen;
        uint16_t dat_p;
		
		//Return value, multiple purpose
		uint8_t retval;
		
		//Wait until button release
		uint8_t vait_til_release = 0;
		
		//Disable watchdog
		//wdt_disable();
		
        cli();
		
		sys_timer_conf();
		wait_for_vcc();
		
        _delay_loop_1(0); // 60us
		
		eeprom_read_block(myip, currentip, 4);
		eeprom_read_block(mymac, currentmac, 6);
		
		//Enable watchdog 
		//wdt_enable(WDTO_2S);
		
        /*initialize enc28j60*/
        enc28j60Init(mymac);
        enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
        _delay_loop_1(0); // 60us
        
        /* Magjack leds configuration, see enc28j60 datasheet, page 11 */
        // LEDB=yellow LEDA=green
        //
        // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
        // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
        enc28j60PhyWrite(PHLCON,0x476);
        
        //init the web server ethernet/ip layer:
        init_udp_or_www_server(mymac,myip);
        www_server_port(MYWWWPORT);
		
		// Reset to default button
        BUTTON_DDR &= ~ RST_TO_DEF_MSK;
        BUTTON_PORT|= RST_TO_DEF_MSK; // internal pullup resistor on
		
		drive_init();
		
		sei();

        while(1){
			//Reset watchdog
			//wdt_reset();
			//Reset to default button
			if( (handle_button( &BUTTON_PORT_PIN, RST_TO_DEF_MSK, RST_TO_DEF, RST_TO_DEF_DELAY) == 2 )&&(!vait_til_release) ){
				eeprom_read_block(myip, defaultip, 4);
				eeprom_read_block(mymac, defaultmac, 6);
				eeprom_update_block(myip, currentip, 4);
				eeprom_update_block(mymac, currentmac, 6);
				change_ip_mac();
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
			buf[BUFFER_SIZE]='\0';
			dat_p = packetloop_arp_icmp_tcp(buf,plen);
			
			if(dat_p==0)
				continue;
				
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
								eeprom_update_block( myip,currentip, 4 );
								eeprom_update_block( mymac,currentmac, 6 );
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
				plen = say_false(buf);
				goto SCPI_REPLY;															
			}
					/*
					if( !strncmp_P((char *)&(buf[dat_p]),PSTR("chan"),4) ){
						if( handle_chan( &plen, buf, (char *)&(buf[dat_p+4]) , &channels ) )
							switch_relays(channels);
						goto SCPI_REPLY;	
					}*/
			continue;		
SCPI_REPLY:																				
			www_server_reply(buf,plen,0);
#endif			
		}
return (0);
}