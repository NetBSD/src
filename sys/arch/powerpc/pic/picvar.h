/*	$NetBSD: picvar.h,v 1.1.2.6 2007/05/03 03:30:48 macallan Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: picvar.h,v 1.1.2.6 2007/05/03 03:30:48 macallan Exp $");

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
	int (*pic_irq_is_enabled)(struct pic_ops *, int);
	void (*pic_enable_irq)(struct pic_ops *, int, int);
	void (*pic_reenable_irq)(struct pic_ops *, int, int);
	void (*pic_disable_irq)(struct pic_ops *, int);
	void (*pic_clear_irq)(struct pic_ops *, int);
	int (*pic_get_irq)(struct pic_ops *);
	void (*pic_ack_irq)(struct pic_ops *, int); /* IRQ numbner */
	void (*pic_establish_irq)(struct pic_ops *, int, int);
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
void	dummy_pic_establish_intr(struct pic_ops *, int, int);

/* address, number of IRQs, enable passthrough */
struct pic_ops *setup_openpic(uint32_t, int, int);

#endif /* PIC_VAR_H */
