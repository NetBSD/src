/*	$NetBSD: clock.c,v 1.2 2004/04/10 23:50:23 cl Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.2 2004/04/10 23:50:23 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/events.h>

#include <dev/clock_subr.h>

#include "config_time.h"		/* for CONFIG_TIME */

static int xen_timer_handler(void *, struct trapframe *);

/* These are peridically updated in shared_info, and then copied here. */
static unsigned long shadow_tsc_stamp;
static uint64_t shadow_system_time;
static unsigned long shadow_time_version;
static struct timeval shadow_tv;

static int timeset;

/*
 * Reads a consistent set of time-base values from Xen, into a shadow data
 * area.  Must be called at splclock.
 */
static void get_time_values_from_xen(void)
{
	do {
		shadow_time_version = HYPERVISOR_shared_info->time_version2;
		__insn_barrier();
		shadow_tv.tv_sec = HYPERVISOR_shared_info->wc_sec;
		shadow_tv.tv_usec = HYPERVISOR_shared_info->wc_usec;
		shadow_tsc_stamp = HYPERVISOR_shared_info->tsc_timestamp;
		shadow_system_time = HYPERVISOR_shared_info->system_time;
		__insn_barrier();
	} while (shadow_time_version != HYPERVISOR_shared_info->time_version1);
}

void
inittodr(time_t base)
{
	int s;

	/*
	 * if the file system time is more than a year older than the
	 * kernel, warn and then set the base time to the CONFIG_TIME.
	 */
	if (base && base < (CONFIG_TIME-SECYR)) {
		printf("WARNING: preposterous time in file system\n");
		base = CONFIG_TIME;
	}

	s = splclock();
	get_time_values_from_xen();
	splx(s);

	time.tv_usec = shadow_tv.tv_usec;
	time.tv_sec = shadow_tv.tv_sec + rtc_offset * 60;
#ifdef DEBUG_CLOCK
	printf("readclock: %ld (%ld)\n", time.tv_sec, base);
#endif
	if (base != 0 && base < time.tv_sec - 5*SECYR)
		printf("WARNING: file system time much less than clock time\n");
	else if (base > time.tv_sec + 5*SECYR) {
		printf("WARNING: clock time much less than file system time\n");
		printf("WARNING: using file system time\n");
		goto fstime;
	}

	timeset = 1;
	return;

fstime:
	timeset = 1;
	time.tv_sec = base;
	printf("WARNING: CHECK AND RESET THE DATE!\n");
}

void
resettodr()
{

	/*
	 * We might have been called by boot() due to a crash early
	 * on.  Don't reset the clock chip in this case.
	 */
	if (!timeset)
		return;
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

	for (k = 0; k < 10 * n; k++);
}

void
xen_microtime(struct timeval *tv)
{
	printf("xen_microtime %p\n", tv);
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

	hardclock((struct clockframe *)regs);

	return 0;
}

void
setstatclockrate(int arg)
{
}
