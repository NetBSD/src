/*	$NetBSD: malta_intr.c,v 1.13.2.1 2007/04/18 20:55:11 ad Exp $	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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

/*
 * Platform-specific interrupt support for the MIPS Malta.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: malta_intr.c,v 1.13.2.1 2007/04/18 20:55:11 ad Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <mips/locore.h>

#include <evbmips/malta/maltavar.h>
#include <evbmips/malta/pci/pcibvar.h>

#include <dev/ic/mc146818reg.h>		/* for malta_cal_timer() */

#include <dev/isa/isavar.h>
#include <dev/pci/pciidereg.h>

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */
const uint32_t ipl_sr_bits[_IPL_N] = {
	0,					/*  0: IPL_NONE */

	MIPS_SOFT_INT_MASK_0,			/*  1: IPL_SOFT */

	MIPS_SOFT_INT_MASK_0,			/*  2: IPL_SOFTCLOCK */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1,		/*  3: IPL_SOFTNET */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1,		/*  4: IPL_SOFTSERIAL */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/*  5: IPL_BIO */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/*  6: IPL_NET */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/*  7: IPL_{TTY,SERIAL} */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0|
		MIPS_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_3|
		MIPS_INT_MASK_4|
		MIPS_INT_MASK_5,		/*  8: IPL_{CLOCK,HIGH} */
};

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given software interrupt priority level.
 * Hardware ipls are port/board specific.
 */
const uint32_t mips_ipl_si_to_sr[SI_NQUEUES] = {
	[SI_SOFT] = MIPS_SOFT_INT_MASK_0,
	[SI_SOFTCLOCK] = MIPS_SOFT_INT_MASK_0,
	[SI_SOFTNET] = MIPS_SOFT_INT_MASK_1,
	[SI_SOFTSERIAL] = MIPS_SOFT_INT_MASK_1,
};

struct malta_cpuintr {
	LIST_HEAD(, evbmips_intrhand) cintr_list;
	struct evcnt cintr_count;
};
#define	NINTRS		5	/* MIPS INT0 - INT4 */

struct malta_cpuintr malta_cpuintrs[NINTRS];
const char *malta_cpuintrnames[NINTRS] = {
	"int 0 (piix4)",
	"int 1 (smi)",
	"int 2 (uart)",
	"int 3 (core hi/gt64120)",
	"int 4 (core lo)",
};

static int	malta_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
static const char
		*malta_pci_intr_string(void *, pci_intr_handle_t);
static const struct evcnt
		*malta_pci_intr_evcnt(void *, pci_intr_handle_t);
static void	*malta_pci_intr_establish(void *, pci_intr_handle_t, int,
		    int (*)(void *), void *);
static void	malta_pci_intr_disestablish(void *, void *);
static void	malta_pci_conf_interrupt(void *, int, int, int, int, int *);
static void	*malta_pciide_compat_intr_establish(void *, struct device *,
		    struct pci_attach_args *, int, int (*)(void *), void *);

void
evbmips_intr_init(void)
{
	struct malta_config *mcp = &malta_configuration;
	int i;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&malta_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&malta_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", malta_cpuintrnames[i]);
	}

	mcp->mc_pc.pc_intr_v = NULL;
	mcp->mc_pc.pc_intr_map = malta_pci_intr_map;
	mcp->mc_pc.pc_intr_string = malta_pci_intr_string;
	mcp->mc_pc.pc_intr_evcnt = malta_pci_intr_evcnt;
	mcp->mc_pc.pc_intr_establish = malta_pci_intr_establish;
	mcp->mc_pc.pc_intr_disestablish = malta_pci_intr_disestablish;
	mcp->mc_pc.pc_conf_interrupt = malta_pci_conf_interrupt;
	mcp->mc_pc.pc_pciide_compat_intr_establish =
	    malta_pciide_compat_intr_establish;
}

void
malta_cal_timer(bus_space_tag_t st, bus_space_handle_t sh)
{
	uint32_t ctrdiff[4], startctr, endctr;
	uint8_t regc;
	int i;

	/* Disable interrupts first. */
	bus_space_write_1(st, sh, 0, MC_REGB);
	bus_space_write_1(st, sh, 1, MC_REGB_SQWE | MC_REGB_BINARY |
	    MC_REGB_24HR);

	/* Initialize for 16Hz. */
	bus_space_write_1(st, sh, 0, MC_REGA);
	bus_space_write_1(st, sh, 1, MC_BASE_32_KHz | MC_RATE_16_Hz);

	/* Run the loop an extra time to prime the cache. */
	for (i = 0; i < 4; i++) {
		// led_display('h', 'z', '0' + i, ' ');

		/* Enable the interrupt. */
		bus_space_write_1(st, sh, 0, MC_REGB);
		bus_space_write_1(st, sh, 1, MC_REGB_PIE | MC_REGB_SQWE |
		    MC_REGB_BINARY | MC_REGB_24HR);

		/* Go to REGC. */
		bus_space_write_1(st, sh, 0, MC_REGC);

		/* Wait for it to happen. */
		startctr = mips3_cp0_count_read();
		do {
			regc = bus_space_read_1(st, sh, 1);
			endctr = mips3_cp0_count_read();
		} while ((regc & MC_REGC_IRQF) == 0);

		/* Already ACK'd. */

		/* Disable. */
		bus_space_write_1(st, sh, 0, MC_REGB);
		bus_space_write_1(st, sh, 1, MC_REGB_SQWE | MC_REGB_BINARY |
		    MC_REGB_24HR);

		ctrdiff[i] = endctr - startctr;
	}

	/* Compute the number of cycles per second. */
	curcpu()->ci_cpu_freq = ((ctrdiff[2] + ctrdiff[3]) / 2) * 16/*Hz*/;

	/* Compute the number of ticks for hz. */
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;

	/* Compute the delay divisor and reciprical. */
	curcpu()->ci_divisor_delay =
	    ((curcpu()->ci_cpu_freq + 500000) / 1000000);
	MIPS_SET_CI_RECIPRICAL(curcpu());

	/*
	 * Get correct cpu frequency if the CPU runs at twice the
	 * external/cp0-count frequency.
	 */
	if (mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		curcpu()->ci_cpu_freq *= 2;

#ifdef DEBUG
	printf("Timer calibration: %lu cycles/sec [(%u, %u) * 16]\n",
	    curcpu()->ci_cpu_freq, ctrdiff[2], ctrdiff[3]);
#endif
}

void *
evbmips_intr_establish(int irq, int (*func)(void *), void *arg)
{
	struct evbmips_intrhand *ih;
	int s;
	
	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;

	s = splhigh();

	/*
	 * Link it into the tables.
	 */
	LIST_INSERT_HEAD(&malta_cpuintrs[0].cintr_list, ih, ih_q);

	/* XXX - should check that MIPS_INT_MASK_0 is set... */

	splx(s);

	return (ih);
}

void
evbmips_intr_disestablish(void *arg)
{
	struct evbmips_intrhand *ih = arg;
	int s;
	
	s = splhigh();

	/*
	 * First, remove it from the table.
	 */
	LIST_REMOVE(ih, ih_q);

	/* XXX - disable MIPS_INT_MASK_0 if list is empty? */
	
	splx(s);

	free(ih, M_DEVBUF);
}

void
evbmips_iointr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
	struct evbmips_intrhand *ih;
	
	/* Check for error interrupts (SMI, GT64120) */
	if (ipending & (MIPS_INT_MASK_1 | MIPS_INT_MASK_3)) {
		if (ipending & MIPS_INT_MASK_1)
			panic("piix4 SMI interrupt");
		if (ipending & MIPS_INT_MASK_3)
			panic("gt64120 error interrupt");
	}

	/*
	 * Read the interrupt pending registers, mask them with the
	 * ones we have enabled, and service them in order of decreasing
	 * priority.
	 */
	if (ipending & MIPS_INT_MASK_0) {
		/* All interrupts are gated through MIPS HW interrupt 0 */
		malta_cpuintrs[0].cintr_count.ev_count++;
		LIST_FOREACH(ih, &malta_cpuintrs[0].cintr_list, ih_q)
			(*ih->ih_func)(ih->ih_arg);
		cause &= ~MIPS_INT_MASK_0;
	}

	/* Re-enable anything that we have processed. */
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));
}

/*
 * YAMON configures pa_intrline correctly (so far), so we trust it to DTRT
 * in the future...
 */
#undef YAMON_IRQ_MAP_BAD

/*
 * PCI interrupt support
 */
static int
malta_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
#ifdef YAMON_IRQ_MAP_BAD
	static const int pciirqmap[12/*device*/][4/*pin*/] = {
		{ -1, -1, -1, 11 },	/* 10: USB */
		{ 10, -1, -1, -1 },	/* 11: Ethernet */
		{ 11, -1, -1, -1 },	/* 12: Audio */
		{ -1, -1, -1, -1 },	/* 13: not used */
		{ -1, -1, -1, -1 },	/* 14: not used */
		{ -1, -1, -1, -1 },	/* 15: not used */
		{ -1, -1, -1, -1 },	/* 16: not used */
		{ -1, -1, -1, -1 },	/* 17: Core card(?) */
		{ 10, 10, 11, 11 },	/* 18: PCI Slot 1 */
		{ 10, 11, 11, 10 },	/* 19: PCI Slot 2 */
		{ 11, 11, 10, 10 },	/* 20: PCI Slot 3 */
		{ 11, 10, 10, 11 },	/* 21: PCI Slot 4 */
	};
	int buspin, device, irq;
#else	/* !YAMON_IRQ_MAP_BAD */
	int buspin;
#endif	/* !YAMON_IRQ_MAP_BAD */

	buspin = pa->pa_intrpin;

	if (buspin == 0) {
		/* No IRQ used. */
		return (1);
	}

	if (buspin > 4) {
		printf("malta_pci_intr_map: bad interrupt pin %d\n", buspin);
		return (1);
	}

#ifdef YAMON_IRQ_MAP_BAD
	pci_decompose_tag(pa->pa_pc, pa->pa_intrtag, NULL, &device, NULL);

	if (device < 10 || device > 21) {
		printf("malta_pci_intr_map: bad device %d\n", device);
		return (1);
	}

	irq = pciirqmap[device - 10][buspin - 1];
	if (irq == -1) {
		printf("malta_pci_intr_map: no mapping for device %d pin %d\n",
		    device, buspin);
		return (1);
	}

	*ihp = irq;
#else	/* !YAMON_IRQ_MAP_BAD */
	*ihp = pa->pa_intrline;
#endif	/* !YAMON_IRQ_MAP_BAD */
	return (0);
}

static const char *
malta_pci_intr_string(void *v, pci_intr_handle_t irq)
{

	return (isa_intr_string(pcib_ic, irq));
}

static const struct evcnt *
malta_pci_intr_evcnt(void *v, pci_intr_handle_t irq)
{

	return (isa_intr_evcnt(pcib_ic, irq));
}

static void *
malta_pci_intr_establish(void *v, pci_intr_handle_t irq, int level,
    int (*func)(void *), void *arg)
{

	return (isa_intr_establish(pcib_ic, irq, IST_LEVEL, level, func, arg));
}

static void
malta_pci_intr_disestablish(void *v, void *arg)
{

	return (isa_intr_disestablish(pcib_ic, arg));
}

static void
malta_pci_conf_interrupt(void *v, int bus, int dev, int func, int swiz,
    int *iline)
{

	/*
	 * We actually don't need to do anything; everything is handled
	 * in pci_intr_map().
	 */
	*iline = 0;
}

void *
malta_pciide_compat_intr_establish(void *v, struct device *dev,
    struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
	pci_chipset_tag_t pc = pa->pa_pc; 
	void *cookie;
	int bus, irq;

	pci_decompose_tag(pc, pa->pa_tag, &bus, NULL, NULL);

	/*
	 * If this isn't PCI bus #0, all bets are off.
	 */
	if (bus != 0)
		return (NULL);

	irq = PCIIDE_COMPAT_IRQ(chan);
	cookie = isa_intr_establish(pcib_ic, irq, IST_EDGE, IPL_BIO, func, arg);
	if (cookie == NULL)
		return (NULL);
	printf("%s: %s channel interrupting at %s\n", dev->dv_xname,
	    PCIIDE_CHANNEL_NAME(chan), malta_pci_intr_string(v, irq));
	return (cookie);
}
