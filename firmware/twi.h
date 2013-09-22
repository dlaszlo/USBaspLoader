#ifndef TWI_H_
#define TWI_H_

#include <inttypes.h>

#define TWI_ERROR 0x01
#define TWI_OK 0x00

/****************************************************************************
TWI Status register definitions
****************************************************************************/
//General Master status codes
#define START		0x08					//START has been transmitted
#define	REP_START	0x10					//Repeated START has been
											//transmitted
//Master Transmitter status codes
#define	MTX_ADR_ACK		0x18				//SLA+W has been tramsmitted
											//and ACK received
#define	MTX_ADR_NACK	0x20				//SLA+W has been tramsmitted
											//and NACK received
#define	MTX_DATA_ACK	0x28				//Data byte has been tramsmitted
											//and ACK received
#define	MTX_DATA_NACK	0x30				//Data byte has been tramsmitted
											//and NACK received
#define	MTX_ARB_LOST	0x38				//Arbitration lost in SLA+W or
											//data bytes
//Master Receiver status codes
#define	MRX_ARB_LOST	0x38				//Arbitration lost in SLA+R or
											//NACK bit
#define	MRX_ADR_ACK		0x40				//SLA+R has been tramsmitted
											//and ACK received
#define	MRX_ADR_NACK	0x48				//SLA+R has been tramsmitted
											//and NACK received
#define	MRX_DATA_ACK	0x50				//Data byte has been received
											//and ACK tramsmitted
#define	MRX_DATA_NACK	0x58				//Data byte has been received
											//and NACK tramsmitted

void TWI_waitInt();
void TWI_getStatus();
void TWI_start();
void TWI_stop();
void TWI_write(uint8_t b);
uint8_t TWI_readAck();
uint8_t TWI_readNack();

void TWI_init();
uint8_t TWI_send(uint8_t, uint8_t *, uint8_t);
uint8_t TWI_receive(uint8_t, uint8_t *, uint8_t, uint8_t);

#endif /* TWI_H_ */
