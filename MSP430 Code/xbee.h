extern unsigned volatile char generalMSG[100];

unsigned volatile char api16Packet(unsigned volatile char data[], unsigned volatile char dataCount, unsigned volatile char transmissionPacket[]);

unsigned volatile char api16DataPacket(unsigned volatile char data[], unsigned volatile char dataCount, unsigned volatile char transmissionPacket[], unsigned volatile char type, unsigned volatile char count);

void uart_init( void );

void uartTx(unsigned volatile char loc);

void sendMSG(unsigned volatile char len);

