/* $NetBSD: pci_2100_a500.c,v 1.3.2.2 2002/06/20 03:37:43 nathanw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_2100_a500.c,v 1.3.2.2 2002/06/20 03:37:43 nathanw Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>

#include <dev/eisa/eisavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/ttwogareg.h>
#include <alpha/pci/ttwogavar.h>
#include <alpha/pci/pci_2100_a500.h>

static bus_space_tag_t pic_iot;
static bus_space_handle_t pic_master_ioh;
static bus_space_handle_t pic_slave_ioh[4];
static bus_space_handle_t pic_elcr_ioh;

static const int pic_slave_to_master[4] = { 1, 3, 4, 5 };

int	dec_2100_a500_pic_intr_map(struct pci_attach_args *,
	    pci_intr_handle_t *);

int	dec_2100_a500_icic_intr_map(struct pci_attach_args *,
	    pci_intr_handle_t *);

const char *dec_2100_a500_intr_string(void *, pci_intr_handle_t);
const struct evcnt *dec_2100_a500_intr_evcnt(void *, pci_intr_handle_t);
void	*dec_2100_a500_intr_establish(void *, pci_intr_handle_t,
	    int, int (*)(void *), void *);
void	dec_2100_a500_intr_disestablish(void *, void *);

int	dec_2100_a500_eisa_intr_map(void *, u_int, eisa_intr_handle_t *);
const char *dec_2100_a500_eisa_intr_string(void *, int);
const struct evcnt *dec_2100_a500_eisa_intr_evcnt(void *, int);
void	*dec_2100_a500_eisa_intr_establish(void *, int, int, int,
	    int (*)(void *), void *);
void	dec_2100_a500_eisa_intr_disestablish(void *, void *);
int	dec_2100_a500_eisa_intr_alloc(void *, int, int, int *);

#define	PCI_STRAY_MAX	5

/*
 * On systems with cascaded 8259s, it's actually 32.  Systems which
 * use the ICIC interrupt logic have 64, however.
 */
#define	SABLE_MAX_IRQ		64
#define	SABLE_8259_MAX_IRQ	32

void	dec_2100_a500_iointr(void *, u_long);

void	dec_2100_a500_pic_enable_intr(struct ttwoga_config *, int, int);
void	dec_2100_a500_pic_init_intr(struct ttwoga_config *);
void	dec_2100_a500_pic_setlevel(struct ttwoga_config *, int, int);
void	dec_2100_a500_pic_eoi(struct ttwoga_config *, int);

void	dec_2100_a500_icic_enable_intr(struct ttwoga_config *, int, int);
void	dec_2100_a500_icic_init_intr(struct ttwoga_config *);
void	dec_2100_a500_icic_setlevel(struct ttwoga_config *, int, int);
void	dec_2100_a500_icic_eoi(struct ttwoga_config *, int);

#define	T2_IRQ_EISA_START	7
#define	T2_IRQ_EISA_COUNT	16

#define	T2_IRQ_IS_EISA(irq)						\
	((irq) >= T2_IRQ_EISA_START &&					\
	 (irq) < (T2_IRQ_EISA_START + T2_IRQ_EISA_COUNT))

const int dec_2100_a500_intr_deftype[SABLE_MAX_IRQ] = {
	IST_LEVEL,		/* PCI slot 0 A */
	IST_LEVEL,		/* on-board SCSI */
	IST_LEVEL,		/* on-board Ethernet */
	IST_EDGE,		/* mouse */
	IST_LEVEL,		/* PCI slot 1 A */
	IST_LEVEL,		/* PCI slot 2 A */
	IST_EDGE,		/* keyboard */
	IST_EDGE,		/* floppy (EISA IRQ 0) */
	IST_EDGE,		/* serial port 1 (EISA IRQ 1) */
	IST_EDGE,		/* parallel port (EISA IRQ 2) */
	IST_NONE,		/* EISA IRQ 3 (edge/level) */
	IST_NONE,		/* EISA IRQ 4 (edge/level) */
	IST_NONE,		/* EISA IRQ 5 (edge/level) */
	IST_NONE,		/* EISA IRQ 6 (edge/level) */
	IST_NONE,		/* EISA IRQ 7 (edge/level) */
	IST_EDGE,		/* serial port 0 (EISA IRQ 8) */
	IST_NONE,		/* EISA IRQ 9 (edge/level) */
	IST_NONE,		/* EISA IRQ 10 (edge/level) */
	IST_NONE,		/* EISA IRQ 11 (edge/level) */
	IST_NONE,		/* EISA IRQ 12 (edge/level) */
	IST_LEVEL,		/* PCI slot 2 B (EISA IRQ 13 n/c) */
	IST_NONE,		/* EISA IRQ 14 (edge/level) */
	IST_NONE,		/* EISA IRQ 15 (edge/level) */
	IST_LEVEL,		/* I2C (XXX double-check this) */
	IST_LEVEL,		/* PCI slot 0 B */
	IST_LEVEL,		/* PCI slot 1 B */
	IST_LEVEL,		/* PCI slot 0 C */
	IST_LEVEL,		/* PCI slot 1 C */
	IST_LEVEL,		/* PCI slot 2 C */
	IST_LEVEL,		/* PCI slot 0 D */
	IST_LEVEL,		/* PCI slot 1 D */
	IST_LEVEL,		/* PCI slot 2 D */

	/*
	 * These are the PCI interrupts on the T3/T4 systems.  See
	 * dec_2100_a500_icic_intr_map() for the mapping.
	 */
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
	IST_LEVEL,
};

void
pci_2100_a500_pickintr(struct ttwoga_config *tcp)
{
	pci_chipset_tag_t pc = &tcp->tc_pc;
	char *cp;
	int i;

	pic_iot = &tcp->tc_iot;

	pc->pc_intr_v = tcp;
	pc->pc_intr_string = dec_2100_a500_intr_string;
	pc->pc_intr_evcnt = dec_2100_a500_intr_evcnt;
	pc->pc_intr_establish = dec_2100_a500_intr_establish;
	pc->pc_intr_disestablish = dec_2100_a500_intr_disestablish;

	/* Not supported on T2. */
	pc->pc_pciide_compat_intr_establish = NULL;

	tcp->tc_intrtab = alpha_shared_intr_alloc(SABLE_MAX_IRQ, 8);
	for (i = 0; i < SABLE_MAX_IRQ; i++) {
		alpha_shared_intr_set_dfltsharetype(tcp->tc_intrtab,
		    i, tcp->tc_hose == 0 ?
		    dec_2100_a500_intr_deftype[i] : IST_LEVEL);
		alpha_shared_intr_set_maxstrays(tcp->tc_intrtab,
		    i, PCI_STRAY_MAX);

		cp = alpha_shared_intr_string(tcp->tc_intrtab, i);
		sprintf(cp, "irq %d", T2_IRQ_IS_EISA(i) ?
		    i - T2_IRQ_EISA_START : i);
		evcnt_attach_dynamic(alpha_shared_intr_evcnt(
		    tcp->tc_intrtab, i), EVCNT_TYPE_INTR, NULL,
		    T2_IRQ_IS_EISA(i) ? "eisa" : "T2", cp);
	}

	/* 64 16-byte vectors per hose. */
	tcp->tc_vecbase = 0x800 + ((64 * 16) * tcp->tc_hose);

	/*
	 * T2 uses a custom layout of cascaded 8259 PICs for interrupt
	 * control.  T3 and T4 use a built-in interrupt controller.
	 *
	 * Note that the external PCI bus (Hose 1) always uses
	 * the new interrupt controller.
	 */
	if (tcp->tc_rev < TRN_T3 && tcp->tc_hose == 0) {
		pc->pc_intr_map = dec_2100_a500_pic_intr_map;
		tcp->tc_enable_intr = dec_2100_a500_pic_enable_intr;
		tcp->tc_setlevel = dec_2100_a500_pic_setlevel;
		tcp->tc_eoi = dec_2100_a500_pic_eoi;
		dec_2100_a500_pic_init_intr(tcp);
	} else {
		pc->pc_intr_map = dec_2100_a500_icic_intr_map;
		tcp->tc_enable_intr = dec_2100_a500_icic_enable_intr;
		tcp->tc_setlevel = dec_2100_a500_icic_setlevel;
		tcp->tc_eoi = dec_2100_a500_icic_eoi;
		dec_2100_a500_icic_init_intr(tcp);
	}
}

void
pci_2100_a500_eisa_pickintr(pci_chipset_tag_t pc, eisa_chipset_tag_t ec)
{

	ec->ec_v = pc->pc_intr_v;
	ec->ec_intr_map = dec_2100_a500_eisa_intr_map;
	ec->ec_intr_string = dec_2100_a500_eisa_intr_string;
	ec->ec_intr_evcnt = dec_2100_a500_eisa_intr_evcnt;
	ec->ec_intr_establish = dec_2100_a500_eisa_intr_establish;
	ec->ec_intr_disestablish = dec_2100_a500_eisa_intr_disestablish;
}

void
pci_2100_a500_isa_pickintr(pci_chipset_tag_t pc, isa_chipset_tag_t ic)
{

	ic->ic_v = pc->pc_intr_v;
	ic->ic_intr_evcnt = dec_2100_a500_eisa_intr_evcnt;
	ic->ic_intr_establish = dec_2100_a500_eisa_intr_establish;
	ic->ic_intr_disestablish = dec_2100_a500_eisa_intr_disestablish;
	ic->ic_intr_alloc = dec_2100_a500_eisa_intr_alloc;
}

/*****************************************************************************
 * PCI interrupt support.
 *****************************************************************************/

int
dec_2100_a500_pic_intr_map(struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	/*
	 * Interrupts in the Sable are even more of a pain than other
	 * Alpha systems.  The interrupt logic is made up of 5 8259
	 * PICs, arranged as follows:
	 *
	 *	Slave 0 --------------------------------+
	 *	0 PCI slot 0 A				|
	 *	1 on-board SCSI				|
	 *	2 on-board Ethernet			|
	 *	3 mouse					|
	 *	4 PCI slot 1 A				|
	 *	5 PCI slot 2 A				|
	 *	6 keyboard				|
	 *	7 floppy (EISA IRQ 0)			|
	 *						|
	 *	Slave 1	------------------------+	|   Master
	 *	0 serial port 1 (EISA IRQ 1)	|	|   0 ESC interrupt
	 *	1 parallel port (EISA IRQ 2)	|	+-- 1 Slave 0
	 *	2 EISA IRQ 3			|	    2 reserved
	 *	3 EISA IRQ 4			+---------- 3 Slave 1
	 *	4 EISA IRQ 5			+---------- 4 Slave 2
	 *	5 EISA IRQ 6			|	+-- 5 Slave 3
	 *	6 EISA IRQ 7			|	|   6 reserved
	 *	7 serial port 0 (EISA IRQ 8)	|	|   7 n/c
	 *					|	|
	 *	Slave 2 ------------------------+	|
	 *	0 EISA IRQ 9				|
	 *	1 EISA IRQ 10				|
	 *	2 EISA IRQ 11				|
	 *	3 EISA IRQ 12				|
	 *	4 PCI slot 2 B (EISA IRQ 13 n/c)	|
	 *	5 EISA IRQ 14				|
	 *	6 EISA IRQ 15				|
	 *	7 I2C					|
	 *						|
	 *	Slave 3 --------------------------------+
	 *	0 PCI slot 0 B
	 *	1 PCI slot 1 B
	 *	2 PCI slot 0 C
	 *	3 PCI slot 1 C
	 *	4 PCI slot 2 C
	 *	5 PCI slot 0 D
	 *	6 PCI slot 1 D
	 *	7 PCI slot 2 D
	 *
	 * Careful readers will note that the PCEB does not handle ISA
	 * interrupts at all; when ISA interrupts are established, they
	 * must be mapped to Sable interrupts.  Thankfully, this is easy
	 * to do.
	 *
	 * The T3 and T4, generally found on Lynx, use a totally different
	 * scheme because they have more PCI interrupts to handle; see below.
	 */
	static const int irqmap[9/*device*/][4/*pin*/] = {
		{ 0x02, -1, -1, -1 },		/* 0: on-board Ethernet */
		{ 0x01, -1, -1, -1 },		/* 1: on-board SCSI */
		{ -1, -1, -1, -1 },		/* 2: invalid */
		{ -1, -1, -1, -1 },		/* 3: invalid */
		{ -1, -1, -1, -1 },		/* 4: invalid */
		{ -1, -1, -1, -1 },		/* 5: invalid */
		{ 0x00, 0x18, 0x1a, 0x1d },	/* 6: PCI slot 0 */
		{ 0x04, 0x19, 0x1b, 0x1e },	/* 7: PCI slot 1 */
		{ 0x05, 0x14, 0x1c, 0x1f },	/* 8: PCI slot 2 */
	};
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device, irq;

	if (buspin == 0) {
		/* No IRQ used. */
		return (1);
	}

	if (buspin > 4) {
		printf("dec_2100_a500_pic_intr_map: bad interrupt pin %d\n",
		    buspin);
		return (1);
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	if (device > 8) {
		printf("dec_2100_a500_pic_intr_map: bad device %d\n",
		    device);
		return (1);
	}

	irq = irqmap[device][buspin - 1];
	if (irq == -1) {
		printf("dec_2100_a500_pic_intr_map: no mapping for "
		    "device %d pin %d\n", device, buspin);
		return (1);
	}
	*ihp = irq;
	return (0);
}

int
dec_2100_a500_icic_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device, irq;

	if (buspin == 0) {
		/* No IRQ used. */
		return (1);
	}

	if (buspin > 4) {
		printf("dec_2100_a500_icic_intr_map: bad interrupt in %d\n",
		    buspin);
		return (1);
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	switch (device) {
	case 0:		/* on-board Ethernet */
		irq = 24;
		break;

	case 1:		/* on-board SCSI */
		irq = 28;
		break;

	case 6:		/* PCI slots */
	case 7:
	case 8:
		irq = (32 + (4 * (device - 6))) + (buspin - 1);
		break;

	default:
		printf("dec_2100_a500_icic_intr_map: bad device %d\n",
		    device);
		return (1);
	}

	*ihp = irq;
	return (0);
}

const char *
dec_2100_a500_intr_string(void *v, pci_intr_handle_t ih)
{
	static char irqstr[15];		/* 11 + 2 + NULL + sanity */

	if (ih >= SABLE_MAX_IRQ)
		panic("dec_2100_a500_intr_string: bogus T2 IRQ 0x%lx\n", ih);

	sprintf(irqstr, "T2 irq %ld", ih);
	return (irqstr);
}

const struct evcnt *
dec_2100_a500_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	struct ttwoga_config *tcp = v;

	if (ih >= SABLE_MAX_IRQ)
		panic("dec_2100_a500_intr_evcnt: bogus T2 IRQ 0x%lx\n", ih);

	return (alpha_shared_intr_evcnt(tcp->tc_intrtab, ih));
}

void *
dec_2100_a500_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{
	struct ttwoga_config *tcp = v;
	void *cookie;

	if (ih >= SABLE_MAX_IRQ)
		panic("dec_2100_a500_intr_establish: bogus IRQ 0x%lx\n",
		    ih);

	cookie = alpha_shared_intr_establish(tcp->tc_intrtab, ih,
	    dec_2100_a500_intr_deftype[ih], level, func, arg, "T2 irq");

	if (cookie != NULL &&
	    alpha_shared_intr_firstactive(tcp->tc_intrtab, ih)) {
		scb_set(tcp->tc_vecbase + SCB_IDXTOVEC(ih),
		    dec_2100_a500_iointr, tcp);
		(*tcp->tc_enable_intr)(tcp, ih, 1);
	}

	return (cookie);
}

void
dec_2100_a500_intr_disestablish(void *v, void *cookie)
{
	struct ttwoga_config *tcp = v;
	struct alpha_shared_intrhand *ih = cookie;
	unsigned int irq = ih->ih_num;
	int s;

	s = splhigh();

	alpha_shared_intr_disestablish(tcp->tc_intrtab, cookie,
	    "T2 irq");
	if (alpha_shared_intr_isactive(tcp->tc_intrtab, irq) == 0) {
		(*tcp->tc_enable_intr)(tcp, irq, 0);
		alpha_shared_intr_set_dfltsharetype(tcp->tc_intrtab,
		    irq, dec_2100_a500_intr_deftype[irq]);
		scb_free(tcp->tc_vecbase + SCB_IDXTOVEC(irq));
	}

	splx(s);
}

/*****************************************************************************
 * EISA interrupt support.
 *****************************************************************************/

int
dec_2100_a500_eisa_intr_map(void *v, u_int eirq, eisa_intr_handle_t *ihp)
{

	if (eirq > 15) {
		printf("dec_2100_a500_eisa_intr_map: bad EISA IRQ %d\n",
		    eirq);
		*ihp = -1;
		return (1);
	}

	/*
	 * EISA IRQ 13 is not connected.
	 */
	if (eirq == 13) {
		printf("dec_2100_a500_eisa_intr_map: EISA IRQ 13 not "
		    "connected\n");
		*ihp = -1;
		return (1);
	}

	/*
	 * Don't map to a T2 IRQ here; we must do this when we hook the
	 * interrupt up, since ISA interrupts aren't explicitly translated.
	 */

	*ihp = eirq;
	return (0);
}

const char *
dec_2100_a500_eisa_intr_string(void *v, int eirq)
{
	static char irqstr[32];

	if (eirq > 15 || eirq == 13)
		panic("dec_2100_a500_eisa_intr_string: bogus EISA IRQ 0x%x\n",
		    eirq);

	sprintf(irqstr, "eisa irq %d (T2 irq %d)", eirq,
	    eirq + T2_IRQ_EISA_START);
	return (irqstr);
}

const struct evcnt *
dec_2100_a500_eisa_intr_evcnt(void *v, int eirq)
{
	struct ttwoga_config *tcp = v;

	if (eirq > 15 || eirq == 13)
		panic("dec_2100_a500_eisa_intr_evcnt: bogus EISA IRQ 0x%x\n",
		    eirq);

	return (alpha_shared_intr_evcnt(tcp->tc_intrtab,
	    eirq + T2_IRQ_EISA_START));
}

void *
dec_2100_a500_eisa_intr_establish(void *v, int eirq, int type, int level,
    int (*fn)(void *), void *arg)
{
	struct ttwoga_config *tcp = v;
	void *cookie;
	int irq;

	if (eirq > 15 || type == IST_NONE)
		panic("dec_2100_a500_eisa_intr_establish: bogus irq or type");

	if (eirq == 13) {
		printf("dec_2100_a500_eisa_intr_establish: EISA IRQ 13 not "
		    "connected\n");
		return (NULL);
	}

	irq = eirq + T2_IRQ_EISA_START;

	/*
	 * We can't change the trigger type of some interrupts.  Don't allow
	 * level triggers to be hooked up to non-changeable edge triggers.
	 */
	if (dec_2100_a500_intr_deftype[irq] == IST_EDGE && type == IST_LEVEL) {
		printf("dec_2100_a500_eisa_intr_establish: non-EDGE on EDGE\n");
		return (NULL);
	}

	cookie = alpha_shared_intr_establish(tcp->tc_intrtab, irq,
	    type, level, fn, arg, "T2 irq");

	if (cookie != NULL &&
	    alpha_shared_intr_firstactive(tcp->tc_intrtab, irq)) {
		scb_set(tcp->tc_vecbase + SCB_IDXTOVEC(irq),
		    dec_2100_a500_iointr, tcp);
		(*tcp->tc_setlevel)(tcp, eirq,
		    alpha_shared_intr_get_sharetype(tcp->tc_intrtab,
						    irq) == IST_LEVEL);
		(*tcp->tc_enable_intr)(tcp, irq, 1);
	}

	return (cookie);
}

void
dec_2100_a500_eisa_intr_disestablish(void *v, void *cookie)
{
	struct ttwoga_config *tcp = v;
	struct alpha_shared_intrhand *ih = cookie;
	int s, irq = ih->ih_num;

	s = splhigh();

	/* Remove it from the link. */
	alpha_shared_intr_disestablish(tcp->tc_intrtab, cookie,
	    "T2 irq");

	if (alpha_shared_intr_isactive(tcp->tc_intrtab, irq) == 0) {
		(*tcp->tc_enable_intr)(tcp, irq, 0);
		alpha_shared_intr_set_dfltsharetype(tcp->tc_intrtab,
		    irq, dec_2100_a500_intr_deftype[irq]);
		scb_free(tcp->tc_vecbase + SCB_IDXTOVEC(irq));
	}

	splx(s);
}

int
dec_2100_a500_eisa_intr_alloc(void *v, int mask, int type, int *eirqp)
{

	/* XXX Not supported right now. */
	return (1);
}

/*****************************************************************************
 * Interrupt support routines.
 *****************************************************************************/

#define	ICIC_ADDR(tcp, addr)						\
do {									\
	alpha_mb();							\
	T2GA((tcp), T2_AIR) = (addr);					\
	alpha_mb();							\
	alpha_mb();							\
	(void) T2GA((tcp), T2_AIR);					\
	alpha_mb();							\
	alpha_mb();							\
} while (0)

#define	ICIC_READ(tcp)	T2GA((tcp), T2_DIR)
#define	ICIC_WRITE(tcp, val)						\
do {									\
	alpha_mb();							\
	T2GA((tcp), T2_DIR) = (val);					\
	alpha_mb();							\
	alpha_mb();							\
} while (0)

void
dec_2100_a500_iointr(void *arg, u_long vec)
{
	struct ttwoga_config *tcp = arg;
	int irq, rv;

	irq = SCB_VECTOIDX(vec - tcp->tc_vecbase);

	rv = alpha_shared_intr_dispatch(tcp->tc_intrtab, irq);
	(*tcp->tc_eoi)(tcp, irq);
	if (rv == 0) {
		alpha_shared_intr_stray(tcp->tc_intrtab, irq, "T2 irq");
		if (ALPHA_SHARED_INTR_DISABLE(tcp->tc_intrtab, irq))
			(*tcp->tc_enable_intr)(tcp, irq, 0);
	}
}

void
dec_2100_a500_pic_enable_intr(struct ttwoga_config *tcp, int irq, int onoff)
{
	int pic;
	u_int8_t bit, mask;

	pic = irq >> 3;
	bit = 1 << (irq & 0x7);

	mask = bus_space_read_1(pic_iot, pic_slave_ioh[pic], 1);
	if (onoff)
		mask &= ~bit;
	else
		mask |= bit;
	bus_space_write_1(pic_iot, pic_slave_ioh[pic], 1, mask);
}

void
dec_2100_a500_icic_enable_intr(struct ttwoga_config *tcp, int irq, int onoff)
{
	u_int64_t bit, mask;

	bit = 1UL << irq;

	ICIC_ADDR(tcp, 0x40);

	mask = ICIC_READ(tcp);
	if (onoff)
		mask &= ~bit;
	else
		mask |= bit;
	ICIC_WRITE(tcp, mask);
}

void
dec_2100_a500_pic_init_intr(struct ttwoga_config *tcp)
{
	static const int picaddr[4] = {
		0x536, 0x53a, 0x53c, 0x53e
	};
	int pic;

	/*
	 * Map the master PIC.
	 */
	if (bus_space_map(pic_iot, 0x534, 2, 0, &pic_master_ioh))
		panic("dec_2100_a500_pic_init_intr: unable to map master PIC");

	/*
	 * Map all slave PICs and mask off the interrupts on them.
	 */
	for (pic = 0; pic < 4; pic++) {
		if (bus_space_map(pic_iot, picaddr[pic], 2, 0,
		    &pic_slave_ioh[pic]))
			panic("dec_2100_a500_pic_init_intr: unable to map "
			    "slave PIC %d", pic);
		bus_space_write_1(pic_iot, pic_slave_ioh[pic], 1, 0xff);
	}

	/*
	 * Map the ELCR registers.
	 */
	if (bus_space_map(pic_iot, 0x26, 2, 0, &pic_elcr_ioh))
		panic("dec_2100_a500_pic_init_intr: unable to map ELCR "
		    "registers");
}

void
dec_2100_a500_icic_init_intr(struct ttwoga_config *tcp)
{

	ICIC_ADDR(tcp, 0x40);
	ICIC_WRITE(tcp, 0xffffffffffffffffUL);
}

void
dec_2100_a500_pic_setlevel(struct ttwoga_config *tcp, int eirq, int level)
{
	int elcr;
	u_int8_t bit, mask;

	switch (eirq) {		/* EISA IRQ */
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		elcr = 0;
		bit = 1 << (eirq - 3);
		break;

	case 9:
	case 10:
	case 11:
		elcr = 0;
		bit = 1 << (eirq - 4);
		break;

	case 12:
		elcr = 1;
		bit = 1 << (eirq - 12);
		break;

	case 14:
	case 15:
		elcr = 1;
		bit = 1 << (eirq - 13);
		break;

	default:
		panic("dec_2100_a500_pic_setlevel: bogus EISA IRQ %d", eirq);
	}

	mask = bus_space_read_1(pic_iot, pic_elcr_ioh, elcr);
	if (level)
		mask |= bit;
	else
		mask &= ~bit;
	bus_space_write_1(pic_iot, pic_elcr_ioh, elcr, mask);
}

void
dec_2100_a500_icic_setlevel(struct ttwoga_config *tcp, int eirq, int level)
{
	u_int64_t bit, mask;

	switch (eirq) {
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 9:
	case 10:
	case 11:
	case 12:
	case 14:
	case 15:
		bit = 1UL << (eirq + T2_IRQ_EISA_START);

		ICIC_ADDR(tcp, 0x50);
		mask = ICIC_READ(tcp);
		if (level)
			mask |= bit;
		else
			mask &= ~bit;
		ICIC_WRITE(tcp, mask);
		break;

	default:
		panic("dec_2100_a500_icic_setlevel: bogus EISA IRQ %d", eirq);
	}
}

void
dec_2100_a500_pic_eoi(struct ttwoga_config *tcp, int irq)
{
	int pic;

	if (irq >= 0 && irq <= 7)
		pic = 0;
	else if (irq >= 8 && irq <= 15)
		pic = 1;
	else if (irq >= 16 && irq <= 23)
		pic = 2;
	else
		pic = 3;

	bus_space_write_1(pic_iot, pic_slave_ioh[pic], 0,
	    0xe0 | (irq - (8 * pic)));
	bus_space_write_1(pic_iot, pic_master_ioh, 0,
	    0xe0 | pic_slave_to_master[pic]);
}

void
dec_2100_a500_icic_eoi(struct ttwoga_config *tcp, int irq)
{

	T2GA(tcp, T2_VAR) = irq;
	alpha_mb();
	alpha_mb();	/* MAGIC */
}
