/* $NetBSD: pci_kn8ae.c,v 1.34 2021/07/04 22:42:36 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: pci_kn8ae.c,v 1.34 2021/07/04 22:42:36 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/syslog.h>
#include <sys/once.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/dwlpxreg.h>
#include <alpha/pci/dwlpxvar.h>

static int	dec_kn8ae_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *dec_kn8ae_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
		    char *, size_t);
static const struct evcnt *dec_kn8ae_intr_evcnt(pci_chipset_tag_t,
		    pci_intr_handle_t);
static void	*dec_kn8ae_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*func)(void *), void *);
static void	dec_kn8ae_intr_disestablish(pci_chipset_tag_t, void *);

static uint32_t imaskcache[DWLPX_NIONODE][DWLPX_NHOSE][NHPC];

static void	kn8ae_spurious(void *, u_long);
static void	kn8ae_enadis_intr(struct dwlpx_config *, pci_intr_handle_t,
		    int);

struct kn8ae_wrapped_pci_intr {
	int	(*ih_fn)(void *);
};

static struct kn8ae_wrapped_pci_intr
    kn8ae_wrapped_pci_intrs[SCB_VECTOIDX(SCB_SIZE - SCB_IOVECBASE)]
    __read_mostly;

static void
kn8ae_intr_wrapper(void *arg, u_long vec)
{
	const u_long idx = SCB_VECTOIDX(vec - SCB_IOVECBASE);

	KERNEL_LOCK(1, NULL);
	kn8ae_wrapped_pci_intrs[idx].ih_fn(arg);
	KERNEL_UNLOCK_ONE(NULL);
}

static ONCE_DECL(pci_kn8ae_once);

static int
pci_kn8ae_init_imaskcache(void)
{
	int io, hose, dev;

	for (io = 0; io < DWLPX_NIONODE; io++) {
		for (hose = 0; hose < DWLPX_NHOSE; hose++) {
			for (dev = 0; dev < NHPC; dev++) {
				imaskcache[io][hose][dev] = DWLPX_IMASK_DFLT;
			}
		}
	}

	return 0;
}

static void
pci_kn8ae_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{

	pc->pc_intr_v = core;
	pc->pc_intr_map = dec_kn8ae_intr_map;
	pc->pc_intr_string = dec_kn8ae_intr_string;
	pc->pc_intr_evcnt = dec_kn8ae_intr_evcnt;
	pc->pc_intr_establish = dec_kn8ae_intr_establish;
	pc->pc_intr_disestablish = dec_kn8ae_intr_disestablish;

	/* Not supported on KN8AE. */
	pc->pc_pciide_compat_intr_establish = NULL;

	RUN_ONCE(&pci_kn8ae_once, pci_kn8ae_init_imaskcache);
}
ALPHA_PCI_INTR_INIT(ST_DEC_21000, pci_kn8ae_pickintr)

#define	IH_MAKE(vec, dev, pin)						\
	((vec) | ((dev) << 16) | ((pin) << 24))

#define	IH_VEC(ih)	((ih) & 0xffff)
#define	IH_DEV(ih)	(((ih) >> 16) & 0xff)
#define	IH_PIN(ih)	(((ih) >> 24) & 0xff)

static int
dec_kn8ae_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
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
	if (buspin < 0 || buspin > 4) {
		printf("dec_kn8ae_intr_map: bad interrupt pin %d\n", buspin);
		return 1;
	}
	pci_decompose_tag(pc, bustag, NULL, &device, NULL);

	mutex_enter(&cpu_lock);
	vec = scb_alloc(kn8ae_spurious, NULL);
	mutex_exit(&cpu_lock);
	if (vec == SCB_ALLOC_FAILED) {
		printf("dec_kn8ae_intr_map: no vector available for "
		    "device %d pin %d\n", device, buspin);
		return 1;
	}

	alpha_pci_intr_handle_init(ihp, IH_MAKE(vec, device, buspin), 0);

	return (0);
}

static const char *
dec_kn8ae_intr_string(pci_chipset_tag_t const pc __unused,
    pci_intr_handle_t const ih, char * const buf, size_t const len)
{
	const u_int ihv = alpha_pci_intr_handle_get_irq(&ih);

	snprintf(buf, len, "vector 0x%x", IH_VEC(ihv));
	return buf;
}

static const struct evcnt *
dec_kn8ae_intr_evcnt(pci_chipset_tag_t const pc __unused,
    pci_intr_handle_t const ih __unused)
{

	/* XXX for now, no evcnt parent reported */
	return (NULL);
}

static void *
dec_kn8ae_intr_establish(
	pci_chipset_tag_t const pc,
	pci_intr_handle_t const ih,
	int const level,
	int (*func)(void *),
	void *arg)
{
	struct dwlpx_config * const ccp = pc->pc_intr_v;
	void *cookie;
	struct scbvec *scb;
	u_long vec;
	int pin, device, hpc;
	void (*scb_func)(void *, u_long);
	const u_int ihv = alpha_pci_intr_handle_get_irq(&ih);
	const u_int flags = alpha_pci_intr_handle_get_flags(&ih);

	device = IH_DEV(ihv);
	pin = IH_PIN(ihv);
	vec = IH_VEC(ihv);

	mutex_enter(&cpu_lock);

	scb = &scb_iovectab[SCB_VECTOIDX(vec - SCB_IOVECBASE)];

	if (scb->scb_func != kn8ae_spurious) {
		mutex_exit(&cpu_lock);
		printf("dec_kn8ae_intr_establish: vector 0x%lx not mapped\n",
		    vec);
		return (NULL);
	}

	/*
	 * NOTE: The PCIA hardware doesn't support interrupt sharing,
	 * so we don't have to worry about it (in theory, at least).
	 */

	if (flags & ALPHA_INTR_MPSAFE) {
		scb_func = (void (*)(void *, u_long))func;
	} else {
		kn8ae_wrapped_pci_intrs[
		    SCB_VECTOIDX(vec - SCB_IOVECBASE)].ih_fn = func;
		    scb_func = kn8ae_intr_wrapper;
	}

	scb_set(vec, scb_func, arg);

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

	mutex_exit(&cpu_lock);

	cookie = (void *) ih.value;

	return (cookie);
}

static void
dec_kn8ae_intr_disestablish(pci_chipset_tag_t const pc, void * const cookie)
{
	struct dwlpx_config * const ccp = pc->pc_intr_v;
	const u_long ihv = (u_long) cookie;
	pci_intr_handle_t ih = { .value = ihv };
	u_long vec;

	vec = IH_VEC(ihv);

	mutex_enter(&cpu_lock);

	kn8ae_enadis_intr(ccp, ih, 0);

	scb_free(vec);

	mutex_exit(&cpu_lock);
}

static void
kn8ae_spurious(void * const arg __unused, u_long const vec)
{
	printf("Spurious interrupt on temporary interrupt vector 0x%lx\n", vec);
}

static void
kn8ae_enadis_intr(struct dwlpx_config *ccp, pci_intr_handle_t ih, int onoff)
{
	struct dwlpx_softc *sc = ccp->cc_sc;
	const u_int ihv = alpha_pci_intr_handle_get_irq(&ih);
	unsigned long paddr;
	uint32_t val;
	int ionode, hose, device, hpc, busp;

	ionode = sc->dwlpx_node - 4;
	hose = sc->dwlpx_hosenum;

	device = IH_DEV(ihv);
	busp = (1 << (IH_PIN(ihv) - 1));

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
	printf("kn8ae_%s_intr: ihv %x imsk 0x%x hpc %d TLSB node %d hose %d\n",
	    onoff? "enable" : "disable", ihv, val, hpc, ionode + 4, hose);
#endif
	const u_long psl = alpha_pal_swpipl(ALPHA_PSL_IPL_HIGH);
	REGVAL(PCIA_IMASK(hpc) + paddr) = val;
	alpha_mb();
	alpha_pal_swpipl(psl);
}
