/*	$NetBSD: kbio.h,v 1.5.124.1 2015/09/22 12:05:53 skrll Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)kbio.h	8.1 (Berkeley) 6/11/93
 */

#ifndef _X68K_KBIO_H_
#define _X68K_KBIO_H_

#include <sys/ioccom.h>
#include <dev/sun/kbio.h>

struct kiockey {
	int	kio_tablemask;	/* whatever */
	u_char	kio_station;	/* key number */
	u_short	kio_entry;	/* HOLE if not present */
	char	kio_text[10];	/* the silly escape sequences (unsupported) */
};

/* Same as bell_info */
struct kbiocbell {
	int volume;
	int pitch;
	int msec;
};

/*
 * Keyboard LED masks (used to set LED's)
 */
#define	LED_ZENKAKU	0x40
#define	LED_HIRAGANA	0x20
#define	LED_INSERT	0x10
#define	LED_CAPS_LOCK	0x08
#define	LED_CODE_INPUT	0x04
#define	LED_ROMAJI	0x02
#define	LED_KANA_LOCK	0x01

/*
 * Values for kio_tablemask. These determine which table to read/modify
 * in KIOC[SG]KEY ioctls. Currently, we only have "non-shift" and "shift"
 * tables.
 */

#define	HOLE	0x302		/* value for kio_entry to say `really type 3' */

#define	KIOCSBELL	_IOW('k', 16, struct kbiocbell) /* set bell parameter */

#endif /* _X68K_KBIO_H */
