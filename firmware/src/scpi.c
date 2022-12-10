/*
 * CProgram1.c
 *
 * Created: 07.09.2015 19:17:10
 *  Author: Николай
 */ 

#include <stdlib.h>
#include <avr/io.h>
#include <string.h>
#include "ip_arp_udp_tcp.h"
#include "string_functions.h"
#include "cold_sw_drv.h"
#include "drive.h"

#ifdef SCPI
uint16_t say_true(uint8_t *buf){
	return( fill_tcp_data_p(buf,0,PSTR("True\n")) );	
}

uint16_t say_false(uint8_t *buf){
	return( fill_tcp_data_p(buf,0,PSTR("False\n")) );	
}

uint16_t say_disabled(uint8_t *buf){
	return( fill_tcp_data_p(buf,0,PSTR("Disabled\n")) );	
}

uint8_t parse_state(char *buf, uint16_t *target)
{
	uint8_t num_buf[2];
	
	if( parse_list( buf, ',' , num_buf, 10, 2) ){
		if( num_buf[0]<=NCHAN && num_buf[0]!=0  ){
			if( num_buf[1] <= NPOS && num_buf[1]!=0){
				modify_sw_state(target, num_buf[0]-1, num_buf[1]-1, 1, 1);
				return(1);
			}
			if( num_buf[1] == 0){
				chan_off(target, num_buf[0]);
				return(1);
			}											
		}			
	}
	return(0);
}

uint16_t mk_state_response(uint8_t *buf , uint16_t *sw_state)
{
	char str[4*NCHAN];
	char strbuf[5];
	
	uint8_t i, j, k=0, stat;
	
	uint16_t plen=0;
	
	for(i=0;i<NCHAN;i++){
		
		itoa(i+1,strbuf,10);
		strcpy(str+k, strbuf);
		k+=strlen(strbuf);
		str[k] = ',';
		k++;
		
		stat = 0;
		for(j=0;j<NPOS;j++)
		{
			if( *sw_state & (1<<(i*NPOS+j)) )
			{
				stat = j+1;
				break;
			}					
		}
		
		itoa(stat,strbuf,10);
		strcpy(str+k, strbuf);
		k+=strlen(strbuf);
		str[k] = ',';
		k++;
	}
	str[k-1]='\0';
	plen=fill_tcp_data(buf,plen,str);
	return(plen);
}
/*
//Handles commands chan[n]?\n, chan[n] [on;off]\n
//Returns 1 if switching of relays needed, 0 if not 
uint8_t handle_chan(uint16_t *plen, uint8_t *buf, char *str, uint16_t *channels ){
	
	char strbuf[3];
	uint8_t i=0;
	
	while(1){
		if( (*str<='9') && (*str>='0') ){
			if(i>1)
				goto RET_F;
			strbuf[i] = *str;
			i++;
			str++;
			continue;	
		}
		if(   (*str == '?')||(*str == ' ')  ){
			strbuf[i]='\0';
			break;
		}
		goto RET_F;
	}
	//now i is channel number
	i = (uint8_t)atoi(strbuf)-1;
	if(i>NCHAN-1)
		goto RET_F;
	//if query	
	if(   (*str =='?')&&(*(str+1)=='\n')  ){	
		if ( *channels & (((uint16_t)1)<< i) )
			*plen = fill_tcp_data_p(buf,0,PSTR("ON\n"));	
		else	
			*plen = fill_tcp_data_p(buf,0,PSTR("OFF\n"));
		return(0);	
	}
	//if command
	if( !strncmp_P(str,PSTR(" on\n"),5) ){
		*channels|= (((uint16_t)1)<< i);		
		goto RET_T;
	}		
	if( !strncmp_P(str,PSTR(" off\n"),5) ){
		*channels&= ~(((uint16_t)1)<< i);
		goto RET_T;
	}
RET_F:			
	*plen = say_false(buf);
	return(0);
RET_T:
	*plen = say_true(buf);
	return(1);			
}
*/
#endif