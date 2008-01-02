/* $NetBSD: cpu.h,v 1.1.2.2 2008/01/02 21:50:48 bouyer Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

#ifndef _ARCH_USERMODE_INCLUDE_CPU_H
#define _ARCH_USERMODE_INCLUDE_CPU_H

#include <sys/device.h>
#include <sys/cpu_data.h>

#include <machine/intrdefs.h>

extern void	cpu_signotify(struct lwp *);
extern void	cpu_need_proftick(struct lwp *);

struct cpu_info {
	device_t	ci_dev;
	struct cpu_info	*ci_self;
	struct cpu_info	*ci_next;
	struct cpu_data	ci_data;
	u_int		ci_cpuid;
	int		ci_want_resched;
	volatile int	ci_mtx_count;
	volatile int	ci_mtx_oldspl;
#if notyet
	lwp_t		*ci_curlwp;
#endif
	lwp_t		*ci_stash;
};

__inline static struct cpu_info * __attribute__((__unused__))
usermode_curcpu(void)
{
	extern struct cpu_info cpu_info_primary;

	return &cpu_info_primary;
}

__inline static void
usermode_delay(unsigned int ms)
{
	extern int usleep(useconds_t);

	usleep(ms);
}

#define	curcpu()	usermode_curcpu()
#define cpu_number()	0

#define cpu_proc_fork(p1, p2)

#define delay(ms)	usermode_delay(ms)
#define DELAY(ms)	usermode_delay(ms)

/* XXXJDM */
struct clockframe {
	uint8_t cf_dummy;
};

#define CLKF_USERMODE(frame)	0
#define CLKF_PC(frame)		0
#define CLKF_INTR(frame)	0

#define cpu_swapin(l)
#define cpu_swapout(l)

#endif /* !_ARCH_USERMODE_INCLUDE_CPU_H */
