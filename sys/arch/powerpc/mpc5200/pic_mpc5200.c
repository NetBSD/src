/*	$NetBSD: pic_mpc5200.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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
 * MPC5200B SIU interrupt controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic_mpc5200.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/ofw/openfirm.h>

#include <machine/intr.h>

#include <powerpc/pic/picvar.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/pic_mpc5200var.h>

/* SIU interrupt-controller registers (offsets from the controller base). */
#define	SIU_PER_MASK	0x00	/* peripheral interrupt mask		*/
#define	SIU_PER_PRI1	0x04	/* peripheral priority / HI-LO select 1	*/
#define	SIU_PER_PRI2	0x08	/* peripheral priority / HI-LO select 2	*/
#define	SIU_PER_PRI3	0x0c	/* peripheral priority / HI-LO select 3	*/
#define	SIU_CTRL	0x10	/* external enable & type / critical enable */
#define	SIU_MAIN_MASK	0x14	/* critical priority + main interrupt mask */
#define	SIU_MAIN_PRI1	0x18	/* main priority 1			*/
#define	SIU_MAIN_PRI2	0x1c	/* main priority 2 (0x1Cf in the draft -- typo) */
#define	SIU_ENC_STATUS	0x24	/* encoded highest-priority pending source */
#define	SIU_CRIT_STATUS	0x28	/* critical interrupt status		*/
#define	SIU_MAIN_STATUS	0x2c	/* main interrupt status		*/
#define	SIU_PER_STATUS	0x30	/* peripheral interrupt status		*/

#define	SIU_REG_SIZE	0x40	/* register window if OF omits a size	*/

/* Fields of the encoded-status register (SIU_ENC_STATUS). */
#define	SIU_ENC_CRIT_VALID	0x00000400
#define	SIU_ENC_CRIT_MASK	0x00000300
#define	SIU_ENC_CRIT_SHIFT	8
#define	SIU_ENC_MAIN_VALID	0x00200000
#define	SIU_ENC_MAIN_MASK	0x001f0000
#define	SIU_ENC_MAIN_SHIFT	16
#define	SIU_ENC_PER_VALID	0x20000000
#define	SIU_ENC_PER_MASK	0x1f000000
#define	SIU_ENC_PER_SHIFT	24

#define	SIU_ENC_CRIT_IS_CASCADE	2	/* critical "2" -> read peripheral */
#define	SIU_ENC_MAIN_IS_CASCADE	4	/* main "4"     -> read peripheral */

/* Reset state programmed at attach time. */
#define	SIU_MAIN_MASK_INIT	0x0001efff /* mask all main but src 4 (per. cascade) */
#define	SIU_PER_MASK_INIT	0xffffff00 /* mask all peripheral sources	*/

#define	SIU_CTRL_INIT		0x0fc01001

/* Number of sources owned by this (primary) PIC. */
#define	MPC5200_NIRQ		96

/* Band membership tests / source extraction for a flat IRQ number. */
#define	MPC5200_IRQ_MAIN_BIT	0x20	/* flat 32..  -> main band	*/
#define	MPC5200_IRQ_PER_BIT	0x40	/* flat 64..  -> peripheral band	*/
#define	MPC5200_IRQ_SOURCE(irq)	((irq) & 0x1f)

static int	mpc5200pic_match(device_t, cfdata_t, void *);
static void	mpc5200pic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mpc5200pic, sizeof(struct mpc5200pic_softc),
    mpc5200pic_match, mpc5200pic_attach, NULL, NULL);

static void	mpc5200_enable_irq(struct pic_ops *, int, int);
static void	mpc5200_disable_irq(struct pic_ops *, int);
static int	mpc5200_get_irq(struct pic_ops *, int);
static void	mpc5200_ack_irq(struct pic_ops *, int);
static void	mpc5200_establish_irq(struct pic_ops *, int, int, int);

static inline uint32_t
siu_read(struct mpc5200_ops *m, bus_size_t off)
{
	return bus_space_read_4(m->bst, m->bsh, off);
}

static inline void
siu_write(struct mpc5200_ops *m, bus_size_t off, uint32_t val)
{
	bus_space_write_4(m->bst, m->bsh, off, val);
}

static bool
mpc5200pic_is_pic(int node, const char *name)
{
	char compat[32];

	if (OF_getprop(node, "compatible", compat, sizeof(compat)) > 0 &&
	    (strcmp(compat, "mpc5200-pic") == 0 ||
	     strcmp(compat, "mpc5200b-pic") == 0))
		return true;

	return strcmp(name, "pic") == 0;
}

static int
mpc5200pic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;

	return mpc5200pic_is_pic(oba->obio_node, oba->obio_name) ? 1 : 0;
}

static void
mpc5200pic_attach(device_t parent, device_t self, void *aux)
{
	struct mpc5200pic_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	struct mpc5200_ops *m = &sc->sc_pic;
	struct pic_ops *pic = &m->pic;
	bus_size_t size;

	sc->sc_dev = self;

	size = oba->obio_size != 0 ? oba->obio_size : SIU_REG_SIZE;
	m->bst = oba->obio_bst;
	if (bus_space_map(m->bst, oba->obio_addr, size, 0, &m->bsh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	pic->pic_cookie = m;
	pic->pic_numintrs = MPC5200_NIRQ;
	pic->pic_enable_irq = mpc5200_enable_irq;
	pic->pic_reenable_irq = mpc5200_enable_irq;
	pic->pic_disable_irq = mpc5200_disable_irq;
	pic->pic_get_irq = mpc5200_get_irq;
	pic->pic_ack_irq = mpc5200_ack_irq;
	pic->pic_establish_irq = mpc5200_establish_irq;
	pic->pic_finish_setup = NULL;
	strlcpy(pic->pic_name, device_xname(self), sizeof(pic->pic_name));

	m->main_mask = SIU_MAIN_MASK_INIT;
	m->per_mask = SIU_PER_MASK_INIT;
	m->ctrl = SIU_CTRL_INIT;

	pic_add(pic);

	siu_write(m, SIU_MAIN_MASK, m->main_mask);
	siu_write(m, SIU_PER_MASK, m->per_mask);
	siu_write(m, SIU_CTRL, m->ctrl);

	/* Flat priorities: dispatch order comes from the encoded register. */
	siu_write(m, SIU_PER_PRI1, 0);
	siu_write(m, SIU_PER_PRI2, 0);
	siu_write(m, SIU_PER_PRI3, 0);
	siu_write(m, SIU_MAIN_PRI1, 0);
	siu_write(m, SIU_MAIN_PRI2, 0);

	aprint_normal(": MPC5200 SIU interrupt controller, %d sources\n",
	    MPC5200_NIRQ);
	aprint_verbose_dev(self, "main_mask=0x%08x per_mask=0x%08x\n",
	    siu_read(m, SIU_MAIN_MASK), siu_read(m, SIU_PER_MASK));
}

static void
mpc5200_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct mpc5200_ops *m = pic->pic_cookie;
	uint32_t bit;

	if (irq & MPC5200_IRQ_MAIN_BIT) {		/* main */
		if (MPC5200_IRQ_SOURCE(irq) > 16)
			return;
		bit = 1u << (16 - MPC5200_IRQ_SOURCE(irq));
		m->main_mask &= ~bit;
		siu_write(m, SIU_MAIN_MASK, m->main_mask);
	} else if (irq & MPC5200_IRQ_PER_BIT) {		/* peripheral */
		if (MPC5200_IRQ_SOURCE(irq) > 23)
			return;
		bit = 1u << (31 - MPC5200_IRQ_SOURCE(irq));
		m->per_mask &= ~bit;
		siu_write(m, SIU_PER_MASK, m->per_mask);
	} else {					/* critical */
		if (MPC5200_IRQ_SOURCE(irq) > 3)
			return;
		bit = (1u << (11 - irq)) | ((uint32_t)type << (22 - irq * 2));
		m->ctrl |= bit;
		siu_write(m, SIU_CTRL, m->ctrl);
	}
}

static void
mpc5200_disable_irq(struct pic_ops *pic, int irq)
{
	struct mpc5200_ops *m = pic->pic_cookie;
	uint32_t bit;

	/*
	 * Flat 36 == main source 4 is the peripheral cascade
	 */
	if (irq == (32 + 4))
		return;

	if (irq & MPC5200_IRQ_MAIN_BIT) {		/* main */
		if (MPC5200_IRQ_SOURCE(irq) > 16)
			return;
		bit = 1u << (16 - MPC5200_IRQ_SOURCE(irq));
		m->main_mask |= bit;
		siu_write(m, SIU_MAIN_MASK, m->main_mask);
	} else if (irq & MPC5200_IRQ_PER_BIT) {		/* peripheral */
		if (MPC5200_IRQ_SOURCE(irq) > 23)
			return;
		bit = 1u << (31 - MPC5200_IRQ_SOURCE(irq));
		m->per_mask |= bit;
		siu_write(m, SIU_PER_MASK, m->per_mask);
	} else {					/* critical */
		if (MPC5200_IRQ_SOURCE(irq) > 3)
			return;
		bit = 1u << (11 - MPC5200_IRQ_SOURCE(irq));
		m->ctrl &= ~bit;
		siu_write(m, SIU_CTRL, m->ctrl);
	}
}

static int
mpc5200_get_irq(struct pic_ops *pic, int mode)
{
	struct mpc5200_ops *m = pic->pic_cookie;
	uint32_t enc;
	int source;

	enc = siu_read(m, SIU_ENC_STATUS);

	if (enc & SIU_ENC_CRIT_VALID) {
		source = (enc & SIU_ENC_CRIT_MASK) >> SIU_ENC_CRIT_SHIFT;
		if (source != SIU_ENC_CRIT_IS_CASCADE)
			return source;			/* flat 0..3 */
		goto peripheral;
	}
	if (enc & SIU_ENC_MAIN_VALID) {
		source = (enc & SIU_ENC_MAIN_MASK) >> SIU_ENC_MAIN_SHIFT;
		if (source != SIU_ENC_MAIN_IS_CASCADE)
			return source + 32;		/* flat 32..48 */
		goto peripheral;
	}
	if (enc & SIU_ENC_PER_VALID) {
 peripheral:
		source = (enc & SIU_ENC_PER_MASK) >> SIU_ENC_PER_SHIFT;
		return source + 64;			/* flat 64..87 */
	}

	return 255;	/* no source pending: treated as spurious */
}

static void
mpc5200_ack_irq(struct pic_ops *pic, int irq)
{
	/*
	 * The SIU has no end-of-interrupt register
	 */
}

static void
mpc5200_establish_irq(struct pic_ops *pic, int irq, int type, int maxlevel)
{
	/*
	 * Nothing to program per source
	 */
}

/*
 * Translate the index'th entry of an OF "interrupts" property into a flat IRQ.
 */
static int
mpc5200_sense_to_ist(uint32_t sense)
{
	/*
	 * The sense cell is only meaningful for the external (critical) IRQ
	 */
	switch (sense) {
	case 1:
		return IST_EDGE_RISING;
	case 2:
		return IST_EDGE_FALLING;
	default:
		return IST_LEVEL;
	}
}

int
obio_decode_interrupt(int node, int index, int *irqp, int *typep)
{
	uint32_t intr[3 * 16];	/* up to 16 interrupts, 3 cells each */
	uint32_t tier, source;
	int len, count, base;

	len = OF_getprop(node, "interrupts", intr, sizeof(intr));
	if (len < (int)(3 * sizeof(uint32_t)))
		return 0;
	count = len / (int)(3 * sizeof(uint32_t));
	if (index < 0 || index >= count)
		return 0;

	tier   = intr[index * 3 + 0];
	source = intr[index * 3 + 1];

	switch (tier) {
	case 0:
		base = 0;		/* critical   -> flat  0.. */
		break;
	case 1:
		base = 32;		/* main       -> flat 32.. */
		break;
	case 2:
		base = 64;		/* peripheral -> flat 64.. */
		break;
	case 3:
		base = 96;		/* BestComm   -> flat 96.. (secondary PIC) */
		break;
	default:
		return 0;
	}

	*irqp = base + source;
	*typep = mpc5200_sense_to_ist(intr[index * 3 + 2]);
	return 1;
}
