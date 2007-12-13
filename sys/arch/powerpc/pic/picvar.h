/*	$NetBSD: picvar.h,v 1.2.12.1 2007/12/13 05:05:21 yamt Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: picvar.h,v 1.2.12.1 2007/12/13 05:05:21 yamt Exp $");

#ifndef PIC_VAR_H
#define PIC_VAR_H

#include <machine/intr.h>

struct pic_ops {
	void *pic_cookie;	/* private stuff / hardware info */
	int pic_intrbase;	/* global number of the 1st IRQ we handle */
	int pic_numintrs;	/* how many IRQs do we handle? */
	/*
	 * all functions that take an IRQ number as argument need a local
	 * interrupt number
	 */
	void (*pic_enable_irq)(struct pic_ops *, int, int);
	void (*pic_reenable_irq)(struct pic_ops *, int, int);
	void (*pic_disable_irq)(struct pic_ops *, int);
	int (*pic_get_irq)(struct pic_ops *, int); /* PIC_GET_* */
	void (*pic_ack_irq)(struct pic_ops *, int); /* IRQ numbner */
	/* IRQ number, type, priority */
	void (*pic_establish_irq)(struct pic_ops *, int, int, int);
	/* finish setup after CPUs are attached */
	void (*pic_finish_setup)(struct pic_ops *);
	char pic_name[16];
};

struct intr_source {
	int is_type;
	int is_level;
	int is_hwirq;
	int is_mask;
	struct intrhand *is_hand;
	struct pic_ops *is_pic;
	struct evcnt is_ev;
	char is_source[16];
};

struct i8259_ops {
	struct pic_ops pic;
	uint32_t pending_events;
	uint32_t enable_mask;
	uint32_t irqs; 
};

/*
 * add a pic, fill in pic_intrbase, return pic_intrbase on success, 
 * -1 otherwise
 * the PIC must be initialized and ready for use
 */
int	pic_add(struct pic_ops *);

void	pic_do_pending_int(void);
void	pic_enable_irq(int);
void	pic_disable_irq(int);
int	pic_handle_intr(void *);
void	pic_mark_pending(int);
void	pic_ext_intr(void);
void	pic_init(void);
const char *intr_typename(int);
void	dummy_pic_establish_intr(struct pic_ops *, int, int, int);

/* this is called after attaching CPUs so PICs can setup interrupt routing */
void pic_finish_setup(void);

/* address, enable passthrough */
#define PIC_IVR_IBM	0
#define PIC_IVR_MOT	1
#define PIC_GET_IRQ	0
#define PIC_GET_RECHECK	1
struct pic_ops *setup_openpic(void *, int);
struct pic_ops *setup_prepivr(int);
struct pic_ops *setup_i8259(void);

/* i8259 common decls */
void i8259_initialize(void);  
void i8259_enable_irq(struct pic_ops *, int, int);
void i8259_disable_irq(struct pic_ops *, int);
void i8259_ack_irq(struct pic_ops *, int);
int i8259_get_irq(struct pic_ops *, int);

/* IPI handler */
int cpuintr(void *);
/* XXX - may need to be PIC specific */
#define IPI_VECTOR 64

#endif /* PIC_VAR_H */
