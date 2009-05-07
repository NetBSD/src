/*	$NetBSD: apic.c,v 1.3 2009/05/07 15:34:49 skrll Exp $	*/

/*	$OpenBSD: apic.c,v 1.7 2007/10/06 23:50:54 krw Exp $	*/

/*
 * Copyright (c) 2005 Michael Shalayeff
 * Copyright (c) 2007 Mark Kettenis
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/pdc.h>
#include <machine/intr.h>

#include <hp700/hp700/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <hp700/dev/elroyreg.h>
#include <hp700/dev/elroyvar.h>

#define APIC_INT_LINE_MASK	0x0000ff00
#define APIC_INT_LINE_SHIFT	8
#define APIC_INT_IRQ_MASK	0x0000001f

#define APIC_INT_LINE(x) (((x) & APIC_INT_LINE_MASK) >> APIC_INT_LINE_SHIFT)
#define APIC_INT_IRQ(x) ((x) & APIC_INT_IRQ_MASK)

/*
 * Interrupt types match the Intel MP Specification.
 */

#define MPS_INTPO_DEF		0
#define MPS_INTPO_ACTHI		1
#define MPS_INTPO_ACTLO		3
#define MPS_INTPO_SHIFT		0
#define MPS_INTPO_MASK		3

#define MPS_INTTR_DEF		0
#define MPS_INTTR_EDGE		1
#define MPS_INTTR_LEVEL		3
#define MPS_INTTR_SHIFT		2
#define MPS_INTTR_MASK		3

#define MPS_INT(p,t) \
    ((((p) & MPS_INTPO_MASK) << MPS_INTPO_SHIFT) | \
     (((t) & MPS_INTTR_MASK) << MPS_INTTR_SHIFT))

struct apic_iv {
	struct elroy_softc *sc;
	pci_intr_handle_t ih;
	int (*handler)(void *);
	void *arg;
	struct apic_iv *next;
	struct evcnt *cnt;
};

struct apic_iv *apic_intr_list[CPU_NINTS];

void apic_write(volatile struct elroy_regs *, uint32_t, uint32_t);
uint32_t apic_read(volatile struct elroy_regs *, uint32_t reg);

void	apic_get_int_tbl(struct elroy_softc *);
uint32_t apic_get_int_ent0(struct elroy_softc *, int);
#ifdef DEBUG
void	apic_dump(struct elroy_softc *);
#endif

void
apic_write(volatile struct elroy_regs *r, uint32_t reg, uint32_t val)
{
	elroy_write32(&r->apic_addr, htole32(reg));
	elroy_write32(&r->apic_data, htole32(val));
	elroy_read32(&r->apic_data);
}

uint32_t
apic_read(volatile struct elroy_regs *r, uint32_t reg)
{
	elroy_write32(&r->apic_addr, htole32(reg));
	return le32toh(elroy_read32(&r->apic_data));
}

void
apic_attach(struct elroy_softc *sc)
{
	volatile struct elroy_regs *r = sc->sc_regs;
	uint32_t data;

	data = apic_read(r, APIC_VERSION);
	sc->sc_nints = (data & APIC_VERSION_NENT) >> APIC_VERSION_NENT_SHIFT;
	aprint_normal(" APIC ver %x, %d pins",
	    data & APIC_VERSION_MASK, sc->sc_nints);

	sc->sc_irq = malloc(sc->sc_nints * sizeof(int), M_DEVBUF,
	    M_NOWAIT | M_ZERO);
	if (sc->sc_irq == NULL)
		panic("apic_attach: can't allocate irq table\n");

	apic_get_int_tbl(sc);

#ifdef DEBUG
	apic_dump(sc);
#endif
}

int
apic_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct elroy_softc *sc = pa->pa_pc->_cookie;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t reg;
	int line;

	reg = pci_conf_read(pc, tag, PCI_INTERRUPT_REG);
#ifdef DEBUG
	printf(" pin=%d line=%d ", PCI_INTERRUPT_PIN(reg),
	    PCI_INTERRUPT_LINE(reg));
#endif
	line = PCI_INTERRUPT_LINE(reg);
	if (sc->sc_irq[line] == 0)
		sc->sc_irq[line] = hp700_intr_allocate_bit(&int_reg_cpu);;
	*ihp = (line << APIC_INT_LINE_SHIFT) | sc->sc_irq[line];
	return (APIC_INT_IRQ(*ihp) == 0);
}

const char *
apic_intr_string(void *v, pci_intr_handle_t ih)
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "line %ld irq %ld",
	    APIC_INT_LINE(ih), APIC_INT_IRQ(ih));

	return (buf);
}

void *
apic_intr_establish(void *v, pci_intr_handle_t ih,
    int pri, int (*handler)(void *), void *arg)
{
	struct elroy_softc *sc = v;
	volatile struct elroy_regs *r = sc->sc_regs;
	hppa_hpa_t hpa = cpu_gethpa(0);
	struct evcnt *cnt;
	struct apic_iv *aiv, *biv;
	void *iv;
	int irq = APIC_INT_IRQ(ih);
	int line = APIC_INT_LINE(ih);
	uint32_t ent0;

	/* no mapping or bogus */
	if (irq <= 0 || irq > 31)
		return (NULL);

	aiv = malloc(sizeof(struct apic_iv), M_DEVBUF, M_NOWAIT);
	if (aiv == NULL)
		return NULL;

	aiv->sc = sc;
	aiv->ih = ih;
	aiv->handler = handler;
	aiv->arg = arg;
	aiv->next = NULL;
	aiv->cnt = NULL;
	if (apic_intr_list[irq]) {
		cnt = malloc(sizeof(struct evcnt), M_DEVBUF, M_NOWAIT);
		if (cnt == NULL) {
			free(aiv, M_DEVBUF);
			return NULL;
		}

		evcnt_attach_dynamic(cnt, EVCNT_TYPE_INTR, NULL,
		    device_xname(sc->sc_dv), "irq" /* XXXNH */);
		biv = apic_intr_list[irq];
		while (biv->next)
			biv = biv->next;
		biv->next = aiv;
		aiv->cnt = cnt;
		return arg;
	}

	if ((iv = hp700_intr_establish(sc->sc_dv, pri, apic_intr,
	     aiv, &int_reg_cpu, irq))) {
		ent0 = (31 - irq) & APIC_ENT0_VEC;
		ent0 |= apic_get_int_ent0(sc, line);
#if 0
		if (cold) {
			sc->sc_imr |= (1 << irq);
			ent0 |= APIC_ENT0_MASK;
		}
#endif
		apic_write(sc->sc_regs, APIC_ENT0(line), APIC_ENT0_MASK);
		apic_write(sc->sc_regs, APIC_ENT1(line),
		    ((hpa & 0x0ff00000) >> 4) | ((hpa & 0x000ff000) << 12));
		apic_write(sc->sc_regs, APIC_ENT0(line), ent0);

		/* Signal EOI. */
		elroy_write32(&r->apic_eoi,
		    htole32((31 - irq) & APIC_ENT0_VEC));

		apic_intr_list[irq] = aiv;
	}

	return (arg);
}

void
apic_intr_disestablish(void *v, void *cookie)
{
}

int
apic_intr(void *v)
{
	struct apic_iv *iv = v;
	struct elroy_softc *sc = iv->sc;
	volatile struct elroy_regs *r = sc->sc_regs;
	int claimed = 0;

	while (iv) {
		if (iv->handler(iv->arg)) {
			if (iv->cnt)
				iv->cnt->ev_count++;
			else
				claimed = 1;
		}
		iv = iv->next;
	}

	/* Signal EOI. */
	elroy_write32(&r->apic_eoi,
	    htole32((31 - APIC_INT_IRQ(iv->ih)) & APIC_ENT0_VEC));

	return (claimed);
}

/* Maximum number of supported interrupt routing entries. */
#define MAX_INT_TBL_SZ	16

void
apic_get_int_tbl(struct elroy_softc *sc)
{
	struct pdc_pat_io_num int_tbl_sz PDC_ALIGNMENT;
	struct pdc_pat_pci_rt int_tbl[MAX_INT_TBL_SZ] PDC_ALIGNMENT;
	size_t size;

	/*
	 * XXX int_tbl should not be allocated on the stack, but we need a
	 * 1:1 mapping, and malloc doesn't provide that.
	 */

	if (pdc_call((iodcio_t)pdc, 0, PDC_PCI_INDEX, PDC_PCI_GET_INT_TBL_SZ,
	    &int_tbl_sz, 0, 0, 0, 0, 0))
		return;

	if (int_tbl_sz.num > MAX_INT_TBL_SZ)
		panic("interrupt routing table too big (%d entries)",
		    int_tbl_sz.num);

	size = int_tbl_sz.num * sizeof(struct pdc_pat_pci_rt);
	sc->sc_int_tbl_sz = int_tbl_sz.num;
	sc->sc_int_tbl = malloc(size, M_DEVBUF, M_NOWAIT);
	if (sc->sc_int_tbl == NULL)
		return;

	if (pdc_call((iodcio_t)pdc, 0, PDC_PCI_INDEX, PDC_PCI_GET_INT_TBL,
	    &int_tbl_sz, 0, &int_tbl, 0, 0, 0))
		return;

	memcpy(sc->sc_int_tbl, int_tbl, size);
}

uint32_t
apic_get_int_ent0(struct elroy_softc *sc, int line)
{
	volatile struct elroy_regs *r = sc->sc_regs;
	int trigger = MPS_INT(MPS_INTPO_DEF, MPS_INTTR_DEF);
	uint32_t ent0 = APIC_ENT0_LOW | APIC_ENT0_LEV;
	int bus, mpspo, mpstr;
	int i;

	bus = le32toh(elroy_read32(&r->busnum)) & 0xff;
	for (i = 0; i < sc->sc_int_tbl_sz; i++) {
		if (bus == sc->sc_int_tbl[i].bus &&
		    line == sc->sc_int_tbl[i].line)
			trigger = sc->sc_int_tbl[i].trigger;
	}

	mpspo = (trigger >> MPS_INTPO_SHIFT) & MPS_INTPO_MASK;
	mpstr = (trigger >> MPS_INTTR_SHIFT) & MPS_INTTR_MASK;

	switch (mpspo) {
	case MPS_INTPO_DEF:
		break;
	case MPS_INTPO_ACTHI:
		ent0 &= ~APIC_ENT0_LOW;
		break;
	case MPS_INTPO_ACTLO:
		ent0 |= APIC_ENT0_LOW;
		break;
	default:
		panic("unknown MPS interrupt polarity %d", mpspo);
	}

	switch(mpstr) {
	case MPS_INTTR_DEF:
		break;
	case MPS_INTTR_LEVEL:
		ent0 |= APIC_ENT0_LEV;
		break;
	case MPS_INTTR_EDGE:
		ent0 &= ~APIC_ENT0_LEV;
		break;
	default:
		panic("unknown MPS interrupt trigger %d", mpstr);
	}

	return ent0;
}

#ifdef DEBUG
void
apic_dump(struct elroy_softc *sc)
{
	int i;

	for (i = 0; i < sc->sc_nints; i++)
		printf("0x%04x 0x%04x\n", apic_read(sc->sc_regs, APIC_ENT0(i)),
		    apic_read(sc->sc_regs, APIC_ENT1(i)));

	for (i = 0; i < sc->sc_int_tbl_sz; i++) {
		printf("type=%x ", sc->sc_int_tbl[i].type);
		printf("len=%d ", sc->sc_int_tbl[i].len);
		printf("itype=%d ", sc->sc_int_tbl[i].itype);			
		printf("trigger=%x ", sc->sc_int_tbl[i].trigger);		
		printf("pin=%x ", sc->sc_int_tbl[i].pin);		
		printf("bus=%d ", sc->sc_int_tbl[i].bus);
		printf("line=%d ", sc->sc_int_tbl[i].line);
		printf("addr=%llx\n", sc->sc_int_tbl[i].addr);
	}
}
#endif
