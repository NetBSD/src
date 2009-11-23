/* $NetBSD: systemsw.h,v 1.7.28.1 2009/11/23 18:46:50 matt Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SBMIPS_SYSTEMSW_H_
#define	_SBMIPS_SYSTEMSW_H_

#include <sys/types.h>

struct systemsw {
	/* ordered to match likely locality. */
	void	(*s_cpu_intr)(uint32_t, uint32_t, vaddr_t, uint32_t);

	void	*s_clock_arg;
	void	(*s_clock_init)(void *);

	void	*s_statclock_arg;
	void	(*s_statclock_init)(void *);
	void	(*s_statclock_setrate)(void *, int);

	void	*(*s_intr_establish)(u_int, u_int,
		    void (*fun)(void *, uint32_t, vaddr_t), void *);
};
extern struct systemsw systemsw;

int	system_set_clockfns(void *, void (*)(void *));
int	system_set_todrfns(void *, void (*)(void *, time_t), void (*)(void *));

#define	cpu_intr_establish(n,s,f,a)	((*systemsw.s_intr_establish)(n,s,f,a))

void	sb1250_icu_init(void);

#endif /* _SBMIPS_SYSTEMSW_H_ */
