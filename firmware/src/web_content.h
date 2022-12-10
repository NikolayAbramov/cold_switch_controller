/*
 * IncFile1.h
 *
 * Created: 04.09.2015 16:33:31
 *  Author: Николай
 */ 


#ifndef WEB_CONTENT_H_
#define WEB_CONTENT_H_
#endif 

extern uint16_t http200ok(void);
extern uint16_t moved_perm(uint8_t*, const char*);

uint16_t page_header(uint8_t*, uint16_t);
uint16_t print_table(uint8_t*, uint16_t, const char*, uint16_t, uint16_t, uint8_t);

extern uint16_t print_mainpage(uint8_t *, uint8_t);
extern int8_t chk_url_for_command(char *, uint16_t*, uint8_t);
extern uint8_t prepare_reset(char *);

extern uint16_t print_settings_page(uint8_t *);
extern int8_t edit_pulse( char * );

extern uint16_t print_sw_conf_page(uint8_t *, uint8_t, uint8_t);
void prepare_auto_detect();

extern uint16_t print_ip_mac_page(uint8_t *, uint8_t *, uint8_t *);
extern int8_t edit_net_sttings( char *, uint8_t *, uint8_t * );

extern uint16_t print_applied_page(uint8_t *, uint8_t *, uint8_t *);
extern uint16_t print_invalid_page(uint8_t *);