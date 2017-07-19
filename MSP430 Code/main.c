#include <msp430.h>
#include "MAX30100.h"
#include "xbee.h"
/*
 * main.c
 */

//***XBEE Variables***//
unsigned char volatile connectData[7] = {0x43, 0x4f, 0x4e, 0x4e, 0x45, 0x43, 0x54};
unsigned char volatile type = 1; //0 - Connection Request Message
					   //1 - Data Message
//General Packet
unsigned volatile char packetLength;
//Control Variables for Response MSG
unsigned volatile char respLoc = 0;
unsigned volatile char respLength = 0;
unsigned volatile char respData;
unsigned volatile char inTrans = 0;
//ACK Control Variables
unsigned volatile char msgReceived = 0;//0 Means NACK, 1 is ACK
unsigned volatile char countNotReceived = 0; //Counts how many messages are not received in a row
//Data Transmit Control
unsigned volatile char msgsSent = 0;
unsigned const char msgsMax = 6;

//***MAX30100 Variables***/
volatile unsigned char ir_buffer[152];
volatile unsigned char red_buffer[152];  //red LED sensor data
unsigned volatile char status1;
unsigned volatile char status2;
unsigned volatile char tempInt = 0;
unsigned volatile char tempFrac = 0;
unsigned volatile char track = 0;

#pragma vector = TIMER0_A1_VECTOR
__interrupt void timer5()
{
	TA0CTL &= ~TAIFG;

	if (type == 0){//Send Connection Request Message
		P2OUT &= ~BIT0; //Wake Up XBEE
		__delay_cycles(13000);//13mSec
		packetLength = api16Packet(connectData, 7, generalMSG);
		sendMSG(packetLength);
		IE2 |= UCA0RXIE;
		return;
	}

	if(type == 1){
		track = 0;
		maxim_max30100_read_reg(REG_INTR_STATUS_1,&status1);
		maxim_max30100_read_reg(REG_INTR_STATUS_2,&status2);
		maxim_max30100_write_reg(REG_TEMP_CONFIG, 0x01);
		P1IE |= BIT3;
		return;
	}
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
	inTrans = 1;
	respLoc = 0;
	while(inTrans == 1){
		while(!(IFG2 & UCA0RXIFG));
		respData = UCA0RXBUF;

		if(respLoc == 0 && respData == 0x7E){
			generalMSG[respLoc] = respData;
			respLoc++;
		}

		else if(respLoc == 1){
			generalMSG[respLoc] = respData;
			respLoc++;
		}
		else if(respLoc == 2){
			generalMSG[respLoc] = respData;
			respLoc++;
			respLength = respData + 4;
		}
		else{
			generalMSG[respLoc] = respData;
			respLoc++;
		}
		if(respLength == respLoc){
			inTrans = 0;
			if(generalMSG[3] == 0x89 && generalMSG[4] == 0x01){
				//Sucessful Transmission
				msgReceived = 1;
				countNotReceived = 0;
				if(type == 0){
					P2OUT |= BIT0; //Hibernate XBEE
					type = 1; //If Connect Message is received, then go to DATA stage
					return;
				}
			}
			else{
				msgReceived = 0;//Error
				countNotReceived++;
			}
			if(countNotReceived == 5){
				type = 0;//Return to
				countNotReceived = 0;
				//TURN ON WARNING LED?
			}
		}
	}

	if (type == 1){//sending multiple data packets
		if(msgReceived == 1){//Succesful Data Packet Sent
			msgsSent++;
			if (msgsSent == msgsMax){
				P2OUT |= BIT0; //Hibernate XBEE
				IE2 &= ~UCA0RXIE;
				msgsSent = 0;
				return;
			}
			switch(msgsSent){
			case 1:
				packetLength = api16DataPacket(ir_buffer+50, 52, generalMSG, 0, msgsSent);
				sendMSG(packetLength);
				break;

			case 2:
				packetLength = api16DataPacket(ir_buffer+100, 52, generalMSG, 0, msgsSent);
				sendMSG(packetLength);
				break;

			case 3:
				packetLength = api16DataPacket(red_buffer, 52, generalMSG, 1, msgsSent);
				sendMSG(packetLength);
				break;

			case 4:
				packetLength = api16DataPacket(red_buffer+50, 52, generalMSG, 1, msgsSent);
				sendMSG(packetLength);
				break;

			case 5:
				packetLength = api16DataPacket(red_buffer+100, 52, generalMSG, 1, msgsSent);
				sendMSG(packetLength);
				break;
			}
		}
		else{
			//NACK Received when Type = 0, End Transmission of Data, Data Lost
			P2OUT |= BIT0; //Hibernate XBEE
			IE2 &= ~UCA0RXIE;
			return;
		}

	}
	else{//If type = 0 and nack is received, place Xbee to sleep again
		P2OUT |= BIT0; //Hibernate XBEE
		IE2 &= ~UCA0RXIE;
	}
}


// Port 1 interrupt service routine for Data Collection
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	P1IFG &= ~BIT3;                     // P1.3 IFG cleared
	maxim_max30100_read_reg(REG_INTR_STATUS_1,&status1);
	maxim_max30100_read_reg(REG_INTR_STATUS_2,&status2);

	if(status1 & 0xC0){
		maxim_max30100_read_fifo(red_buffer+track, ir_buffer+track);
		track= track + 2;
	}
	if(status2 & 0x02){
		maxim_max30100_read_reg(REG_TEMP_INTR,&tempInt);
		maxim_max30100_read_reg(REG_TEMP_FRAC,&tempFrac);
		ir_buffer[150] = 0x00;
		ir_buffer[151] = tempInt;
		red_buffer[150] = 0x00;
		red_buffer[151] = tempFrac;
	}
	if (track == 150){
		//maxim_max30100_reset();
		P1IE &= ~BIT3;
		P1IFG = 0;
		P2OUT &= ~BIT0; //Wake Up XBEE
		__delay_cycles(13000);//13mSec
		IE2 |= UCA0RXIE;
		packetLength = api16DataPacket(ir_buffer, 52, generalMSG, 0, msgsSent);
		sendMSG(packetLength);
		return;
	}

}

int main(void) {

	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    DCOCTL = 0; // Select lowest DCOx and MODx settings<
    BCSCTL1 = CALBC1_1MHZ; // Set DCO
    DCOCTL = CALDCO_1MHZ;
    BCSCTL3 |= LFXT1S_2;                      // LFXT1 = VLO = 12khz

    //Turn OFF all Unused Ports
    P2DIR = 0xFF; // All P2.x outputs<
    P2OUT &= 0x00; // All P2.x reset

    //Init i2c and UART
    uart_init();
    setup_i2c();
    IE2      &=  ~UCA0RXIE;

    //MAX30100
    maxim_max30100_reset();
    maxim_max30100_init();


    P1IE &= ~BIT3;
    //Xbee Sleep Pin Set High
    P2OUT &= ~BIT0; //Wake Up XBEE
    type = 0;

    __enable_interrupt();
    //Setting Up Timer A0
    TA0CCR0 = 56000;		                // 5 Second Timer Interrupt
    TA0CTL  |= TASSEL_1 + MC_1 + TACLR;	// ACLK is 12kHz
    //MC_0 = STOP             MC_1 = UP
    TA0CTL |= TAIE; //Enable Intterupt


    while(1){
    	__bis_SR_register(LPM0_bits + GIE);
    }
    return 0;

}
