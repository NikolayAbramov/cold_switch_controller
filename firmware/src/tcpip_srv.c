/*
 * CFile1.c
 *
 * Created: 30.09.2023 22:16:55
 *  Author: Nikolay
 */ 

#include <avr/eeprom.h>
#include "cold_sw_drv.h"
#include "tcpip_srv.h"
#include "cold_sw_drv.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"
#include "timeout.h"

uint8_t defaultmac[6] EEMEM = DEFAULTMAC;
uint8_t defaultip[4] EEMEM = DEFAULTIP;

uint8_t currentip[4] EEMEM = DEFAULTIP;
uint8_t currentmac[6] EEMEM = DEFAULTMAC;

uint8_t myip[4];
uint8_t mymac[6];

#ifdef DEBUG
uint16_t econ1_rxen_fault_cnt = 0;
uint16_t estat_bufer_fault_cnt = 0;
uint16_t eir_rxerif_fault_cnt = 0;
#endif

//Load current IP and MAC addresses from EEPROM
void load_ip_mac(void)
{
	eeprom_read_block(myip, currentip, 4);
	eeprom_read_block(mymac, currentmac, 6);
}

void init_enc28j60(void)
{
	enc28j60Init(mymac);
	enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
	_delay_loop_1(0); // 60us
	
	/* Magjack leds configuration, see enc28j60 datasheet, page 11 */
	// LEDB=yellow LEDA=green
	//
	// 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
	// enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
	enc28j60PhyWrite(PHLCON,0x476);
}

//Initializes tcp/ip server
void init_tcpip_server(void)
{
	load_ip_mac();
	/*Initialize enc28j60*/
	init_enc28j60();
	//Init the web server ethernet/ip layer:
	init_udp_or_www_server(mymac,myip);
	www_server_port(MYWWWPORT);
}

void change_ip_mac(void)
{
	enc28J60WriteMAC (mymac);
	init_udp_or_www_server(mymac,myip);
}

//Update current IP and MAC in EEPROM
//from myip and mymac variables
void change_current_ip_mac(void)
{
	eeprom_update_block( myip,currentip, 4 );
	eeprom_update_block( mymac,currentmac, 6 );
}

//Resets IP and MAC to defaults
void set_default_ip_mac(void)
{
	eeprom_read_block(myip, defaultip, 4);
	eeprom_read_block(mymac, defaultmac, 6);
	eeprom_update_block(myip, currentip, 4);
	eeprom_update_block(mymac, currentmac, 6);
	change_ip_mac();
}

//Checks if enc28j60 it doesn't freeze
uint8_t enc28j60FreezCheck(void)
{
	uint8_t econ1_rxen;
	uint8_t estat_bufer;
	uint8_t eir_rxerif;
	
	econ1_rxen = enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_RXEN;
	estat_bufer = enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ESTAT) & ESTAT_BUFER;
	eir_rxerif = enc28j60ReadOp(ENC28J60_READ_CTRL_REG, EIR) & EIR_RXERIF;

#ifdef DEBUG
	if (!econ1_rxen)
		econ1_rxen_fault_cnt++;
	if (estat_bufer)
		estat_bufer_fault_cnt++;
	if (eir_rxerif)
		eir_rxerif_fault_cnt++;
#endif
	
	if (!econ1_rxen ||  (estat_bufer && eir_rxerif))
	{
		return 1;
	}
	return 0;
}

//Periodically checks if this piece of crap doesn't freeze
//And reset it if it does
void enc28J60_maintainance(uint32_t period)
{
	static uint32_t start_time = 0;
	
	if (elapsed_time(start_time) >  period)
	{
		start_time = cpu_time();
		if( enc28j60FreezCheck() )
		{
			enc28j60HardReset();
			init_enc28j60();
		}
	}
}
