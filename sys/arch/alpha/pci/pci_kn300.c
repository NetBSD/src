/* $NetBSD: pci_kn300.c,v 1.10 1999/02/12 06:25:14 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: pci_kn300.c,v 1.10 1999/02/12 06:25:14 thorpej Exp $");

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
#define	NPIN		4

#define	NVEC	(MAX_MC_BUS * MCPCIA_PER_MCBUS * MCPCIA_MAXSLOT * NPIN)
static char savunit[NVEC];
static char savirqs[NVEC];
extern struct mcpcia_softc *mcpcias;

struct mcpcia_config *mcpcia_eisaccp = NULL;

static struct alpha_shared_intr *kn300_pci_intr;

#ifdef EVCNT_COUNTERS
struct evcnt kn300_intr_evcnt;
#endif

static char * kn300_spurious __P((int));
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

		kn300_pci_intr = alpha_shared_intr_alloc(NVEC);
		for (g = 0; g < NVEC; g++) {
			alpha_shared_intr_set_maxstrays(kn300_pci_intr, g, 25);
			savunit[g] = (char) -1;
			savirqs[g] = (char) -1;
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
#else
#error	"It will be impossible for you to have a console"
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
	 *	bits 0..1	buspin-1
	 *	bits 2..4	PCI Slot (0..7- yes, some don't exist)
	 *	bits 5..7	MID-4
	 *	bits 8..10	7-GID
	 *	bits 11-15	IRQ
	 */
	*ihp = (pci_intr_handle_t)
		(buspin - 1			    )	|
		((device & 0x7)			<< 2)	|
		((ccp->cc_sc->mcpcia_mid - 4)	<< 5)	|
		((7 - ccp->cc_sc->mcpcia_gid)	<< 8)	|
		(kn300_irq << 11);
	return (0);
}

const char *
dec_kn300_intr_string(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{
        static char irqstr[64];
	sprintf(irqstr, "kn300 irq %ld PCI Interrupt Pin %c",
		(ih >> 11) & 0x1f, (int)(ih & 0x3) + 'A');
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
	void *cookie;
	int v;

	v = ih & 0x3ff;
	cookie = alpha_shared_intr_establish(kn300_pci_intr, v, IST_LEVEL,
	    level, func, arg, "kn300 vector");

	if (cookie != NULL && alpha_shared_intr_isactive(kn300_pci_intr, v)) {
		struct mcpcia_config *ccp = ccv;
		savunit[v] = ccp->cc_sc->mcpcia_dev.dv_unit;
		savirqs[v] = (ih >> 11) & 0x1f;
		kn300_enable_intr(ccv, (int)((ih >> 11) & 0x1f));
		alpha_mb();
	}
	return (cookie);
}

void    
dec_kn300_intr_disestablish(ccv, cookie)
        void *ccv, *cookie;
{
	panic("dec_kn300_intr_disestablish not implemented");
}

static char *
kn300_spurious(ih)
	int ih;
{
        static char str[48];
	int pidx, slot, midx, gidx;

	pidx = ih & 0x3;
	slot = (ih >> 2) & 0x7;
	midx = (ih >> 5) & 0x7;
	gidx = (ih >> 8) & 0x7;
	sprintf(str, "mcbus%d mid %d PCI Slot %d PCI Interrupt Pin %c irq",
	    gidx, midx + 4, slot, pidx + 'A');
	return (str);
}

void
kn300_iointr(framep, vec)
	void *framep;
	unsigned long vec;
{
	struct mcpcia_softc *mcp;
	int v, gidx, midx;

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

	v = (int) vec - MCPCIA_VEC_PCI;

	
	midx = v / 0x200;
	gidx = midx / 4;
	midx = midx % 4;


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
	 * Convert vector offset into a dense number.
	 */

	v /= 0x10;
#ifndef	EVCNT_COUNTERS
	if (savirqs[v] >= 0 && savirqs[v] <= INTRCNT_KN300_NCR810) {
		intrcnt[INTRCNT_KN300_IRQ + savirqs[v]]++;
	}
#endif

	if (alpha_shared_intr_dispatch(kn300_pci_intr, v)) {
		return;
	}
	/*
	 * Nobody fielded the interrupt?
	 */
	alpha_shared_intr_stray(kn300_pci_intr, savirqs[v], kn300_spurious(v));
	if (ALPHA_SHARED_INTR_DISABLE(kn300_pci_intr, v) == 0)
		return;
	/*
	 * Search for the controlling mcpcia.
	 */
	for (mcp = mcpcias; mcp != NULL; mcp = mcp->mcpcia_next) {
		if (mcp->mcpcia_dev.dv_unit == savunit[v]) {
			break;
		}
	}
	/*
	 * And if found, disable this IRQ level. Guess what?
	 * This may kill other things too.
	 */
	if (mcp) {
		kn300_disable_intr(&mcp->mcpcia_cc, savirqs[v]);
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
