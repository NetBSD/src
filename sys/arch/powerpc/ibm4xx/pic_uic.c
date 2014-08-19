/*	$NetBSD: pic_uic.c,v 1.3.6.1 2014/08/20 00:03:19 tls Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic_uic.c,v 1.3.6.1 2014/08/20 00:03:19 tls Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/evcnt.h>
#include <sys/cpu.h>

#include <machine/intr.h>
#include <machine/psl.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/cpu.h>

#include <powerpc/pic/picvar.h>


/*
 * Number of interrupts (hard + soft), irq number legality test,
 * mapping of irq number to mask and a way to pick irq number
 * off a mask of active intrs.
 */
#define	IRQ_TO_MASK(irq) 	(0x80000000UL >> ((irq) & 0x1f))
#define	IRQ_OF_MASK(mask) 	__builtin_clz(mask)

static void	uic_enable_irq(struct pic_ops *, int, int);
static void	uic_disable_irq(struct pic_ops *, int);
static int	uic_get_irq(struct pic_ops *, int);
static void	uic_ack_irq(struct pic_ops *, int);
static void	uic_establish_irq(struct pic_ops *, int, int, int);

struct uic {
	uint32_t uic_intr_enable;	/* cached intr enable mask */
	uint32_t (*uic_mf_intr_status)(void);
	uint32_t (*uic_mf_intr_enable)(void);
	void (*uic_mt_intr_enable)(uint32_t);
	void (*uic_mt_intr_ack)(uint32_t);
};

/*
 * Platform specific code may override any of the above.
 */
#ifdef PPC_IBM403

#include <powerpc/ibm4xx/dcr403cgx.h>

static uint32_t
uic403_mfdcr_intr_status(void)
{
	return mfdcr(DCR_EXISR);
}

static uint32_t
uic403_mfdcr_intr_enable(void)
{
	return mfdcr(DCR_EXIER);
}

static void
uic403_mtdcr_intr_ack(uint32_t v)
{
	mtdcr(DCR_EXISR, v);
}

static void
uic403_mtdcr_intr_enable(uint32_t v)
{
	mtdcr(DCR_EXIER, v);
}

struct uic uic403 = {
	.uic_intr_enable =	0,
	.uic_mf_intr_status =	uic403_mfdcr_intr_status,
	.uic_mf_intr_enable =	uic403_mfdcr_intr_enable,
	.uic_mt_intr_enable =	uic403_mtdcr_intr_enable,
	.uic_mt_intr_ack =	uic403_mtdcr_intr_ack,
};

struct pic_ops pic_uic403 = {
	.pic_cookie = &uic403,
	.pic_numintrs = 32,
	.pic_enable_irq = uic_enable_irq,
	.pic_reenable_irq = uic_enable_irq,
	.pic_disable_irq = uic_disable_irq,
	.pic_establish_irq = uic_establish_irq,
	.pic_get_irq = uic_get_irq,
	.pic_ack_irq = uic_ack_irq,
	.pic_finish_setup = NULL,
	.pic_name = "uic0"
};

#else /* Generic 405/440/460 Universal Interrupt Controller */

#include <powerpc/ibm4xx/dcr4xx.h>

#include "opt_uic.h"

/* 405EP/405GP/405GPr/Virtex-4 */

static uint32_t
uic0_mfdcr_intr_status(void)
{
	return mfdcr(DCR_UIC0_BASE + DCR_UIC_MSR);
}

static uint32_t
uic0_mfdcr_intr_enable(void)
{
	return mfdcr(DCR_UIC0_BASE + DCR_UIC_ER);
}

static void
uic0_mtdcr_intr_ack(uint32_t v)
{
	mtdcr(DCR_UIC0_BASE + DCR_UIC_SR, v);
}

static void
uic0_mtdcr_intr_enable(uint32_t v)
{
	mtdcr(DCR_UIC0_BASE + DCR_UIC_ER, v);
}

struct uic uic0 = {
	.uic_intr_enable =	0,
	.uic_mf_intr_status =	uic0_mfdcr_intr_status,
	.uic_mf_intr_enable =	uic0_mfdcr_intr_enable,
	.uic_mt_intr_enable =	uic0_mtdcr_intr_enable,
	.uic_mt_intr_ack =	uic0_mtdcr_intr_ack,
};

struct pic_ops pic_uic0 = {
	.pic_cookie = &uic0,
	.pic_numintrs = 32,
	.pic_enable_irq = uic_enable_irq,
	.pic_reenable_irq = uic_enable_irq,
	.pic_disable_irq = uic_disable_irq,
	.pic_establish_irq = uic_establish_irq,
	.pic_get_irq = uic_get_irq,
	.pic_ack_irq = uic_ack_irq,
	.pic_finish_setup = NULL,
	.pic_name = "uic0"
};

#ifdef MULTIUIC

/* 440EP/440GP/440SP/405EX/440SPe/440GX */

static uint32_t
uic1_mfdcr_intr_status(void)
{
	return mfdcr(DCR_UIC1_BASE + DCR_UIC_MSR);
}

static uint32_t
uic1_mfdcr_intr_enable(void)
{
	return mfdcr(DCR_UIC1_BASE + DCR_UIC_ER);
}

static void
uic1_mtdcr_intr_ack(uint32_t v)
{
	mtdcr(DCR_UIC1_BASE + DCR_UIC_SR, v);
}

static void
uic1_mtdcr_intr_enable(uint32_t v)
{
	mtdcr(DCR_UIC1_BASE + DCR_UIC_ER, v);
}

extern struct pic_ops pic_uic1;

static void
uic1_finish_setup(struct pic_ops *pic)
{
	intr_establish(30, IST_LEVEL, IPL_HIGH, pic_handle_intr, &pic_uic1);
}

struct uic uic1 = {
	.uic_intr_enable =	0,
	.uic_mf_intr_status =	uic1_mfdcr_intr_status,
	.uic_mf_intr_enable =	uic1_mfdcr_intr_enable,
	.uic_mt_intr_enable =	uic1_mtdcr_intr_enable,
	.uic_mt_intr_ack =	uic1_mtdcr_intr_ack,
};

struct pic_ops pic_uic1 = {
	.pic_cookie = &uic1,
	.pic_numintrs = 32,
	.pic_enable_irq = uic_enable_irq,
	.pic_reenable_irq = uic_enable_irq,
	.pic_disable_irq = uic_disable_irq,
	.pic_establish_irq = uic_establish_irq,
	.pic_get_irq = uic_get_irq,
	.pic_ack_irq = uic_ack_irq,
	.pic_finish_setup = uic1_finish_setup,
	.pic_name = "uic1"
};

/* 440EP/440GP/440SP/405EX/440SPe */

static uint32_t
uic2_mfdcr_intr_status(void)
{
	return mfdcr(DCR_UIC2_BASE + DCR_UIC_MSR);
}

static uint32_t
uic2_mfdcr_intr_enable(void)
{
	return mfdcr(DCR_UIC2_BASE + DCR_UIC_ER);
}

static void
uic2_mtdcr_intr_ack(uint32_t v)
{
	mtdcr(DCR_UIC2_BASE + DCR_UIC_SR, v);
}

static void
uic2_mtdcr_intr_enable(uint32_t v)
{
	mtdcr(DCR_UIC2_BASE + DCR_UIC_ER, v);
}

extern struct pic_ops pic_uic2;

static void
uic2_finish_setup(struct pic_ops *pic)
{
	intr_establish(28, IST_LEVEL, IPL_HIGH, pic_handle_intr, &pic_uic2);
}

static struct uic uic2 = {
	.uic_intr_enable =	0,
	.uic_mf_intr_status =	uic2_mfdcr_intr_status,
	.uic_mf_intr_enable =	uic2_mfdcr_intr_enable,
	.uic_mt_intr_enable =	uic2_mtdcr_intr_enable,
	.uic_mt_intr_ack =	uic2_mtdcr_intr_ack,
};

struct pic_ops pic_uic2 = {
	.pic_cookie = &uic2,
	.pic_numintrs = 32,
	.pic_enable_irq = uic_enable_irq,
	.pic_reenable_irq = uic_enable_irq,
	.pic_disable_irq = uic_disable_irq,
	.pic_establish_irq = uic_establish_irq,
	.pic_get_irq = uic_get_irq,
	.pic_ack_irq = uic_ack_irq,
	.pic_finish_setup = uic2_finish_setup,
	.pic_name = "uic2"
};

#endif /* MULTIUIC */
#endif /* !PPC_IBM403 */

/* Write External Enable Immediate */
#define	wrteei(en) 		__asm volatile ("wrteei %0" : : "K"(en))

/* Enforce In Order Execution of I/O */
#define	eieio() 		__asm volatile ("eieio")

/*
 * Set up interrupt mapping array.
 */
void
intr_init(void)
{
#ifdef PPC_IBM403
	struct pic_ops * const pic = &pic_uic403;
#else
	struct pic_ops * const pic = &pic_uic0;
#endif
	struct uic * const uic = pic->pic_cookie;

	uic->uic_mt_intr_enable(0x00000000); 	/* mask all */
	uic->uic_mt_intr_ack(0xffffffff);	/* acknowledge all */

	pic_add(pic);
}

static void
uic_disable_irq(struct pic_ops *pic, int irq)
{
	struct uic * const uic = pic->pic_cookie;
	const uint32_t irqmask = IRQ_TO_MASK(irq);
	if ((uic->uic_intr_enable & irqmask) == 0)
		return;
	uic->uic_intr_enable ^= irqmask;
	(*uic->uic_mt_intr_enable)(uic->uic_intr_enable);
#ifdef IRQ_DEBUG
	printf("%s: %s: irq=%d, mask=%08x\n", __func__,
	    pic->pic_name, irq, irqmask);
#endif
}

static void
uic_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct uic * const uic = pic->pic_cookie;
	const uint32_t irqmask = IRQ_TO_MASK(irq);
	if ((uic->uic_intr_enable & irqmask) != 0)
		return;
	uic->uic_intr_enable ^= irqmask;
	(*uic->uic_mt_intr_enable)(uic->uic_intr_enable);
#ifdef IRQ_DEBUG
	printf("%s: %s: irq=%d, mask=%08x\n", __func__,
	    pic->pic_name, irq, irqmask);
#endif
}

static void
uic_ack_irq(struct pic_ops *pic, int irq)
{
	struct uic * const uic = pic->pic_cookie;
	const uint32_t irqmask = IRQ_TO_MASK(irq);

	(*uic->uic_mt_intr_ack)(irqmask);
}

static int
uic_get_irq(struct pic_ops *pic, int dummy)
{
	struct uic * const uic = pic->pic_cookie;

	const uint32_t irqmask = (*uic->uic_mf_intr_status)();
	if (irqmask == 0)
		return 255;
	return IRQ_OF_MASK(irqmask);
}

/*
 * Register an interrupt handler.
 */
static void
uic_establish_irq(struct pic_ops *pic, int irq, int type, int ipl)
{
}
