/*  $NetBSD: i2c_bus.c,v 1.1.30.1 2002/01/10 19:53:58 thorpej Exp $   */
 
/*
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i2c_bus.c,v 1.1.30.1 2002/01/10 19:53:58 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/i2c/i2c_bus.h>

/* Support functions for the i2c serial bus */

void i2c_send_start(adapter)
	i2c_adapter_t * adapter;
{
	adapter->set_bit(adapter->adapter_softc, I2C_CLOCK);
	adapter->set_bit(adapter->adapter_softc, I2C_DATA);
	adapter->set_bit(adapter->adapter_softc, I2C_TXEN);
	adapter->clr_bit(adapter->adapter_softc, I2C_DATA);
	adapter->clr_bit(adapter->adapter_softc, I2C_CLOCK);
}

void i2c_send_stop(adapter)
	i2c_adapter_t * adapter;
{
	adapter->set_bit(adapter->adapter_softc, I2C_DATA);
	adapter->set_bit(adapter->adapter_softc, I2C_CLOCK);
	adapter->clr_bit(adapter->adapter_softc, I2C_CLOCK);
	adapter->clr_bit(adapter->adapter_softc, I2C_DATA);
	adapter->set_bit(adapter->adapter_softc, I2C_CLOCK);
	adapter->set_bit(adapter->adapter_softc, I2C_DATA);
}

int i2c_write_byte(adapter, byte)
	i2c_adapter_t *adapter;
	u_int8_t byte;
{
	u_int8_t mask;
	int retval;

	adapter->set_bit(adapter->adapter_softc, I2C_TXEN);

	for (mask = 0x80; mask != 0; mask = mask >> 1) {
		if (byte & mask)
			adapter->set_bit(adapter->adapter_softc, I2C_DATA);
		else
			adapter->clr_bit(adapter->adapter_softc, I2C_DATA);
		adapter->set_bit(adapter->adapter_softc, I2C_CLOCK);
		adapter->clr_bit(adapter->adapter_softc, I2C_CLOCK);
	}
	adapter->clr_bit(adapter->adapter_softc, I2C_TXEN);
	adapter->set_bit(adapter->adapter_softc, I2C_CLOCK);
	retval = adapter->read_bit(adapter->adapter_softc, I2C_DATA);
	adapter->clr_bit(adapter->adapter_softc, I2C_CLOCK);
	/* XXX adapter->set_bit(adapter->adapter_softc, I2C_TXEN); */
	return retval;
}

int i2c_read_byte(adapter)
	i2c_adapter_t *adapter;
{
	int i;
	u_int8_t retval = 0;

	adapter->clr_bit(adapter->adapter_softc, I2C_TXEN);

	for (i=0; i < 8; i++) {
		retval = retval << 1;
		adapter->set_bit(adapter->adapter_softc, I2C_CLOCK);
		if (adapter->read_bit(adapter->adapter_softc, I2C_DATA))
			retval |= 1;
		adapter->clr_bit(adapter->adapter_softc, I2C_CLOCK);
	}
	return retval;
}
