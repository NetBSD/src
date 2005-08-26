/*	$NetBSD: mpacpi.c,v 1.34 2005/08/26 13:19:38 drochner Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpacpi.c,v 1.34 2005/08/26 13:19:38 drochner Exp $");

#include "opt_acpi.h"
#include "opt_mpbios.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <machine/specialreg.h>
#include <machine/cpuvar.h>
#include <machine/bus.h>
#include <machine/mpacpi.h>
#include <machine/mpbiosvar.h>

#include <machine/i82093reg.h>
#include <machine/i82093var.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#include <dev/isa/isareg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/ppbreg.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_madt.h>

#include "pci.h"

#ifdef ACPI_DEBUG_OUTPUT
#define _COMPONENT ACPI_HARDWARE
#define _THIS_MODULE "mpacpi"
#endif

/* XXX room for PCI-to-PCI bus */
#define BUS_BUFFER (16)

#if NPCI > 0
struct mpacpi_pcibus {
	TAILQ_ENTRY(mpacpi_pcibus) mpr_list;
	ACPI_HANDLE mpr_handle;		/* Same thing really, but.. */
	ACPI_BUFFER mpr_buf;		/* preserve _PRT */
	int mpr_bus;			/* PCI bus number */
};

static TAILQ_HEAD(, mpacpi_pcibus) mpacpi_pcibusses;

#endif

static int mpacpi_print(void *, const char *);
static int mpacpi_submatch(struct device *, struct cfdata *,
	const int *, void *);

/* acpi_madt_walk callbacks */
static ACPI_STATUS mpacpi_count(APIC_HEADER *, void *);
static ACPI_STATUS mpacpi_config_cpu(APIC_HEADER *, void *);
static ACPI_STATUS mpacpi_config_ioapic(APIC_HEADER *, void *);
static ACPI_STATUS mpacpi_nonpci_intr(APIC_HEADER *, void *);

#if NPCI > 0
/* Callbacks for the ACPI namespace walk */
static ACPI_STATUS mpacpi_pcibus_cb(ACPI_HANDLE, UINT32, void *, void **);
static int mpacpi_derive_bus(ACPI_HANDLE, struct acpi_softc *);

static int mpacpi_pcircount(struct mpacpi_pcibus *);
static int mpacpi_pciroute(struct mpacpi_pcibus *);
static int mpacpi_find_pcibusses(struct acpi_softc *);

static void mpacpi_print_pci_intr(int);
#endif

static void mpacpi_config_irouting(struct acpi_softc *);

static void mpacpi_print_intr(struct mp_intr_map *);
static void mpacpi_print_isa_intr(int);

int mpacpi_nioapic;			/* number of ioapics */
int mpacpi_ncpu;			/* number of cpus */
int mpacpi_nintsrc;			/* number of non-device interrupts */

#if NPCI > 0
static int mpacpi_npci;
static int mpacpi_maxpci;
static int mpacpi_npciroots;
#endif

static int mpacpi_intr_index;
static paddr_t mpacpi_lapic_base = LAPIC_BASE;

static int
mpacpi_print(void *aux, const char *pnp)
{
	struct cpu_attach_args * caa = (struct cpu_attach_args *) aux;
	if (pnp)
		printf("%s at %s:",caa->caa_name, pnp);
	return (UNCONF);
}

static int
mpacpi_submatch(struct device *parent, struct cfdata *cf,
	const int *ldesc, void *aux)
{
	struct cpu_attach_args * caa = (struct cpu_attach_args *) aux;
	if (strcmp(caa->caa_name, cf->cf_name))
		return 0;

	return (config_match(parent, cf, aux));
}

/*
 * Handle special interrupt sources and overrides from the MADT.
 * This is a callback function for acpi_madt_walk().
 */
static ACPI_STATUS
mpacpi_nonpci_intr(APIC_HEADER *hdrp, void *aux)
{
	int *index = aux, pin, lindex;
	struct mp_intr_map *mpi;
	MADT_NMI_SOURCE *ioapic_nmi;
	MADT_LOCAL_APIC_NMI *lapic_nmi;
	MADT_INTERRUPT_OVERRIDE *isa_ovr;
	struct ioapic_softc *ioapic;

	switch (hdrp->Type) {
	case APIC_NMI:
		ioapic_nmi = (MADT_NMI_SOURCE *)hdrp;
		ioapic = ioapic_find_bybase(ioapic_nmi->Interrupt);
		if (ioapic == NULL)
			break;
		mpi = &mp_intrs[*index];
		(*index)++;
		mpi->next = NULL;
		mpi->bus = NULL;
		mpi->type = MPS_INTTYPE_NMI;
		mpi->ioapic = ioapic;
		pin = ioapic_nmi->Interrupt - ioapic->sc_apic_vecbase;
		mpi->ioapic_pin = pin;
		mpi->bus_pin = -1;
		mpi->redir = (IOAPIC_REDLO_DEL_NMI<<IOAPIC_REDLO_DEL_SHIFT);
		ioapic->sc_pins[pin].ip_map = mpi;
		mpi->ioapic_ih = APIC_INT_VIA_APIC |
		    (ioapic->sc_apicid << APIC_INT_APIC_SHIFT) |
		    (pin << APIC_INT_PIN_SHIFT);
		mpi->flags = ioapic_nmi->Polarity |
		    (ioapic_nmi->TriggerMode << 2);
		mpi->global_int = ioapic_nmi->Interrupt;
		break;
	case APIC_LOCAL_NMI:
		lapic_nmi = (MADT_LOCAL_APIC_NMI *)hdrp;
		mpi = &mp_intrs[*index];
		(*index)++;
		mpi->next = NULL;
		mpi->bus = NULL;
		mpi->ioapic = NULL;
		mpi->type = MPS_INTTYPE_NMI;
		mpi->ioapic_pin = lapic_nmi->Lint;
		mpi->cpu_id = lapic_nmi->ProcessorId;
		mpi->redir = (IOAPIC_REDLO_DEL_NMI<<IOAPIC_REDLO_DEL_SHIFT);
		mpi->global_int = -1;
		break;
	case APIC_XRUPT_OVERRIDE:
		isa_ovr = (MADT_INTERRUPT_OVERRIDE *)hdrp;
		if (isa_ovr->Source > 15 || isa_ovr->Source == 2)
			break;
		ioapic = ioapic_find_bybase(isa_ovr->Interrupt);
		if (ioapic == NULL)
			break;
		pin = isa_ovr->Interrupt - ioapic->sc_apic_vecbase;
		lindex = isa_ovr->Source;
		/*
		 * IRQ 2 was skipped in the default setup.
		 */
		if (lindex > 2)
			lindex--;
		mpi = &mp_intrs[lindex];
		mpi->ioapic_ih = APIC_INT_VIA_APIC |
		    (ioapic->sc_apicid << APIC_INT_APIC_SHIFT) |
		    (pin << APIC_INT_PIN_SHIFT);
		mpi->bus_pin = isa_ovr->Source;
		mpi->ioapic_pin = pin;
		mpi->sflags |= MPI_OVR;
		mpi->redir = 0;
		switch (isa_ovr->Polarity) {
		case MPS_INTPO_ACTHI:
			mpi->redir &= ~IOAPIC_REDLO_ACTLO;
			break;
		case MPS_INTPO_DEF:
		case MPS_INTPO_ACTLO:
			mpi->redir |= IOAPIC_REDLO_ACTLO;
			break;
		}
		mpi->redir |= (IOAPIC_REDLO_DEL_FIXED<<IOAPIC_REDLO_DEL_SHIFT);
		switch (isa_ovr->TriggerMode) {
		case MPS_INTTR_DEF:
		case MPS_INTTR_LEVEL:
			mpi->redir |= IOAPIC_REDLO_LEVEL;
			break;
		case MPS_INTTR_EDGE:
			mpi->redir &= ~IOAPIC_REDLO_LEVEL;
			break;
		}
		mpi->flags = isa_ovr->Polarity | (isa_ovr->TriggerMode << 2);
		ioapic->sc_pins[pin].ip_map = mpi;
	default:
		break;
	}
	return AE_OK;
}

/*
 * Count various MP resources present in the MADT.
 * This is a callback function for acpi_madt_walk().
 */
static ACPI_STATUS
mpacpi_count(APIC_HEADER *hdrp, void *aux)
{
	MADT_ADDRESS_OVERRIDE *lop;

	switch (hdrp->Type) {
	case APIC_PROCESSOR:
		mpacpi_ncpu++;
		break;
	case APIC_IO:
		mpacpi_nioapic++;
		break;
	case APIC_NMI:
	case APIC_LOCAL_NMI:
		mpacpi_nintsrc++;
		break;
	case APIC_ADDRESS_OVERRIDE:
		lop = (MADT_ADDRESS_OVERRIDE *)hdrp;
		mpacpi_lapic_base = lop->Address;;
	default:
		break;
	}
	return AE_OK;
}

static ACPI_STATUS
mpacpi_config_cpu(APIC_HEADER *hdrp, void *aux)
{
	struct device *parent = aux;
	MADT_PROCESSOR_APIC *p;
	struct cpu_attach_args caa;

	if (hdrp->Type == APIC_PROCESSOR) {
		p = (MADT_PROCESSOR_APIC *)hdrp;
		if (p->ProcessorEnabled) {
			if (p->LocalApicId == lapic_cpu_number())
				caa.cpu_role = CPU_ROLE_BP;
			else
				caa.cpu_role = CPU_ROLE_AP;
			caa.caa_name = "cpu";
			caa.cpu_number = p->LocalApicId;
			caa.cpu_func = &mp_cpu_funcs;
			config_found_sm_loc(parent, "cpubus", NULL,
				&caa, mpacpi_print, mpacpi_submatch);
		}
	}
	return AE_OK;
}

static ACPI_STATUS
mpacpi_config_ioapic(APIC_HEADER *hdrp, void *aux)
{
	struct device *parent = aux;
	struct apic_attach_args aaa;
	MADT_IO_APIC *p;

	if (hdrp->Type == APIC_IO) {
		p = (MADT_IO_APIC *)hdrp;
		aaa.aaa_name = "ioapic";
		aaa.apic_id = p->IoApicId;
		aaa.apic_address = p->Address;
		aaa.apic_version = -1;
		aaa.flags = IOAPIC_VWIRE;
		aaa.apic_vecbase = p->Interrupt;
		config_found_sm_loc(parent, "cpubus", NULL, &aaa,
			mpacpi_print, mpacpi_submatch);
	}
	return AE_OK;
}

int
mpacpi_scan_apics(struct device *self)
{
	int rv = 0;

	if (acpi_madt_map() != AE_OK)
		return 0;

	mpacpi_ncpu = mpacpi_nintsrc = mpacpi_nioapic = 0;
	acpi_madt_walk(mpacpi_count, self);

	lapic_boot_init(mpacpi_lapic_base);

	acpi_madt_walk(mpacpi_config_cpu, self);

	if (mpacpi_ncpu == 0)
		goto done;

	acpi_madt_walk(mpacpi_config_ioapic, self);

#if NPCI > 0
	/*
	 * If PCI routing tables can't be built we report failure
	 * and let MPBIOS do the work.
	 */
	if ((acpi_find_quirks() & (ACPI_QUIRK_BADPCI | ACPI_QUIRK_BADIRQ)) != 0)
		goto done;
#endif
	rv = 1;
done:
	acpi_madt_unmap();
	return rv;
}

#if NPCI > 0

/*
 * Find all PCI busses from ACPI namespace and construct mpacpi_pcibusses list.
 *
 * Note:
 * We cannot find all PCI busses in the system from ACPI namespace.
 * For example, a PCI-to-PCI bridge on an add-on PCI card is not
 * described in the ACPI namespace.
 * We search valid devices which have _PRT (PCI interrupt routing table)
 * method.
 * Such devices are either one of PCI root bridge or PCI-to-PCI bridge.
 */
static int
mpacpi_find_pcibusses(struct acpi_softc *acpi)
{
	ACPI_HANDLE sbhandle;

	if (AcpiGetHandle(ACPI_ROOT_OBJECT, "\\_SB_", &sbhandle) != AE_OK)
		return ENOENT;
	TAILQ_INIT(&mpacpi_pcibusses);
	AcpiWalkNamespace(ACPI_TYPE_DEVICE, sbhandle, 100,
		    mpacpi_pcibus_cb, acpi, NULL);
	return 0;
}

static const char * const pciroot_hid[] = {
	"PNP0A03",			/* PCI root bridge */
	NULL
};

/*
 * mpacpi_derive_bus:
 *
 * Derive PCI bus number for the ACPI handle.
 *
 * If a device is not a PCI root bridge, it doesn't have _BBN method
 * and we have no direct method to know the bus number.
 * We have to walk up to search its root bridge and then walk down
 * to resolve the bus number.
 */
static int
mpacpi_derive_bus(ACPI_HANDLE handle, struct acpi_softc *acpi)
{
	ACPI_HANDLE parent, current;
	ACPI_STATUS rv;
	ACPI_INTEGER val;
	ACPI_DEVICE_INFO *devinfo;
	ACPI_BUFFER buf;
	struct ac_dev {
		TAILQ_ENTRY(ac_dev) list;
		ACPI_HANDLE handle;
	};
	TAILQ_HEAD(, ac_dev) dev_list;
	struct ac_dev *dev;
	pcireg_t binf, class, dvid;
	pcitag_t tag;
	int bus;

	bus = -1;
	TAILQ_INIT(&dev_list);

	/* first, search parent root bus */
	for (current = handle;; current = parent) {
		dev = malloc(sizeof(struct ac_dev), M_TEMP, M_WAITOK|M_ZERO);
		if (dev == NULL)
			return -1;
		dev->handle = current;
		TAILQ_INSERT_HEAD(&dev_list, dev, list);

		rv = AcpiGetParent(current, &parent);
		if (ACPI_FAILURE(rv))
			return -1;

		buf.Pointer = NULL;
		buf.Length = ACPI_ALLOCATE_BUFFER;
		rv = AcpiGetObjectInfo(parent, &buf);
		if (ACPI_FAILURE(rv))
			return -1;

		devinfo = buf.Pointer;
		if (acpi_match_hid(devinfo, pciroot_hid)) {
			rv = acpi_eval_integer(parent, METHOD_NAME__BBN, &val);
			AcpiOsFree(buf.Pointer);
			if (ACPI_SUCCESS(rv))
				bus = ACPI_LOWORD(val);
			else
				/* assume bus = 0 */
				bus = 0;
			break;
		}

		AcpiOsFree(buf.Pointer);
	}

	/*
	 * second, we walk down from the root to the target
	 * resolving the bus number
	 */
	TAILQ_FOREACH(dev, &dev_list, list) {
		rv = acpi_eval_integer(dev->handle, METHOD_NAME__ADR, &val);
		if (ACPI_FAILURE(rv))
			return -1;

		tag = pci_make_tag(acpi->sc_pc, bus,
		    ACPI_HIWORD(val), ACPI_LOWORD(val));

		/* check if this device exists */
		dvid = pci_conf_read(acpi->sc_pc, tag, PCI_ID_REG);
		if (PCI_VENDOR(dvid) == PCI_VENDOR_INVALID ||
		    PCI_VENDOR(dvid) == 0)
			return -1;

		/* check if this is a bridge device */
		class = pci_conf_read(acpi->sc_pc, tag, PCI_CLASS_REG);
		if (PCI_CLASS(class) != PCI_CLASS_BRIDGE ||
		    PCI_SUBCLASS(class) != PCI_SUBCLASS_BRIDGE_PCI)
			return -1;

		/* if this is a bridge, get secondary bus */
		binf = pci_conf_read(acpi->sc_pc, tag, PPB_REG_BUSINFO);
		bus = PPB_BUSINFO_SECONDARY(binf);
	}

	/* cleanup */
	while (!TAILQ_EMPTY(&dev_list)) {
		dev = TAILQ_FIRST(&dev_list);
		TAILQ_REMOVE(&dev_list, dev, list);
		free(dev, M_TEMP);
	}

	return bus;
}

/*
 * Callback function for a namespace walk through ACPI space, finding all
 * PCI root and subordinate busses.
 */
static ACPI_STATUS
mpacpi_pcibus_cb(ACPI_HANDLE handle, UINT32 level, void *p, void **status)
{
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	ACPI_INTEGER val;
	ACPI_DEVICE_INFO *devinfo;
	struct mpacpi_pcibus *mpr;
	struct acpi_softc *acpi = p;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_BUFFER;

	/* get _HID, _CID and _STA */
	rv = AcpiGetObjectInfo(handle, &buf);
	if (ACPI_FAILURE(rv))
		return AE_OK;

	devinfo = buf.Pointer;

#define ACPI_STA_OK (ACPI_STA_DEV_PRESENT|ACPI_STA_DEV_ENABLED|ACPI_STA_DEV_OK)

	/* if this device is not active, ignore it */
	if ((devinfo->Valid & ACPI_VALID_STA) &&
	    (devinfo->CurrentStatus & ACPI_STA_OK) != ACPI_STA_OK)
		goto out;

	mpr = malloc(sizeof (struct mpacpi_pcibus), M_TEMP, M_WAITOK|M_ZERO);
	if (mpr == NULL) {
		AcpiOsFree(buf.Pointer);
		return AE_NO_MEMORY;
	}

	/* try get _PRT. if this fails, we're not interested in it */
	rv = acpi_get(handle, &mpr->mpr_buf, AcpiGetIrqRoutingTable);
	if (ACPI_FAILURE(rv)) {
		free(mpr, M_TEMP);
		goto out;
	}

	/* check whether this is PCI root bridge or not */
	if (acpi_match_hid(devinfo, pciroot_hid)) {
		/* this is PCI root bridge */

		/* get the bus number */
		rv = acpi_eval_integer(handle, METHOD_NAME__BBN, &val);
		if (ACPI_SUCCESS(rv)) {
			mpr->mpr_bus = ACPI_LOWORD(val);
		} else {
			/*
			 * This often happens on systems which have only
			 * one PCI root bus, assuming 0 will be ok.
			 *
			 * If there is a system that have multiple PCI
			 * root but doesn't describe _BBN for every root,
			 * the ASL is *broken*.
			 */
			if (mpacpi_npciroots != 0)
				panic("mpacpi: ASL is broken");

			printf("mpacpi: could not get bus number, "
				    "assuming bus 0\n");
			mpr->mpr_bus = 0;
		}
		if (mp_verbose)
			printf("mpacpi: found root PCI bus %d at level %u\n",
			    mpr->mpr_bus, level);
		mpacpi_npciroots++;
	} else {
		/* this is subordinate PCI bus (behind PCI-to-PCI bridge) */

		/* we have no direct method to get the bus number... */
		mpr->mpr_bus = mpacpi_derive_bus(handle, acpi);

		if (mpr->mpr_bus < 0) {
			if (mp_verbose)
				printf("mpacpi: failed to derive bus number, ignoring\n");
			free(mpr, M_TEMP);
			goto out;
		}
		if (mp_verbose)
			printf("mpacpi: found subordinate bus %d at level %u\n",
			    mpr->mpr_bus, level);
	}

	mpr->mpr_handle = handle;
	TAILQ_INSERT_TAIL(&mpacpi_pcibusses, mpr, mpr_list);

	if (mpr->mpr_bus > mpacpi_maxpci)
		mpacpi_maxpci = mpr->mpr_bus;

	mpacpi_npci++;

 out:
	AcpiOsFree(buf.Pointer);
	return AE_OK;
}

/*
 * Find all static PRT entries for a PCI bus.
 */
static int
mpacpi_pciroute(struct mpacpi_pcibus *mpr)
{
	ACPI_PCI_ROUTING_TABLE *ptrp;
	char *p;
	struct mp_intr_map *mpi;
	struct mp_bus *mpb;
	struct ioapic_softc *ioapic;
	unsigned dev;
	int pin;

	if (mp_verbose)
		printf("mpacpi: configuring PCI bus %d int routing\n",
		    mpr->mpr_bus);

	mpb = &mp_busses[mpr->mpr_bus];
	mpb->mb_intrs = NULL;
	mpb->mb_name = "pci";
	mpb->mb_idx = mpr->mpr_bus;
	mpb->mb_intr_print = mpacpi_print_pci_intr;
	mpb->mb_intr_cfg = NULL;
	mpb->mb_data = 0;

	for (p = mpr->mpr_buf.Pointer; ; p += ptrp->Length) {
		ptrp = (ACPI_PCI_ROUTING_TABLE *)p;
		if (ptrp->Length == 0)
			break;
		dev = ACPI_HIWORD(ptrp->Address);
		if (ptrp->Source[0] != 0)
			continue;
		ioapic = ioapic_find_bybase(ptrp->SourceIndex);
		if (ioapic == NULL)
			continue;
		mpi = &mp_intrs[mpacpi_intr_index++];
		mpi->bus = mpb;
		mpi->bus_pin = (dev << 2) | ptrp->Pin;
		mpi->type = MPS_INTTYPE_INT;

		/* Defaults for PCI (active low, level triggered) */
		mpi->redir = (IOAPIC_REDLO_DEL_FIXED<<IOAPIC_REDLO_DEL_SHIFT) |
		    IOAPIC_REDLO_LEVEL | IOAPIC_REDLO_ACTLO;
		mpi->flags = MPS_INTPO_ACTLO | (MPS_INTTR_LEVEL << 2);
		mpi->cpu_id = 0;

		pin = ptrp->SourceIndex - ioapic->sc_apic_vecbase;
		mpi->ioapic = ioapic;
		mpi->ioapic_pin = pin;
		mpi->ioapic_ih = APIC_INT_VIA_APIC |
		    (ioapic->sc_apicid << APIC_INT_APIC_SHIFT) |
		    (pin << APIC_INT_PIN_SHIFT);
		ioapic->sc_pins[pin].ip_map = mpi;
		mpi->next = mpb->mb_intrs;
		mpi->global_int = ptrp->SourceIndex;
		mpb->mb_intrs = mpi;
	}

	AcpiOsFree(mpr->mpr_buf.Pointer);
	mpr->mpr_buf.Pointer = NULL;	/* be preventive to bugs */

	return 0;
}

/*
 * Count number of elements in _PRT
 */
static int
mpacpi_pcircount(struct mpacpi_pcibus *mpr)
{
	int count = 0;
	ACPI_PCI_ROUTING_TABLE *PrtElement;
	UINT8 *Buffer;

	for (Buffer = mpr->mpr_buf.Pointer;; Buffer += PrtElement->Length) {
		PrtElement = (ACPI_PCI_ROUTING_TABLE *)Buffer;
		if (PrtElement->Length == 0)
			break;
		count++;
	}

	return count;
}
#endif

/*
 * Set up the interrupt config lists, in the same format as the mpbios does.
 */
static void
mpacpi_config_irouting(struct acpi_softc *acpi)
{
#if NPCI > 0
	struct mpacpi_pcibus *mpr;
#endif
	int nintr;
	int i;
	struct mp_bus *mbp;
	struct mp_intr_map *mpi;
	struct ioapic_softc *ioapic;

	nintr = mpacpi_nintsrc + NUM_LEGACY_IRQS - 1;
#if NPCI > 0
	TAILQ_FOREACH(mpr, &mpacpi_pcibusses, mpr_list) {
		nintr += mpacpi_pcircount(mpr);
	}

	mp_isa_bus = mpacpi_maxpci + BUS_BUFFER; /* XXX */
#else
	mp_isa_bus = 0;
#endif
	mp_nbus = mp_isa_bus + 1;
	mp_nintr = nintr;

	mp_busses = malloc(sizeof(struct mp_bus) * mp_nbus, M_DEVBUF,
	    M_NOWAIT|M_ZERO);
	if (mp_busses == NULL)
		panic("can't allocate mp_busses");

	mp_intrs = malloc(sizeof(struct mp_intr_map) * mp_nintr, M_DEVBUF,
	    M_NOWAIT | M_ZERO);
	if (mp_intrs == NULL)
		panic("can't allocate mp_intrs");

	mbp = &mp_busses[mp_isa_bus];
	mbp->mb_name = "isa";
	mbp->mb_idx = 0;
	mbp->mb_intr_print = mpacpi_print_isa_intr;
	mbp->mb_intr_cfg = NULL;
	mbp->mb_intrs = &mp_intrs[0];
	mbp->mb_data = 0;

	ioapic = ioapic_find_bybase(0);
	if (ioapic == NULL)
		panic("can't find first ioapic");

	/*
	 * Set up default identity mapping for ISA irqs to first ioapic.
	 */
	for (i = mpacpi_intr_index = 0; i < NUM_LEGACY_IRQS; i++) {
		if (i == 2)
			continue;
		mpi = &mp_intrs[mpacpi_intr_index];
		if (mpacpi_intr_index < (NUM_LEGACY_IRQS - 2))
			mpi->next = &mp_intrs[mpacpi_intr_index + 1];
		else
			mpi->next = NULL;
		mpi->bus = mbp;
		mpi->bus_pin = i;
		mpi->ioapic_pin = i;
		mpi->ioapic = ioapic;
		mpi->type = MPS_INTTYPE_INT;
		mpi->cpu_id = 0;
		mpi->ioapic_ih = APIC_INT_VIA_APIC |
		    (ioapic->sc_apicid << APIC_INT_APIC_SHIFT) |
		    (i << APIC_INT_PIN_SHIFT);
		mpi->redir = (IOAPIC_REDLO_DEL_FIXED<<IOAPIC_REDLO_DEL_SHIFT);
		mpi->flags = MPS_INTPO_DEF | (MPS_INTTR_DEF << 2);
		mpi->global_int = i;
		ioapic->sc_pins[i].ip_map = mpi;
		mpacpi_intr_index++;
	}

	if (acpi_madt_map() != AE_OK)
		panic("failed to map the MADT a second time");

	acpi_madt_walk(mpacpi_nonpci_intr, &mpacpi_intr_index);
	acpi_madt_unmap();

#if NPCI > 0
	TAILQ_FOREACH(mpr, &mpacpi_pcibusses, mpr_list) {
		mpacpi_pciroute(mpr);
	}
#endif
	mp_nintr = mpacpi_intr_index;
}

/*
 * XXX code duplication with mpbios.c
 */

#if NPCI > 0
static void
mpacpi_print_pci_intr(int intr)
{
	printf(" device %d INT_%c", (intr>>2)&0x1f, 'A' + (intr & 0x3));
}
#endif

static void
mpacpi_print_isa_intr(int intr)
{
	printf(" irq %d", intr);
}

static const char inttype_fmt[] = "\177\020"
		"f\0\2type\0" "=\1NMI\0" "=\2SMI\0" "=\3ExtINT\0";

static const char flagtype_fmt[] = "\177\020"
		"f\0\2pol\0" "=\1Act Hi\0" "=\3Act Lo\0"
		"f\2\2trig\0" "=\1Edge\0" "=\3Level\0";

static void
mpacpi_print_intr(struct mp_intr_map *mpi)
{
	char buf[256];
	int pin;
	struct ioapic_softc *sc;
	const char *busname;

	sc = mpi->ioapic;
	pin = mpi->ioapic_pin;
	if (mpi->bus != NULL)
		busname = mpi->bus->mb_name;
	else {
		switch (mpi->type) {
		case MPS_INTTYPE_NMI:
			busname = "NMI";
			break;
		case MPS_INTTYPE_SMI:
			busname = "SMI";
			break;
		case MPS_INTTYPE_ExtINT:
			busname = "ExtINT";
			break;
		default:
			busname = "<unknown>";
			break;
		}
	}

	printf("%s: pin %d attached to %s",
	    sc ? sc->sc_pic.pic_dev.dv_xname : "local apic",
	    pin, busname);

	if (mpi->bus != NULL) {
		if (mpi->bus->mb_idx != -1)
			printf("%d", mpi->bus->mb_idx);
		(*(mpi->bus->mb_intr_print))(mpi->bus_pin);
	}

	printf(" (type %s",
	    bitmask_snprintf(mpi->type, inttype_fmt, buf, sizeof(buf)));

	printf(" flags %s)\n",
	    bitmask_snprintf(mpi->flags, flagtype_fmt, buf, sizeof(buf)));
}


int
mpacpi_find_interrupts(void *self)
{
	ACPI_OBJECT_LIST arglist;
	ACPI_OBJECT arg;
	ACPI_STATUS rv;
	struct acpi_softc *acpi = self;
	int i;

#ifdef MPBIOS
	/*
	 * If MPBIOS was enabled, and did the work (because the initial
	 * MADT scan failed for some reason), there's nothing left to
	 * do here. Same goes for the case where no I/O APICS were found.
	 */
	if (mpbios_scanned)
		return 0;
#endif

	if (mpacpi_nioapic == 0)
		return 0;

	/*
	 * Switch us into APIC mode by evaluating _PIC(1).
	 * Needs to be done now, since it has an effect on
	 * the interrupt information we're about to retrieve.
	 */
	arglist.Count = 1;
	arglist.Pointer = &arg;
	arg.Type = ACPI_TYPE_INTEGER;
	arg.Integer.Value = 1;	/* I/O APIC mode (0 = PIC, 2 = IOSAPIC) */
	rv = AcpiEvaluateObject(NULL, "\\_PIC", &arglist, NULL);
	if (ACPI_FAILURE(rv)) {
		if (mp_verbose)
			printf("mpacpi: switch to APIC mode failed\n");
		return 0;
	}

#if NPCI > 0
	mpacpi_find_pcibusses(acpi);
	if (mp_verbose)
		printf("mpacpi: %d PCI busses\n", mpacpi_npci);
#endif
	mpacpi_config_irouting(acpi);
	if (mp_verbose)
		for (i = 0; i < mp_nintr; i++)
			mpacpi_print_intr(&mp_intrs[i]);
	return 0;
}

#if NPCI > 0

int
mpacpi_pci_attach_hook(struct device *parent, struct device *self,
		       struct pcibus_attach_args *pba)
{
	struct mp_bus *mpb;

#ifdef MPBIOS
	if (mpbios_scanned != 0 || mpacpi_nioapic == 0)
		return ENOENT;
#endif

	if (TAILQ_EMPTY(&mpacpi_pcibusses))
		return 0;

	/*
	 * If this bus is not found in mpacpi_find_pcibusses
	 * (i.e. behind PCI-to-PCI bridge), register as an extra bus.
	 *
	 * at this point, mp_busses[] are as follows:
	 *  mp_busses[0 .. mpacpi_maxpci] : PCI
	 *  mp_busses[mpacpi_maxpci + BUS_BUFFER] : ISA
	 */
	if (pba->pba_bus >= mp_isa_bus)
		panic("Increase BUS_BUFFER in mpacpi.c!");

	mpb = &mp_busses[pba->pba_bus];
	if (mpb->mb_name != NULL) {
		if (strcmp(mpb->mb_name, "pci"))
			return EINVAL;
	} else
		/*
		 * As we cannot find all PCI-to-PCI bridge in
		 * mpacpi_find_pcibusses, some of the MP_busses may remain
		 * uninitialized.
		 */
		mpb->mb_name = "pci";

	mpb->mb_configured = 1;
	mpb->mb_pci_bridge_tag = pba->pba_bridgetag;
	mpb->mb_pci_chipset_tag = pba->pba_pc;

	if (pba->pba_bus > mpacpi_maxpci)
		mpacpi_maxpci = pba->pba_bus;

	return 0;
}

int
mpacpi_scan_pci(struct device *self, struct pcibus_attach_args *pba,
	        cfprint_t print)
{
	int i;
	struct mp_bus *mpb;
	struct pci_attach_args;

	for (i = 0; i < mp_nbus; i++) {
		mpb = &mp_busses[i];
		if (mpb->mb_name == NULL)
			continue;
		if (!strcmp(mpb->mb_name, "pci") && mpb->mb_configured == 0) {
			pba->pba_bus = i;
			config_found_ia(self, "pcibus", pba, print);
		}
	}
	return 0;
}

#endif
