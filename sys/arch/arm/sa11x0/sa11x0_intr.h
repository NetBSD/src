/*	$NetBSD: sa11x0_intr.h,v 1.1.2.1 2007/07/31 15:31:43 rjs Exp $	*/

/*-
 * Copyright (c) 2001, The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Ichiro FUKUHARA.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#ifndef _SA11X0_INTR_H
#define _SA11X0_INTR_H

#define	ARM_IRQ_HANDLER	_C_LABEL(sa11x0_irq_handler)

#define NIRQS		0x20

#ifndef _LOCORE

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <machine/atomic.h>
#include <machine/intr.h>
#include <arm/softintr.h>

#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>

vaddr_t saipic_base;
#define read_icu(offset) (*(volatile uint32_t *)(saipic_base+(offset)))
#define write_icu(offset,value) \
 (*(volatile uint32_t *)(saipic_base+(offset))=(value))

#define IMASK_ICU	0
#define IMASK_SOFTINT	1
#define IMASK_WORDSHIFT 5	/* log2(32) */
#define IMASK_BITMASK	~((~0) << IMASK_WORDSHIFT)

/*
 * interrupt mask bit vector
 */
typedef struct {
	u_int32_t bits[2];
} imask_t /*__attribute__ ((aligned(16)))*/;

extern volatile int current_spl_level;
extern volatile imask_t intr_mask;
extern volatile int softint_pending;
extern imask_t sa11x0_imask[];
void sa11x0_do_pending(void);

static inline void imask_zero(imask_t *);
static inline void imask_zero_v(volatile imask_t *);
static inline void imask_dup_v(imask_t *, const volatile imask_t *);
static inline void imask_and(imask_t *, const imask_t *);
static inline void imask_andnot_v(volatile imask_t *, const imask_t *);
static inline void imask_andnot_icu_vv(volatile imask_t *, const volatile imask_t *);
static inline int imask_empty(const imask_t *);
static inline void imask_orbit(imask_t *, int);
static inline void imask_orbit_v(volatile imask_t *, int);
static inline void imask_clrbit(imask_t *, int);
static inline void imask_clrbit_v(volatile imask_t *, int);
static inline u_int32_t imask_andbit_v(const volatile imask_t *, int);
static inline int imask_test_v(const volatile imask_t *, const imask_t *);

static inline void
imask_zero(imask_t *idp)
{
	idp->bits[IMASK_ICU]  = 0;
	idp->bits[IMASK_SOFTINT] = 0;
}

static inline void
imask_zero_v(volatile imask_t *idp)
{
	idp->bits[IMASK_ICU]  = 0;
	idp->bits[IMASK_SOFTINT] = 0;
}

static inline void
imask_dup_v(imask_t *idp, const volatile imask_t *isp)
{
	*idp = *isp;
}

static inline void
imask_and(imask_t *idp, const imask_t *isp)
{
	idp->bits[IMASK_ICU]  &= isp->bits[IMASK_ICU];
	idp->bits[IMASK_SOFTINT] &= isp->bits[IMASK_SOFTINT];
}

static inline void
imask_andnot_v(volatile imask_t *idp, const imask_t *isp)
{
	idp->bits[IMASK_ICU]  &= ~isp->bits[IMASK_ICU]; 
	idp->bits[IMASK_SOFTINT] &= ~isp->bits[IMASK_SOFTINT];
}

static inline void
imask_andnot_icu_vv(volatile imask_t *idp, const volatile imask_t *isp)
{
	idp->bits[IMASK_ICU]  &= ~isp->bits[IMASK_ICU]; 
}

static inline int
imask_empty(const imask_t *isp)
{
	return (! (isp->bits[IMASK_ICU] | isp->bits[IMASK_SOFTINT]));
}

static inline void
imask_orbit(imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] |= (1 << (bitno&IMASK_BITMASK));
}

static inline void
imask_orbit_v(volatile imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] |= (1 << (bitno&IMASK_BITMASK));
}

static inline void
imask_clrbit(imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] &= ~(1 << (bitno&IMASK_BITMASK));
}

static inline void
imask_clrbit_v(volatile imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] &= ~(1 << (bitno&IMASK_BITMASK));
}

static inline u_int32_t
imask_andbit_v(const volatile imask_t *idp, int bitno)
{
	return idp->bits[bitno>>IMASK_WORDSHIFT] & (1 << (bitno&IMASK_BITMASK));
}

static inline int
imask_test_v(const volatile imask_t *idp, const imask_t *isp)
{
	return ((idp->bits[IMASK_ICU]  & isp->bits[IMASK_ICU]) ||
		(idp->bits[IMASK_SOFTINT] & isp->bits[IMASK_SOFTINT]));
}

#define SI_TO_IRQBIT(si)  (1U<<(si+32))

static inline void
sa11x0_setipl(int new)
{

	current_spl_level = new;
	intr_mask = sa11x0_imask[current_spl_level];
	write_icu(SAIPIC_MR, intr_mask.bits[IMASK_ICU]);
}


static inline void
sa11x0_splx(int new)
{
	int psw;

	psw = disable_interrupts(I32_bit);
	sa11x0_setipl(new);
	restore_interrupts(psw);

	/* If there are software interrupts to process, do it. */
	if (softint_pending & intr_mask.bits[IMASK_SOFTINT])
		sa11x0_do_pending();
}


static inline int
sa11x0_splraise(int ipl)
{
	int old, psw;

	old = current_spl_level;
	if (ipl > current_spl_level) {
		psw = disable_interrupts(I32_bit);
		sa11x0_setipl(ipl);
		restore_interrupts(psw);
	}

	return old;
}

static inline int
sa11x0_spllower(int ipl)
{
	int old = current_spl_level;
	int psw = disable_interrupts(I32_bit);

	sa11x0_splx(ipl);
	restore_interrupts(psw);
	return old;
}

static inline void
sa11x0_setsoftintr(int si)
{

	atomic_set_bit((u_int *)__UNVOLATILE(&softint_pending),
	    SI_TO_IRQBIT(si));

	/* Process unmasked pending soft interrupts. */
	if (softint_pending & intr_mask.bits[IMASK_SOFTINT])
		sa11x0_do_pending();
}


int	_splraise(int);
int	_spllower(int);
void	splx(int);
void	_setsoftintr(int);

#if !defined(ARM_SPL_NOINLINE)

#define splx(new)		sa11x0_splx(new)
#define	_spllower(ipl)		sa11x0_spllower(ipl)
#define	_splraise(ipl)		sa11x0_splraise(ipl)
#define	_setsoftintr(si)	sa11x0_setsoftintr(si)

#endif	/* !ARM_SPL_NOINTR */

/*
 * This function *MUST* be called very early on in a port's
 * initarm() function, before ANY spl*() functions are called.
 *
 * The parameter is the virtual address of the SA11x0's Interrupt
 * Controller registers.
 */
void sa11x0_intr_bootstrap(vaddr_t);

void sa11x0_irq_handler(void *);
void *sa11x0_intr_establish(int irqno, int level,
			    int (*func)(void *), void *cookie);
void sa11x0_intr_disestablish(void *);
void sa11x0_update_intr_masks(int irqno, int level);

#endif /* ! _LOCORE */

#endif /* _SA11X0_INTR_H */
