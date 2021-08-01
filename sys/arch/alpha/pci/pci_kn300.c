/* $NetBSD: pci_kn300.c,v 1.38.6.1 2021/08/01 22:42:02 thorpej Exp $ */

/*
 * Copyright (c) 1998 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_kn300.c,v 1.38.6.1 2021/08/01 22:42:02 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/mcbus/mcbusvar.h>
#include <alpha/mcbus/mcbusreg.h>
#include <alpha/pci/mcpciareg.h>
#include <alpha/pci/mcpciavar.h>

#include "sio.h"
#if NSIO > 0 || NPCEB > 0
#include <alpha/pci/siovar.h>
#endif

static int	dec_kn300_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *dec_kn300_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
		    char *, size_t);
static const struct evcnt *dec_kn300_intr_evcnt(pci_chipset_tag_t,
		    pci_intr_handle_t);
static void	*dec_kn300_intr_establish(pci_chipset_tag_t,
		    pci_intr_handle_t, int, int (*func)(void *), void *);
static void	dec_kn300_intr_disestablish(pci_chipset_tag_t, void *);

#define	KN300_PCEB_IRQ	16
#define	KN300_STRAY_MAX	25
#define	NPIN		4

#define	NIRQ	(MAX_MC_BUS * MCPCIA_PER_MCBUS * MCPCIA_MAXSLOT * NPIN)
static int savirqs[NIRQ];

static struct alpha_shared_intr *kn300_pci_intr;

static struct mcpcia_config *mcpcia_eisaccp = NULL;

static void	kn300_iointr(void *, unsigned long);
static void	kn300_enable_intr(struct mcpcia_config *, int);
static void	kn300_disable_intr(struct mcpcia_config *, int);

static void
pci_kn300_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{
	struct mcpcia_config *ccp = core;
	struct evcnt *ev;
	const char *cp;

	if (kn300_pci_intr == NULL) {
		int g;

		kn300_pci_intr = alpha_shared_intr_alloc(NIRQ);
		for (g = 0; g < NIRQ; g++) {
			alpha_shared_intr_set_maxstrays(kn300_pci_intr, g,
			    KN300_STRAY_MAX);

			ev = alpha_shared_intr_evcnt(kn300_pci_intr, g);
			cp = alpha_shared_intr_string(kn300_pci_intr, g);

			evcnt_attach_dynamic(ev, EVCNT_TYPE_INTR, NULL,
			    "kn300", cp);

			savirqs[g] = (char) -1;
		}
	}

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_kn300_intr_map;
	pc->pc_intr_string = dec_kn300_intr_string;
	pc->pc_intr_evcnt = dec_kn300_intr_evcnt;
	pc->pc_intr_establish = dec_kn300_intr_establish;
	pc->pc_intr_disestablish = dec_kn300_intr_disestablish;

	/* Not supported on KN300. */
	pc->pc_pciide_compat_intr_establish = NULL;

	if (EISA_PRESENT(REGVAL(MCPCIA_PCI_REV(ccp)))) {
		mcpcia_eisaccp = ccp;
#if NSIO > 0 || NPCEB > 0
		sio_intr_setup(pc, &ccp->cc_iot);
		kn300_enable_intr(ccp, KN300_PCEB_IRQ);
#endif
	}
}
ALPHA_PCI_INTR_INIT(ST_DEC_4100, pci_kn300_pickintr)

static int
dec_kn300_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	struct mcpcia_config *ccp = (struct mcpcia_config *)pc->pc_intr_v;
	int device;
	int mcpcia_irq;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin < 0 || buspin > 4) {
		printf("dec_kn300_intr_map: bad interrupt pin %d\n", buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);

	/*
	 * On MID 5 device 1 is the internal NCR 53c810.
	 */
	if (ccp->cc_mid == 5 && device == 1) {
		mcpcia_irq = 16;
	} else if (device >= 2 && device <= 5) {
		mcpcia_irq = (device - 2) * 4 + buspin - 1;
	} else {
		printf("dec_kn300_intr_map: weird device number %d\n", device);
		return(1);
	}

	/*
	 * handle layout:
	 *
	 *	Determine kn300 IRQ (encoded in SCB vector):
	 *	bits 0..1	buspin-1
	 *	bits 2..4	PCI Slot (0..7- yes, some don't exist)
	 *	bits 5..7	MID-4
	 *	bits 8..10	7-GID
	 *
	 *	Software only:
	 *	bits 11-15	MCPCIA IRQ
	 */
	const u_int irq = 
		(buspin - 1			    )	|
		((device & 0x7)			<< 2)	|
		((ccp->cc_mid - 4)		<< 5)	|
		((7 - ccp->cc_gid)		<< 8)	|
		(mcpcia_irq			<< 11);
	alpha_pci_intr_handle_init(ihp, irq, 0);
	return (0);
}

static const char *
dec_kn300_intr_string(pci_chipset_tag_t const pc __unused,
    pci_intr_handle_t const ih, char * const buf, size_t const len)
{
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih) & 0x3ff;

	snprintf(buf, len, "kn300 irq %u", irq);
	return buf;
}

static const struct evcnt *
dec_kn300_intr_evcnt(pci_chipset_tag_t const pc __unused,
    pci_intr_handle_t const ih)
{
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih) & 0x3ff;

	return alpha_shared_intr_evcnt(kn300_pci_intr, irq);
}

static void *
dec_kn300_intr_establish(
	pci_chipset_tag_t const pc,
	pci_intr_handle_t const ih,
	int const level,
	int (*func)(void *),
	void *arg)
{
	struct mcpcia_config * const ccp = pc->pc_intr_v;
	void *cookie;
	const u_int ihv = alpha_pci_intr_handle_get_irq(&ih);
	const u_int irq = ihv & 0x3ff;
	const u_int flags = alpha_pci_intr_handle_get_flags(&ih);

	cookie = alpha_shared_intr_alloc_intrhand(kn300_pci_intr, irq,
	    IST_LEVEL, level, flags, func, arg, "kn300");

	if (cookie == NULL)
		return NULL;

	mutex_enter(&cpu_lock);

	if (! alpha_shared_intr_link(kn300_pci_intr, cookie, "kn300")) {
		mutex_exit(&cpu_lock);
		alpha_shared_intr_free_intrhand(cookie);
		return NULL;
	}

	if (alpha_shared_intr_firstactive(kn300_pci_intr, irq)) {
		scb_set(MCPCIA_VEC_PCI + SCB_IDXTOVEC(irq), kn300_iointr, NULL);
		alpha_shared_intr_set_private(kn300_pci_intr, irq, ccp);
		savirqs[irq] = (ihv >> 11) & 0x1f;
		alpha_mb();
		kn300_enable_intr(ccp, savirqs[irq]);
	}

	mutex_exit(&cpu_lock);

	return cookie;
}

static void
dec_kn300_intr_disestablish(pci_chipset_tag_t const pc __unused,
    void * const cookie __unused)
{
	panic("dec_kn300_intr_disestablish not implemented");
}

static void
kn300_iointr(void * const arg __unused, unsigned long const vec)
{
	struct mcpcia_softc *mcp;
	u_long irq;

	irq = SCB_VECTOIDX(vec - MCPCIA_VEC_PCI);

	if (alpha_shared_intr_dispatch(kn300_pci_intr, irq)) {
		/*
		 * Any claim of an interrupt at this level is a hint to
		 * reset the stray interrupt count- elsewise a slow leak
		 * over time will cause this level to be shutdown.
		 */
		alpha_shared_intr_reset_strays(kn300_pci_intr, irq);
		return;
	}

	/*
	 * If we haven't finished configuring yet, or there is no mcp
	 * registered for this level yet, just return.
	 */
	mcp = alpha_shared_intr_get_private(kn300_pci_intr, irq);
	if (mcp == NULL || mcp->mcpcia_cc == NULL)
		return;

	/*
	 * We're getting an interrupt for a device we haven't enabled.
	 * We had better not try and use -1 to find the right bit to disable.
	 */
	if (savirqs[irq] == -1) {
		printf("kn300_iointr: stray interrupt vector 0x%lx\n", vec);
		return;
	}

	/*
	 * Stray interrupt; disable the IRQ on the appropriate MCPCIA
	 * if we've reached the limit.
	 */
	alpha_shared_intr_stray(kn300_pci_intr, irq, "kn300");
	if (ALPHA_SHARED_INTR_DISABLE(kn300_pci_intr, irq) == 0)
		return;
	kn300_disable_intr(mcp->mcpcia_cc, savirqs[irq]);
}

static void
kn300_enable_intr(struct mcpcia_config * const ccp, int const irq)
{
	alpha_mb();
	REGVAL(MCPCIA_INT_MASK0(ccp)) |= (1 << irq);
	alpha_mb();
}

static void
kn300_disable_intr(struct mcpcia_config * const ccp, int const irq)
{
	alpha_mb();
	REGVAL(MCPCIA_INT_MASK0(ccp)) &= ~(1 << irq);
	alpha_mb();
}
