/*	$NetBSD: intr.h,v 1.1 2001/04/06 13:13:03 fredette Exp $	*/

/*
 * Copyright (c) 2001 Matt Fredette.
 * Copyright (c) 1998 Matt Thomas.
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
 * 3. The name of the company nor the names of the authors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SUN2_INTR_H_
#define _SUN2_INTR_H_

#include <sys/queue.h>

/*
 * Interrupt levels.  Right now these correspond to real
 * hardware levels, but I don't think anything counts on
 * that (yet?).
 */
#define _IPL_SOFT_LEVEL1	1
#define _IPL_SOFT_LEVEL2	2
#define _IPL_SOFT_LEVEL3	3
#define _IPL_SOFT_LEVEL_MIN	1
#define _IPL_SOFT_LEVEL_MAX	3
#define IPL_SOFTNET  		_IPL_SOFT_LEVEL1
#define IPL_SOFTCLOCK		_IPL_SOFT_LEVEL1
#define IPL_SOFTSERIAL		_IPL_SOFT_LEVEL3
#define	IPL_NET			3
#define	IPL_CLOCK		5
#define	IPL_SERIAL		6

#ifdef _KERNEL
LIST_HEAD(sh_head, softintr_handler);

struct softintr_head {
	int shd_ipl;
	struct sh_head shd_intrs;
};

struct softintr_handler {
	struct softintr_head *sh_head;
	LIST_ENTRY(softintr_handler) sh_link;
	void (*sh_func)(void *);
	void *sh_arg;
	int sh_pending;
};

extern void softintr_init __P((void));
extern void *softintr_establish __P((int, void (*)(void *), void *));
extern void softintr_disestablish __P((void *));
extern void isr_soft_request __P((int level));

static __inline void
softintr_schedule(void *arg)
{
	struct softintr_handler * const sh = arg;
	if (sh->sh_pending == 0) {
		sh->sh_pending = 1;
		isr_soft_request(sh->sh_head->shd_ipl);
	}
}

#endif /* _KERNEL */
#endif	/* _SUN2_INTR_H */
