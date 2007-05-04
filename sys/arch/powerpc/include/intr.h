/*	$NetBSD: intr.h,v 1.1.2.1 2007/05/04 02:37:04 macallan Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: intr.h,v 1.1.2.1 2007/05/04 02:37:04 macallan Exp $");

#ifndef POWERPC_INTR_MACHDEP_H
#define POWERPC_INTR_MACHDEP_H

void *intr_establish(int, int, int, int (*)(void *), void *);
void intr_disestablish(void *);
const char *intr_typename(int);

/* Interrupt priority `levels'. */
#define	IPL_NONE	0	/* nothing */
#define	IPL_SOFTCLOCK	1	/* timeouts */
#define	IPL_SOFTNET	2	/* protocol stacks */
#define	IPL_BIO		3	/* block I/O */
#define	IPL_NET		4	/* network */
#define	IPL_SOFTSERIAL	5	/* serial */
#define	IPL_AUDIO	6	/* audio */
#define	IPL_TTY		7	/* terminal */
#define	IPL_LPT		IPL_TTY
#define	IPL_VM		8	/* memory allocation */
#define	IPL_CLOCK	9
#define	IPL_STATCLOCK	10	/* clock */
#define	IPL_SCHED	11
#define	IPL_SERIAL	12	/* serial */
#define	IPL_LOCK	13
#define	IPL_HIGH	14	/* everything */
#define	NIPL		15

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */


#endif /* POWERPC_INTR_MACHDEP_H */
