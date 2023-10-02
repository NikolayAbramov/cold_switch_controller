/*
 * tcpip_srv.h
 *
 * Created: 02.10.2023 0:22:08
 *  Author: Nikolay
 */ 

#ifndef TCPIP_SRV_H_
#define TCPIP_SRV_H_

//IP and MAC addresses
extern uint8_t myip[4];
extern uint8_t mymac[6];

#ifdef DEBUG
//enc26j60 fault counters
extern uint16_t econ1_rxen_fault_cnt;
extern uint16_t estat_bufer_fault_cnt;
extern uint16_t eir_rxerif_fault_cnt;
#endif

//Initializes tcp/ip server
extern void init_tcpip_server(void);
//Resets IP and MAC to defaults
extern void set_default_ip_mac(void);
extern void change_current_ip_mac(void);
extern void change_ip_mac(void);
//Periodically checks if this piece of crap doesn't freeze
extern void enc28J60_maintainance(uint32_t);

#endif /* TCPIP_SRV_H_ */