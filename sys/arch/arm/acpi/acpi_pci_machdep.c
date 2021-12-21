/* $NetBSD: acpi_pci_machdep.c,v 1.21 2021/12/21 11:02:38 skrll Exp $ */

/*-
 * Copyright (c) 2018, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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

#include "opt_pci.h"

#define	_INTR_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_pci_machdep.c,v 1.21 2021/12/21 11:02:38 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/cpu.h>

#include <arm/cpufunc.h>

#include <arm/pic/picvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_mcfg.h>
#include <dev/acpi/acpi_pci.h>

#include <arm/acpi/acpi_iort.h>
#include <arm/acpi/acpi_pci_machdep.h>

#ifdef PCI_SMCCC
#include <arm/pci/pci_smccc.h>
#endif

#include <arm/pci/pci_msi_machdep.h>

struct acpi_pci_prt {
	u_int				prt_segment;
	u_int				prt_bus;
	ACPI_HANDLE			prt_handle;
	TAILQ_ENTRY(acpi_pci_prt)	prt_list;
};

static TAILQ_HEAD(, acpi_pci_prt) acpi_pci_irq_routes =
    TAILQ_HEAD_INITIALIZER(acpi_pci_irq_routes);

struct acpi_pci_pct {
	struct acpi_pci_context		pct_ap;
	TAILQ_ENTRY(acpi_pci_pct)	pct_list;
};

static TAILQ_HEAD(, acpi_pci_pct) acpi_pci_chipset_tags =
    TAILQ_HEAD_INITIALIZER(acpi_pci_chipset_tags);

struct acpi_pci_intr {
	struct pic_softc		pi_pic;
	int				pi_irqbase;
	int				pi_irq;
	uint32_t			pi_unblocked;
	void				*pi_ih;
	TAILQ_ENTRY(acpi_pci_intr)	pi_list;
};

static TAILQ_HEAD(, acpi_pci_intr) acpi_pci_intrs =
    TAILQ_HEAD_INITIALIZER(acpi_pci_intrs);

static const struct acpi_pci_quirk acpi_pci_mcfg_quirks[] = {
	/* OEM ID	OEM Table ID	Revision	Seg	Func */
	{ "AMAZON",	"GRAVITON",	0,		-1,	acpi_pci_graviton_init },
	{ "ARMLTD",	"ARMN1SDP",	0x20181101,	0,	acpi_pci_n1sdp_init },
	{ "ARMLTD",	"ARMN1SDP",	0x20181101,	1,	acpi_pci_n1sdp_init },
	{ "NXP   ",     "LX2160  ",     0,              -1,	acpi_pci_layerscape_gen4_init },
};

#ifdef PCI_SMCCC
static const struct acpi_pci_quirk acpi_pci_smccc_quirk = {
	.q_segment = -1,
	.q_init = acpi_pci_smccc_init,
};
#endif

pci_chipset_tag_t acpi_pci_md_get_chipset_tag(struct acpi_softc *, int, int);

static void	acpi_pci_md_attach_hook(device_t, device_t,
				       struct pcibus_attach_args *);
static int	acpi_pci_md_bus_maxdevs(void *, int);
static pcitag_t	acpi_pci_md_make_tag(void *, int, int, int);
static void	acpi_pci_md_decompose_tag(void *, pcitag_t, int *, int *, int *);
static u_int	acpi_pci_md_get_segment(void *);
static uint32_t	acpi_pci_md_get_devid(void *, uint32_t);
static uint32_t	acpi_pci_md_get_frameid(void *, uint32_t);
static pcireg_t	acpi_pci_md_conf_read(void *, pcitag_t, int);
static void	acpi_pci_md_conf_write(void *, pcitag_t, int, pcireg_t);
static int	acpi_pci_md_conf_hook(void *, int, int, int, pcireg_t);
static void	acpi_pci_md_conf_interrupt(void *, int, int, int, int, int *);

static int	acpi_pci_md_intr_map(const struct pci_attach_args *,
				    pci_intr_handle_t *);
static const char *acpi_pci_md_intr_string(void *, pci_intr_handle_t,
					  char *, size_t);
static const struct evcnt *acpi_pci_md_intr_evcnt(void *, pci_intr_handle_t);
static int	acpi_pci_md_intr_setattr(void *, pci_intr_handle_t *, int,
					uint64_t);
static void *	acpi_pci_md_intr_establish(void *, pci_intr_handle_t,
					 int, int (*)(void *), void *,
					 const char *);
static void	acpi_pci_md_intr_disestablish(void *, void *);

struct arm32_pci_chipset arm_acpi_pci_chipset = {
	.pc_attach_hook = acpi_pci_md_attach_hook,
	.pc_bus_maxdevs = acpi_pci_md_bus_maxdevs,
	.pc_make_tag = acpi_pci_md_make_tag,
	.pc_decompose_tag = acpi_pci_md_decompose_tag,
	.pc_get_segment = acpi_pci_md_get_segment,
	.pc_get_devid = acpi_pci_md_get_devid,
	.pc_get_frameid = acpi_pci_md_get_frameid,
	.pc_conf_read = acpi_pci_md_conf_read,
	.pc_conf_write = acpi_pci_md_conf_write,
	.pc_conf_hook = acpi_pci_md_conf_hook,
	.pc_conf_interrupt = acpi_pci_md_conf_interrupt,

	.pc_intr_map = acpi_pci_md_intr_map,
	.pc_intr_string = acpi_pci_md_intr_string,
	.pc_intr_evcnt = acpi_pci_md_intr_evcnt,
	.pc_intr_setattr = acpi_pci_md_intr_setattr,
	.pc_intr_establish = acpi_pci_md_intr_establish,
	.pc_intr_disestablish = acpi_pci_md_intr_disestablish,
};

static ACPI_STATUS
acpi_pci_md_pci_link(ACPI_HANDLE handle, pci_chipset_tag_t pc, int bus)
{
	ACPI_PCI_ROUTING_TABLE *prt;
	ACPI_HANDLE linksrc;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	void *linkdev;

	rv = acpi_get(handle, &buf, AcpiGetIrqRoutingTable);
	if (ACPI_FAILURE(rv))
		return rv;

	for (char *p = buf.Pointer; ; p += prt->Length) {
		prt = (ACPI_PCI_ROUTING_TABLE *)p;
		if (prt->Length == 0)
			break;

		const u_int dev = ACPI_HIWORD(prt->Address);
		if (prt->Source[0] != 0) {
			aprint_debug("ACPI: %s dev %u INT%c on lnkdev %s\n",
			    acpi_name(handle), dev, 'A' + (prt->Pin & 3), prt->Source);
			rv = AcpiGetHandle(ACPI_ROOT_OBJECT, prt->Source, &linksrc);
			if (ACPI_FAILURE(rv)) {
				aprint_debug("ACPI: AcpiGetHandle failed for '%s': %s\n",
				    prt->Source, AcpiFormatException(rv));
				continue;
			}

			linkdev = acpi_pci_link_devbyhandle(linksrc);
			acpi_pci_link_add_reference(linkdev, pc, 0, bus, dev, prt->Pin & 3);
		} else {
			aprint_debug("ACPI: %s dev %u INT%c on globint %d\n",
			    acpi_name(handle), dev, 'A' + (prt->Pin & 3), prt->SourceIndex);
		}
	}

	return AE_OK;
}

static void
acpi_pci_md_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
	struct acpi_pci_context *ap = pba->pba_pc->pc_conf_v;
	struct acpi_pci_prt *prt, *prtp;
	struct acpi_devnode *ad;
	ACPI_HANDLE handle;
	int seg, bus, dev, func;

	seg = ap->ap_seg;
	handle = NULL;

	if (pba->pba_bridgetag) {
		/*
		 * Find the PCI address of our parent bridge and look for the
		 * corresponding ACPI device node. If there is no node for this
		 * bus, use the parent bridge routing information.
		 */
		acpi_pci_md_decompose_tag(NULL, *pba->pba_bridgetag, &bus, &dev, &func);
		ad = acpi_pcidev_find(seg, bus, dev, func);
		if (ad != NULL) {
			handle = ad->ad_handle;
		} else {
			/* No routes defined for this bus, copy from parent */
			TAILQ_FOREACH(prtp, &acpi_pci_irq_routes, prt_list)
				if (prtp->prt_bus == bus) {
					handle = prtp->prt_handle;
					break;
				}
		}
	} else {
		/*
		 * Lookup the ACPI device node for the root bus.
		 */
		ad = acpi_pciroot_find(seg, 0);
		if (ad != NULL)
			handle = ad->ad_handle;
	}

	if (handle != NULL) {
		prt = kmem_alloc(sizeof(*prt), KM_SLEEP);
		prt->prt_bus = pba->pba_bus;
		prt->prt_segment = ap->ap_seg;
		prt->prt_handle = handle;
		TAILQ_INSERT_TAIL(&acpi_pci_irq_routes, prt, prt_list);
	}

	acpimcfg_map_bus(self, pba->pba_pc, pba->pba_bus);

	if (ad != NULL) {
		/*
		 * This is a new ACPI managed bus. Add PCI link references.
		 */
		acpi_pci_md_pci_link(ad->ad_handle, pba->pba_pc, pba->pba_bus);
	}
}

static int
acpi_pci_md_bus_maxdevs(void *v, int busno)
{
	return 32;
}

static pcitag_t
acpi_pci_md_make_tag(void *v, int b, int d, int f)
{
	return (b << 16) | (d << 11) | (f << 8);
}

static void
acpi_pci_md_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp)
		*bp = (tag >> 16) & 0xff;
	if (dp)
		*dp = (tag >> 11) & 0x1f;
	if (fp)
		*fp = (tag >> 8) & 0x7;
}

static u_int
acpi_pci_md_get_segment(void *v)
{
	struct acpi_pci_context * const ap = v;

	return ap->ap_seg;
}

static uint32_t
acpi_pci_md_get_devid(void *v, uint32_t devid)
{
	struct acpi_pci_context * const ap = v;

	return acpi_iort_pci_root_map(ap->ap_seg, devid);
}

static uint32_t
acpi_pci_md_get_frameid(void *v, uint32_t devid)
{
	struct acpi_pci_context * const ap = v;

	return acpi_iort_its_id_map(ap->ap_seg, devid);
}

static pcireg_t
acpi_pci_md_conf_read(void *v, pcitag_t tag, int offset)
{
	struct acpi_pci_context * const ap = v;
	pcireg_t val;

	if (offset < 0 || offset >= PCI_EXTCONF_SIZE)
		return (pcireg_t) -1;

	if (ap->ap_conf_read != NULL)
		ap->ap_conf_read(&ap->ap_pc, tag, offset, &val);
	else
		acpimcfg_conf_read(&ap->ap_pc, tag, offset, &val);

	return val;
}

static void
acpi_pci_md_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct acpi_pci_context * const ap = v;

	if (offset < 0 || offset >= PCI_EXTCONF_SIZE)
		return;

	if (ap->ap_conf_write != NULL)
		ap->ap_conf_write(&ap->ap_pc, tag, offset, val);
	else
		acpimcfg_conf_write(&ap->ap_pc, tag, offset, val);
}

static int
acpi_pci_md_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{
	return PCI_CONF_DEFAULT;
}

static void
acpi_pci_md_conf_interrupt(void *v, int bus, int dev, int ipin, int sqiz, int *ilinep)
{
}

static struct acpi_pci_prt *
acpi_pci_md_intr_find_prt(pci_chipset_tag_t pc, u_int bus)
{
	struct acpi_pci_prt *prt, *prtp;
	u_int segment;

	segment = pci_get_segment(pc);

	prt = NULL;
	TAILQ_FOREACH(prtp, &acpi_pci_irq_routes, prt_list)
		if (prtp->prt_segment == segment && prtp->prt_bus == bus) {
			prt = prtp;
			break;
		}

	return prt;
}

static int
acpi_pci_md_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ih)
{
	struct acpi_pci_prt *prt;
	ACPI_PCI_ROUTING_TABLE *tab;
	int line, pol, trig, error;
	ACPI_HANDLE linksrc;
	ACPI_BUFFER buf;
	void *linkdev;

	if (pa->pa_intrpin == PCI_INTERRUPT_PIN_NONE)
		return EINVAL;

	prt = acpi_pci_md_intr_find_prt(pa->pa_pc, pa->pa_bus);
	if (prt == NULL)
		return ENXIO;

	if (ACPI_FAILURE(acpi_get(prt->prt_handle, &buf, AcpiGetIrqRoutingTable)))
		return EIO;

	error = ENOENT;
	for (char *p = buf.Pointer; ; p += tab->Length) {
		tab = (ACPI_PCI_ROUTING_TABLE *)p;
		if (tab->Length == 0)
			break;

		if (pa->pa_device == ACPI_HIWORD(tab->Address) &&
		    (pa->pa_intrpin - 1) == (tab->Pin & 3)) {
			if (tab->Source[0] != 0) {
				if (ACPI_FAILURE(AcpiGetHandle(ACPI_ROOT_OBJECT, tab->Source, &linksrc)))
					goto done;
				linkdev = acpi_pci_link_devbyhandle(linksrc);
				*ih = acpi_pci_link_route_interrupt(linkdev,
				    pa->pa_pc, tab->SourceIndex,
				    &line, &pol, &trig);
				error = 0;
				goto done;
			} else {
				*ih = tab->SourceIndex;
				error = 0;
				goto done;
			}
		}
	}

done:
	ACPI_FREE(buf.Pointer);
	return error;
}

static const char *
acpi_pci_md_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	const int irq = __SHIFTOUT(ih, ARM_PCI_INTR_IRQ);
	const int vec = __SHIFTOUT(ih, ARM_PCI_INTR_MSI_VEC);

	if (ih & ARM_PCI_INTR_MSIX)
		snprintf(buf, len, "irq %d (MSI-X vec %d)", irq, vec);
	else if (ih & ARM_PCI_INTR_MSI)
		snprintf(buf, len, "irq %d (MSI vec %d)", irq, vec);
	else
		snprintf(buf, len, "irq %d", irq);

	return buf;
}

static const struct evcnt *
acpi_pci_md_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return NULL;
}

static int
acpi_pci_md_intr_setattr(void *v, pci_intr_handle_t *ih, int attr, uint64_t data)
{
	switch (attr) {
	case PCI_INTR_MPSAFE:
		if (data)
			*ih |= ARM_PCI_INTR_MPSAFE;
		else
			*ih &= ~ARM_PCI_INTR_MPSAFE;
		return 0;
	default:
		return ENODEV;
	}
}

static struct acpi_pci_intr *
acpi_pci_md_intr_lookup(int irq)
{
	struct acpi_pci_intr *pi;

	TAILQ_FOREACH(pi, &acpi_pci_intrs, pi_list)
		if (pi->pi_irq == irq)
			return pi;

	return NULL;
}

static void
acpi_pci_md_unblock_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irqmask)
{
	struct acpi_pci_intr * const pi = (struct acpi_pci_intr *)pic;

	pi->pi_unblocked |= irqmask;
}

static void
acpi_pci_md_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irqmask)
{
	struct acpi_pci_intr * const pi = (struct acpi_pci_intr *)pic;

	pi->pi_unblocked &= ~irqmask;
}

static int
acpi_pci_md_find_pending_irqs(struct pic_softc *pic)
{
	struct acpi_pci_intr * const pi = (struct acpi_pci_intr *)pic;

	pic_mark_pending_sources(pic, 0, pi->pi_unblocked);

	return 1;
}

static void
acpi_pci_md_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
}

static void
acpi_pci_md_source_name(struct pic_softc *pic, int irq, char *buf, size_t len)
{
	snprintf(buf, len, "slot %d", irq);
}

static struct pic_ops acpi_pci_pic_ops = {
	.pic_unblock_irqs = acpi_pci_md_unblock_irqs,
	.pic_block_irqs = acpi_pci_md_block_irqs,
	.pic_find_pending_irqs = acpi_pci_md_find_pending_irqs,
	.pic_establish_irq = acpi_pci_md_establish_irq,
	.pic_source_name = acpi_pci_md_source_name,
};

static void *
acpi_pci_md_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*callback)(void *), void *arg, const char *xname)
{
	struct acpi_pci_context * const ap = v;
	struct acpi_pci_intr *pi;
	int slot;

	if ((ih & (ARM_PCI_INTR_MSI | ARM_PCI_INTR_MSIX)) != 0)
		return arm_pci_msi_intr_establish(&ap->ap_pc, ih, ipl, callback, arg, xname);

	const int irq = (int)__SHIFTOUT(ih, ARM_PCI_INTR_IRQ);
	const int mpsafe = (ih & ARM_PCI_INTR_MPSAFE) ? IST_MPSAFE : 0;

	pi = acpi_pci_md_intr_lookup(irq);
	if (pi == NULL) {
		pi = kmem_zalloc(sizeof(*pi), KM_SLEEP);
		pi->pi_irq = irq;
		snprintf(pi->pi_pic.pic_name, sizeof(pi->pi_pic.pic_name),
		    "PCI irq %d", irq);
		pi->pi_pic.pic_maxsources = 32;
		pi->pi_pic.pic_ops = &acpi_pci_pic_ops;
		pi->pi_irqbase = pic_add(&pi->pi_pic, PIC_IRQBASE_ALLOC);
		TAILQ_INSERT_TAIL(&acpi_pci_intrs, pi, pi_list);
		pi->pi_ih = intr_establish_xname(irq, IPL_VM, IST_LEVEL | IST_MPSAFE,
		    pic_handle_intr, &pi->pi_pic, device_xname(ap->ap_dev));
	}
	if (pi->pi_ih == NULL)
		return NULL;

	/* Find a free slot */
	for (slot = 0; slot < pi->pi_pic.pic_maxsources; slot++)
		if (pi->pi_pic.pic_sources[slot] == NULL)
			break;
	if (slot == pi->pi_pic.pic_maxsources)
		return NULL;

	return intr_establish_xname(pi->pi_irqbase + slot, ipl, IST_LEVEL | mpsafe,
	    callback, arg, xname);
}

static void
acpi_pci_md_intr_disestablish(void *v, void *vih)
{
	intr_disestablish(vih);
}

const struct acpi_pci_quirk *
acpi_pci_md_find_quirk(int seg)
{
	ACPI_STATUS rv;
	ACPI_TABLE_MCFG *mcfg;
	u_int n;

	rv = AcpiGetTable(ACPI_SIG_MCFG, 0, (ACPI_TABLE_HEADER **)&mcfg);
	if (ACPI_FAILURE(rv)) {
#ifdef PCI_SMCCC
		uint32_t ver = pci_smccc_version();
		aprint_debug("%s: SMCCC version %#x\n", __func__, ver);
		if (PCI_SMCCC_SUCCESS(ver)) {
			return &acpi_pci_smccc_quirk;
		}
#endif
		return NULL;
	}

	for (n = 0; n < __arraycount(acpi_pci_mcfg_quirks); n++) {
		const struct acpi_pci_quirk *q = &acpi_pci_mcfg_quirks[n];
		if (memcmp(q->q_oemid, mcfg->Header.OemId, ACPI_OEM_ID_SIZE) == 0 &&
		    memcmp(q->q_oemtableid, mcfg->Header.OemTableId, ACPI_OEM_TABLE_ID_SIZE) == 0 &&
		    q->q_oemrevision == mcfg->Header.OemRevision &&
		    (q->q_segment == -1 || q->q_segment == seg))
			return q;
	}

	return NULL;
}

pci_chipset_tag_t
acpi_pci_md_get_chipset_tag(struct acpi_softc *sc, int seg, int bbn)
{
	struct acpi_pci_pct *pct = NULL, *pctp;
	const struct acpi_pci_quirk *q;

	TAILQ_FOREACH(pctp, &acpi_pci_chipset_tags, pct_list)
		if (pctp->pct_ap.ap_seg == seg) {
			pct = pctp;
			break;
		}

	if (pct == NULL) {
		pct = kmem_zalloc(sizeof(*pct), KM_SLEEP);
		pct->pct_ap.ap_dev = sc->sc_dev;
		pct->pct_ap.ap_pc = arm_acpi_pci_chipset;
		pct->pct_ap.ap_pc.pc_conf_v = &pct->pct_ap;
		pct->pct_ap.ap_pc.pc_intr_v = &pct->pct_ap;
		pct->pct_ap.ap_seg = seg;
		pct->pct_ap.ap_bus = bbn;
		pct->pct_ap.ap_maxbus = -1;
		pct->pct_ap.ap_bst = acpi_softc->sc_memt;

		q = acpi_pci_md_find_quirk(seg);
		if (q != NULL)
			q->q_init(&pct->pct_ap);

		TAILQ_INSERT_TAIL(&acpi_pci_chipset_tags, pct, pct_list);
	}

	return &pct->pct_ap.ap_pc;
}
__strong_alias(acpi_get_pci_chipset_tag,acpi_pci_md_get_chipset_tag);
