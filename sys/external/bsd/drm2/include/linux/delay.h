/*	$NetBSD: delay.h,v 1.3 2014/07/16 20:56:25 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_DELAY_H_
#define _LINUX_DELAY_H_

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <machine/param.h>

#define	MAX_UDELAY_MS	5

static inline void
udelay(unsigned int usec)
{
	DELAY(usec);
}

static inline void
usleep_range(unsigned long minimum, unsigned long maximum __unused)
{
	DELAY(minimum);
}

static inline void
mdelay(unsigned int msec)
{

	if (msec < MAX_UDELAY_MS)
		udelay(msec * 1000);
	else
		while (msec--)
			udelay(1000);
}

static inline void
msleep(unsigned int msec)
{
	if ((hz < 1000) && (msec < (1000/hz)))
		mdelay(msec);
	else
		(void)kpause("lnxmslep", false, mstohz(msec), NULL);
}

static inline void
ndelay(unsigned int nsec)
{

	DELAY(nsec / 1000);
}

#endif  /* _LINUX_DELAY_H_ */
