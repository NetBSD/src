/* $NetBSD: pci_kn8ae.c,v 1.4.2.3 1997/07/22 06:00:52 cgd Exp $ */

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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_kn8ae.c,v 1.4.2.3 1997/07/22 06:00:52 cgd Exp $");

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

#include <alpha/pci/dwlpxreg.h>
#include <alpha/pci/dwlpxvar.h>
#include <alpha/pci/pci_kn8ae.h>

#ifndef EVCNT_COUNTERS
#include <machine/intrcnt.h>
#endif

int	dec_kn8ae_intr_map __P((void *, pcitag_t, int, int,
	    pci_intr_handle_t *));
const char *dec_kn8ae_intr_string __P((void *, pci_intr_handle_t));
void	*dec_kn8ae_intr_establish __P((void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *));
void	dec_kn8ae_intr_disestablish __P((void *, void *));

#define	NIONODE	5
#define	NHOSE	4
struct vectab {
	int (*func) __P((void *));
	void *arg;
} vectab[NIONODE][NHOSE][DWLPX_MAXDEV];
static u_int32_t imaskcache[NIONODE][NHOSE][NHPC];

#ifdef EVCNT_COUNTERS
struct evcnt kn8ae_intr_evcnt;
#endif

int	kn8ae_spurious __P((void *));
void	kn8ae_iointr __P((void *framep, unsigned long vec));
void	kn8ae_enadis_intr __P((pci_intr_handle_t, int));
void	kn8ae_enable_intr __P((pci_intr_handle_t irq));
void	kn8ae_disable_intr __P((pci_intr_handle_t irq));

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
        pc->pc_intr_establish = dec_kn8ae_intr_establish;
        pc->pc_intr_disestablish = dec_kn8ae_intr_disestablish;

	if (!first) {
		return;
	}

	for (io = 0; io < NIONODE; io++) {
		for (hose = 0; hose < NHOSE; hose++) {
			for (dev = 0; dev < DWLPX_MAXDEV; dev++) {
				vectab[io][hose][dev].func = kn8ae_spurious;
				vectab[io][hose][dev].arg = (void *)
				    (u_int64_t) DWLPX_MVEC(io, hose, dev);
			}
		}
	}
	for (io = 0; io < NIONODE; io++) {
		for (hose = 0; hose < NHOSE; hose++) {
			for (dev = 0; dev < NHPC; dev++) {
				imaskcache[io][hose][dev] = DWLPX_IMASK_DFLT;
			}
		}
	}
	set_iointr(kn8ae_iointr);
}

int     
dec_kn8ae_intr_map(ccv, bustag, buspin, line, ihp)
        void *ccv;
        pcitag_t bustag; 
        int buspin, line;
        pci_intr_handle_t *ihp;
{
	int device, ionode, hose;
	struct dwlpx_config *ccp = ccv;
	pci_chipset_tag_t pc = &ccp->cc_pc;

        if (buspin == 0) {
                /* No IRQ used. */
                return 1;
        }
        if (buspin > 4) {
                printf("pci_map_int: bad interrupt pin %d\n", buspin);
                return 1;
        }
	alpha_pci_decompose_tag(pc, bustag, NULL, &device, NULL);
	ionode = ccp->cc_sc->dwlpx_node - 4;
	hose = ccp->cc_sc->dwlpx_hosenum;

	/*
	 * handle layout:
	 *	bits 0..15	DWLPX_MVEC(ionode, hose, device)
	 *	bits 16-24	buspin (1..N)
	 *	bits 24-31	IPL
	 */
	*ihp = DWLPX_MVEC(ionode, hose, device) | (buspin << 16) | (14 << 24);
	return (0);
}

const char *
dec_kn8ae_intr_string(ccv, ih)
	void *ccv;
	pci_intr_handle_t ih;
{
	static char irqstr[64];
        sprintf(irqstr, "kn8ae irq %d vector 0x%x PCI Interrupt Pin %c",
	    (ih >> 24), ih & 0xffff, (((ih >> 16) & 0x7) - 1) + 'A');
        return (irqstr);
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
	void *cookie = NULL;
	int ionode, hose, device, s;
	struct vectab *vp;

	ionode	= ccp->cc_sc->dwlpx_node - 4;
	hose	= ccp->cc_sc->dwlpx_hosenum;
	device	= DWLPX_MVEC_PCISLOT(ih);

	if (ionode < 0 || ionode >= NIONODE) {
		panic("dec_kn8ae_intr_establish: bad ionode %d\n", ionode);
	}
	if (hose < 0 || hose >= NHOSE) {
		panic("dec_kn8ae_intr_establish: bad hose %d\n", hose);
	}
	if (device < 0 || device >= DWLPX_MAXDEV) {
		panic("dec_kn8ae_intr_establish: bad device %d\n", device);
	}

	vp = &vectab[ionode][hose][device];
	if (vp->func != kn8ae_spurious) {
		printf("dec_kn8ae_intr_establish: vector 0x%x already used\n",
		    ih & 0xffff);
		return (cookie);
	}

	s = splhigh();
	vp->func = func;
	vp->arg = arg;
	(void) splx(s);
	kn8ae_enable_intr(ih);
	cookie = (void *) (u_int64_t) DWLPX_MVEC(ionode, hose, device);
	return (cookie);
}

void    
dec_kn8ae_intr_disestablish(ccv, cookie)
        void *ccv, *cookie;
{
	int ionode, hose, device, s;
	struct vectab *vp;

	ionode = DWLPX_MVEC_IONODE(cookie);
	hose = DWLPX_MVEC_HOSE(cookie);
	device = DWLPX_MVEC_PCISLOT(cookie);
	if (ionode < 0 || ionode >= NIONODE || hose < 0 || hose >= NHOSE ||
	    device < 0 || device >= DWLPX_MAXDEV) {
		return;
	}
	vp = &vectab[ionode][hose][device];
	s = splhigh();
	vp->func = kn8ae_spurious;
	vp->arg = cookie;
	(void) splx(s);
}

void
kn8ae_iointr(framep, vec)
	void *framep;
	unsigned long vec;
{
	struct vectab *vp;
	int ionode, hose, device;
	if ((vec & DWLPX_VEC_MARK) == 0) {
		panic("kn8ae_iointr: vec 0x%x\n", vec);
	}
#ifdef	EVCNT_COUNTERS
	kn8ae_intr_evcnt.ev_count++;
#else
	;	/* XXX */
#endif
	ionode = DWLPX_MVEC_IONODE(vec);
	hose = DWLPX_MVEC_HOSE(vec);
	device = DWLPX_MVEC_PCISLOT(vec);

	if (ionode < 0 || ionode >= NIONODE || hose < 0 || hose >= NHOSE ||
	    device < 0 || device >= DWLPX_MAXDEV) {
		panic("kn8ae_iointr: malformed vector 0x%x\n", vec);
	}
	vp = &vectab[ionode][hose][device];
	if ((*vp->func)(vp->arg) == 0) {
#if	0
		printf("kn8ae_iointr: TLSB Node %d Hose %d Slot %d - "
		    " unclaimed interrupt\n", ionode + 4, hose, device);
#endif
	}
}

int
kn8ae_spurious(arg)
	void *arg;
{
	int ionode, hose, device;
	ionode = DWLPX_MVEC_IONODE(arg);
	hose = DWLPX_MVEC_HOSE(arg);
	device = DWLPX_MVEC_PCISLOT(arg);
	printf("Spurious Interrupt from TLSB Node %d Hose %d Slot %d\n",
	    ionode + 4, hose, device);
	return (-1);
}


void
kn8ae_enadis_intr(irq, onoff)
	pci_intr_handle_t irq;
	int onoff;
{
	unsigned long paddr;
	u_int32_t val;
	int ionode, hose, device, hpc, busp, s;

	ionode = DWLPX_MVEC_IONODE(irq);
	hose = DWLPX_MVEC_HOSE(irq);
	device = DWLPX_MVEC_PCISLOT(irq);
	busp = 1 << (((irq >> 16) & 0xff) - 1);
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

void
kn8ae_enable_intr(irq)
	pci_intr_handle_t irq;
{
	kn8ae_enadis_intr(irq, 1);
}

void
kn8ae_disable_intr(irq)
	pci_intr_handle_t irq;
{
	kn8ae_enadis_intr(irq, 0);
}
