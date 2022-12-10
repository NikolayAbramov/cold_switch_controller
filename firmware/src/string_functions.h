/*
 * IncFile1.h
 *
 * Created: 07.09.2015 13:53:32
 *  Author: Николай
 */ 
extern unsigned char h2int(char c);
extern void int2h(char c, char *hstr);
extern uint16_t htoi(char *str);
extern uint8_t isnum(char *str);
extern uint8_t parse_list(char *str, char delimiter, uint8_t *buf, int8_t mode, uint8_t nbytes );
extern void mk_list(char *resultstr, uint8_t *bytes, uint8_t len, char separator, uint8_t base);
extern uint8_t to_lower_case(char *str, char termchar, uint8_t maxlen);