/*	$NetBSD: iomd_irqhandler.c,v 1.13.22.4 2008/02/09 13:01:38 chris Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * IRQ/FIQ initialisation, claim, release and handler routines
 *
 *	from: irqhandler.c,v 1.14 1997/04/02 21:52:19 christos Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iomd_irqhandler.c,v 1.13.22.4 2008/02/09 13:01:38 chris Exp $");

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>

#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>

#include <machine/intr.h>
#include <machine/cpu.h>

/*
 * void irq_init(void)
 *
 * Initialise the IRQ/FIQ sub system
 */

static void iomd_set_irq_mask(uint32_t intr_enabled);
static void iomd7500_set_irq_mask(uint32_t intr_enabled);
static struct intrline pic_irqlines[NIRQS];

static struct pic_softc iomd_pic =
{
	.pic_ops.pic_set_irq_hardware_mask = iomd_set_irq_mask,
	.pic_ops.pic_set_irq_hardware_type = NULL,
	.pic_pre_assigned_irqs = 0,
	.pic_nirqs = NIRQS,
	.pic_name = "iomd",
	.pic_intrlines = pic_irqlines
};


void
iomd_irq_init(void)
{
	/* Clear the IRQ/FIQ masks in the IOMD */
	IOMD_WRITE_BYTE(IOMD_IRQMSKA, 0x00);
	IOMD_WRITE_BYTE(IOMD_IRQMSKB, 0x00);

	switch (IOMD_ID) {
	case RPC600_IOMD_ID:
		break;
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
		IOMD_WRITE_BYTE(IOMD_IRQMSKC, 0x00);
		IOMD_WRITE_BYTE(IOMD_IRQMSKD, 0x00);
		iomd_pic.pic_ops.pic_set_irq_hardware_mask = iomd7500_set_irq_mask;
		break;
	default:
		//printf("Unknown IOMD id (%d) found in irq_init()\n", IOMD_ID);
		;
	}

	IOMD_WRITE_BYTE(IOMD_FIQMSK, 0x00);
	IOMD_WRITE_BYTE(IOMD_DMAMSK, 0x00);

	arm_intr_register_pic(&iomd_pic);
}


static void
iomd_set_irq_mask(uint32_t intr_enabled)
{
	IOMD_WRITE_BYTE(IOMD_IRQMSKA, intr_enabled & 0xff);
	IOMD_WRITE_BYTE(IOMD_IRQMSKB, (intr_enabled >> 8) & 0xff);
	IOMD_WRITE_BYTE(IOMD_DMAMSK,  (intr_enabled >> 16) & 0xff);
}

static void
iomd7500_set_irq_mask(uint32_t intr_enabled)
{
	IOMD_WRITE_BYTE(IOMD_IRQMSKA, intr_enabled & 0xff);
	IOMD_WRITE_BYTE(IOMD_IRQMSKB, (intr_enabled >> 8) & 0xff);
	IOMD_WRITE_BYTE(IOMD_IRQMSKC, (intr_enabled >> 16) & 0xff);
	IOMD_WRITE_BYTE(IOMD_IRQMSKD, (intr_enabled >> 24) & 0xef);
	IOMD_WRITE_BYTE(IOMD_DMAMSK,  (intr_enabled >> 27) & 0x10);
}

void *
intr_claim(int irq, int level, const char *name, int (*ih_func)(void *),
    void *ih_arg)
{
	return arm_intr_claim(&iomd_pic, irq, IST_LEVEL, level, name, ih_func, ih_arg);
}


void
intr_release(void *arg)
{
	arm_intr_disestablish(&iomd_pic, arg);
}
/*
 * void disable_irq(int irq)
 *
 * Disables a specific irq. The irq is removed from the master irq mask
 */

void
disable_irq(int irq)
{
	arm_intr_soft_disable_irq(&iomd_pic, irq);
}  


/*
 * void enable_irq(int irq)
 *
 * Enables a specific irq. The irq is added to the master irq mask
 * This routine should be used with caution. A handler should already
 * be installed.
 */

void
enable_irq(int irq)
{
	arm_intr_soft_enable_irq(&iomd_pic, irq);
}  


static uint32_t
iomd_irq_status(void)
{
	uint32_t result = 0;
	uint32_t tmp;


	tmp = IOMD_READ_BYTE(IOMD_IRQRQA);
	result |= tmp;
	tmp = IOMD_READ_BYTE(IOMD_IRQRQB);
	result |= tmp << 8;

	switch (IOMD_ID) {
	case RPC600_IOMD_ID:
		/* XXX break DMA into seperate PIC */
		tmp = IOMD_READ_BYTE(IOMD_DMARQ);
		result |= tmp << 16;
		break;
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
		tmp = IOMD_READ_BYTE(IOMD_IRQRQC);
		result |= tmp << 16;
		tmp = IOMD_READ_BYTE(IOMD_IRQRQD);
		result |= tmp << 24;
		/* XXX break DMA into seperate PIC */
		tmp = IOMD_READ_BYTE(IOMD_DMARQ);
		result |= tmp << 27;
		break;
	}

	/* clear iomd irqa bits */
	IOMD_WRITE_BYTE(IOMD_IRQRQA, result & 0x7d);

	return result;
}

void
iomd_intr_dispatch(struct clockframe *frame)
{
	uint32_t hwpend;

	hwpend = iomd_irq_status();

	arm_intr_queue_irqs(&iomd_pic, hwpend);

	arm_intr_process_pending_ipls(frame, current_ipl_level);
}

/*
 * void stray_irqhandler(u_int mask)
 *
 * Handler for stray interrupts. This gets called if a handler cannot be
 * found for an interrupt.
 */

void
stray_irqhandler(u_int mask)
{
	static u_int stray_irqs = 0;

	if (++stray_irqs <= 8)
		log(LOG_ERR, "Stray interrupt %08x%s\n", mask,
		    stray_irqs >= 8 ? ": stopped logging" : "");
}
