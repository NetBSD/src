/*	$NetBSD: if_lereg.h,v 1.10 2003/08/07 16:27:32 agc Exp $	*/

/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_lereg.h	7.1 (Berkeley) 5/8/90
 */

/*
 * DIO registers, offsets from lestd[0]
 */
#define	LER0_ID			0x01		/* ID */
#define	LER0_STATUS		0x03		/* interrupt enable/status */

#define	LER0_SIZE		4

/*
 * Control and status bits -- LER0_STATUS
 */
#define	LE_IE			0x80		/* interrupt enable */
#define	LE_IR			0x40		/* interrupt requested */
#define	LE_LOCK			0x08		/* lock status register */
#define	LE_ACK			0x04		/* ack of lock */
#define	LE_JAB			0x02		/* loss of tx clock (???) */

/*
 * LANCE registers, offsets from lestd[1]
 */
#define	LER1_RDP		0x00		/* data port */
#define	LER1_RAP		0x02		/* register select port */

#define	LER1_SIZE		4

/*
 * LANCE buffer area.
 */
#define	LE_BUFSIZE		16384
