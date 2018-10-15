/* $NetBSD: acpi_pci_machdep.c,v 1.1 2018/10/15 11:35:03 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_pci_machdep.c,v 1.1 2018/10/15 11:35:03 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/extent.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

#include <machine/cpu.h>

#include <arm/cpufunc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_mcfg.h>
#include <dev/acpi/acpi_pci.h>

#define	IH_INDEX_MASK			0x0000ffff
#define	IH_MPSAFE			0x80000000

struct acpi_pci_prt {
	u_int				prt_bus;
	ACPI_HANDLE			prt_handle;
	TAILQ_ENTRY(acpi_pci_prt)	prt_list;
};

static TAILQ_HEAD(, acpi_pci_prt) acpi_pci_irq_routes =
    TAILQ_HEAD_INITIALIZER(acpi_pci_irq_routes);

static void	acpi_pci_md_attach_hook(device_t, device_t,
				       struct pcibus_attach_args *);
static int	acpi_pci_md_bus_maxdevs(void *, int);
static pcitag_t	acpi_pci_md_make_tag(void *, int, int, int);
static void	acpi_pci_md_decompose_tag(void *, pcitag_t, int *, int *, int *);
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
					 int, int (*)(void *), void *);
static void	acpi_pci_md_intr_disestablish(void *, void *);

struct arm32_pci_chipset arm_acpi_pci_chipset = {
	.pc_attach_hook = acpi_pci_md_attach_hook,
	.pc_bus_maxdevs = acpi_pci_md_bus_maxdevs,
	.pc_make_tag = acpi_pci_md_make_tag,
	.pc_decompose_tag = acpi_pci_md_decompose_tag,
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
acpi_pci_md_pci_link(ACPI_HANDLE handle, int bus)
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
			acpi_pci_link_add_reference(linkdev, 0, bus, dev, prt->Pin & 3);
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
	struct acpi_pci_prt *prt, *prtp;
	struct acpi_devnode *ad;
	ACPI_HANDLE handle;
	int seg, bus, dev, func;

	seg = 0;	/* XXX segment */
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

	if (ad != NULL) {
		/*
		 * This is a new ACPI managed bus. Add PCI link references.
		 */
		acpi_pci_md_pci_link(ad->ad_handle, pba->pba_bus);
	}

	if (handle != NULL) {
		prt = kmem_alloc(sizeof(*prt), KM_SLEEP);
		prt->prt_bus = pba->pba_bus;
		prt->prt_handle = handle;
		TAILQ_INSERT_TAIL(&acpi_pci_irq_routes, prt, prt_list);
	}

	acpimcfg_map_bus(self, pba->pba_pc, pba->pba_bus);
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

static pcireg_t
acpi_pci_md_conf_read(void *v, pcitag_t tag, int offset)
{
	pcireg_t val;

	if (offset < 0 || offset >= PCI_EXTCONF_SIZE)
		return (pcireg_t) -1;

	acpimcfg_conf_read(&arm_acpi_pci_chipset, tag, offset, &val);

	return val;
}

static void
acpi_pci_md_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	if (offset < 0 || offset >= PCI_EXTCONF_SIZE)
		return;

	acpimcfg_conf_write(&arm_acpi_pci_chipset, tag, offset, val);
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
acpi_pci_md_intr_find_prt(u_int bus)
{
	struct acpi_pci_prt *prt, *prtp;

	prt = NULL;
	TAILQ_FOREACH(prtp, &acpi_pci_irq_routes, prt_list)
		if (prtp->prt_bus == bus) {
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

	prt = acpi_pci_md_intr_find_prt(pa->pa_bus);
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
				*ih = acpi_pci_link_route_interrupt(linkdev, tab->SourceIndex,
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
	snprintf(buf, len, "irq %d", (int)(ih & IH_INDEX_MASK));
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
			*ih |= IH_MPSAFE;
		else
			*ih &= ~IH_MPSAFE;
		return 0;
	default:
		return ENODEV;
	}
}

static void *
acpi_pci_md_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*callback)(void *), void *arg)
{
	const int irq = ih & IH_INDEX_MASK;
	const int mpsafe = (ih & IH_MPSAFE) ? IST_MPSAFE : 0;

	return intr_establish(irq, ipl, IST_LEVEL | mpsafe, callback, arg);
}

static void
acpi_pci_md_intr_disestablish(void *v, void *vih)
{
	intr_disestablish(vih);
}
