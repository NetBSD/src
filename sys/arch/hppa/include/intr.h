/*	$NetBSD: intr.h,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $	*/
/*	$OpenBSD: intr.h,v 1.26 2009/12/29 13:11:40 jsing Exp $	*/

/*-
 * Copyright (c) 1998, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Jason R. Thorpe, and by Matthew Fredette.
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

#ifndef _HPPA_INTR_H_
#define _HPPA_INTR_H_

#include <machine/psl.h>
#include <machine/intrdefs.h>

#include <sys/evcnt.h>

#ifndef _LOCORE

#ifdef _KERNEL

struct cpu_info;

/*
 * The maximum number of bits in a cpl value/spl mask, the maximum number of
 * bits in an interrupt request register, and the maximum number of interrupt
 * registers.
 */
#define	HPPA_INTERRUPT_BITS	(32)
#define	CPU_NINTS		HPPA_INTERRUPT_BITS	/* Use this one */

/*
 * This describes one HPPA interrupt register.
 */
struct hppa_interrupt_register {
	bool ir_iscpu;
	const char *ir_name;		/* name for this intr reg */
	struct cpu_info *ir_ci;		/* cpu this intr reg  */

	/*
	 * The virtual address of the mask, request and level
	 * registers.
	 */
	volatile int *ir_mask;
	volatile int *ir_req;
	volatile int *ir_level;

	/*
	 * This array has one entry for each bit in the interrupt request
	 * register.
	 *
	 * If the 24 most significant bits are set, the low 8 bits are the
	 * index of the hppa_interrupt_register that this interrupt bit leads
	 * to, with zero meaning that the interrupt bit is unused.
	 *
	 * Otherwise these bits correspond to hppa_interrupt_bits. That is,
	 * these bits are ORed to ipending_new in hppa_intr_ipending() when
	 * an interrupt happens.
	 *
	 * Note that this array is indexed by HP bit number, *not* by "normal"
	 * bit number.  In other words, the least significant bit in the inter-
	 * rupt register corresponds to array index 31.
	 */

	unsigned int ir_bits_map[HPPA_INTERRUPT_BITS];

#define	IR_BIT_MASK		0xffffff00
#define	IR_BIT_REG(x)		(IR_BIT_MASK | (x))
#define	IR_BIT_UNUSED		IR_BIT_REG(0)
#define	IR_BIT_USED_P(x)	(((x) & IR_BIT_MASK) != IR_BIT_MASK)
#define	IR_BIT_NESTED_P(x)	(((x) & IR_BIT_MASK) == IR_BIT_MASK)

	int ir_bits;		/* mask of allocatable bit numbers */
	int ir_rbits;		/* mask of reserved (for lasi/asp) bit numbers */
};

struct hppa_interrupt_bit {

	/*
	 * The interrupt register this bit is in.  Some handlers, e.g
	 * apic_intr, don't make use of an hppa_interrupt_register, but are
	 * nested.
	 */
	struct hppa_interrupt_register *ib_reg;

	/*
	 * The priority level associated with this bit, e.g, IPL_BIO, IPL_NET,
	 * etc.
	 */
	int ib_ipl;

	/*
	 * The spl mask for this bit.  This starts out as the spl bit assigned
	 * to this particular interrupt, and later gets fleshed out by the mask
	 * calculator to be the full mask that we need to raise spl to when we
	 * get this interrupt.
	 */
	int ib_spl;

	/* The interrupt name. */
	char ib_name[16];

	/* The interrupt event count. */
	struct evcnt ib_evcnt;

	/*
	 * The interrupt handler and argument for this bit.  If the argument is
	 * NULL, the handler gets the trapframe.
	 */
	int (*ib_handler)(void *);
	void *ib_arg;

};

void	hppa_intr_bootstrap(void);
void	hppa_intr_initialise(struct cpu_info *);
void	hppa_interrupt_register_establish(struct cpu_info *,
    struct hppa_interrupt_register *);
void *	hppa_intr_establish(int, int (*)(void *), void *,
    struct hppa_interrupt_register *, int);
int	hppa_intr_allocate_bit(struct hppa_interrupt_register *, int);
void	hppa_intr_enable(void);

/* splraise()/spllower() are in locore.S */
int splraise(int);
void spllower(int);

/*
 * Miscellaneous
 */
#define	spl0()		spllower(0)
#define	splx(x)		spllower(x)

typedef int ipl_t;
typedef struct {
	ipl_t _ipl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return splraise(icookie._ipl);
}

#include <sys/spl.h>
#endif

#define	setsoftast(l)	((l)->l_md.md_astpending = 1)

#ifdef MULTIPROCESSOR

struct cpu_info;

void	 hppa_ipi_init(struct cpu_info *);
int	 hppa_ipi_intr(void *arg);
int	 hppa_ipi_send(struct cpu_info *, u_long);
int	 hppa_ipi_broadcast(u_long);
#endif

#endif /* !_LOCORE */

#endif /* !_HPPA_INTR_H_ */
