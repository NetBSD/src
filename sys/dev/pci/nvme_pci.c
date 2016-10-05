/*	$NetBSD: nvme_pci.c,v 1.2.2.4 2016/10/05 20:55:43 skrll Exp $	*/
/*	$OpenBSD: nvme_pci.c,v 1.3 2016/04/14 11:18:32 dlg Exp $ */

/*
 * Copyright (c) 2014 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*-
 * Copyright (C) 2016 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nvme_pci.c,v 1.2.2.4 2016/10/05 20:55:43 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/bitops.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/interrupt.h>
#include <sys/kmem.h>
#include <sys/pmf.h>
#include <sys/module.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/ic/nvmereg.h>
#include <dev/ic/nvmevar.h>

int nvme_pci_force_intx = 0;
int nvme_pci_mpsafe = 1;
int nvme_pci_mq = 1;		/* INTx: ioq=1, MSI/MSI-X: ioq=ncpu */

#define NVME_PCI_BAR		0x10

#ifndef __HAVE_PCI_MSI_MSIX
#define pci_intr_release(pc, intrs, nintrs) \
	kmem_free(intrs, sizeof(*intrs) * nintrs)
#define pci_intr_establish_xname(pc, ih, level, intrhand, intrarg, xname) \
	pci_intr_establish(pc, ih, level, intrhand, intrarg)
#endif

struct nvme_pci_softc {
	struct nvme_softc	psc_nvme;

	pci_chipset_tag_t	psc_pc;
	pci_intr_handle_t	*psc_intrs;
	int			psc_nintrs;
};

static int	nvme_pci_match(device_t, cfdata_t, void *);
static void	nvme_pci_attach(device_t, device_t, void *);
static int	nvme_pci_detach(device_t, int);
static int	nvme_pci_rescan(device_t, const char *, const int *);

CFATTACH_DECL3_NEW(nvme_pci, sizeof(struct nvme_pci_softc),
    nvme_pci_match, nvme_pci_attach, nvme_pci_detach, NULL, nvme_pci_rescan,
    nvme_childdet, DVF_DETACH_SHUTDOWN);

static int	nvme_pci_intr_establish(struct nvme_softc *,
		    uint16_t, struct nvme_queue *);
static int	nvme_pci_intr_disestablish(struct nvme_softc *, uint16_t);
static int	nvme_pci_setup_intr(struct pci_attach_args *,
		    struct nvme_pci_softc *);

static int
nvme_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_MASS_STORAGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_NVM &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_NVM_NVME)
		return 1;

	return 0;
}

static void
nvme_pci_attach(device_t parent, device_t self, void *aux)
{
	struct nvme_pci_softc *psc = device_private(self);
	struct nvme_softc *sc = &psc->psc_nvme;
	struct pci_attach_args *pa = aux;
	pcireg_t memtype, reg;
	bus_addr_t memaddr;
	int flags, error;
#ifdef __HAVE_PCI_MSI_MSIX
	int msixoff;
#endif

	sc->sc_dev = self;
	psc->psc_pc = pa->pa_pc;
	if (pci_dma64_available(pa))
		sc->sc_dmat = pa->pa_dmat64;
	else
		sc->sc_dmat = pa->pa_dmat;

	pci_aprint_devinfo(pa, NULL);

	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	if ((reg & PCI_COMMAND_MASTER_ENABLE) == 0) {
		reg |= PCI_COMMAND_MASTER_ENABLE;
        	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, reg);
	}

	/* Map registers */
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, NVME_PCI_BAR);
	if (PCI_MAPREG_TYPE(memtype) != PCI_MAPREG_TYPE_MEM) {
		aprint_error_dev(self, "invalid type (type=0x%x)\n", memtype);
		return;
	}
	sc->sc_iot = pa->pa_memt;
	error = pci_mapreg_info(pa->pa_pc, pa->pa_tag, PCI_MAPREG_START,
	    memtype, &memaddr, &sc->sc_ios, &flags);
	if (error) {
		aprint_error_dev(self, "can't get map info\n");
		return;
	}

#ifdef __HAVE_PCI_MSI_MSIX
	if (pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_MSIX, &msixoff,
	    NULL)) {
		pcireg_t msixtbl;
		uint32_t table_offset;
		int bir;

		msixtbl = pci_conf_read(pa->pa_pc, pa->pa_tag,
		    msixoff + PCI_MSIX_TBLOFFSET);
		table_offset = msixtbl & PCI_MSIX_TBLOFFSET_MASK;
		bir = msixtbl & PCI_MSIX_PBABIR_MASK;
		if (bir == 0) {
			sc->sc_ios = table_offset;
		}
	}
#endif /* __HAVE_PCI_MSI_MSIX */

	error = bus_space_map(sc->sc_iot, memaddr, sc->sc_ios, flags,
	    &sc->sc_ioh);
	if (error != 0) {
		aprint_error_dev(self, "can't map mem space (error=%d)\n",
		    error);
		return;
	}

	/* Establish interrupts */
	if (nvme_pci_setup_intr(pa, psc) != 0) {
		aprint_error_dev(self, "unable to allocate interrupt\n");
		goto unmap;
	}
	sc->sc_intr_establish = nvme_pci_intr_establish;
	sc->sc_intr_disestablish = nvme_pci_intr_disestablish;

	sc->sc_ih = kmem_zalloc(sizeof(*sc->sc_ih) * psc->psc_nintrs, KM_SLEEP);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to allocate ih memory\n");
		goto intr_release;
	}

	if (sc->sc_use_mq) {
		sc->sc_softih = kmem_zalloc(
		    sizeof(*sc->sc_softih) * psc->psc_nintrs, KM_SLEEP);
		if (sc->sc_softih == NULL) {
			aprint_error_dev(self,
			    "unable to allocate softih memory\n");
			goto intr_free;
		}
	}

	if (nvme_attach(sc) != 0) {
		/* error printed by nvme_attach() */
		goto softintr_free;
	}

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	SET(sc->sc_flags, NVME_F_ATTACHED);
	return;

softintr_free:
	if (sc->sc_softih) {
		kmem_free(sc->sc_softih,
		    sizeof(*sc->sc_softih) * psc->psc_nintrs);
	}
intr_free:
	kmem_free(sc->sc_ih, sizeof(*sc->sc_ih) * psc->psc_nintrs);
	sc->sc_nq = 0;
intr_release:
	pci_intr_release(pa->pa_pc, psc->psc_intrs, psc->psc_nintrs);
	psc->psc_nintrs = 0;
unmap:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	sc->sc_ios = 0;
}

static int
nvme_pci_rescan(device_t self, const char *attr, const int *flags)
{

	return nvme_rescan(self, attr, flags);
}

static int
nvme_pci_detach(device_t self, int flags)
{
	struct nvme_pci_softc *psc = device_private(self);
	struct nvme_softc *sc = &psc->psc_nvme;
	int error;

	if (!ISSET(sc->sc_flags, NVME_F_ATTACHED))
		return 0;

	error = nvme_detach(sc, flags);
	if (error)
		return error;

	if (sc->sc_softih) {
		kmem_free(sc->sc_softih,
		    sizeof(*sc->sc_softih) * psc->psc_nintrs);
		sc->sc_softih = NULL;
	}
	kmem_free(sc->sc_ih, sizeof(*sc->sc_ih) * psc->psc_nintrs);
	pci_intr_release(psc->psc_pc, psc->psc_intrs, psc->psc_nintrs);
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	return 0;
}

static int
nvme_pci_intr_establish(struct nvme_softc *sc, uint16_t qid,
    struct nvme_queue *q)
{
	struct nvme_pci_softc *psc = (struct nvme_pci_softc *)sc;
	char intr_xname[INTRDEVNAMEBUF];
	char intrbuf[PCI_INTRSTR_LEN];
	const char *intrstr = NULL;
	int (*ih_func)(void *);
	void *ih_arg;
#ifdef __HAVE_PCI_MSI_MSIX
	int error;
#endif

	KASSERT(sc->sc_use_mq || qid == NVME_ADMIN_Q);
	KASSERT(sc->sc_ih[qid] == NULL);

	if (nvme_pci_mpsafe) {
		pci_intr_setattr(psc->psc_pc, &psc->psc_intrs[qid],
		    PCI_INTR_MPSAFE, true);
	}

#ifdef __HAVE_PCI_MSI_MSIX
	if (!sc->sc_use_mq) {
#endif
		snprintf(intr_xname, sizeof(intr_xname), "%s",
		    device_xname(sc->sc_dev));
		ih_arg = sc;
		ih_func = nvme_intr;
#ifdef __HAVE_PCI_MSI_MSIX
	}
	else {
		if (qid == NVME_ADMIN_Q) {
			snprintf(intr_xname, sizeof(intr_xname), "%s adminq",
			    device_xname(sc->sc_dev));
		} else {
			snprintf(intr_xname, sizeof(intr_xname), "%s ioq%d",
			    device_xname(sc->sc_dev), qid);
		}
		ih_arg = q;
		ih_func = nvme_intr_msi;
	}
#endif /* __HAVE_PCI_MSI_MSIX */

	/* establish hardware interrupt */
	sc->sc_ih[qid] = pci_intr_establish_xname(psc->psc_pc,
	    psc->psc_intrs[qid], IPL_BIO, ih_func, ih_arg, intr_xname);
	if (sc->sc_ih[qid] == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to establish %s interrupt\n", intr_xname);
		return 1;
	}

	/* if MSI, establish also the software interrupt */
	if (sc->sc_softih) {
		sc->sc_softih[qid] = softint_establish(
		    SOFTINT_BIO|(nvme_pci_mpsafe ? SOFTINT_MPSAFE : 0),
		    nvme_softintr_msi, q);
		if (sc->sc_softih[qid] == NULL) {
			pci_intr_disestablish(psc->psc_pc, sc->sc_ih[qid]);
			sc->sc_ih[qid] = NULL;

			aprint_error_dev(sc->sc_dev,
			    "unable to establish %s soft interrupt\n",
			    intr_xname);
			return 1;
		}
	}

	intrstr = pci_intr_string(psc->psc_pc, psc->psc_intrs[qid], intrbuf,
	    sizeof(intrbuf));
	if (!sc->sc_use_mq) {
		aprint_normal_dev(sc->sc_dev, "interrupting at %s\n", intrstr);
	}
#ifdef __HAVE_PCI_MSI_MSIX
	else if (qid == NVME_ADMIN_Q) {
		aprint_normal_dev(sc->sc_dev,
		    "for admin queue interrupting at %s\n", intrstr);
	} else if (!nvme_pci_mpsafe) {
		aprint_normal_dev(sc->sc_dev,
		    "for io queue %d interrupting at %s\n", qid, intrstr);
	} else {
		kcpuset_t *affinity;
		cpuid_t affinity_to;

		kcpuset_create(&affinity, true);
		affinity_to = (qid - 1) % ncpu;
		kcpuset_set(affinity, affinity_to);
		error = interrupt_distribute(sc->sc_ih[qid], affinity, NULL);
		kcpuset_destroy(affinity);
		aprint_normal_dev(sc->sc_dev,
		    "for io queue %d interrupting at %s", qid, intrstr);
		if (error == 0)
			aprint_normal(" affinity to cpu%lu", affinity_to);
		aprint_normal("\n");
	}
#endif
	return 0;
}

static int
nvme_pci_intr_disestablish(struct nvme_softc *sc, uint16_t qid)
{
	struct nvme_pci_softc *psc = (struct nvme_pci_softc *)sc;

	KASSERT(sc->sc_use_mq || qid == NVME_ADMIN_Q);
	KASSERT(sc->sc_ih[qid] != NULL);

	if (sc->sc_softih) {
		softint_disestablish(sc->sc_softih[qid]);
		sc->sc_softih[qid] = NULL;
	}

	pci_intr_disestablish(psc->psc_pc, sc->sc_ih[qid]);
	sc->sc_ih[qid] = NULL;

	return 0;
}

static int
nvme_pci_setup_intr(struct pci_attach_args *pa, struct nvme_pci_softc *psc)
{
	struct nvme_softc *sc = &psc->psc_nvme;
#ifdef __HAVE_PCI_MSI_MSIX
	int error;
	int counts[PCI_INTR_TYPE_SIZE], alloced_counts[PCI_INTR_TYPE_SIZE];
	pci_intr_handle_t *ihps;
	int max_type, intr_type;
#else
	pci_intr_handle_t ih;
#endif /* __HAVE_PCI_MSI_MSIX */

#ifdef __HAVE_PCI_MSI_MSIX
	if (nvme_pci_force_intx) {
		max_type = PCI_INTR_TYPE_INTX;
		goto force_intx;
	}

	/* MSI-X */
	max_type = PCI_INTR_TYPE_MSIX;
	counts[PCI_INTR_TYPE_MSIX] = min(pci_msix_count(pa->pa_pc, pa->pa_tag),
	    ncpu + 1);
	if (counts[PCI_INTR_TYPE_MSIX] > 0) {
		memset(alloced_counts, 0, sizeof(alloced_counts));
		alloced_counts[PCI_INTR_TYPE_MSIX] = counts[PCI_INTR_TYPE_MSIX];
		if (pci_intr_alloc(pa, &ihps, alloced_counts,
		    PCI_INTR_TYPE_MSIX)) {
			counts[PCI_INTR_TYPE_MSIX] = 0;
		} else {
			counts[PCI_INTR_TYPE_MSIX] =
			    alloced_counts[PCI_INTR_TYPE_MSIX];
			pci_intr_release(pa->pa_pc, ihps,
			    alloced_counts[PCI_INTR_TYPE_MSIX]);
		}
	}
	if (counts[PCI_INTR_TYPE_MSIX] < 2) {
		counts[PCI_INTR_TYPE_MSIX] = 0;
		max_type = PCI_INTR_TYPE_MSI;
	} else if (!nvme_pci_mq || !nvme_pci_mpsafe) {
		counts[PCI_INTR_TYPE_MSIX] = 2;	/* adminq + 1 ioq */
	}

retry_msi:
	/* MSI */
	counts[PCI_INTR_TYPE_MSI] = pci_msi_count(pa->pa_pc, pa->pa_tag);
	if (counts[PCI_INTR_TYPE_MSI] > 0) {
		while (counts[PCI_INTR_TYPE_MSI] > ncpu + 1) {
			if (counts[PCI_INTR_TYPE_MSI] / 2 <= ncpu + 1)
				break;
			counts[PCI_INTR_TYPE_MSI] /= 2;
		}
		memset(alloced_counts, 0, sizeof(alloced_counts));
		alloced_counts[PCI_INTR_TYPE_MSI] = counts[PCI_INTR_TYPE_MSI];
		if (pci_intr_alloc(pa, &ihps, alloced_counts,
		    PCI_INTR_TYPE_MSI)) {
			counts[PCI_INTR_TYPE_MSI] = 0;
		} else {
			counts[PCI_INTR_TYPE_MSI] =
			    alloced_counts[PCI_INTR_TYPE_MSI];
			pci_intr_release(pa->pa_pc, ihps,
			    alloced_counts[PCI_INTR_TYPE_MSI]);
		}
	}
	if (counts[PCI_INTR_TYPE_MSI] < 1) {
		counts[PCI_INTR_TYPE_MSI] = 0;
		if (max_type == PCI_INTR_TYPE_MSI)
			max_type = PCI_INTR_TYPE_INTX;
	} else if (!nvme_pci_mq || !nvme_pci_mpsafe) {
		if (counts[PCI_INTR_TYPE_MSI] > 2)
			counts[PCI_INTR_TYPE_MSI] = 2;	/* adminq + 1 ioq */
	}

force_intx:
	/* INTx */
	counts[PCI_INTR_TYPE_INTX] = 1;

	memcpy(alloced_counts, counts, sizeof(counts));
	error = pci_intr_alloc(pa, &ihps, alloced_counts, max_type);
	if (error) {
		if (max_type != PCI_INTR_TYPE_INTX) {
retry:
			memset(counts, 0, sizeof(counts));
			if (max_type == PCI_INTR_TYPE_MSIX) {
				max_type = PCI_INTR_TYPE_MSI;
				goto retry_msi;
			} else {
				max_type = PCI_INTR_TYPE_INTX;
				goto force_intx;
			}
		}
		return error;
	}

	intr_type = pci_intr_type(pa->pa_pc, ihps[0]);
	if (alloced_counts[intr_type] < counts[intr_type]) {
		if (intr_type != PCI_INTR_TYPE_INTX) {
			pci_intr_release(pa->pa_pc, ihps,
			    alloced_counts[intr_type]);
			max_type = intr_type;
			goto retry;
		}
		return EBUSY;
	}

	psc->psc_intrs = ihps;
	psc->psc_nintrs = alloced_counts[intr_type];
	if (intr_type == PCI_INTR_TYPE_MSI) {
		if (alloced_counts[intr_type] > ncpu + 1)
			alloced_counts[intr_type] = ncpu + 1;
	}
	sc->sc_use_mq = alloced_counts[intr_type] > 1;
	sc->sc_nq = sc->sc_use_mq ? alloced_counts[intr_type] - 1 : 1;

#else /* !__HAVE_PCI_MSI_MSIX */
        if (pci_intr_map(pa, &ih)) {
                aprint_error_dev(sc->sc_dev, "couldn't map interrupt\n");
                return EBUSY;
        }

	psc->psc_intrs = kmem_zalloc(sizeof(ih), KM_SLEEP);
	psc->psc_intrs[0] = ih;
	psc->psc_nintrs = 1;
	sc->sc_use_mq = 0;
	sc->sc_nq = 1;
#endif /* __HAVE_PCI_MSI_MSIX */

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, nvme, "pci,dk_subr");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
nvme_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	devmajor_t cmajor, bmajor;
	extern const struct cdevsw nvme_cdevsw;
#endif
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = config_init_component(cfdriver_ioconf_nvme_pci,
		    cfattach_ioconf_nvme_pci, cfdata_ioconf_nvme_pci);
		if (error)
			break;

		bmajor = cmajor = NODEVMAJOR;
		error = devsw_attach(nvme_cd.cd_name, NULL, &bmajor,
		    &nvme_cdevsw, &cmajor);
		if (error) {
			aprint_error("%s: unable to register devsw\n",
			    nvme_cd.cd_name);
			/* do not abort, just /dev/nvme* will not work */
		}
		break;
	case MODULE_CMD_FINI:
		devsw_detach(NULL, &nvme_cdevsw);

		error = config_fini_component(cfdriver_ioconf_nvme_pci,
		    cfattach_ioconf_nvme_pci, cfdata_ioconf_nvme_pci);
		break;
	default:
		break;
	}
#endif
	return error;
}
