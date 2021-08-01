/* $NetBSD: intr.h,v 1.83.6.1 2021/08/01 22:42:01 thorpej Exp $ */

/*-
 * Copyright (c) 2000, 2001, 2002 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _ALPHA_INTR_H_
#define _ALPHA_INTR_H_

#include <sys/evcnt.h>
#include <machine/alpha_cpu.h>

/*
 * The Alpha System Control Block.  This is 8k long, and you get
 * 16 bytes per vector (i.e. the vector numbers are spaced 16
 * apart).
 *
 * This is sort of a "shadow" SCB -- rather than the CPU jumping
 * to (SCBaddr + (16 * vector)), like it does on the VAX, we get
 * a vector number in a1.  We use the SCB to look up a routine/arg
 * and jump to it.
 *
 * Since we use the SCB only for I/O interrupts, we make it shorter
 * than normal, starting it at vector 0x800 (the start of the I/O
 * interrupt vectors).
 */
#define	SCB_IOVECBASE	0x0800
#define	SCB_VECSIZE	0x0010
#define	SCB_SIZE	0x2000

#define	SCB_VECTOIDX(x)	((x) >> 4)
#define	SCB_IDXTOVEC(x)	((x) << 4)

#define	SCB_NIOVECS	SCB_VECTOIDX(SCB_SIZE - SCB_IOVECBASE)

struct scbvec {
	void	(*scb_func)(void *, u_long);
	void	*scb_arg;
};

/*
 * Alpha interrupts come in at one of 3 levels:
 *
 *	i/o level 1
 *	i/o level 2
 *	clock level
 *
 * However, since we do not have any way to know which hardware
 * level a particular i/o interrupt comes in on, we have to
 * whittle it down to 2.  In addition, there are 2 software interrupt
 * levels available to system software.
 */

#define	IPL_NONE	ALPHA_PSL_IPL_0
#define	IPL_SOFTCLOCK	ALPHA_PSL_IPL_SOFT_LO
#define	IPL_SOFTBIO	ALPHA_PSL_IPL_SOFT_LO
#ifdef __HAVE_FAST_SOFTINTS
#define	IPL_SOFTNET	ALPHA_PSL_IPL_SOFT_HI
#define	IPL_SOFTSERIAL	ALPHA_PSL_IPL_SOFT_HI
#else
#define	IPL_SOFTNET	ALPHA_PSL_IPL_SOFT_LO
#define	IPL_SOFTSERIAL	ALPHA_PSL_IPL_SOFT_LO
#endif /* __HAVE_FAST_SOFTINTS */
#define	IPL_VM		ALPHA_PSL_IPL_IO_HI
#define	IPL_SCHED	ALPHA_PSL_IPL_CLOCK
#define	IPL_HIGH	ALPHA_PSL_IPL_HIGH

#define	SOFTINT_CLOCK_MASK	__BIT(SOFTINT_CLOCK)
#define	SOFTINT_BIO_MASK	__BIT(SOFTINT_BIO)
#define	SOFTINT_NET_MASK	__BIT(SOFTINT_NET)
#define	SOFTINT_SERIAL_MASK	__BIT(SOFTINT_SERIAL)

#define	ALPHA_IPL1_SOFTINTS	(SOFTINT_CLOCK_MASK | SOFTINT_BIO_MASK)
#define	ALPHA_IPL2_SOFTINTS	(SOFTINT_NET_MASK | SOFTINT_SERIAL_MASK)

#define	ALPHA_ALL_SOFTINTS	(ALPHA_IPL1_SOFTINTS | ALPHA_IPL2_SOFTINTS)

typedef int ipl_t;
typedef struct {
	uint8_t _psl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{
	return (ipl_cookie_t){._psl = (uint8_t)ipl};
}

#define	IST_UNUSABLE	-1	/* interrupt cannot be used */
#define	IST_NONE	0	/* none (dummy) */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifdef	_KERNEL

/* IPL-lowering/restoring macros */
void	spllower(int);

#define	splx(s)		spllower(s)
#define	spl0()		spllower(ALPHA_PSL_IPL_0)

/* IPL-raising functions/macros */
static __inline int
_splraise(int s)
{
	int cur = (int)alpha_pal_rdps();
	return (s > cur ? (int)alpha_pal_swpipl(s) : cur);
}

#define	splraiseipl(icookie)	_splraise((icookie)._psl)

#include <sys/spl.h>

/* Fast soft interrupt dispatch. */
void	alpha_softint_dispatch(int);
void	alpha_softint_switchto(struct lwp *, int, struct lwp *);

/*
 * Interprocessor interrupts.  In order how we want them processed.
 */
#define	ALPHA_IPI_HALT			(1UL << 0)
#define	ALPHA_IPI_PRIMARY_CC		(1UL << 1)
#define	ALPHA_IPI_SHOOTDOWN		(1UL << 2)
#define	ALPHA_IPI_AST			(1UL << 3)
#define	ALPHA_IPI_PAUSE			(1UL << 4)
#define	ALPHA_IPI_XCALL			(1UL << 5)
#define	ALPHA_IPI_GENERIC		(1UL << 6)

#define	ALPHA_NIPIS			7	/* must not exceed 64 */

struct cpu_info;
struct trapframe;

void	alpha_ipi_init(struct cpu_info *);
void	alpha_ipi_process(struct cpu_info *, struct trapframe *);
void	alpha_send_ipi(unsigned long, unsigned long);
void	alpha_broadcast_ipi(unsigned long);
void	alpha_multicast_ipi(unsigned long, unsigned long);

/*
 * Alpha shared-interrupt-line common code.
 */

#define	ALPHA_INTR_MPSAFE		0x01

struct alpha_shared_intrhand {
	TAILQ_ENTRY(alpha_shared_intrhand)
		ih_q;
	struct alpha_shared_intr *ih_intrhead;
	int	(*ih_fn)(void *);
	void	*ih_arg;
	int	(*ih_real_fn)(void *);
	void	*ih_real_arg;
	int	ih_level;
	int	ih_type;
	unsigned int ih_num;
};

struct alpha_shared_intr {
	TAILQ_HEAD(,alpha_shared_intrhand)
		intr_q;
	struct evcnt intr_evcnt;
	char	*intr_string;
	void	*intr_private;
	struct cpu_info *intr_cpu;
	int	intr_sharetype;
	int	intr_dfltsharetype;
	int	intr_nstrays;
	int	intr_maxstrays;
};

#define	ALPHA_SHARED_INTR_DISABLE(asi, num)				\
	((asi)[num].intr_maxstrays != 0 &&				\
	 (asi)[num].intr_nstrays == (asi)[num].intr_maxstrays)

struct alpha_shared_intr *alpha_shared_intr_alloc(unsigned int);
int	alpha_shared_intr_dispatch(struct alpha_shared_intr *,
	    unsigned int);
struct alpha_shared_intrhand *
	alpha_shared_intr_alloc_intrhand(struct alpha_shared_intr *,
	    unsigned int, int, int, int, int (*)(void *), void *,
	    const char *);
void	alpha_shared_intr_free_intrhand(struct alpha_shared_intrhand *);
bool	alpha_shared_intr_link(struct alpha_shared_intr *,
	    struct alpha_shared_intrhand *, const char *);
void	alpha_shared_intr_unlink(struct alpha_shared_intr *,
	    struct alpha_shared_intrhand *, const char *);
int	alpha_shared_intr_get_sharetype(struct alpha_shared_intr *,
	    unsigned int);
int	alpha_shared_intr_isactive(struct alpha_shared_intr *,
	    unsigned int);
int	alpha_shared_intr_firstactive(struct alpha_shared_intr *,
	    unsigned int);
void	alpha_shared_intr_set_dfltsharetype(struct alpha_shared_intr *,
	    unsigned int, int);
void	alpha_shared_intr_set_maxstrays(struct alpha_shared_intr *,
	    unsigned int, int);
void	alpha_shared_intr_reset_strays(struct alpha_shared_intr *,
	    unsigned int);
void	alpha_shared_intr_stray(struct alpha_shared_intr *, unsigned int,
	    const char *);
void	alpha_shared_intr_set_private(struct alpha_shared_intr *,
	    unsigned int, void *);
void	*alpha_shared_intr_get_private(struct alpha_shared_intr *,
	    unsigned int);
void	alpha_shared_intr_set_cpu(struct alpha_shared_intr *,
	    unsigned int, struct cpu_info *ci);
struct cpu_info	*
	alpha_shared_intr_get_cpu(struct alpha_shared_intr *,
	    unsigned int);
void	alpha_shared_intr_set_string(struct alpha_shared_intr *,
	    unsigned int, char *);
const char *alpha_shared_intr_string(struct alpha_shared_intr *,
	    unsigned int);
struct evcnt *alpha_shared_intr_evcnt(struct alpha_shared_intr *,
	    unsigned int);

extern struct scbvec scb_iovectab[];
extern void (*alpha_intr_redistribute)(void);

void	scb_init(void);
void	scb_set(u_long, void (*)(void *, u_long), void *);
u_long	scb_alloc(void (*)(void *, u_long), void *);
void	scb_free(u_long);

#define	SCB_ALLOC_FAILED	((u_long) -1)

#endif /* _KERNEL */
#endif /* ! _ALPHA_INTR_H_ */
