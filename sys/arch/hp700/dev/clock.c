/*	$NetBSD: clock.c,v 1.5 2006/09/15 07:51:17 skrll Exp $	*/

/*	$OpenBSD: clock.c,v 1.10 2001/08/31 03:13:42 mickey Exp $	*/

/*
 * Copyright (c) 1998,1999 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF MIND,
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.5 2006/09/15 07:51:17 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>

#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/psl.h>
#include <machine/intr.h>
#include <machine/reg.h>
#include <machine/cpufunc.h>
#include <machine/autoconf.h>

#include <hp700/hp700/machdep.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

void
cpu_initclocks(void)
{
	extern u_int cpu_hzticks;
	u_int time_inval;

	/* Start the interval timer. */
	mfctl(CR_ITMR, time_inval);
	mtctl(time_inval + cpu_hzticks, CR_ITMR);
}

int
clock_intr(void *v)
{
	struct clockframe *frame = v;

	/* printf ("clock int 0x%x @ 0x%x for %p\n", t,
	   CLKF_PC(frame), curproc); */

	cpu_initclocks();
	if (!cold)
		hardclock(frame);

#if 0
	ddb_regs = *frame;
	db_show_regs(NULL, 0, 0, NULL);
#endif

	/* printf ("clock out 0x%x\n", t); */

	return 1;
}

void
setstatclockrate(int newhz)
{
	/* nothing we can do */
}

