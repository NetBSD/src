/* 	$NetBSD: arm_intr.h,v 1.1.2.6 2008/01/26 19:27:10 chris Exp $	*/

/*
 * Copyright (c) 2007 Christopher Gilbert
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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

#ifndef _ARM_INTR_H_
#define _ARM_INTR_H_

#include <arm/armreg.h>

#define IPL_NONE	0	/* nothing */
#define IPL_SOFTCLOCK	1	/* clock software interrupts */
#define IPL_SOFTBIO	2	/* block i/o */
#define IPL_SOFTNET	3	/* network software interrupts */
#define IPL_SOFTSERIAL	4	/* serial software interrupts */
#define IPL_VM		5	/* memory allocation */
#define	IPL_SCHED	6	/* clock */
#define IPL_HIGH	7	/* everything */

#define NIPL		8

#define IPL_STATCLOCK	IPL_SCHED	/* statclock */
#define	IPL_LPT		IPL_VM
#define IPL_IRQBUS	IPL_VM	/* this irq collates other irq */

#define	__NEWINTR	/* enables new hooks in cpu_fork()/cpu_switch() */

#ifndef _LOCORE
#include <arm/cpufunc.h>
#include <arm/cpu.h>

typedef uint8_t ipl_t;
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
	extern int _splraise(int);

	return _splraise(icookie._ipl);
}
#endif

#endif


#ifdef _ARM_CPU_INFO_
#ifndef _ARM_INTR_CI
#define _ARM_INTR_CI

#ifndef _LOCORE

#define current_ipl_level (curcpu()->ci_current_ipl_level)
/* 
 * note that ipls_pending must be altered with irqs disabled
 * as it's normally modified as a read, modify, write
 */
#define ipls_pending (curcpu()->ci_ipls_pending)
extern void arm_intr_splx_lifter(int newspl);

static inline void __attribute__((__unused__))
arm_intr_splx(int newspl)
{
    /* look for interrupts at the next ipl or higher */
    uint32_t iplvalue = (2 << newspl);

    /* Don't let the compiler re-order this code with preceding code */
    __insn_barrier();

    if (ipls_pending >= iplvalue)
        arm_intr_splx_lifter(newspl);

    current_ipl_level = newspl;
    return;
}

static inline int __attribute__((__unused__))
arm_intr_splraise(int ipl)
{
	int	old;

	old = current_ipl_level;
	if (ipl > current_ipl_level)
		current_ipl_level = ipl;

	/* Don't let the compiler re-order this code with subsequent code */
	__insn_barrier();

	return (old);
}

static inline int __attribute__((__unused__))
arm_intr_spllower(int ipl)
{
	int old = current_ipl_level;

	arm_intr_splx(ipl);
	return(old);
}

/* should only be defined in footbridge_intr.c */
#if !defined(ARM_SPL_NOINLINE)

#define splx(newspl)		arm_intr_splx(newspl)
#define	_spllower(ipl)		arm_intr_spllower(ipl)
#define	_splraise(ipl)		arm_intr_splraise(ipl)
void	_setsoftintr(int);

#else

int	_splraise(int);
int	_spllower(int);
void	splx(int);
void	_setsoftintr(int);

#endif /* ! ARM_SPL_NOINLINE */

/* public apis in arm_irqhandler.c*/
typedef void *irqhandler_t;
typedef struct irq_group *irqgroup_t;
typedef void *irq_hardware_cookie_t;
void arm_intr_init(void);
void arm_intr_enable_irqs(void);

/* this overlaps with armv6 pic stuff */
/* operations that are performed on the PIC */
struct pic_ops {
	void (*pic_set_irq_hardware_mask)(uint32_t intr_enabled);
	void (*pic_set_irq_hardware_type)(int irq_line, int type);
};

struct pic_softc {
	const struct pic_ops pic_ops;
	
	int pic_nirqs;			/* number of irq lines */
	uint32_t pic_pre_assigned_irqs; /* irqs that have a pre-assigned use */
	char pic_name[16];

	/* everything below here is initialized by the arm_intr code */
	TAILQ_ENTRY(pic_softc) irq_pics_list;	/* link on pics list */

	/* Software copy of the IRQs we have enabled in hardware */
	uint32_t pic_intr_enabled;

	/*
	 * Interrupt lines that have been enabled from software,
	 * This allows drivers to establish an interrupt handler, but
	 * disable the irq.  Common on acorn32.
	 */
	uint32_t pic_soft_enabled;

	struct intrline *pic_intrlines;	/* array of interrupt lines */
};

struct pic_softc *
arm_intr_register_pic(struct pic_softc *);

irqhandler_t
arm_intr_claim(struct pic_softc*, int irq, int type, int ipl, const char *name, int (*func)(void *), void *arg);
void arm_intr_disestablish(struct pic_softc *, irqhandler_t cookie);

void arm_intr_schedule(struct pic_softc *, irqhandler_t);

const struct evcnt * arm_intr_evcnt(struct pic_softc *, int ih);

void arm_intr_soft_enable_irq(struct pic_softc *, int irq);
void arm_intr_soft_disable_irq(struct pic_softc *, int irq);
void arm_intr_queue_irqs(struct pic_softc *pic, uint32_t hqpend);
void arm_intr_process_pending_ipls(struct clockframe *frame, int target_ipl_level);

/* autoconf's like to print out the IPL masks */
extern void arm_intr_print_all_masks(void);

#include <sys/device.h>
#include <sys/queue.h>
#include <machine/irqhandler.h>

#define	splsoft()	_splraise(IPL_SOFT)

#define	spl0()		(void)_spllower(IPL_NONE)
#define	spllowersoftclock() (void)_spllower(IPL_SOFTCLOCK)


#include <sys/spl.h>

/* Use generic software interrupt support. */
#include <arm/softintr.h>

struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_func)(void *);		/* handler */
	void *ih_arg;			/* arg for handler */
	int ih_ipl;			/* IPL_* */
	int ih_irq;			/* IRQ number */
};

#define	IRQNAMESIZE	sizeof("abcdefghijklmnopqrstuvwxyz irq 31")
#define	IST_UNUSABLE	-1      /* interrupt cannot be used */
#define	IST_NONE	0       /* none (dummy) */
#define	IST_PULSE	1       /* pulsed */
#define	IST_EDGE	2       /* edge-triggered */
#define	IST_LEVEL	3       /* level-triggered */

struct intrline {
	TAILQ_HEAD(, intrhand) il_handler_list;	/* handler list */
	TAILQ_ENTRY(intrline)  il_ipl_list;/* link on iplq list */
	struct evcnt il_ev;		/* event counter */
	int il_levels;			/* IPL_*'s this IRQ has */
	int il_ipl;			/* highest ipl this IRQ has */
	int il_ist;			/* share type */
	int il_irq;			/* irq line */
	int il_mask;
	struct pic_softc *il_pic;	/* which pic owns this irq */
	bool il_pending;		/* is there an irq pending */
	char il_name[IRQNAMESIZE];	/* interrupt name */
};

struct iplq {
	TAILQ_HEAD(, intrline) ipl_il_list;	/* irqs list */
	struct evcnt ipl_ev;		/* event counter */
	char ipl_name[IRQNAMESIZE];	/* ipl name */
};

#endif /* _LOCORE */
#endif
#endif	/* _ARM_INTR_H */
