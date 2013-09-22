#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "twi.h"

uint8_t _status;

void TWI_waitInt()
{
	while (!(TWCR & (1 << TWINT)))
		;
}

void TWI_getStatus(void)
{
	_status = TWSR & 0xF8;
}

void TWI_start()
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	TWI_waitInt();
	TWI_getStatus();
}

void TWI_stop()
{
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void TWI_write(uint8_t b)
{
	TWDR = b;
	TWCR = (1 << TWINT) | (1 << TWEN);
	TWI_waitInt();
	TWI_getStatus();
}

uint8_t TWI_readAck(void)
{
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	TWI_waitInt();
	TWI_getStatus();
	return TWDR ;
}

uint8_t TWI_readNack(void)
{
	TWCR = (1 << TWINT) | (1 << TWEN);
	TWI_waitInt();
	TWI_getStatus();
	return TWDR ;
}

void TWI_init(void)
{
	TWSR = 0x00;
	TWBR = 0x0C;
	//enable TWI
	TWCR = (1 << TWEN);
}

uint8_t TWI_send(uint8_t addr, uint8_t * sb, uint8_t sb_size)
{
	TWI_start();
	if (_status != START)
	{
		return TWI_ERROR;
	}
	TWI_write(addr << 1);
	if (_status != MTX_ADR_ACK)
	{
		return TWI_ERROR;
	}
	int i = 0;
	for (; i < sb_size; i++)
	{
		TWI_write(sb[i]);
		if (_status != MTX_DATA_ACK)
		{
			return TWI_ERROR;
		}
	}
	TWI_stop();
	return TWI_OK;
}

uint8_t TWI_receive(uint8_t addr, uint8_t * buff, uint8_t sb_size,
		uint8_t rb_size)
{
	TWI_start();
	if (_status != START)
	{
		return TWI_ERROR;
	}
	TWI_write(addr << 1);
	if (_status != MTX_ADR_ACK)
	{
		return TWI_ERROR;
	}
	int i = 0;
	for (; i < sb_size; i++)
	{
		TWI_write(buff[i]);
		if (_status != MTX_DATA_ACK)
		{
			return TWI_ERROR;
		}
	}
	TWI_start();
	if (_status != REP_START)
	{
		return TWI_ERROR;
	}
	TWI_write((addr << 1) | 1);
	if (_status != MRX_ADR_ACK)
	{
		return TWI_ERROR;
	}
	for (; i < rb_size; i++)
	{
		if (i + 1 >= rb_size)
		{
			buff[i] = TWI_readNack();
			if (_status != MRX_DATA_NACK)
			{
				return TWI_ERROR;
			}
		}
		else
		{
			buff[i] = TWI_readAck();
			if (_status != MRX_DATA_ACK)
			{
				return TWI_ERROR;
			}
		}
	}
	TWI_stop();
	return TWI_OK;
}

