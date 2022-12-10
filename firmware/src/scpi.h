/*
 * IncFile1.h
 *
 * Created: 07.09.2015 19:17:30
 *  Author: Николай
 */
#include "cold_sw_drv.h"

#ifdef SCPI
extern uint16_t say_true(uint8_t *buf);
extern uint16_t say_false(uint8_t *buf);
extern uint16_t say_disabled(uint8_t *buf);

extern uint8_t parse_state(char *buf, uint16_t *channels);
extern uint16_t mk_state_response(uint8_t *buf , uint16_t *sw_state);
//extern uint8_t handle_chan(uint16_t *plen, uint8_t *buf, char *str, uint16_t *channels );
#endif