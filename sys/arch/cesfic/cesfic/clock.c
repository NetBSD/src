/* $NetBSD: clock.c,v 1.2 2003/07/15 01:29:19 lukem Exp $ */

/*
 * Copyright (c) 1997, 1999
 *	Matthias Drochner.  All rights reserved.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.2 2003/07/15 01:29:19 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>

char *clockbase;

extern void sic_enable_int(int, int, int, int, int);
void
cpu_initclocks()
{
	mainbus_map(0x5a000000, 0x1000, 0, (void **)&clockbase);
	sic_enable_int(25, 0, 1, 6, 0);
}

#if 0
void readrtc()
{
}
#endif

void
microtime(tvp)
	struct timeval *tvp;
{
	static struct timeval lasttime;
	int s = splhigh();

	/* make sure it's monotonic */
	if (timercmp(&time, &lasttime, <))
		*tvp = lasttime;
	else
		*tvp = lasttime = time;
	splx(s);
}

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(base)
	time_t base;
{

	time.tv_sec = base; /* XXX NFS server time - OK for diskless systems */
}

/*
 * Restore the time of day hardware after a time change.
 */
void
resettodr()
{
}

void
setstatclockrate(newhz)
	int newhz;
{
}

void otherclock __P((int));

void
otherclock(sr)
	int sr;
{
	printf("otherclock(%x)\n", sr);
}
