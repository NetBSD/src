/* $NetBSD: iiccontrol.c,v 1.1 1996/01/31 23:16:04 mark Exp $ */

/*
 * Copyright (c) 1994 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * iiccontrol.c
 *
 * Routines to communicate with IIC bus devices
 *
 * Created      : 13/10/94
 * Last updated : 28/08/95
 *
 * Based of kate/display/iiccontrol.c
 *
 *    $Id: iiccontrol.c,v 1.1 1996/01/31 23:16:04 mark Exp $
 */

#include <sys/types.h>
#include <machine/iomd.h>
#include <machine/katelib.h>
#include <machine/cpu.h>
#include <machine/irqhandler.h>

#define iic_bitdelay 10

int iic_control __P((u_char /*address*/, u_char */*buffer*/, int /*count*/));
void iic_set_state __P((int /*data*/, int /*clock*/));
void iic_set_state_and_ack __P((int /*data*/, int /*clock*/));
void iic_delay __P((int /*delay*/));

static int iic_getack __P((void));
static void iic_write_bit __P((int /*bit*/));
static int iic_write_byte __P((u_char /*value*/));
static u_char iic_read_byte __P((void));
static void iic_start_bit __P((void));
static void iic_stop_bit __P((void));

/*
 * Main entry to IIC driver.
 */

int
iic_control(address, buffer, count)
	u_char address;
	u_char *buffer;
	int count;
{
	int loop;

/* Send the start bit */

	iic_start_bit();

/* Send the address */

	if (!iic_write_byte(address)) {
		iic_stop_bit();
		return(-1);
	}

/* Read or write the data as required */

	if ((address & 1) == 0) {
/* Write bytes */
		for (loop = 0; loop < count; ++loop) {
			if (!iic_write_byte(buffer[loop])) {
				iic_stop_bit();
				return(-1);
			}
		}
	}
	else {
/* Read bytes */
		for (loop = 0; loop < count; ++loop) {
			buffer[loop] = iic_read_byte();

/* Send final acknowledge */

			if (loop == (count - 1))
				iic_write_bit(1);
			else
				iic_write_bit(0);
		}
	}

/* Send stop bit */

	iic_stop_bit();

	return(0);
}


static int
iic_getack()
{
	u_int oldirqstate;
	int ack;

	iic_set_state(1, 0);
	oldirqstate = disable_interrupts(I32_bit);
	iic_set_state_and_ack(1, 1);
	ack = ReadByte(IOMD_IOCR);
	iic_set_state(1, 0);
	restore_interrupts(oldirqstate);

	return((ack & 1) == 0);
}


static void
iic_write_bit(bit)
	int bit;
{
	u_int oldirqstate;

	iic_set_state(bit, 0);
	oldirqstate = disable_interrupts(I32_bit);
	iic_set_state_and_ack(bit, 1);
	iic_set_state(bit, 0);
	restore_interrupts(oldirqstate);
}


static int
iic_write_byte(value)
	u_char value;
{
	int loop;
	int bit;

	for (loop = 0x80; loop != 0; loop = loop >> 1) {
		bit = ((value & loop) != 0);
		iic_write_bit(bit);
	}

	return(iic_getack());
}


static u_char
iic_read_byte()
{
	int loop;
	u_char byte;
	u_int oldirqstate;

	iic_set_state(1,0);

	byte = 0;

	for (loop = 0; loop < 8; ++loop) {
		oldirqstate = disable_interrupts(I32_bit);
		iic_set_state_and_ack(1, 1);
		byte = (byte << 1) + (ReadByte(IOMD_IOCR) & 1);
		iic_set_state(1, 0);
		restore_interrupts(oldirqstate);
	}

	return(byte);
}


static void
iic_start_bit()
{
	iic_set_state(1, 1);
	iic_set_state(0, 1);
	iic_delay(10);
	iic_set_state(0, 0);
}


static void
iic_stop_bit()
{
	iic_set_state(0, 1);
	iic_set_state(1, 1);
}

/* End of iiccontrol.c */
