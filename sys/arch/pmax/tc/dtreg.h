/*	$NetBSD: dtreg.h,v 1.1.2.1 2002/03/15 14:22:50 ad Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)dtopreg.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * Log:	dtop.h,v
 * Revision 2.3  92/03/05  17:08:17  rpd
 * 	Define how many buttons and coordinates we can take.
 * 	[92/03/05            af]
 *
 * Revision 2.2  92/03/02  18:32:17  rpd
 * 	Created from DEC specs:
 * 	"DESKTOPinterconnect Description and Protocol Specification"
 * 	Version 0.9, Jun 17 1991
 * 	"Open Desktop Bus, Locator Device Protocol Specification"
 * 	Version 0.4, Dec 13 1990
 * 	"Open Desktop Bus, Keyboard Device Protocol Specification"
 * 	Version 0.7, Jan 9 1991
 * 	[92/01/19            af]
 *
 */
/*
 *	File: dtop.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	1/92
 *
 *	Definitions for the Desktop serial bus (i2c aka ACCESS).
 */

#ifndef	_DTREG_H_
#define	_DTREG_H_

#define	DT_MAX_DEVICES		14
#define	DT_MAX_MSG_SIZE		36	/* 3 hdr + 32 data + 1 checksum */

/*
 * Standard addresses.
 */
#define	DT_ADDR_HOST		0x50	/* address for the (only) host */
#define	DT_ADDR_FIRST		0x52	/* first assignable address */
#define	DT_ADDR_MOUSE		0x6a	/* mouse */
#define	DT_ADDR_KBD		0x6c	/* keyboard */
#define	DT_ADDR_DEFAULT		0x6e	/* power-up default address */
#define	DT_ADDR_LAST		0x6c	/* last, inclusive */

/*
 * Standard messages
 */
#define	DT_MSG_RESET		0xf0	/* preceded by 0x81: P,len 1 */
#define	DT_MSG_ID_REQUEST	0xf1	/* preceded by 0x81: P,len 1 */
#define	DT_MSG_ASSIGN_ADDRESS	0xf2	/* preceded by 0x9e: P,len 30 */
					/* followed by a dt_id_reply_t */
					/* and by the new_IC_address */
#define	DT_MSG_CAP_REQUEST	0xf3	/* preceded by 0x83: P,len 3 */
					/* followed by a 16 bit u_offset */
#define	DT_MSG_APPL_TEST	0xb1	/* precede by P, sub, len 1 */

/* from devices to host */

#define	DT_MSG_ATTENTION	0xe0	/* preceded by P, len */
#	define DT_ATN_OK_STATUS	0x00	/* anything else bad */
					/* followed by 0-30 bytes */

#define	DT_MSG_ID_REPLY	0xe1	/* preceded by P,len (29..32) */

struct dt_ident {
	u_char	revision[8];	/* ascii, blank padded */
	u_char	vendor[8];
	u_char	module[8];
	int	device_number;	/* 32 bits cpl-2 */
	u_char	optional[4];	/* 0-3 optional bytes, ignore */
};

#define	DT_MSG_CAP_REPLY	0xe3	/* preceded by P,len (3..32) */
					/* followed by 16 bit u_offset */
					/* followed by data */

#define	DT_MSG_APPL_SIGNAL	0xa0	/* application level signal */
#	define DT_SIG_ATTENTION	0x00
#	define DT_SIG_RESET		0x01
#	define DT_SIG_HALT		0x02

#define	DT_MSG_APPL_TREPLY	0xa1	/* followed by status (0-->ok) */
					/* and 0..30 bytes of result data  */

/* reserved message codes (testing, manifacturing) */

#define	DT_MSG_RES0		0xc0
#define	DT_MSG_RES1		0xc1
#define	DT_MSG_RES2		0xc2
#define	DT_MSG_RES3		0xc3


/*
 * Device specific definitions:  Keyboard
 */

/* From host to keyboard. */

#define	DT_KMSG_CLICK		0x01	/* preceded by P, sub len 2 */
#	define	DT_CLICK_VOLUME_MAX	0x7	/* followed by one byte */

#define	DT_KMSG_BELL		0x02	/* preceded by P, sub len 2 */
					/* same as above */

#define	DT_KMSG_LED		0x03	/* preceded by P, sub len 2 */
					/* four lower bits turn leds on */

#define	DT_KMSG_POLL		0x04	/* preceded by P, sub len 1 */

/* Keyboard sends up to 11 codes in a data message, distinguished values: */
#define	DT_KBD_EMPTY		0x00
#define	DT_KBD_OUT_ERR		0x01
#define	DT_KBD_IN_ERR		0x02

#define	DT_KBD_KEY_MIN	0x08
#define	DT_KBD_KEY_MAX	0xff

/* Powerup status values */
#define	DT_KBD_OK	0x00
#define	DT_KBD_ROM_FAIL	0x01
#define	DT_KBD_RAM_FAIL	0x02
#define	DT_KBD_KEY_DOWN	0x03

/*
 * Device specific definitions:  Locators (mouse)
 */

struct dt_locator_msg {
	u_short	buttons;	/* 1->pressed */
	short	x;
	short	y;
	short	z;
	/* possibly 3 more dimensions for gloves */
};

#define	DT_LMSG_SET_RATE	0x01	/* preceded by P,sub, len 2 */
					/* followed by sampling interval,
					   from 8 to 25 msecs (0->polled */
#define	DT_LMSG_POLL		0x02	/* preceded by P,sub, len 1 */

/* Powerup status values: 0 ok, else.. */
#define	DT_MOUSE_ROM_FAIL	0x01
#define	DT_MOUSE_RAM_FAIL	0x02
#define	DT_MOUSE_KEY_DOWN	0x03

#endif	/* !_DTREG_H_ */
