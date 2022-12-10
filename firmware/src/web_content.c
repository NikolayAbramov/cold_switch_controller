/*
 * web_content.c
 *
 * Created: 04.09.2015 16:32:53
 *  Author: Николай
 */ 

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
// http://www.nongnu.org/avr-libc/changes-1.8.html:
#define __PROG_TYPES_COMPAT__
#include <avr/pgmspace.h>

#include "ip_arp_udp_tcp.h"
#include "websrv_help_functions.h"
#include "cold_sw_drv.h"
#include "string_functions.h"
#include "drive.h"
#include "fixed_point.h"
#include "timeout.h"

uint16_t http200ok(uint8_t *buf)
{
        return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t moved_perm(uint8_t *buf, const char* location)
{
	uint16_t plen=0;
	
	plen=fill_tcp_data_p(buf,plen,PSTR("HTTP/1.0 301 Moved Permanently\r\nLocation: "));
	plen = fill_tcp_data_p(buf,plen,location);
	plen=fill_tcp_data_p(buf,plen,PSTR("\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"
		"301 Moved Permanently</h1>\n"));
	return(plen);
}

uint16_t page_header(uint8_t *buf, uint16_t plen)
{	
	plen = fill_tcp_data_p(buf, plen, PSTR("<h2>Cold RF switch</h2>"));
	return(plen);
}

uint16_t print_table(uint8_t *buf, uint16_t plen, const char* page, uint16_t data, uint16_t enabled, uint8_t locked)
{
	uint8_t i,j;
	uint16_t mask;
	char strbuf[8];
	
	plen = fill_tcp_data_p(buf, plen, PSTR("<pre>   "));
	//Table header Ch1 Ch2 ...
	for(j=0; j<NCHAN; j++)
	{
		itoa(j+1,strbuf,10);
		plen = fill_tcp_data_p(buf, plen, PSTR("\tCh"));
		plen = fill_tcp_data_len(buf,plen, (uint8_t*) strbuf, strlen(strbuf));
	}
	
	for(i=0; i<NPOS; i++)
	{	
		plen = fill_tcp_data_p(buf, plen, PSTR("\n "));
		itoa(i+1,strbuf,10);
		plen = fill_tcp_data_len(buf,plen, (uint8_t*) strbuf, strlen(strbuf));
		
		for(j=0; j<NCHAN; j++)
		{
			mask = ((uint16_t)1) << (i + j*NPOS);
			//Send switch position
			if(locked){
				if(enabled & mask){
					if(data & mask)
						plen=fill_tcp_data_p(buf,plen,PSTR("\t[ON]"));
					else
						plen=fill_tcp_data_p(buf,plen,PSTR("\t[x]"));
				}else{
					plen=fill_tcp_data_p(buf,plen,PSTR("\t    "));
				}											
			}else{
				if(enabled & mask){
					plen=fill_tcp_data_p(buf,plen,PSTR("\t<a href=\"."));
					plen=fill_tcp_data_p(buf,plen,page);
					plen=fill_tcp_data_p(buf,plen,PSTR("?pos="));
					itoa(i+1,strbuf,10);
					plen = fill_tcp_data_len( buf,plen, (uint8_t*) strbuf, strlen(strbuf) );
					//Than channel id
					plen=fill_tcp_data_p(buf,plen,PSTR("&ch="));
					itoa(j+1,strbuf,10);
					plen = fill_tcp_data_len( buf,plen, (uint8_t*) strbuf, strlen(strbuf) );
					//Than action to execute
					plen=fill_tcp_data_p(buf,plen,PSTR("&act="));
					if(data & mask)
					//OFF action if ON
						plen=fill_tcp_data_p(buf,plen,PSTR("0\">[ON]</a>"));
					else
						plen=fill_tcp_data_p(buf,plen,PSTR("1\">[x]</a>"));
				}else{
					plen=fill_tcp_data_p(buf,plen,PSTR("\t    "));
				}											
			}						
		}						
	}
	return(plen);
}

//If locked then all controls are locked. It's used during switching.

uint16_t print_mainpage(uint8_t *buf, uint8_t locked)
{
        uint16_t plen=0;
		uint8_t i;
		char strbuf[8];
		
        plen = http200ok(buf);
		plen = fill_tcp_data_p(buf, plen,PSTR("<style>form{display: inline-block;}</style>\n"));
		plen = page_header(buf, plen);
        plen = fill_tcp_data_p(buf, plen, PSTR("\n"));
		//Table
		plen = print_table(buf, plen, PSTR("/"), sw_state, enabled_pos, locked);
		//Reset buttons
		plen = fill_tcp_data_p(buf, plen,PSTR("\n<br>  \t"));
		for(i=1;i<=NCHAN;i++)
		{
			plen = fill_tcp_data_p(buf, plen,PSTR("<form action=/reset method=get><input type=hidden name=ch value=\""));
			itoa(i,strbuf,10);
			plen = fill_tcp_data_len(buf,plen, (uint8_t*) strbuf, strlen(strbuf));
			plen = fill_tcp_data_p(buf, plen,PSTR("\"><input type=submit value=\"Reset\""));
			if(locked)
				plen = fill_tcp_data_p(buf, plen,PSTR("disabled"));	
			plen = fill_tcp_data_p(buf, plen,PSTR("></form>\t"));
		}		
		
		if(locked){
			plen=fill_tcp_data_p(buf,plen,PSTR("\n<br>[Pulse parameters]\t[Switch configuration]\n"
				"<br>[IP and MAC]\t[Refresh]\n"
				"<br>Busy..."));
				
		}else{
			plen=fill_tcp_data_p(buf,plen,PSTR("\n<br><a href=\"./settings\">[Pulse parameters]</a>\t<a href=\"./sw_conf\">[Switch configuration]</a>\n"
				"<br><a href=\"./ip_mac\">[IP and MAC]</a>\t<a href=\".\">[Refresh]</a>\n"
				"<br>"));
		}
		plen=fill_tcp_data_p(buf,plen,PSTR("\n</pre><hr>N. Abramov, 2016. Based on <b>tuxgraphics</b> TCP/IP stack."));
						
		plen=fill_tcp_data_p(buf,plen,PSTR("\n<meta http-equiv=\"refresh\" content=\""));
		
		if(locked)
			plen=fill_tcp_data_p(buf,plen,PSTR("1"));
		else
			plen=fill_tcp_data_p(buf,plen,PSTR("3"));
				
		plen=fill_tcp_data_p(buf,plen,PSTR("\">"));	
		//Debug info
		/*
		itoa((int)TIFR1,strbuf,10);
		plen = fill_tcp_data_len(buf,plen, (uint8_t*) strbuf, strlen(strbuf));
		plen=fill_tcp_data_p(buf,plen,PSTR("  "));	
		
		itoa(plen,strbuf,10);
		plen = fill_tcp_data_len(buf,plen, (uint8_t*) strbuf, strlen(strbuf));
        */
		return(plen);
}

// Handle actions from main page
int8_t chk_url_for_command(char *str, uint16_t *target, uint8_t release)
{
        //Channel, position, action
		uint8_t chan, pos, act;
		char strbuf[3];
		
        if ( find_key_value(str, strbuf, 3, "ch") ){
			chan = atoi(strbuf)-1;
			if( (chan>=0) && (chan<NCHAN) ){
				if ( find_key_value(str, strbuf, 3, "pos") ){
					pos = atoi(strbuf)-1;
					if( (pos>=0) && (pos<NPOS) ){
						if( find_key_value(str, strbuf, 3, "act") ){
							act = atoi(strbuf);
							if( act==1 || act==0 ){	
								modify_sw_state(target, chan, pos, act, release);
								return (1);
							}						 
						}
					}		
				}
			}				
		}											
		return(0);
}

uint8_t prepare_reset(char *str)
{
	char strbuf[3];
	uint8_t chan, offset,i;
	uint16_t mask;
	
	if ( find_key_value(str, strbuf, 3, "ch") ){
		chan = atoi(strbuf)-1;
		if(chan<NCHAN){
			offset = chan*NPOS;
			new_sw_state = sw_state;
			for(i=0;i<NPOS;i++)
			{
				mask = 1<<(offset+i);
				sw_state |= mask;
				new_sw_state &= ~mask;
			}
			sw_state &= enabled_pos;			
			return(1);
		}				 
	}
return(0);			
}

uint16_t print_settings_page(uint8_t *buf)
{
	uint16_t plen=0;
	char strbuf[6];
	
    plen=http200ok(buf);
	plen = fill_tcp_data_p(buf, plen, PSTR("<h2>Cold RF switch</h2>\n<h3>Pulse parameters</h3><pre>\n"
		"<form action=/settings method=get>"
		"Width:\t\t"));
	itoa( pulse.phys_width, strbuf, 10);
    plen=fill_tcp_data(buf, plen, strbuf);
	plen = fill_tcp_data_p(buf, plen, PSTR("\t<input type=text size=5 name=width value=\""));
	plen = fill_tcp_data(buf, plen, strbuf);
	plen = fill_tcp_data_p(buf,plen,PSTR("\"> ms <="));
	itoa( MAX_WIDTH, strbuf, 10);
    plen=fill_tcp_data(buf, plen, strbuf);
	
	plen = fill_tcp_data_p(buf,plen,PSTR("<br>\nCurrent:\t"));
	itoa( pulse.phys_current, strbuf, 10);
    plen = fill_tcp_data(buf, plen, strbuf);	
	plen = fill_tcp_data_p(buf, plen, PSTR("\t<input type=text size=5 name=current value=\""));
    plen = fill_tcp_data(buf, plen, strbuf);
	
	plen = fill_tcp_data_p(buf,plen,PSTR("\"> mA <="));
	itoa( MAX_CURRENT, strbuf, 10);
    plen=fill_tcp_data(buf, plen, strbuf);
	
	plen = fill_tcp_data_p(buf,plen,PSTR("<br>\nDelay:\t\t"));
	itoa( pulse.phys_delay, strbuf, 10);
	plen=fill_tcp_data(buf, plen, strbuf);
	plen = fill_tcp_data_p(buf, plen, PSTR("\t<input type=text size=5 name=delay value=\""));
	plen=fill_tcp_data(buf, plen, strbuf);
	   
	plen=fill_tcp_data_p(buf,plen,PSTR("\"> ms <="));
	itoa( MAX_DELAY, strbuf, 10);
    plen=fill_tcp_data(buf, plen, strbuf);
	
	plen=fill_tcp_data_p(buf,plen,PSTR("<br>\n<input type=submit value=\"Apply\"></form><hr>"
		"<a href=\".\">[Home]</a>\t<a href=\"./settings\">[Refresh]</a>"));

    return(plen);
}

int8_t edit_pulse( char *str )
{
	pulse_str new_pulse;
	char strbuf[5];
	
	if( find_key_value(str, strbuf, 5, "width") ){
		if( !isnum(strbuf) )
			return(1);
		new_pulse.phys_width = atoi(strbuf);
		if(new_pulse.phys_width > MAX_WIDTH)
			return(1);
		new_pulse.width = round_fix( (uint32_t)new_pulse.phys_width*TICS_CONST, TICS_EXP);	
	}
	
	if( find_key_value(str, strbuf, 5, "delay") ){
		if( !isnum(strbuf) )
			return(2);
		new_pulse.phys_delay = atoi(strbuf);
		if(new_pulse.phys_delay > MAX_DELAY)
			return(2);
		new_pulse.delay = (uint16_t)round_fix( (uint32_t)new_pulse.phys_delay*TICS_CONST, TICS_EXP);
	}
	
	if( find_key_value(str, strbuf, 5, "current") ){
		if( !isnum(strbuf) )
			return(3);	
		new_pulse.phys_current = atoi(strbuf);
		if(new_pulse.phys_current>MAX_CURRENT)
			return(3);
		new_pulse.current = round_fix( (uint32_t)new_pulse.phys_current*PWM_CONST, PWM_EXP);
	}
	
	pulse = new_pulse;
	return(0);	
}

uint16_t print_sw_conf_page(uint8_t *buf, uint8_t upd, uint8_t locked)
{
	uint16_t plen=0;
	
	plen = http200ok(buf);
	plen = page_header(buf, plen);
    plen = fill_tcp_data_p(buf, plen, PSTR("<h3>Switch configuration</h3>\n"));
	plen = print_table(buf, plen, PSTR("/sw_conf"), enabled_pos, 0xFFFF, locked);
	
	plen = fill_tcp_data_p(buf, plen, PSTR("\n<br><form action=/sw_conf/auto_detect method=get><input type=submit value=\"Auto detect\" "));
	if(locked){
		plen = fill_tcp_data_p(buf, plen, PSTR("disabled></form>" 
												"\n<br>Busy..."
												"\n<hr>[Home]\t[Refresh]\n"));
	}else{
		plen = fill_tcp_data_p(buf, plen, PSTR("></form>"
												"\n<br>"
												"\n<hr><a href=\".\">[Home]</a>\t<a href=\"./sw_conf\">[Refresh]</a>\n"));
	}
	
	if(upd)
		plen=fill_tcp_data_p(buf,plen,PSTR("\n<meta http-equiv=\"refresh\" content=\"1\">"));			
    return(plen);
}

void prepare_auto_detect()
{
	uint8_t i;
	
	sw_state = 0;
	for(i=0;i<NPOS*NCHAN;i++)
		sw_state |= 1<<i;
	
	new_sw_state = 0;
}

uint16_t print_ip_mac_page(uint8_t *buf, uint8_t *mymac, uint8_t *myip)
{
        uint16_t plen=0;
		char strbuf[18];
		
        plen=http200ok(buf);
		plen = fill_tcp_data_p(buf, plen, PSTR("<h2>Cold RF switch</h2>\n<h3>Network settings</h3><pre>\n"
			"<form action=/ip_mac method=get>"
			"IP:\t<input type=text size=20 name=ip value=\""));
        
		mk_list(strbuf,myip,4,'.',10);
        plen=fill_tcp_data(buf,plen,strbuf);
        
		plen=fill_tcp_data_p(buf,plen,PSTR("\"><br>\n"
		"MAC:\t<input type=text size=20 name=mac value=\""));
        
		mk_list(strbuf,mymac,6,'.',16);
        plen=fill_tcp_data(buf,plen,strbuf);
        
		plen=fill_tcp_data_p(buf,plen,PSTR("\"><br>\n"
        "<input type=submit value=\"Apply\"></form><hr>"
		"<a href=\".\">[Home]</a>\t<a href=\"./ip_mac\">[Refresh]</a>"));

        return(plen);
}

int8_t edit_net_sttings( char *str, uint8_t *mymac, uint8_t *myip )
{
	char strbuf[18];
	
	if( find_key_value(str, strbuf, 18, "ip") ){
		if( !parse_list(strbuf, '.', myip, 10, 4)  )
			return 0;		
	}
	
	if( find_key_value(str, strbuf, 18, "mac") ){
		if( !parse_list(strbuf, '.', mymac, 16, 6) )
			return 0;
	}
	return(1);	
}

uint16_t print_applied_page(uint8_t *buf, uint8_t *mymac, uint8_t *myip)
{
	uint16_t plen;
	char strbuf[18];
	
	plen=http200ok(buf);
	plen = fill_tcp_data_p(buf, plen, PSTR("<h2>Cold RF switch</h2><pre>Network settings applied successfully.\n\nIP:\t"));
	mk_list(strbuf,myip,4,'.',10);
    plen=fill_tcp_data(buf,plen,strbuf);
	plen = fill_tcp_data_p(buf, plen, PSTR("\nMAC:\t"));
	mk_list(strbuf,mymac,6,'.',16);
    plen=fill_tcp_data(buf,plen,strbuf);
	
	plen = fill_tcp_data_p(buf, plen, PSTR( "<hr><a href=\"http://"));
	mk_list(strbuf,myip,4,'.',10);
    plen=fill_tcp_data(buf,plen,strbuf);
	plen = fill_tcp_data_p(buf, plen, PSTR("\">[Home]</a>"));
	return(plen);
}

uint16_t print_invalid_page(uint8_t *buf)
{
	uint16_t plen;
	plen=http200ok(buf);
	plen = fill_tcp_data_p(buf, plen, PSTR("<h2>Cold RF switch</h2><pre>Invalid value.<hr>"
	"<a href=\".\">[Home]</a>"));
	return(plen);
}