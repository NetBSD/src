/*  $NetBSD: i2c_bus.h,v 1.1 1997/10/17 17:21:19 bouyer Exp $   */
 
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

/* The i2c bus data definitions */
/*
 * Services provided by the adapter :
 * set/clear/read bit, where bit may be I2C_DATA, I2C_CLOCK, I2C_TXEN.
 */

typedef struct _i2c_adapter {
	void *adapter_softc; /* the adapter's private datas */
	void (*set_bit)  __P((void *adapter_softc, u_int8_t bit));
	void (*clr_bit)  __P((void *adapter_softc, u_int8_t bit));
	int  (*read_bit) __P((void *adapter_softc, u_int8_t bit));
} i2c_adapter_t;

#define I2C_DATA  0x00
#define I2C_CLOCK 0x01
#define I2C_TXEN 0x02
 
/* services provided by the i2c serial bus */

int i2c_write_byte __P((i2c_adapter_t *, u_int8_t));
int i2c_read_byte __P((i2c_adapter_t *));
void i2c_send_start __P((i2c_adapter_t *));
void i2c_send_stop __P((i2c_adapter_t *));
