/*	$NetBSD: s3c2xx0_intr.h,v 1.3 2003/07/30 18:25:50 bsh Exp $ */

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Derived from i80321_intr.h */

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _S3C2XX0_INTR_H_
#define _S3C2XX0_INTR_H_

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <machine/atomic.h>
#include <machine/intr.h>
#include <arm/softintr.h>

#include <arm/s3c2xx0/s3c2xx0reg.h>

typedef int (* s3c2xx0_irq_handler_t)(void *);

extern volatile uint32_t *s3c2xx0_intr_mask_reg;

extern __volatile int current_spl_level;
extern __volatile int intr_mask;
extern __volatile int global_intr_mask;
extern __volatile int softint_pending;
extern int s3c2xx0_imask[];
extern int s3c2xx0_ilevel[];

void s3c2xx0_do_pending(int);
void s3c2xx0_update_intr_masks( int, int );

static __inline void
s3c2xx0_mask_interrupts(int mask)
{
	int save = disable_interrupts(I32_bit);
	global_intr_mask |= mask;
	*s3c2xx0_intr_mask_reg = intr_mask & ~global_intr_mask;
	restore_interrupts(save);
}

static __inline void
s3c2xx0_unmask_interrupts(int mask)
{
	int save = disable_interrupts(I32_bit);
	global_intr_mask &= ~mask;
	*s3c2xx0_intr_mask_reg = intr_mask & ~global_intr_mask;
	restore_interrupts(save);
}

static __inline void
s3c2xx0_setipl(int new)
{
	current_spl_level = new;
	intr_mask = s3c2xx0_imask[current_spl_level];
	*s3c2xx0_intr_mask_reg = intr_mask & ~global_intr_mask;
}


static __inline void
s3c2xx0_splx(int new)
{
	int psw;

	psw = disable_interrupts(I32_bit);
	s3c2xx0_setipl(new);
	restore_interrupts(psw);

	/* If there are software interrupts to process, do it. */
	if (softint_pending & intr_mask)
		s3c2xx0_do_pending(0);
}


static __inline int
s3c2xx0_splraise(int ipl)
{
	int	old, psw;

	old = current_spl_level;
	if( ipl > current_spl_level ){
		psw = disable_interrupts(I32_bit);
		s3c2xx0_setipl(ipl);
		restore_interrupts(psw);
	}

	return (old);
}

static __inline int
s3c2xx0_spllower(int ipl)
{
	int old = current_spl_level;
	int psw = disable_interrupts(I32_bit);
	s3c2xx0_splx(ipl);
	restore_interrupts(psw);
	return(old);
}

static __inline void
s3c2xx0_setsoftintr(int si)
{
	atomic_set_bit( (u_int *)&softint_pending, SI_TO_IRQBIT(si) );

	/* Process unmasked pending soft interrupts. */
	if ( softint_pending & intr_mask )
		s3c2xx0_do_pending(0);
}


int	_splraise(int);
int	_spllower(int);
void	splx(int);
void	_setsoftintr(int);

#if !defined(EVBARM_SPL_NOINLINE)

#define splx(new)		s3c2xx0_splx(new)
#define	_spllower(ipl)		s3c2xx0_spllower(ipl)
#define	_splraise(ipl)		s3c2xx0_splraise(ipl)
#define	_setsoftintr(si)	s3c2xx0_setsoftintr(si)

#endif	/* !EVBARM_SPL_NOINTR */


/*
 * interrupt dispatch table.
 */
#ifdef MULTIPLE_HANDLERS_ON_ONE_IRQ
struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	s3c2xx0_irq_handler_t ih_func;	/* handler */
	void *ih_arg;			/* arg for handler */
};
#endif

struct s3c2xx0_intr_dispatch {
#ifdef MULTIPLE_HANDLERS_ON_ONE_IRQ
	TAILQ_HEAD(,intrhand) list;
#else
	s3c2xx0_irq_handler_t func;
#endif
	void *cookie;		/* NULL for stackframe */
	int level;
	/* struct evbnt ev; */
};

/* used by s3c2{80,40,41}0 interrupt handler */
void s3c2xx0_intr_init(struct s3c2xx0_intr_dispatch *, int );

#endif _S3C2XX0_INTR_H_

