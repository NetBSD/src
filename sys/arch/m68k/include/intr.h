/*	$NetBSD: intr.h,v 1.5 2024/01/16 01:16:46 thorpej Exp $	*/

/*-
 * Copyright (c) 2023, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _M68k_INTR_H_
#define	_M68k_INTR_H_

#include <sys/types.h>
#include <machine/psl.h>

/*
 * Logical interrupt priority levels -- these are distinct from
 * the hardware interrupt priority levels of the m68k.
 */
#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1	/* clock software interrupts */
#define	IPL_SOFTBIO	2	/* block device software interrupts */
#define	IPL_SOFTNET	3	/* network software interrupts */
#define	IPL_SOFTSERIAL	4	/* serial device software interrupts */
#define	IPL_VM		5	/* all interrupts that can allocate memory */
#define	IPL_SCHED	6	/* scheduler / hard clock interrupts */
#define	IPL_HIGH	7	/* blocks all interrupts */
#define	NIPL		8

/*
 * Abstract ISR priorities.  These allow sorting of latency-sensitive
 * devices earlier on the shared auto-vectored interrupt lists.
 */
#define	ISRPRI_BIO		0	/* a block I/O device */
#define	ISRPRI_MISC		0	/* misc. devices */
#define	ISRPRI_NET		1	/* a network interface */
#define	ISRPRI_TTY		2	/* a serial port */
#define	ISRPRI_DISPLAY		2	/* display devices / framebuffers */
#define	ISRPRI_TTYNOBUF		3	/* a particularly bad serial port */
#define	ISRPRI_AUDIO		4	/* audio devices */

#if defined(_KERNEL) || defined(_KMEMUSER)
typedef struct {
	uint16_t _psl;		/* physical manifestation of logical IPL_* */
} ipl_cookie_t;
#endif

#ifdef _KERNEL
extern volatile int idepth;		/* interrupt depth */
extern const uint16_t ipl2psl_table[NIPL];

typedef int ipl_t;		/* logical IPL_* value */

static inline bool
cpu_intr_p(void)
{
	return idepth != 0;
}

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{
	return (ipl_cookie_t){._psl = ipl2psl_table[ipl]};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{
	return _splraise(icookie._psl);
}

/*
 * These are essentially constant equivalents of what's in
 * ipl2psl_table[] to avoid the memory reference.
 */
#define	splsoftclock()	_splraise(PSL_S | MACHINE_PSL_IPL_SOFTCLOCK)
#define	splsoftbio()	_splraise(PSL_S | MACHINE_PSL_IPL_SOFTBIO)
#define	splsoftnet()	_splraise(PSL_S | MACHINE_PSL_IPL_SOFTNET)
#define	splsoftserial()	_splraise(PSL_S | MACHINE_PSL_IPL_SOFTSERIAL)
#define	splvm()		_splraise(PSL_S | MACHINE_PSL_IPL_VM)
#define	splsched()	_splraise(PSL_S | MACHINE_PSL_IPL_SCHED)
#define	splhigh()	spl7()

/*
 * XXX TODO: Support for hardware-assisted soft interrupts (sun68k)
 * XXX and fast-soft-interrupts (others).
 */
#define	spl0()		_spl0()
#define	splx(s)		_splx(s)

#ifdef _M68K_INTR_PRIVATE
#include <sys/queue.h>

struct m68k_intrhand {
	LIST_ENTRY(m68k_intrhand)	ih_link;
	int				(*ih_func)(void *);
	void				*ih_arg;
	struct evcnt			*ih_evcnt;
	int				ih_ipl;	/* m68k IPL, not IPL_* */
	int				ih_vec;
	int				ih_isrpri;
};
LIST_HEAD(m68k_intrhand_list, m68k_intrhand);

struct m68k_ih_allocfuncs {
	struct m68k_intrhand *	(*alloc)(int km_flag);
	void			(*free)(struct m68k_intrhand *);
};
#else
struct m68k_ih_allocfuncs;
#endif /* _M68K_INTR_PRIVATE */

#include <sys/evcnt.h>

#ifdef __HAVE_LEGACY_INTRCNT
#define	m68k_count_intr(x)						\
do {									\
	extern u_int intrcnt[];						\
	intrcnt[(x)]++;							\
	curcpu()->ci_data.cpu_nintr++;					\
} while (/*CONSTCOND*/0)
#else
/*
 * This is exposed here so that platform-specific interrupt handlers
 * can access it.
 */
extern struct evcnt m68k_intr_evcnt[];

#define	m68k_count_intr(x)						\
do {									\
	/* 32-bit counter should be sufficient for m68k. */		\
	m68k_intr_evcnt[(x)].ev_count32++;				\
	curcpu()->ci_data.cpu_nintr++;					\
} while (/*CONSTCOND*/0)
#endif /* __HAVE_LEGACY_INTRCNT */

/*
 * Common m68k interrupt dispatch:
 *
 * ==> m68k_intr_init(const struct m68k_ih_allocfuncs *allocfuncs)
 *
 * Initialize the interrupt system.  If the platform needs to store
 * additional information in the interrupt handle, then it can provide
 * its own alloc/free routines.  Otherwise, pass NULL to get the default.
 * If a platform doesn't want the special allocator behavior, calling
 * this function is optional; it will be done for you on the first call
 * to m68k_intr_establish().
 *
 * ==> m68k_intr_establish(int (*func)(void *), void *arg,
 *         struct evcnt *ev, int vec, int ipl, int flags)
 *
 * Establish an interrupt handler.  If vec is 0, then the handler is
 * registered in the auto-vector list corresponding to the specified
 * m68k interrupt priroity level (this is NOT an IPL_* value).  Otherwise.
 * the handler is registered at the specified vector.
 *
 * Vectored interrupts are not sharable.  The interrupt vector must be
 * within the platform's "user vector" region, which is generally defined
 * as vectors 64-255, although some platforms may use vectors that start
 * below 64 (in which case, that platform must define MACHINE_USERVEC_START
 * to override the default).
 *
 * Vectored interrupt support is not included by default in order to reduce
 * the memory footprint.  If a platform wishes to enable vectored interrupts,
 * then it should define __HAVE_M68K_INTR_VECTORED in its <machine/types.h>
 * and genassym.cf.
 *
 * ==> m68k_intr_disestablish(void *ih)
 *
 * Removes a previously-established interrupt handler.  Returns true
 * if there are no more handlers on the list that handler was on.  This
 * information can be used to e.g. disable interrupts on a PIC.
 */
void	m68k_intr_init(const struct m68k_ih_allocfuncs *);
void	*m68k_intr_establish(int (*)(void *), void *, struct evcnt *,
	    int/*vec*/, int/*m68k ipl*/, int/*isrpri*/, int/*flags*/);
bool	m68k_intr_disestablish(void *);

#ifdef __HAVE_M68K_INTR_VECTORED
void	*m68k_intrvec_intrhand(int vec);	/* XXX */
#endif

#endif /* _KERNEL */

#endif /* _M68k_INTR_H_ */
