/*	$NetBSD: akbdvar.h,v 1.9.88.1 2012/11/20 03:01:30 tls Exp $	*/

/*
 * Copyright (C) 1998	Colin Wood
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Colin Wood.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MAC68K_KBDVAR_H_
#define _MAC68K_KBDVAR_H_

#include <machine/adbsys.h>

/*
 * State info, per keyboard instance.
 */
struct akbd_softc {
	/* ADB info */
	int	        origaddr;       /* ADB device type (ADBADDR_KBD) */
	int	        adbaddr;        /* current ADB address */
	int	        handler_id;     /* type of keyboard */

	u_int8_t	sc_leds;	/* current LED state */
	device_t	sc_wskbddev;
};

/* LED register bits, inverse of actual register value */
#define LED_NUMLOCK	0x1
#define LED_CAPSLOCK	0x2
#define LED_SCROLL_LOCK	0x4

int	akbd_cnattach(void);
void    kbd_adbcomplete(uint8_t *, void *, int);

#endif /* _MAC68K_KBDVAR_H_ */
