/*
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *	from: @(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
 *	disksubr.c,v 1.2 1993/05/22 07:59:47 cgd Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/dkbad.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>

/* encoding of disk minor numbers, should be elsewhere... */
#define dkunit(dev)		(minor(dev) >> 3)
#define dkpart(dev)		(minor(dev) & 7)
#define dkminor(unit, part)	(((unit) << 3) | (part))

#define	b_cylin	b_resid

/*
 * Attempt to read a disk label from a device
 * using the indicated stategy routine.
 * The label must be partly set up before this:
 * secpercyl, secsize and anything required for a block i/o read
 * operation in the driver's strategy/start routines
 * must be filled in before calling us.
 *
 * Returns null on success and an error string on failure.
 */
char *
cpu_readdisklabel(dev, strat, lp, osdep)
	dev_t dev;
	int (*strat)();
	register struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
    return "No architecture-specific disklabel";
}

/*
 * Check new disk label for sensibility
 * before setting it.
 */
int cpu_setdisklabel(olp, nlp, openmask, osdep)
	register struct disklabel *olp, *nlp;
	u_long openmask;
	struct cpu_disklabel *osdep;
{
    return EINVAL;
}


/*
 * Write disk label back to device after modification.
 */
int cpu_writedisklabel(dev, strat, lp, osdep)
	dev_t dev;
	int (*strat)();
	register struct disklabel *lp;
	struct cpu_disklabel *osdep;
{
    return EINVAL;
}
