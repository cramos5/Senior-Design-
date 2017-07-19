#include <msp430.h>
unsigned volatile char generalMSG[70];

void uart_init(void)
{
	UCA0CTL0 = 0;
	UCA0CTL1 = UCSWRST;
	UCA0CTL1 |= UCSSEL_2;
	P1SEL = BIT1 + BIT2;
	P1SEL2= BIT1 + BIT2;
    UCA0BR0   =  6;                       // 1MHz 9600
    UCA0BR1   =  0;                       // 1MHz 9600
    UCA0MCTL |=  UCBRF_8 + UCOS16;        // Will Oversample, MOD = 8
    UCA0CTL1 &= ~UCSWRST;                 // **Initialize USCI state machine**
    IE2      &=  ~UCA0RXIE;                // Enable USCI_A0 RX Interrupt
	UCA0CTL1 &= ~UCSWRST;
}

void uartTx(unsigned volatile char loc){
	unsigned char data = generalMSG[loc];
	while(!(IFG2 & UCA0TXIFG));//Make Sure Buffer Is Empty
	UCA0TXBUF = data;
}

void sendMSG(unsigned volatile char len){
	int i = 0;
	while (i < len){
		uartTx(i);
		i++;
	}
}

unsigned volatile char api16Packet(unsigned volatile char data[], unsigned volatile char dataCount, unsigned volatile char transmissionPacket[]){

	int volatile i = 0;

	int volatile packetLoc = 0;
	unsigned volatile char checkSum;

	transmissionPacket[0] = 0x7E; //Start Delimiter

	transmissionPacket[1] = 0x00; //Length MSB
	transmissionPacket[2] = 0x00; //Length LSB, Will Be Reassigned After

	transmissionPacket[3] = 0x01; //Frame Type
	transmissionPacket[4] = 0x01; //ACK Requested

	transmissionPacket[5] = 0x00; //16 - Source Address MSB
	transmissionPacket[6] = 0x00; //16 - Source Address LSB

	transmissionPacket[7] = 0x00; //No options enables, maybe encryption later on

	packetLoc = 8;

	while(dataCount > 0){ //Adding Data To Packet
		transmissionPacket[packetLoc] = data[packetLoc-8];
		dataCount--;
		packetLoc++;
	}

	//Creating Checksum
	checkSum = 0;
	for(i = 3; i < packetLoc ; i++){
		checkSum = checkSum + transmissionPacket[i];
	}
	checkSum = 0xFF - (checkSum & 0xFF);

	//Adding Final Checksum
	transmissionPacket[packetLoc] = checkSum;
	transmissionPacket[2] = (packetLoc+1) - 4;//Adjusting Length

	return(packetLoc+1);
}

unsigned volatile char api16DataPacket(unsigned volatile char data[], unsigned volatile char dataCount, unsigned volatile char transmissionPacket[], unsigned volatile char type, unsigned volatile char count){

	int volatile i = 0;
	char volatile temp;
	int volatile packetLoc = 0;
	unsigned volatile char checkSum;

	transmissionPacket[0] = 0x7E; //Start Delimiter

	transmissionPacket[1] = 0x00; //Length MSB
	transmissionPacket[2] = 0x00; //Length LSB, Will Be Reassigned After

	transmissionPacket[3] = 0x01; //Frame Type
	transmissionPacket[4] = 0x01; //ACK Requested

	transmissionPacket[5] = 0x00; //16 - Source Address MSB
	transmissionPacket[6] = 0x00; //16 - Source Address LSB

	transmissionPacket[7] = 0x00; //No options enables, maybe encryption later on

	packetLoc = 8;

	temp = type >> 8;
	transmissionPacket[packetLoc] = temp;
	packetLoc++;
	transmissionPacket[packetLoc] = type;
	packetLoc++;

	temp = count >> 8;
	transmissionPacket[packetLoc] = temp;
	packetLoc++;
	transmissionPacket[packetLoc] = count;
	packetLoc++;

	while(dataCount > 0){ //Adding Data To Packet
		transmissionPacket[packetLoc] = data[packetLoc-12];
		dataCount--;
		packetLoc++;
	}

	//Creating Checksum
	checkSum = 0;
	for(i = 3; i < packetLoc ; i++){
		checkSum = checkSum + transmissionPacket[i];
	}
	checkSum = 0xFF - (checkSum & 0xFF);


	//Adding Final Checksum
	transmissionPacket[packetLoc] = checkSum;
	transmissionPacket[2] = (packetLoc+1) - 4;//Adjusting Length

	return(packetLoc+1);
}
