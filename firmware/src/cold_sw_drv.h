/*
 * Created: 04.09.2015 18:22:47
 *  Author: Николай
 */ 
#define SCPI

#define DEFAULTIP {10,20,61,223}
#define DEFAULTMAC {0x67,0x43,0x19,0x79,0x01,0x36}
	
// listen port for tcp/www:
#define MYWWWPORT 80

// listen port for udp
#define MYUDPPORT 1200
//Web page buffer size
#define BUFFER_SIZE 1500

//Maximal scpi command lenght. Must be <= BUFFER_SIZE
#define MAX_SCPI_LEN 255

//Buttons
#define BUTTON_PORT_PIN PINB
#define BUTTON_PORT PORTB
#define BUTTON_DDR DDRB

//Reset to default button. Resets IP and MAC.
#define RST_TO_DEF 1 //Button ID
#define RST_TO_DEF_MSK 1 //Port mask
#define RST_TO_DEF_DELAY 23437// 3 sconds

//Number of channels 
#define NCHAN 2
//Number of relay positions per channel
#define NPOS 6