/* $NetBSD: pci_kn8ae.c,v 1.18.4.2 2001/08/25 06:15:03 thorpej Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
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

__KERNEL_RCSID(0, "$NetBSD: pci_kn8ae.c,v 1.18.4.2 2001/08/25 06:15:03 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/dwlpxreg.h>
#include <alpha/pci/dwlpxvar.h>
#include <alpha/pci/pci_kn8ae.h>

int	dec_kn8ae_intr_map __P((struct pci_attach_args *,
	    pci_intr_handle_t *));
const char *dec_kn8ae_intr_string __P((void *, pci_intr_handle_t));
const struct evcnt *dec_kn8ae_intr_evcnt __P((void *, pci_intr_handle_t));
void	*dec_kn8ae_intr_establish __P((void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *));
void	dec_kn8ae_intr_disestablish __P((void *, void *));

static u_int32_t imaskcache[DWLPX_NIONODE][DWLPX_NHOSE][NHPC];

void	kn8ae_spurious __P((void *, u_long));
void	kn8ae_enadis_intr __P((struct dwlpx_config *, pci_intr_handle_t, int));

void
pci_kn8ae_pickintr(ccp, first)
	struct dwlpx_config *ccp;
	int first;
{
	int io, hose, dev;
	pci_chipset_tag_t pc = &ccp->cc_pc;

        pc->pc_intr_v = ccp;
        pc->pc_intr_map = dec_kn8ae_intr_map;
        pc->pc_intr_string = dec_kn8ae_intr_string;
	pc->pc_intr_evcnt = dec_kn8ae_intr_evcnt;
        pc->pc_intr_establish = dec_kn8ae_intr_establish;
        pc->pc_intr_disestablish = dec_kn8ae_intr_disestablish;

	/* Not supported on KN8AE. */
	pc->pc_pciide_compat_intr_establish = NULL;

	if (!first) {
		return;
	}

	for (io = 0; io < DWLPX_NIONODE; io++) {
		for (hose = 0; hose < DWLPX_NHOSE; hose++) {
			for (dev = 0; dev < NHPC; dev++) {
				imaskcache[io][hose][dev] = DWLPX_IMASK_DFLT;
			}
		}
	}
}

#define	IH_MAKE(vec, dev, pin)						\
	((vec) | ((dev) << 16) | ((pin) << 24))

#define	IH_VEC(ih)	((ih) & 0xffff)
#define	IH_DEV(ih)	(((ih) >> 16) & 0xff)
#define	IH_PIN(ih)	(((ih) >> 24) & 0xff)

int     
dec_kn8ae_intr_map(pa, ihp)
	struct pci_attach_args *pa;
        pci_intr_handle_t *ihp;
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device;
	u_long vec;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin > 4) {
		printf("dec_kn8ae_intr_map: bad interrupt pin %d\n", buspin);
		return 1;
	}
	alpha_pci_decompose_tag(pc, bustag, NULL, &device, NULL);

	vec = scb_alloc(kn8ae_spurious, NULL);
	if (vec == SCB_ALLOC_FAILED) {
		printf("dec_kn8ae_intr_map: no vector available for "
		    "device %d pin %d\n", device, buspin);
		return 1;
	}

	*ihp = IH_MAKE(vec, device, buspin);

	return (0);
}

const char *
dec_kn8ae_intr_string(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{
	static char irqstr[64];

	sprintf(irqstr, "vector 0x%lx", IH_VEC(ih));

        return (irqstr);
}

const struct evcnt *
dec_kn8ae_intr_evcnt(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{

	/* XXX for now, no evcnt parent reported */
	return (NULL);
}

void *
dec_kn8ae_intr_establish(ccv, ih, level, func, arg)
        void *ccv;
        pci_intr_handle_t ih;
        int level;
        int (*func) __P((void *));
	void *arg;
{           
	struct dwlpx_config *ccp = ccv;
	void *cookie;
	struct scbvec *scb;
	u_long vec;
	int pin, device, hpc;

	device = IH_DEV(ih);
	pin = IH_PIN(ih);
	vec = IH_VEC(ih);

	scb = &scb_iovectab[SCB_VECTOIDX(vec - SCB_IOVECBASE)];

	if (scb->scb_func != kn8ae_spurious) {
		printf("dec_kn8ae_intr_establish: vector 0x%lx not mapped\n",
		    vec);
		return (NULL);
	}

	/*
	 * NOTE: The PCIA hardware doesn't support interrupt sharing,
	 * so we don't have to worry about it (in theory, at least).
	 */

	scb->scb_arg = arg;
	alpha_mb();
	scb->scb_func = (void (*)(void *, u_long))func;
	alpha_mb();

	if (device < 4) {
		hpc = 0;
	} else if (device < 8) {
		device -= 4;
		hpc = 1;
	} else {
		device -= 8;
		hpc = 2;
	}
	REGVAL(PCIA_DEVVEC(hpc, device, pin) + ccp->cc_sysbase) = vec;

	kn8ae_enadis_intr(ccp, ih, 1);

	cookie = (void *) ih;

	return (cookie);
}

void    
dec_kn8ae_intr_disestablish(ccv, cookie)
        void *ccv, *cookie;
{
	struct dwlpx_config *ccp = ccv;
	pci_intr_handle_t ih = (u_long) cookie;
	struct scbvec *scb;
	u_long vec;

	vec = IH_VEC(ih);

	scb = &scb_iovectab[SCB_VECTOIDX(vec - SCB_IOVECBASE)];

	kn8ae_enadis_intr(ccp, ih, 0);

	scb_free(vec);
}

void
kn8ae_spurious(void *arg, u_long vec)
{
	printf("Spurious interrupt on temporary interrupt vector 0x%lx\n", vec);
}

void
kn8ae_enadis_intr(ccp, irq, onoff)
	struct dwlpx_config *ccp;
	pci_intr_handle_t irq;
	int onoff;
{
	struct dwlpx_softc *sc = ccp->cc_sc;
	unsigned long paddr;
	u_int32_t val;
	int ionode, hose, device, hpc, busp, s;

	ionode = sc->dwlpx_node - 4;
	hose = sc->dwlpx_hosenum;

	device = IH_DEV(irq);
	busp = (1 << (IH_PIN(irq) - 1));

	paddr = (1LL << 39);
	paddr |= (unsigned long) ionode << 36;
	paddr |= (unsigned long) hose << 34;

	if (device < 4) {
		hpc = 0;
	} else if (device < 8) {
		hpc = 1;
		device -= 4;
	} else {
		hpc = 2;
		device -= 8;
	}
	busp <<= (device << 2);
	val = imaskcache[ionode][hose][hpc];
	if (onoff)
		val |= busp;
	else
		val &= ~busp;
	imaskcache[ionode][hose][hpc] = val;
#if	0
	printf("kn8ae_%s_intr: irq %lx imsk 0x%x hpc %d TLSB node %d hose %d\n",
	    onoff? "enable" : "disable", irq, val, hpc, ionode + 4, hose);
#endif
	s = splhigh();
	REGVAL(PCIA_IMASK(hpc) + paddr) = val;
	alpha_mb();
	(void) splx(s);
}
