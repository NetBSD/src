/*	$NetBSD: msvar.h,v 1.2 2008/03/28 18:19:56 tsutsui Exp $	*/

/*-
 * Copyright (c) 2001 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define MS_S_BYTE	0		/* start byte */
#define MS_X_BYTE	1		/* second byte */
#define MS_Y_BYTE	2		/* third byte */
#define MS_DB_SIZE	3

#define MS_S_MARK	0x80		/* start mark (first byte)*/
#define MS_S_XSIGN	0x08		/* MSB(sign bit) of X */
#define MS_S_YSIGN	0x10		/* MSB(sign bit) of Y */
#define MS_S_SW1	0x01		/* left button is pressed */
#define MS_S_SW2	0x02		/* right button is pressed */
#define MS_S_SW3	0x04		/* middle button is pressed */

#define MS_X_DATA	0x7f		/* data bits of X (second byte) */
#define MS_Y_DATA	0x7f		/* data bits of Y (third byte) */

struct ms_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bt;
	bus_space_handle_t sc_bh;
	device_t sc_wsmousedev;
	bus_size_t sc_offset; /* ms data port offset */
	int sc_ndata;
	u_char sc_buf[MS_DB_SIZE];
};

void ms_intr(struct ms_softc *);
