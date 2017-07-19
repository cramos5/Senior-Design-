#include "max30100.h"
#include <msp430.h>

void setup_i2c(){
	P1REN &= ~(BIT6 | BIT7);
	P1OUT |= BIT6 | BIT7;
	P1SEL |= BIT6 | BIT7;                     // Assign I2C pins to USCI_B0
	P1SEL2|= BIT6 | BIT7;                     // Assign I2C pins to USCI_B0
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST|UCMODE_3|UCSYNC;         // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2|UCSWRST;              // Use SMCLK, keep SW reset
	UCB0BR0 = 10;                             // fSCL = SMCLK/10 = 100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = 0x57;       //Trying 57 or 51                  // Set slave address
	UCB0CTL1 &= ~UCSWRST;  // Clear SW reset, resume operation
	__delay_cycles(500000);

	//Setting Up MAX Port Interrupt
    P1DIR &= ~BIT3;
    P1REN &= ~BIT3; // Enable internal pull-up/down resistors
    P1IE |= BIT3; // P1.3 interrupt enabled
    P1IES |= BIT3; // P1.3 Hi/lo edge
    P1IFG &= ~BIT3; // P1.3 IFG cleared

}

void maxim_max30100_read_reg(unsigned volatile char reg_addr, unsigned volatile char *data)
{
	while (UCB0CTL1 & UCTXSTP); // Ensure stop condition got sent
	UCB0CTL1 |= UCTR + UCTXSTT; // I2C Start Condition with Transmit Mode
	while((IFG2 & UCB0TXIFG) == 0);
	UCB0TXBUF = reg_addr; //Assigned Register Addr
	while((IFG2 & UCB0TXIFG) == 0); //Check Until Transmission is complete
	UCB0CTL1 &= ~UCTR ; // Change to Receiver Mode
	UCB0CTL1 |= UCTXSTT; // Repeat Start Condition
	while (UCB0CTL1 & UCTXSTT); //Check Until Transmission is complete
	UCB0CTL1 |= UCTXNACK; //Generate NACK
	UCB0CTL1 |= UCTXSTP; //Generate Stop Condition
	while((IFG2 & UCB0RXIFG) == 0); // Check until Data is fully received
	*data = UCB0RXBUF; // I2C stop condition
}

void maxim_max30100_read_fifo(unsigned volatile char *red_sample, unsigned volatile char *ir_sample)
{
	char volatile unsigned un_temp = 0;
	char volatile unsigned ir1 = 0;
	char volatile unsigned ir2 = 0;
	char volatile unsigned ir3 = 0;
	char volatile unsigned red1 = 0;
	char volatile unsigned red2 = 0;
	char volatile unsigned red3 = 0;
	char volatile unsigned uch_temp = 0;
	char volatile unsigned temp = 0;
	//Clear Samples
	*ir_sample=0;
	*red_sample=0;

	//Sending Register Address
	while (UCB0CTL1 & UCTXSTP);
	UCB0CTL1 |= UCTR + UCTXSTT;
	while((IFG2 & UCB0TXIFG) == 0);
	UCB0TXBUF = REG_FIFO_DATA;
	while((IFG2 & UCB0TXIFG) == 0);



	//Receiving Data
	UCB0CTL1 &= ~UCTR ;
	UCB0CTL1 |= UCTXSTT;
	while (UCB0CTL1 & UCTXSTT);
	while((IFG2 & UCB0RXIFG) == 0);
	red1= UCB0RXBUF;
	red1 <<= 6;
	while((IFG2 & UCB0RXIFG) == 0);
	red2 = UCB0RXBUF;
	red1 = red1|(red2>>2);
	while((IFG2 & UCB0RXIFG) == 0);
	red3 = UCB0RXBUF;
	red2 = (red2<<6)|(red3>>2);
	while((IFG2 & UCB0RXIFG) == 0);
	ir1 = UCB0RXBUF;
	ir1 <<= 6;
	while((IFG2 & UCB0RXIFG) == 0);
	*(ir_sample + 1) = UCB0RXBUF;
	ir2 = UCB0RXBUF;
	ir1 = ir1|(ir2>>2);
	while((IFG2 & UCB0RXIFG) == 0);
	UCB0CTL1 |= UCTXNACK;
	UCB0CTL1 |= UCTXSTP;
	while (UCB0CTL1 & UCTXSTP);
	ir3 = UCB0RXBUF;
	ir2 = (ir2<<6)|(ir3>>2);

	*(ir_sample) = ir1;
	*(ir_sample + 1) = ir2;

	*(red_sample) = red1;
	*(red_sample + 1) = red2;

}

void maxim_max30100_write_reg(unsigned volatile char reg_addr, unsigned volatile char data)
{
	/*
	while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
	UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
	UCB0TXBUF = reg_addr;//Send Start

	while (UCB0CTL1 & UCTXSTT);
	while(!(IFG2 & UCB0TXIFG));           // Ensure its Fully Transmitted

	UCB0TXBUF = data;
	while(!(IFG2 & UCB0TXIFG));

	UCB0CTL1 |= UCTXSTP;   				   //Stop Condition
	while (UCB0CTL1 & UCTXSTP);
	*/

	while (UCB0CTL1 & UCTXSTP); // Ensure stop condition got sent
	UCB0CTL1 |= UCTR + UCTXSTT; // I2C Start Condition with Transmit Mode
	while((IFG2 & UCB0TXIFG) == 0);
	UCB0TXBUF = reg_addr; //Assigned Register Addr
	while((IFG2 & UCB0TXIFG) == 0); //Check Until Transmission is complete
	UCB0TXBUF = data; //Assigned Register Addr
	while((IFG2 & UCB0TXIFG) == 0); //Check Until Transmission is complete
	UCB0CTL1 |= UCTXSTP; //Generate Stop Condition
	while (UCB0CTL1 & UCTXSTP);
}

void maxim_max30100_reset()
{
    char temp;
	maxim_max30100_write_reg(REG_MODE_CONFIG,0x40);
	maxim_max30100_read_reg(REG_INTR_STATUS_1,&temp);
	maxim_max30100_read_reg(REG_INTR_STATUS_2,&temp);
}


void maxim_max30100_init()
{

	//Enabling Almost Full interrupt and Temperature
	maxim_max30100_write_reg(REG_INTR_ENABLE_1,0xc0);
	maxim_max30100_write_reg(0x02,0xc0);

	maxim_max30100_write_reg(REG_INTR_ENABLE_2,0x02);

	//Clearing Registers
	maxim_max30100_write_reg(REG_FIFO_WR_PTR,0x00);
	maxim_max30100_write_reg(REG_OVF_COUNTER,0x00);
	maxim_max30100_write_reg(REG_FIFO_RD_PTR,0x00);

	/***Configuring***/

	//Enable Temperature
	maxim_max30100_write_reg(REG_TEMP_CONFIG, 0x01);

	//Setting Up FIFO for 4 sample avg, fifo rollover = false, fifo almust full = 17
	maxim_max30100_write_reg(REG_FIFO_CONFIG,0x5f);

	//Setting for SPO2 MODE
	maxim_max30100_write_reg(REG_MODE_CONFIG,0x03);

	//SPO2: 100 Samples/sec, !6 Res, 1.6 pw
	maxim_max30100_write_reg(REG_SPO2_CONFIG,0x27);

	//LED 1 is 7ma
	maxim_max30100_write_reg(REG_LED1_PA,0x24);
	//LED 2 is 7ma
	maxim_max30100_write_reg(REG_LED2_PA,0x24);

	//LED 1 is 7ma
	maxim_max30100_write_reg(REG_PILOT_PA,0x7f);

}

void maxim_max30100_temperature()
{	//Enable Temperature Reading
	maxim_max30100_write_reg(REG_MODE_CONFIG,0x0B);
}
