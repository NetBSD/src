/* $NetBSD: arckbdreg.h,v 1.2 2002/03/24 23:37:44 bjh21 Exp $ */
/*-
 * Copyright (c) 1997, 1998 Ben Harris
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * arckbdreg.h -  Archimedes keyboard codes
 */

#ifndef _ARCKBDREG_H
#define _ARCKBDREG_H

#define ARCKBD_HRST	0xff	/* keyboard reset */
#define ARCKBD_RAK1	0xfe	/* first reset acknowledge */
#define ARCKBD_RAK2	0xfd	/* second reset acknowledge */
#define ARCKBD_RQPD	0x40	/* reserved */
#define ARCKBD_PDAT	0xe0	/* response to RQPD */
#define ARCKBD_IS_PDAT(d)	(((d) & 0xf0) == ARCKBD_PDAT)
#define ARCKBD_RQID	0x20	/* request keyboard ID */
#define ARCKBD_KBID	0x80	/* keyboard ID (bottom 6 bits) */
#define ARCKBD_IS_KBID(d)	(((d) & 0xc0) == ARCKBD_KBID)
#define ARCKBD_KDDA	0xc0	/* key down data (bottom 4 bits plus next byte) */
#define ARCKBD_IS_KDDA(d)	(((d) & 0xf0) == ARCKBD_KDDA)
#define ARCKBD_KUDA	0xd0	/* key up data (bottom 4 bits plus next byte) */
#define ARCKBD_IS_KUDA(d)	(((d) & 0xf0) == ARCKBD_KUDA)
#define ARCKBD_RQMP	0x22	/* request mouse position */
#define ARCKBD_MDAT	0x00	/* mouse movement data */
#define ARCKBD_IS_MDAT(d)	(((d) & 0x80) == ARCKBD_MDAT)
#define ARCKBD_BACK	0x3f	/* acknowledge first byte of pair */
#define ARCKBD_NACK	0x30	/* acknowledge -- send no more */
#define ARCKBD_SACK	0x31	/* acknowledge -- send only keyup/down */
#define ARCKBD_MACK	0x32	/* acknowledge -- send only mouse events */
#define ARCKBD_SMAK	0x33	/* acknowledge -- send mouse or kbd events */
#define ARCKBD_LEDS	0x00	/* Set LEDs: */
#define ARCKBD_LEDS_CAPSLOCK	0x01	/* Caps lock */
#define ARCKBD_LEDS_NUMLOCK	0x02	/* Num lock */
#define ARCKBD_LEDS_SCROLLLOCK	0x04	/* Scroll lock */
#define ARCKBD_PRST	0x21	/* reserved */

#define ARCKBD_BAUD	31250	/* Data rate on keyboard serial link */

/* Keyboard ID codes */

/* Inferred from the ARMsi documentation */
#define ARCKBD_KBID_A500	0x80	/*  A500 prototype keyboard */
/* Determined by experiment */
#define ARCKBD_KBID_UK		0x81	/*  Archimedes UK keyboard */
/* These are from the RISC OS 2 PRM, page 1666: */
#define ARCKBD_KBID_MASTER	0x82
#define ARCKBD_KBID_COMPACT	0x83
#define ARCKBD_KBID_IT		0x84
#define ARCKBD_KBID_ES		0x85
#define ARCKBD_KBID_FR		0x86
#define ARCKBD_KBID_DE		0x87
#define ARCKBD_KBID_PT		0x88
#define ARCKBD_KBID_ESPERANTO  	0x89
#define ARCKBD_KBID_GR		0x8a
#define ARCKBD_KBID_SE		0x8b
#define ARCKBD_KBID_FI		0x8c
#define ARCKBD_KBID_DK		0x8e
#define ARCKBD_KBID_NO		0x8f
#define ARCKBD_KBID_IS		0x90
#define ARCKBD_KBID_CA1		0x91
#define ARCKBD_KBID_CA2		0x92
#define ARCKBD_KBID_CA		0x93
#define ARCKBD_KBID_TR		0x94
#define ARCKBD_KBID_ARABIC     	0x95
#define ARCKBD_KBID_IE		0x96
#define ARCKBD_KBID_HK		0x97

#endif /* !_ARCKBDREG_H */
