/*	$NetBSD: kbdreg.h,v 1.5.142.1 2015/09/22 12:05:38 skrll Exp $	*/

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
 *
 * from: Header: kbio.h,v 1.4 92/11/26 01:16:32 torek Exp  (LBL)
 */

#ifndef _MACHINE_KBDREG_H_
#define _MACHINE_KBDREG_H_

#include <sys/ioccom.h>

struct kbdbell {
	u_int volume;		/* volume of bell (0-64) */
	u_int pitch;		/* pitch of bell (10-20000) */
	u_int duration;		/* duration of bell */
};

#define	KIOCTRANS	_IOW('k', 0, int)	/* set translation mode */
			/* (we only accept TR_UNTRANS_EVENT) */
#define	KIOCGTRANS	_IOR('k', 5, int)	/* get translation mode */
#define	KIOCSDIRECT	_IOW('k', 10, int)	/* keys to console? */
#define	KIOCRINGBELL	_IOW('k', 15, struct kbdbell) /* ring bell */

#define	TR_UNTRANS_EVENT	3

#endif /* _MACHINE_KBDREG_H_ */
