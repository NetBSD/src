/* $NetBSD: pci_kn300.c,v 1.4 1998/04/24 01:25:19 mjacob Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: pci_kn300.c,v 1.4 1998/04/24 01:25:19 mjacob Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <vm/vm.h>

#include <machine/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/mcbus/mcbusvar.h>
#include <alpha/mcbus/mcbusreg.h>
#include <alpha/pci/mcpciareg.h>
#include <alpha/pci/mcpciavar.h>
#include <alpha/pci/pci_kn300.h>

#ifndef EVCNT_COUNTERS
#include <machine/intrcnt.h>
#endif

#include "sio.h"
#if	NSIO
#include <alpha/pci/siovar.h>
#endif

int
dec_kn300_intr_map __P((void *, pcitag_t, int, int, pci_intr_handle_t *));
const char *dec_kn300_intr_string __P((void *, pci_intr_handle_t));
void * dec_kn300_intr_establish __P((void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *));
void	dec_kn300_intr_disestablish __P((void *, void *));

#define	KN300_PCEB_IRQ	16
#define	NPIN	4

static struct vectab {
	int (*func) __P((void *));
	void *arg;
	int irq;
} vectab[MAX_MC_BUS][MCPCIA_PER_MCBUS][MCPCIA_MAXSLOT][NPIN];

struct mcpcia_config *mcpcia_eisaccp = NULL;

#ifdef EVCNT_COUNTERS
struct evcnt kn300_intr_evcnt;
#endif

int	kn300_spurious __P((void *));
void	kn300_iointr __P((void *, unsigned long));
void	kn300_enable_intr __P((struct mcpcia_config *, int));
void	kn300_disable_intr __P((struct mcpcia_config *, int));

void
pci_kn300_pickintr(ccp, first)
	struct mcpcia_config *ccp;
	int first;
{
	pci_chipset_tag_t pc = &ccp->cc_pc;

	if (first) {
		int g;
		for (g = 0; g < MAX_MC_BUS; g++) {
			int m;
			for (m = 0; m < MCPCIA_PER_MCBUS; m++) {
				int s;
				for (s = 0; s < MCPCIA_MAXSLOT; s++) {
					int p;
					for (p = 0; p < NPIN; p++) {
						struct vectab *vp;
						vp = &vectab[g][m][s][p];
						vp->func = kn300_spurious;
						vp->arg = (void *) ((long)
							(g << 0) |
							(m << 3) |
							(s << 6) |
							(p << 9));
						vp->irq = -1;
					}
				}
			}
		}
		set_iointr(kn300_iointr);
	}

	pc->pc_intr_v = ccp;
	pc->pc_intr_map = dec_kn300_intr_map;
	pc->pc_intr_string = dec_kn300_intr_string;
	pc->pc_intr_establish = dec_kn300_intr_establish;
	pc->pc_intr_disestablish = dec_kn300_intr_disestablish;

	/* Not supported on KN300. */
	pc->pc_pciide_compat_intr_establish = NULL;

#if	NSIO
	if (EISA_PRESENT(REGVAL(MCPCIA_PCI_REV(ccp->cc_sc)))) {
		extern void dec_kn300_cons_init __P((void));
		bus_space_tag_t iot = &ccp->cc_iot;
		if (mcpcia_eisaccp) {
			printf("Huh? There's only supposed to be one eisa!\n");
		}
		sio_intr_setup(pc, iot);
		kn300_enable_intr(ccp, KN300_PCEB_IRQ);
		mcpcia_eisaccp = ccp;
		dec_kn300_cons_init();	/* XXXXXXXXXXXXXXXXXXX */
	}
#endif
}

int     
dec_kn300_intr_map(ccv, bustag, buspin, line, ihp)
        void *ccv;
        pcitag_t bustag; 
        int buspin, line;
        pci_intr_handle_t *ihp;
{
	struct mcpcia_config *ccp = ccv;
	pci_chipset_tag_t pc = &ccp->cc_pc;
	int device;
	int kn300_irq;

        if (buspin == 0) {
                /* No IRQ used. */
                return 1;
        }
        if (buspin > 4 || buspin < 0) {
                printf("dec_kn300_intr_map: bad interrupt pin %d\n", buspin);
                return 1;
        }

	alpha_pci_decompose_tag(pc, bustag, NULL, &device, NULL);

	if (device == 1) {
		/*
		 * This can only be the NCR810 SCSI.
		 * XXX: Do we need to check this more closely?
		 */
		if (EISA_PRESENT(REGVAL(MCPCIA_PCI_REV(ccp->cc_sc)))) {
			printf("XXXXX: How can you have an EISA in the spot "
				"as an NCR 810?\n");
		}
		kn300_irq = 16;
	} else if (device >= 2 && device <= 5) {
		kn300_irq = (device - 2) * 4;
	} else {
                printf("dec_kn300_intr_map: weird device number %d\n", device);
                return(1);
	}
	/*
	 * handle layout:
	 *
	 *	bits 0..2	7-GID
	 *	bits 3..5	MID-4
	 *	bits 6..8	PCI Slot (0..7- yes, some don't exist)
	 *	bits 9..10	buspin-1
	 *	bits 11-15	IRQ
	 */
	*ihp = (pci_intr_handle_t)
		(7 - ccp->cc_sc->mcpcia_gid)		|
		((ccp->cc_sc->mcpcia_mid - 4) << 3)	|
		(device << 6)				|
		((buspin-1) << 9)			|
		(kn300_irq << 11);
	return (0);
}

const char *
dec_kn300_intr_string(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{
        static char irqstr[64];
	sprintf(irqstr, "kn300 irq %d PCI Interrupt Pin %c",
		(ih >> 11) & 0x1f, ((ih >> 9) & 0x3) + 'A');
	return (irqstr);
}

void *
dec_kn300_intr_establish(ccv, ih, level, func, arg)
        void *ccv;
        pci_intr_handle_t ih;
        int level;
        int (*func) __P((void *));
        void *arg;
{           
	struct vectab *vp;
	int gidx, midx, slot, pidx, s;
	void *cookie = NULL;

	gidx = ih & 0x7;
	midx = (ih >> 3) & 0x7;
	slot = (ih >> 6) & 0x7;
	pidx = (ih >> 9) & 0x3;

	/*
	 * XXX: FIXME - for PCI bridges, the framework here is such that
	 * XXX: we're mapping the mapping for the bridge, not the
	 * XXX: device behind the bridge, so we're not cognizant of the
	 * XXX: pin swizzling that could go on. We really ought to fix
	 * XXX: this, but for a first cut, assuming all pins map to the
	 * XXX: same device and filling in all pin vectors is probably
	 * XXX: okay.
	 */

	for (pidx = 0; pidx < 4; pidx++) {
		vp = &vectab[gidx][midx][slot][pidx];
		if (vp->func != kn300_spurious) {
			printf("dec_kn300_intr_establish: vector cookie 0x%x "
				"already used\n", ih);
			return (cookie);
		}
		s = splhigh();
		vp->func = func;
		vp->arg = arg;
		vp->irq = (ih >> 11) & 0x1f;
		(void) splx(s);
	}

	kn300_enable_intr(ccv, (int)((ih >> 11) & 0x1f));
	cookie = (void *) ih;
	return (cookie);
}

void    
dec_kn300_intr_disestablish(ccv, cookie)
        void *ccv, *cookie;
{
	pci_intr_handle_t ih = (pci_intr_handle_t) cookie;
	int gidx, midx, slot, pidx, s;
	struct vectab *vp;

	gidx = ih & 0x7;
	midx = (ih >> 3) & 0x7;
	slot = (ih >> 6) & 0x7;
	pidx = (ih >> 9) & 0x3;
	vp = &vectab[gidx][midx][slot][pidx];
	s = splhigh();
	vp->func = kn300_spurious;
	vp->arg = cookie;
	vp->irq = -1;
	(void) splx(s);
	kn300_disable_intr(ccv, (int)((ih >> 11) & 0x1f));
}

int
kn300_spurious(arg)
	void *arg;
{
	pci_intr_handle_t ih = (pci_intr_handle_t) arg;
	int gidx, midx, slot, pidx;

	gidx = ih & 0x7;
	midx = (ih >> 3) & 0x7;
	slot = (ih >> 6) & 0x7;
	pidx = (ih >> 9) & 0x3;
	printf("Spurious Interrupt from mcbus%d MID %d Slot %d "
		"PCI Interrupt Pin %c\n", gidx, midx + 4, slot, pidx + 'A');
	/*
	 * XXX: We *could*, if we have recorded all the mcpcia softcs
	 * XXX: that we've configured, and this spurious interrupt
	 * XXX: falls within this range, disable this interrupt.
	 *
	 * XXX: Later.
	 */
	return (-1);
}

void
kn300_iointr(framep, vec)
	void *framep;
	unsigned long vec;
{
	int base, gidx, midx, slot, pidx;
	struct vectab *vp;

	if (vec >= MCPCIA_VEC_EISA && vec < MCPCIA_VEC_PCI) {
#if NSIO
		sio_iointr(framep, vec);
		return;
#else
		printf("kn300_iointr: (E)ISA interrupt support not configured"
			" for vector 0x%x", vec);
		kn300_disable_intr(mcpcia_eisaccp, KN300_PCEB_IRQ);
#endif
	} 

	base = (int) vec - MCPCIA_VEC_PCI;

	midx = base / 0x200;
	gidx = midx / 4;
	midx = midx % 4;
	slot = (base % 0x200) / 0x10;
	pidx = ((slot / 4) & 0x3);
	slot = slot / 4;

	if (gidx >= MAX_MC_BUS || midx >= MCPCIA_PER_MCBUS ||
	    slot >= MCPCIA_MAXSLOT || pidx >= NPIN) {
		panic("kn300_iointr: vec 0x%x (mcbus%d mid%d slot %d pin%d)",
		    vec, gidx, midx+4, slot, pidx);
	}

#ifdef	EVCNT_COUNTERS
	kn300_intr_evcnt.ev_count++;
#endif
	/*
	 * Check for i2c bus interrupts XXXXXXXXX
	 */
	if (gidx == 0 && midx == 0 && vec == MCPCIA_I2C_CVEC) {
#ifndef	EVCNT_COUNTERS
		intrcnt[INTRCNT_KN300_I2C_CTRL]++;
#endif
		printf("i2c: controller interrupt\n");
		return;
	}
	if (gidx == 0 && midx == 0 && vec == MCPCIA_I2C_BVEC) {
#ifndef	EVCNT_COUNTERS
		intrcnt[INTRCNT_KN300_I2C_BUS]++;
#endif
		printf("i2c: bus interrupt\n");
		return;
	}

	/*
	 * Now, for the main stuff...
	 */
	vp = &vectab[gidx][midx][slot][pidx];
#ifndef	EVCNT_COUNTERS
	if (vp->irq >= 0 && vp->irq <= INTRCNT_KN300_NCR810) {
		intrcnt[INTRCNT_KN300_IRQ + vp->irq]++;
	}
#endif
	if ((*vp->func)(vp->arg) == 0) {
#if	0
		printf("Unclaimed interrupt from mcbus%d MID %d Slot %d "
		    "PCI Interrupt Pin %c\n", gidx, midx + 4, slot, pidx + 'A');
#endif
	}
}

void
kn300_enable_intr(ccp, irq)
	struct mcpcia_config *ccp;
	int irq;
{
	alpha_mb();
	REGVAL(MCPCIA_INT_MASK0(ccp->cc_sc)) |= (1 << irq);
	alpha_mb();
}

void
kn300_disable_intr(ccp, irq)
	struct mcpcia_config *ccp;
	int irq;
{
	alpha_mb();
	REGVAL(MCPCIA_INT_MASK0(ccp->cc_sc)) &= ~(1 << irq);
	alpha_mb();
}
