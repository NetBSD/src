/*	$NetBSD: lwpctl.h,v 1.1.18.2 2008/01/09 01:58:11 matt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#if !defined(_SYS_LWPCTL_H_)
#define	_SYS_LWPCTL_H_

/*
 * Note on compatibility:
 *
 * This must be the same size for both 32 and 64-bit processes, since
 * the same format will be used by both.
 *
 * Removal of unused fields is OK, as long as the change in layout
 * does not affect supported fields.
 *
 * It is OK to add fields to this structure, since the kernel allocates
 * the space.  Re-use of fields is more complicated - see the feature
 * word passed to the system call.
 *
 * lc_reserved is just to reduce the size of the bitmap slightly, and
 * can be reused.
 */
typedef struct lwpctl {
	volatile int	lc_curcpu;
	int		lc_reserved;
} lwpctl_t;

#define	LWPCTL_CPU_NONE		(-1)
#define	LWPCTL_CPU_EXITED	(-2)

#define	LWPCTL_FEATURE_CURCPU	0x00000001

#if defined(_KERNEL)

#include <sys/mutex.h>

#include <uvm/uvm_extern.h>

typedef struct lcpage {
	TAILQ_ENTRY(lcpage) lcp_chain;
	vaddr_t		lcp_uaddr;
	vaddr_t		lcp_kaddr;
	u_int		lcp_nfree;
	u_int		lcp_rotor;
	u_int		lcp_bitmap[1];
} lcpage_t;

typedef struct lcproc {
	kmutex_t	lp_lock;
	struct uvm_object *lp_uao;
	TAILQ_HEAD(,lcpage) lp_pages;
	vaddr_t		lp_cur;
	vaddr_t		lp_max;
	vaddr_t		lp_uva;
} lcproc_t;

#define	LWPCTL_PER_PAGE		((PAGE_SIZE / sizeof(lwpctl_t)) & ~31)
#define	LWPCTL_BITMAP_ENTRIES	(LWPCTL_PER_PAGE >> 5)
#define	LWPCTL_BITMAP_SZ	(LWPCTL_BITMAP_ENTRIES * sizeof(u_int))
#define	LWPCTL_LCPAGE_SZ	\
    (sizeof(lcpage_t) - sizeof(u_int) + LWPCTL_BITMAP_SZ)
#define	LWPCTL_UAREA_SZ		\
    (round_page(MAX_LWP_PER_PROC * sizeof(lwpctl_t)))

int	lwp_ctl_alloc(vaddr_t *);
void	lwp_ctl_free(lwp_t *);
void	lwp_ctl_exit(void);

#endif /* defined(_KERNEL) */

#endif /* !_SYS_LWPCTL_H_ */
