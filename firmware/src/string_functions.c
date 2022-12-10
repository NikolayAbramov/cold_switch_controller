/*
 * CProgram1.c
 *
 * Created: 07.09.2015 13:51:21
 *  Author: Николай
 */ 
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
// convert a single character to a 2 digit hex str
// a terminating '\0' is added
void int2h(char c, char *hstr)
{
        hstr[1]=(c & 0xf)+'0';
        if ((c & 0xf) >9){
                hstr[1]=(c & 0xf) - 10 + 'a';
        }
        c=(c>>4)&0xf;
        hstr[0]=c+'0';
        if (c > 9){
                hstr[0]=c - 10 + 'a';
        }
        hstr[2]='\0';
}

// convert a single hex digit character to its integer value
unsigned char h2int(char c)
{
        if (c >= '0' && c <='9'){
                return((unsigned char)c - '0');
        }
        if (c >= 'a' && c <='f'){
                return((unsigned char)c - 'a' + 10);
        }
        if (c >= 'A' && c <='F'){
                return((unsigned char)c - 'A' + 10);
        }
        return(0);
}
//converts a hex string to integer
uint16_t htoi(char *str)
{
		uint8_t i, len;
		uint16_t n=0, n1=1;
		
		len = strlen(str);
		for(i = 0; i<len; i++)
		{				
			n = n + (uint16_t)h2int(str[len-i-1])*n1;
			n1 = n1*16;
		}
		return(n);		
}
//Checks if str is decimal number
uint8_t isnum(char *str)
{
	while( *str != '\0' )
	{
		if( !(*str>='0' && *str<='9') )
			return 0;
		str++;			
	}
	return(1);		
}
//Parse list of characters into uint8_t array *buf
//mode = 2 for binary, 10 for decimal, 16 hex
//returns 1 on success  of 0 if fail
///////////////////////////////////
uint8_t parse_list(char *str, char delimiter, uint8_t *buf, int8_t mode, uint8_t nbytes)
{
	uint8_t i=0;//digit counter
	uint8_t j=0;//byte counter	
	int16_t n;
	char strbuf[4];
	char *str_begin;
	str_begin = str;
	//validate
	while(1){
		if( ((*str=='0' || *str=='1')&&(mode==2)) || (( *str>='0' && *str<='9' )&&(mode!=2)) || ( ( ( *str>='A' && *str<='F' )||( *str>='a' && *str<='f' ) ) && (mode==16) ) ){
			i++;
			if( ( i>1 && mode==2 ) || (i>3) || (i>2 && mode==16) )
				return(0);
			str++;
			continue;
		}			
		if( *str == delimiter ){
			j++;
			if(j >=nbytes || i==0 )
				return(0);	
			str++;
			i=0;	
			continue;	
		}
		if( (*str == '\0')||(*str == '\n') ){
			if(i==0)
				return(0);
			j++;
			if(j==nbytes)
				break;
		}			
		return(0);	
	}
	//Than parse
	str = str_begin;
	i=0;
	j=0;						
	while(1){
		if( (*str == delimiter) || (*str=='\0') || (*str == '\n') ){
			strbuf[i] = '\0';
			if(mode==16){
				n = htoi(strbuf);
			}else{
				n = (uint16_t)atoi(strbuf);
				if( n>255 )
					return(0);
			}
			buf[j] = (uint8_t)n;
			if(*str == delimiter){
				j++;
				i=0;
				str++;
				continue;
			}
			break;									
		}
		strbuf[i] = *str;
		i++;
		str++;
	}
	return(1);	
}

//make separated list from uint8_t array.
void mk_list(char *resultstr, uint8_t *bytes, uint8_t len, char separator, uint8_t base)
{
        uint8_t i=0;
        uint8_t j=0;
		
        while(i<len){
                itoa((int)bytes[i], &resultstr[j] , base);
				while(resultstr[j]){j++;};
                if (separator){
					resultstr[j]=separator;
					j++;
                }
                i++;
        }
        j--;
        resultstr[j]='\0';
}

uint8_t to_lower_case(char *str, char termchar, uint8_t maxlen)
{
	int i=0;
	
	while( (str[i]!=termchar)&&(i < maxlen) ){
		if( str[i]<='Z' && str[i]>='A')
			str[i] = str[i] -'A'+'a';
		i++;
	}
	return(i);
}