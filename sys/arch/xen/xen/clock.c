/*	$NetBSD: clock.c,v 1.1 2004/03/11 21:44:08 cl Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 *      This product includes software developed by Christian Limpach.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.1 2004/03/11 21:44:08 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/events.h>

static int xen_timer_handler(void *, struct trapframe *);

void
inittodr(base)
	time_t base;
{
	/* printf("inittodr %08lx\n", base); */
}

void
resettodr()
{
	/* panic("resettodr"); */
}

void
startrtclock()
{
	uint64_t __cpu_khz;
	unsigned long cpu_khz;
    
	__cpu_khz = HYPERVISOR_shared_info->cpu_freq;
	cpu_khz = (u32) (__cpu_khz/1000);

	printf("Xen reported: %lu.%03lu MHz processor.\n", 
	    cpu_khz / 1000, cpu_khz % 1000);
}

/*
 * Wait approximately `n' microseconds.
 */
void
xen_delay(int n)
{
	int k;
	/* panic("xen_delay %d\n", n); */
	for (k = 0; k < 10 * n; k++);
}

void
xen_microtime(struct timeval *tv)
{
	panic("xen_microtime %p\n", tv);
}

void
xen_initclocks()
{

	event_set_handler(_EVENT_TIMER, (int (*)(void *))xen_timer_handler,
	    IPL_CLOCK);
	hypervisor_enable_event(_EVENT_TIMER);
}

static int
xen_timer_handler(void *arg, struct trapframe *regs)
{
#if 0
	static int hztest;

	if ((hztest--) == 0) {
		hztest = hz - 1;
		printf("ping!!! hz=%d regs %p level %d ipending %08x\n", hz,
		    regs, cpu_info_primary.ci_ilevel,
		    cpu_info_primary.ci_ipending);
	}
#endif

	/* printf("timer event\n"); */
	hardclock((struct clockframe *)regs);

	return 0;
}

void
setstatclockrate(arg)
	int arg;
{
	/* printf("setstatclockrate\n"); */
}
