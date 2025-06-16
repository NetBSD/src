/*	$NetBSD: adbvar.h,v 1.6 2025/06/16 08:00:50 macallan Exp $ */

/*-
 * Copyright (c) 2006 Michael Lorenz
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adbvar.h,v 1.6 2025/06/16 08:00:50 macallan Exp $");

#ifndef ADBVAR_H
#define ADBVAR_H

#define ADB_CMDADDR(cmd)	((u_int8_t)((cmd) & 0xf0) >> 4)
#define ADBFLUSH(dev)		((((u_int8_t)(dev) & 0x0f) << 4) | 0x01)
#define ADBLISTEN(dev, reg)	((((u_int8_t)(dev) & 0x0f) << 4) | 0x08 | (reg))
#define ADBTALK(dev, reg)	((((u_int8_t)(dev) & 0x0f) << 4) | 0x0c | (reg))

	/* Interesting default addresses */
#define	ADBADDR_SECURE	1		/* Security dongles */
#define ADBADDR_MAP	2		/* Mapped devices (keyboards/pads) */
#define ADBADDR_REL	3		/* Relative positioning devices
					   (mice, trackballs/pads) */
#define ADBADDR_ABS	4		/* Absolute positioning devices
					   (graphics tablets) */
#define ADBADDR_DATATX	5
#define ADBADDR_RSRVD	6		/* Reserved by Apple */
#define ADBADDR_MISC	7		/* Miscellaneous appliances */
#define ADBADDR_DONGLE	ADBADDR_SECURE
#define ADBADDR_KBD	ADBADDR_MAP
#define ADBADDR_MS	ADBADDR_REL
#define ADBADDR_TABLET	ADBADDR_ABS
#define ADBADDR_MODEM	ADBADDR_DATATX

#define ADBADDR_APM	0xac0ff		/* A faux-addr for the APM driver to
					   latch onto */

	/* Interesting keyboard handler IDs */
#define ADB_STDKBD	1
#define ADB_EXTKBD	2
#define ADB_ISOKBD	4
#define ADB_EXTISOKBD	5
#define ADB_KBDII	8
#define ADB_ISOKBDII	9
#define ADB_PBKBD	12
#define ADB_PBISOKBD	13
#define ADB_ADJKPD	14
#define ADB_ADJKBD	16
#define ADB_ADJISOKBD	17
#define ADB_ADJJAPKBD	18
#define ADB_PBEXTISOKBD	20
#define ADB_PBEXTJAPKBD	21
#define ADB_JPKBDII	22
#define ADB_PBEXTKBD	24
#define ADB_DESIGNKBD	27	/* XXX Needs to be verified XXX */
#define ADB_PBJPKBD	30
#define ADB_PBG3KBD	195
#define ADB_IBOOKKBD	196	/* iBook, probably others? */
#define ADB_PBG3JPKBD	201

	/* Interesting mouse handler IDs */
#define ADBMS_100DPI	1
#define ADBMS_200DPI	2
#define ADBMS_MSA3	3	/* Mouse Systems A3 Mouse */
#define ADBMS_EXTENDED	4	/* Extended mouse protocol */
#define ADBMS_USPEED	0x2f	/* MicroSpeed mouse */
#define ADBMS_UCONTOUR	0x66	/* Contour mouse */
#define ADBMS_TURBO	50	/* Kensington Turbo Mouse */

	/* Interesting tablet handler ID */
#define ADB_ARTPAD	58	/* WACOM ArtPad II tablet */

	/* Interesting miscellaneous handler ID */
#define ADB_POWERKEY	34	/* Sophisticated Circuits PowerKey */
				/* (intelligent power tap) */

#define ADBK_KEYVAL(key)	((key) & 0x7f)
#define ADBK_PRESS(key)		(((key) & 0x80) == 0)
#define ADBK_KEYDOWN(key)	(key)
#define ADBK_KEYUP(key)		((key) | 0x80)

/* EMP device classes */
#define MSCLASS_TABLET		0
#define MSCLASS_MOUSE		1
#define MSCLASS_TRACKBALL	2
#define MSCLASS_TRACKPAD	3

struct adb_bus_accessops {
	void *cookie;
	/* cookie, poll, address/command, length, data */
	int (*send)(void *, int, int, int, uint8_t *);
	void (*poll)(void *);
	void (*autopoll)(void *, int);	/* bitmask of ADB addresses to poll */ 
	int (*set_handler)(void *, void (*)(void *, int, uint8_t *), void *);
};

struct adb_device {
	void *cookie;
	void (*handler)(void *, int, uint8_t *);
	int current_addr;
	int original_addr;
	int handler_id;
};

struct adb_attach_args {
	struct adb_bus_accessops *ops;
	struct adb_device *dev;
};
	
int nadb_print(void *, const char *);


#endif /* ADBVAR_H */
