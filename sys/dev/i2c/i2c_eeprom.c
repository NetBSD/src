/*  $NetBSD: i2c_eeprom.c,v 1.2 2001/11/13 12:25:27 lukem Exp $   */
 
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

 /*
  * driver for Microchip Technology, 24C01A/02A/04A eeproms 
  * Data Sheet available from www.microchip.com
  */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i2c_eeprom.c,v 1.2 2001/11/13 12:25:27 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/i2c/i2c_bus.h>
#include <dev/i2c/i2c_eeprom.h>

int i2c_eeprom_read(adapter, addr)
	i2c_adapter_t * adapter;
	u_int8_t addr;
{
	int retval;

	i2c_send_start(adapter);
	if (i2c_write_byte(adapter, 0xa0) != 0) {
#if I2C_DEBUG
		printf("i2c_eeprom_read: write 0xa0\n");
#endif
		return -1;
	}
	if (i2c_write_byte(adapter, addr) != 0) {
#if I2C_DEBUG
        printf("i2c_eeprom_read: write addr\n");
#endif
		return -1;
	}

	i2c_send_start(adapter);
	if (i2c_write_byte(adapter, 0xa1)  != 0) {
#if I2C_DEBUG
		printf("i2c_eeprom_read: write 0xa1\n");
#endif
		return -1;
	}
	retval = i2c_read_byte(adapter);

	i2c_send_stop(adapter);
	return retval;
}
