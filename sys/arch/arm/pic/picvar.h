/*	$NetBSD: picvar.h,v 1.7.2.1 2014/08/20 00:02:47 tls Exp $	*/
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
#ifndef _ARM_PIC_PICVAR_H_
#define _ARM_PIC_PICVAR_H_

#ifdef MULTIPROCESSOR
#include <sys/kcpuset.h>
#endif

int	_splraise(int);
int	_spllower(int);
void	splx(int);
const char *
	intr_typename(int);

struct pic_softc;
struct intrsource;
struct cpu_info;

#define	IPI_AST			0	/* just get an interrupt */
#define	IPI_XCALL		1	/* xcall */
#define	IPI_NOP			2	/* just get an interrupt (armv6) */
#define	IPI_SHOOTDOWN		3	/* cause a tlb shootdown */
#define	IPI_DDB			4	/* enter DDB */
#define	IPI_GENERIC		5	/* generic IPI */
#ifdef __HAVE_PREEMPTION
#define	IPI_KPREEMPT		6	/* cause a preemption */
#define	NIPI			7
#else
#define	NIPI			6
#endif

int	pic_handle_intr(void *);
void	pic_mark_pending(struct pic_softc *pic, int irq);
void	pic_mark_pending_source(struct pic_softc *pic, struct intrsource *is);
uint32_t pic_mark_pending_sources(struct pic_softc *pic, size_t irq_base,
	    uint32_t pending);
void	*pic_establish_intr(struct pic_softc *pic, int irq, int ipl, int type,
	    int (*func)(void *), void *arg);
int	pic_alloc_irq(struct pic_softc *pic);
void	pic_disestablish_source(struct intrsource *is);
void	pic_do_pending_ints(register_t psw, int newipl, void *frame);
void	pic_dispatch(struct intrsource *is, void *frame);

void	*intr_establish(int irq, int ipl, int type, int (*func)(void *),
	    void *arg);
void	intr_disestablish(void *);
#ifdef MULTIPROCESSOR
void	intr_cpu_init(struct cpu_info *);
void	intr_ipi_send(const kcpuset_t *, u_long ipi);
#endif

#ifdef _INTR_PRIVATE

#include "opt_arm_intr_impl.h"

#include <sys/evcnt.h>
#include <sys/percpu.h>

#ifndef PIC_MAXPICS
#define PIC_MAXPICS	32
#endif
#ifndef PIC_MAXSOURCES
#define	PIC_MAXSOURCES	64
#endif
#ifndef PIC_MAXMAXSOURCES
#define	PIC_MAXMAXSOURCES	128
#endif

struct intrsource {
	int (*is_func)(void *);
	void *is_arg;
	struct pic_softc *is_pic;		/* owning PIC */
	uint8_t is_type;			/* IST_xxx */
	uint8_t is_ipl;				/* IPL_xxx */
	uint8_t is_irq;				/* local to pic */
	uint8_t is_iplidx;
	bool is_mpsafe;
	char is_source[16];
};

struct pic_percpu {
	struct evcnt *pcpu_evs;
	char *pcpu_name;
	uint32_t pcpu_magic;
};

#define	PICPERCPU_MAGIC		0xfeedface

struct pic_softc {
	const struct pic_ops *pic_ops;
	struct intrsource **pic_sources;
	volatile uint32_t pic_pending_irqs[(PIC_MAXSOURCES + 31) / 32];
	volatile uint32_t pic_blocked_irqs[(PIC_MAXSOURCES + 31) / 32];
	volatile uint32_t pic_pending_ipls;
	size_t pic_maxsources;
	percpu_t *pic_percpu;
	uint8_t pic_id;
	int16_t pic_irqbase;
	char pic_name[14];
};

struct pic_ops {
	void (*pic_unblock_irqs)(struct pic_softc *, size_t, uint32_t);
	void (*pic_block_irqs)(struct pic_softc *, size_t, uint32_t);
	int (*pic_find_pending_irqs)(struct pic_softc *);

	void (*pic_establish_irq)(struct pic_softc *, struct intrsource *);
	void (*pic_source_name)(struct pic_softc *, int, char *, size_t);

#ifdef __HAVE_PIC_SET_PRIORITY
	void (*pic_set_priority)(struct pic_softc *, int);
#endif
#ifdef MULTIPROCESSOR
	void (*pic_cpu_init)(struct pic_softc *, struct cpu_info *);
	void (*pic_ipi_send)(struct pic_softc *, const kcpuset_t *, u_long);
#endif
};

#ifdef __HAVE_PIC_SET_PRIORITY
/*
 * This is used to update a hardware pic with a value corresponding
 * to the ipl being set.
 */
struct cpu_info;
void	pic_set_priority(struct cpu_info *, int);
#else
/* Using an inline causes catch-22 problems with cpu.h */
#define	pic_set_priority(ci, newipl)	((void)((ci)->ci_cpl = (newipl)))
#endif

void	pic_add(struct pic_softc *, int);
void	pic_do_pending_int(void);
#ifdef MULTIPROCESSOR
int	pic_ipi_nop(void *);
int	pic_ipi_xcall(void *);
int	pic_ipi_generic(void *);
int	pic_ipi_shootdown(void *);
int	pic_ipi_ddb(void *);
#endif
#ifdef __HAVE_PIC_FAST_SOFTINTS
int	pic_handle_softint(void *);
#endif

extern struct pic_softc * pic_list[PIC_MAXPICS];
#endif /* _INTR_PRIVATE */

#endif /* _ARM_PIC_PICVAR_H_ */
