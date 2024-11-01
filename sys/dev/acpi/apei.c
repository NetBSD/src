/*	$NetBSD: apei.c,v 1.3.4.3 2024/11/01 14:45:36 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

/*
 * APEI: ACPI Platform Error Interface
 *
 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html
 *
 * XXX dtrace probes
 *
 * XXX call _OSC appropriately to announce to the platform that we, the
 * OSPM, support APEI
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apei.c,v 1.3.4.3 2024/11/01 14:45:36 martin Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/endian.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/uuid.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/apei_bertvar.h>
#include <dev/acpi/apei_cper.h>
#include <dev/acpi/apei_einjvar.h>
#include <dev/acpi/apei_erstvar.h>
#include <dev/acpi/apei_hestvar.h>
#include <dev/acpi/apei_interp.h>
#include <dev/acpi/apeivar.h>
#include <dev/pci/pcireg.h>

#define	_COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("apei")

static int apei_match(device_t, cfdata_t, void *);
static void apei_attach(device_t, device_t, void *);
static int apei_detach(device_t, int);

static void apei_get_tables(struct apei_tab *);
static void apei_put_tables(struct apei_tab *);

static void apei_identify(struct apei_softc *, const char *,
    const ACPI_TABLE_HEADER *);

CFATTACH_DECL_NEW(apei, sizeof(struct apei_softc),
    apei_match, apei_attach, apei_detach, NULL);

static int
apei_match(device_t parent, cfdata_t match, void *aux)
{
	struct apei_tab tab;
	int prio = 0;

	/*
	 * If we have any of the APEI tables, match.
	 */
	apei_get_tables(&tab);
	if (tab.bert || tab.einj || tab.erst || tab.hest)
		prio = 1;
	apei_put_tables(&tab);

	return prio;
}

static void
apei_attach(device_t parent, device_t self, void *aux)
{
	struct apei_softc *sc = device_private(self);
	const struct sysctlnode *sysctl_hw_acpi;
	int error;

	aprint_naive("\n");
	aprint_normal(": ACPI Platform Error Interface\n");

	pmf_device_register(self, NULL, NULL);

	sc->sc_dev = self;
	apei_get_tables(&sc->sc_tab);

	/*
	 * Get the sysctl hw.acpi node.  This should already be created
	 * but I don't see an easy way to get at it.  If this fails,
	 * something is seriously wrong, so let's stop here.
	 */
	error = sysctl_createv(&sc->sc_sysctllog, 0,
	    NULL, &sysctl_hw_acpi, 0,
	    CTLTYPE_NODE, "acpi", NULL, NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to create sysctl hw.acpi: %d\n", error);
		return;
	}

	/*
	 * Create sysctl hw.acpi.apei.
	 */
	error = sysctl_createv(&sc->sc_sysctllog, 0,
	    &sysctl_hw_acpi, &sc->sc_sysctlroot, 0,
	    CTLTYPE_NODE, "apei",
	    SYSCTL_DESCR("ACPI Platform Error Interface"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to create sysctl hw.acpi.apei: %d\n", error);
		return;
	}

	/*
	 * Set up BERT, EINJ, ERST, and HEST.
	 */
	if (sc->sc_tab.bert) {
		apei_identify(sc, "BERT", &sc->sc_tab.bert->Header);
		apei_bert_attach(sc);
	}
	if (sc->sc_tab.einj) {
		apei_identify(sc, "EINJ", &sc->sc_tab.einj->Header);
		apei_einj_attach(sc);
	}
	if (sc->sc_tab.erst) {
		apei_identify(sc, "ERST", &sc->sc_tab.erst->Header);
		apei_erst_attach(sc);
	}
	if (sc->sc_tab.hest) {
		apei_identify(sc, "HEST", &sc->sc_tab.hest->Header);
		apei_hest_attach(sc);
	}
}

static int
apei_detach(device_t self, int flags)
{
	struct apei_softc *sc = device_private(self);
	int error;

	/*
	 * Detach children.  We don't currently have any but this is
	 * harmless without children and mandatory if we ever sprouted
	 * them, so let's just leave it here for good measure.
	 *
	 * After this point, we are committed to detaching; failure is
	 * forbidden.
	 */
	error = config_detach_children(self, flags);
	if (error)
		return error;

	/*
	 * Tear down all the sysctl nodes first, before the software
	 * state backing them goes away.
	 */
	sysctl_teardown(&sc->sc_sysctllog);
	sc->sc_sysctlroot = NULL;

	/*
	 * Detach the software state for the APEI tables.
	 */
	if (sc->sc_tab.hest)
		apei_hest_detach(sc);
	if (sc->sc_tab.erst)
		apei_erst_detach(sc);
	if (sc->sc_tab.einj)
		apei_einj_detach(sc);
	if (sc->sc_tab.bert)
		apei_bert_detach(sc);

	/*
	 * Release the APEI tables and we're done.
	 */
	apei_put_tables(&sc->sc_tab);
	pmf_device_deregister(self);
	return 0;
}

/*
 * apei_get_tables(tab)
 *
 *	Get references to whichever APEI-related tables -- BERT, EINJ,
 *	ERST, HEST -- are available in the system.
 */
static void
apei_get_tables(struct apei_tab *tab)
{
	ACPI_STATUS rv;

	/*
	 * Probe the BERT -- Boot Error Record Table.
	 */
	rv = AcpiGetTable(ACPI_SIG_BERT, 0, (ACPI_TABLE_HEADER **)&tab->bert);
	if (ACPI_FAILURE(rv))
		tab->bert = NULL;

	/*
	 * Probe the EINJ -- Error Injection Table.
	 */
	rv = AcpiGetTable(ACPI_SIG_EINJ, 0, (ACPI_TABLE_HEADER **)&tab->einj);
	if (ACPI_FAILURE(rv))
		tab->einj = NULL;

	/*
	 * Probe the ERST -- Error Record Serialization Table.
	 */
	rv = AcpiGetTable(ACPI_SIG_ERST, 0, (ACPI_TABLE_HEADER **)&tab->erst);
	if (ACPI_FAILURE(rv))
		tab->erst = NULL;

	/*
	 * Probe the HEST -- Hardware Error Source Table.
	 */
	rv = AcpiGetTable(ACPI_SIG_HEST, 0, (ACPI_TABLE_HEADER **)&tab->hest);
	if (ACPI_FAILURE(rv))
		tab->hest = NULL;
}

/*
 * apei_put_tables(tab)
 *
 *	Release the tables acquired by apei_get_tables.
 */
static void
apei_put_tables(struct apei_tab *tab)
{

	if (tab->bert != NULL) {
		AcpiPutTable(&tab->bert->Header);
		tab->bert = NULL;
	}
	if (tab->einj != NULL) {
		AcpiPutTable(&tab->einj->Header);
		tab->einj = NULL;
	}
	if (tab->erst != NULL) {
		AcpiPutTable(&tab->erst->Header);
		tab->erst = NULL;
	}
	if (tab->hest != NULL) {
		AcpiPutTable(&tab->hest->Header);
		tab->hest = NULL;
	}
}

/*
 * apei_identify(sc, name, header)
 *
 *	Identify the APEI-related table header for dmesg.
 */
static void
apei_identify(struct apei_softc *sc, const char *name,
    const ACPI_TABLE_HEADER *h)
{

	aprint_normal_dev(sc->sc_dev, "%s:"
	    " OemId <%6.6s,%8.8s,%08x>"
	    " AslId <%4.4s,%08x>\n",
	    name,
	    h->OemId, h->OemTableId, h->OemRevision,
	    h->AslCompilerId, h->AslCompilerRevision);
}

/*
 * apei_cper_guid_dec(buf, uuid)
 *
 *	Decode a Common Platform Error Record UUID/GUID from an ACPI
 *	table at buf into a sys/uuid.h struct uuid.
 */
static void
apei_cper_guid_dec(const uint8_t buf[static 16], struct uuid *uuid)
{

	uuid_dec_le(buf, uuid);
}

/*
 * apei_format_guid(uuid, s)
 *
 *	Format a UUID as a string.  This uses C initializer notation,
 *	not UUID notation, in order to match the text in the UEFI
 *	specification.
 */
static void
apei_format_guid(const struct uuid *uuid, char guidstr[static 69])
{

	snprintf(guidstr, 69, "{0x%08x,0x%04x,0x%04x,"
	    "{0x%02x,%02x,"
	    "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}}",
	    uuid->time_low, uuid->time_mid, uuid->time_hi_and_version,
	    uuid->clock_seq_hi_and_reserved, uuid->clock_seq_low,
	    uuid->node[0], uuid->node[1], uuid->node[2],
	    uuid->node[3], uuid->node[4], uuid->node[5]);
}

/*
 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html#memory-error-section
 */

static const char *const cper_memory_error_type[] = {
#define	F(LN, SN, V)	[LN] = #SN,
	CPER_MEMORY_ERROR_TYPES(F)
#undef	F
};

/*
 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#generic-error-status-block
 *
 * The acpica names ACPI_HEST_GEN_ERROR_* appear to coincide with this
 * but are designated as being intended for Generic Error Data Entries
 * rather than Generic Error Status Blocks.
 */
static const char *const apei_gesb_severity[] = {
	[0] = "recoverable",
	[1] = "fatal",
	[2] = "corrected",
	[3] = "none",
};

/*
 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#generic-error-data-entry
 */
static const char *const apei_gede_severity[] = {
	[ACPI_HEST_GEN_ERROR_RECOVERABLE] = "recoverable",
	[ACPI_HEST_GEN_ERROR_FATAL] = "fatal",
	[ACPI_HEST_GEN_ERROR_CORRECTED] = "corrected",
	[ACPI_HEST_GEN_ERROR_NONE] = "none",
};

/*
 * N.2.5. Memory Error Section
 *
 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html#memory-error-section
 */
static const struct uuid CPER_MEMORY_ERROR_SECTION =
    {0xa5bc1114,0x6f64,0x4ede,0xb8,0x63,{0x3e,0x83,0xed,0x7c,0x83,0xb1}};

static void
apei_cper_memory_error_report(struct apei_softc *sc, const void *buf,
    size_t len, const char *ctx, bool ratelimitok)
{
	const struct cper_memory_error *ME = buf;
	char bitbuf[1024];

	/*
	 * If we've hit the rate limit, skip printing the error.
	 */
	if (!ratelimitok)
		goto out;

	snprintb(bitbuf, sizeof(bitbuf),
	    CPER_MEMORY_ERROR_VALIDATION_BITS_FMT, ME->ValidationBits);
	aprint_debug_dev(sc->sc_dev, "%s: ValidationBits=%s\n", ctx, bitbuf);
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_ERROR_STATUS) {
		/*
		 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html#error-status
		 */
		/* XXX define this format somewhere */
		snprintb(bitbuf, sizeof(bitbuf), "\177\020"
		    "f\010\010"	"ErrorType\0"
			"=\001"		"ERR_INTERNAL\0"
			"=\004"		"ERR_MEM\0"
			"=\005"		"ERR_TLB\0"
			"=\006"		"ERR_CACHE\0"
			"=\007"		"ERR_FUNCTION\0"
			"=\010"		"ERR_SELFTEST\0"
			"=\011"		"ERR_FLOW\0"
			"=\020"		"ERR_BUS\0"
			"=\021"		"ERR_MAP\0"
			"=\022"		"ERR_IMPROPER\0"
			"=\023"		"ERR_UNIMPL\0"
			"=\024"		"ERR_LOL\0"
			"=\025"		"ERR_RESPONSE\0"
			"=\026"		"ERR_PARITY\0"
			"=\027"		"ERR_PROTOCOL\0"
			"=\030"		"ERR_ERROR\0"
			"=\031"		"ERR_TIMEOUT\0"
			"=\032"		"ERR_POISONED\0"
		    "b\020"	"AddressError\0"
		    "b\021"	"ControlError\0"
		    "b\022"	"DataError\0"
		    "b\023"	"ResponderDetected\0"
		    "b\024"	"RequesterDetected\0"
		    "b\025"	"FirstError\0"
		    "b\026"	"Overflow\0"
		    "\0", ME->ErrorStatus);
		device_printf(sc->sc_dev, "%s: ErrorStatus=%s\n", ctx, bitbuf);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_PHYSICAL_ADDRESS) {
		device_printf(sc->sc_dev, "%s: PhysicalAddress=0x%"PRIx64"\n",
		    ctx, ME->PhysicalAddress);
	}
	if (ME->ValidationBits &
	    CPER_MEMORY_ERROR_VALID_PHYSICAL_ADDRESS_MASK) {
		device_printf(sc->sc_dev, "%s: PhysicalAddressMask=0x%"PRIx64
		    "\n", ctx, ME->PhysicalAddressMask);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_NODE) {
		device_printf(sc->sc_dev, "%s: Node=0x%"PRIx16"\n", ctx,
		    ME->Node);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_CARD) {
		device_printf(sc->sc_dev, "%s: Card=0x%"PRIx16"\n", ctx,
		    ME->Card);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_MODULE) {
		device_printf(sc->sc_dev, "%s: Module=0x%"PRIx16"\n", ctx,
		    ME->Module);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_BANK) {
		device_printf(sc->sc_dev, "%s: Bank=0x%"PRIx16"\n", ctx,
		    ME->Bank);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_DEVICE) {
		device_printf(sc->sc_dev, "%s: Device=0x%"PRIx16"\n", ctx,
		    ME->Device);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_ROW) {
		device_printf(sc->sc_dev, "%s: Row=0x%"PRIx16"\n", ctx,
		    ME->Row);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_COLUMN) {
		device_printf(sc->sc_dev, "%s: Column=0x%"PRIx16"\n", ctx,
		    ME->Column);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_BIT_POSITION) {
		device_printf(sc->sc_dev, "%s: BitPosition=0x%"PRIx16"\n",
		    ctx, ME->BitPosition);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_REQUESTOR_ID) {
		device_printf(sc->sc_dev, "%s: RequestorId=0x%"PRIx64"\n",
		    ctx, ME->RequestorId);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_RESPONDER_ID) {
		device_printf(sc->sc_dev, "%s: ResponderId=0x%"PRIx64"\n",
		    ctx, ME->ResponderId);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_TARGET_ID) {
		device_printf(sc->sc_dev, "%s: TargetId=0x%"PRIx64"\n",
		    ctx, ME->TargetId);
	}
	if (ME->ValidationBits & CPER_MEMORY_ERROR_VALID_MEMORY_ERROR_TYPE) {
		const uint8_t t = ME->MemoryErrorType;
		const char *n = t < __arraycount(cper_memory_error_type)
		    ? cper_memory_error_type[t] : NULL;

		if (n) {
			device_printf(sc->sc_dev, "%s: MemoryErrorType=%d"
			    " (%s)\n", ctx, t, n);
		} else {
			device_printf(sc->sc_dev, "%s: MemoryErrorType=%d\n",
			    ctx, t);
		}
	}

out:	/*
	 * XXX pass this through to uvm(9) or userland for decisions
	 * like page retirement
	 */
	return;
}

/*
 * N.2.7. PCI Express Error Section
 *
 * https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html#pci-express-error-section
 */
static const struct uuid CPER_PCIE_ERROR_SECTION =
    {0xd995e954,0xbbc1,0x430f,0xad,0x91,{0xb4,0x4d,0xcb,0x3c,0x6f,0x35}};

static const char *const cper_pcie_error_port_type[] = {
#define	F(LN, SN, V)	[LN] = #SN,
	CPER_PCIE_ERROR_PORT_TYPES(F)
#undef	F
};

static void
apei_cper_pcie_error_report(struct apei_softc *sc, const void *buf, size_t len,
    const char *ctx, bool ratelimitok)
{
	const struct cper_pcie_error *PE = buf;
	char bitbuf[1024];

	/*
	 * If we've hit the rate limit, skip printing the error.
	 */
	if (!ratelimitok)
		goto out;

	snprintb(bitbuf, sizeof(bitbuf),
	    CPER_PCIE_ERROR_VALIDATION_BITS_FMT, PE->ValidationBits);
	aprint_debug_dev(sc->sc_dev, "%s: ValidationBits=%s\n", ctx, bitbuf);
	if (PE->ValidationBits & CPER_PCIE_ERROR_VALID_PORT_TYPE) {
		const uint32_t t = PE->PortType;
		const char *n = t < __arraycount(cper_pcie_error_port_type)
		    ? cper_pcie_error_port_type[t] : NULL;

		if (n) {
			device_printf(sc->sc_dev, "%s: PortType=%"PRIu32
			    " (%s)\n", ctx, t, n);
		} else {
			device_printf(sc->sc_dev, "%s: PortType=%"PRIu32"\n",
			    ctx, t);
		}
	}
	if (PE->ValidationBits & CPER_PCIE_ERROR_VALID_VERSION) {
		/* XXX BCD */
		device_printf(sc->sc_dev, "%s: Version=0x08%"PRIx32"\n",
		    ctx, PE->Version);
	}
	if (PE->ValidationBits & CPER_PCIE_ERROR_VALID_COMMAND_STATUS) {
		/* XXX move me to pcireg.h */
		snprintb(bitbuf, sizeof(bitbuf), "\177\020"
			/* command */
		    "b\000"	"IO_ENABLE\0"
		    "b\001"	"MEM_ENABLE\0"
		    "b\002"	"MASTER_ENABLE\0"
		    "b\003"	"SPECIAL_ENABLE\0"
		    "b\004"	"INVALIDATE_ENABLE\0"
		    "b\005"	"PALETTE_ENABLE\0"
		    "b\006"	"PARITY_ENABLE\0"
		    "b\007"	"STEPPING_ENABLE\0"
		    "b\010"	"SERR_ENABLE\0"
		    "b\011"	"BACKTOBACK_ENABLE\0"
		    "b\012"	"INTERRUPT_DISABLE\0"
			/* status */
		    "b\023"	"INT_STATUS\0"
		    "b\024"	"CAPLIST_SUPPORT\0"
		    "b\025"	"66MHZ_SUPPORT\0"
		    "b\026"	"UDF_SUPPORT\0"
		    "b\027"	"BACKTOBACK_SUPPORT\0"
		    "b\030"	"PARITY_ERROR\0"
		    "f\031\002"	"DEVSEL\0"
			"=\000"		"FAST\0"
			"=\001"		"MEDIUM\0"
			"=\002"		"SLOW\0"
		    "b\033"	"TARGET_TARGET_ABORT\0"
		    "b\034"	"MASTER_TARGET_ABORT\0"
		    "b\035"	"MASTER_ABORT\0"
		    "b\036"	"SPECIAL_ERROR\0"
		    "b\037"	"PARITY_DETECT\0"
		    "\0", PE->CommandStatus);
		device_printf(sc->sc_dev, "%s: CommandStatus=%s\n",
		    ctx, bitbuf);
	}
	if (PE->ValidationBits & CPER_PCIE_ERROR_VALID_DEVICE_ID) {
		device_printf(sc->sc_dev, "%s: DeviceID:"
		    " VendorID=0x%04"PRIx16
		    " DeviceID=0x%04"PRIx16
		    " ClassCode=0x%06"PRIx32
		    " Function=%"PRIu8
		    " Device=%"PRIu8
		    " Segment=%"PRIu16
		    " Bus=%"PRIu8
		    " SecondaryBus=%"PRIu8
		    " Slot=0x%04"PRIx16
		    " Reserved0=0x%02"PRIx8
		    "\n",
		    ctx,
		    le16dec(PE->DeviceID.VendorID),
		    le16dec(PE->DeviceID.DeviceID),
		    (PE->DeviceID.ClassCode[0] |	/* le24dec */
			((uint32_t)PE->DeviceID.ClassCode[1] << 8) |
			((uint32_t)PE->DeviceID.ClassCode[2] << 16)),
		    PE->DeviceID.Function, PE->DeviceID.Device,
		    le16dec(PE->DeviceID.Segment), PE->DeviceID.Bus,
		    PE->DeviceID.SecondaryBus, le16dec(PE->DeviceID.Slot),
		    PE->DeviceID.Reserved0);
	}
	if (PE->ValidationBits & CPER_PCIE_ERROR_VALID_DEVICE_SERIAL) {
		device_printf(sc->sc_dev, "%s: DeviceSerial={%016"PRIx64"}\n",
		    ctx, PE->DeviceSerial);
	}
	if (PE->ValidationBits & CPER_PCIE_ERROR_VALID_BRIDGE_CONTROL_STATUS) {
		/* XXX snprintb */
		device_printf(sc->sc_dev, "%s: BridgeControlStatus=%"PRIx32
		    "\n", ctx, PE->BridgeControlStatus);
	}
	if (PE->ValidationBits & CPER_PCIE_ERROR_VALID_CAPABILITY_STRUCTURE) {
		uint32_t dcsr, dsr;
		char hex[9*sizeof(PE->CapabilityStructure)/4];
		unsigned i;

		/*
		 * Display a hex dump of each 32-bit register in the
		 * PCIe capability structure.
		 */
		__CTASSERT(sizeof(PE->CapabilityStructure) % 4 == 0);
		for (i = 0; i < sizeof(PE->CapabilityStructure)/4; i++) {
			snprintf(hex + 9*i, sizeof(hex) - 9*i, "%08"PRIx32" ",
			    le32dec(&PE->CapabilityStructure[4*i]));
		}
		hex[sizeof(hex) - 1] = '\0';
		device_printf(sc->sc_dev, "%s: CapabilityStructure={%s}\n",
		    ctx, hex);

		/*
		 * If the Device Status Register has any bits set,
		 * highlight it in particular -- these are probably
		 * error bits.
		 */
		dcsr = le32dec(&PE->CapabilityStructure[PCIE_DCSR]);
		dsr = __SHIFTOUT(dcsr, __BITS(31,16));
		if (dsr != 0) {
			/*
			 * XXX move me to pcireg.h; note: high
			 * half of DCSR
			 */
			snprintb(bitbuf, sizeof(bitbuf), "\177\020"
			    "b\000"	"CORRECTABLE_ERROR\0"
			    "b\001"	"NONFATAL_UNCORRECTABLE_ERROR\0"
			    "b\002"	"FATAL_ERROR\0"
			    "b\003"	"UNSUPPORTED_REQUEST\0"
			    "b\004"	"AUX_POWER\0"
			    "b\005"	"TRANSACTIONS_PENDING\0"
			    "\0", dsr);
			device_printf(sc->sc_dev, "%s: PCIe Device Status:"
			    " %s\n",
			    ctx, bitbuf);
		}
	}
	if (PE->ValidationBits & CPER_PCIE_ERROR_VALID_AER_INFO) {
		uint32_t uc_status, uc_sev;
		uint32_t cor_status;
		uint32_t control;
		char hex[9*sizeof(PE->AERInfo)/4];
		unsigned i;

		/*
		 * Display a hex dump of each 32-bit register in the
		 * PCIe Advanced Error Reporting extended capability
		 * structure.
		 */
		__CTASSERT(sizeof(PE->AERInfo) % 4 == 0);
		for (i = 0; i < sizeof(PE->AERInfo)/4; i++) {
			snprintf(hex + 9*i, sizeof(hex) - 9*i, "%08"PRIx32" ",
			    le32dec(&PE->AERInfo[4*i]));
		}
		hex[sizeof(hex) - 1] = '\0';
		device_printf(sc->sc_dev, "%s: AERInfo={%s}\n", ctx, hex);

			/* XXX move me to pcireg.h */
#define	PCI_AER_UC_STATUS_FMT	"\177\020"				      \
	"b\000"	"UNDEFINED\0"						      \
	"b\004"	"DL_PROTOCOL_ERROR\0"					      \
	"b\005"	"SURPRISE_DOWN_ERROR\0"					      \
	"b\014"	"POISONED_TLP\0"					      \
	"b\015"	"FC_PROTOCOL_ERROR\0"					      \
	"b\016"	"COMPLETION_TIMEOUT\0"					      \
	"b\017"	"COMPLETION_ABORT\0"					      \
	"b\020"	"UNEXPECTED_COMPLETION\0"				      \
	"b\021"	"RECEIVER_OVERFLOW\0"					      \
	"b\022"	"MALFORMED_TLP\0"					      \
	"b\023"	"ECRC_ERROR\0"						      \
	"b\024"	"UNSUPPORTED_REQUEST_ERROR\0"				      \
	"b\025"	"ACS_VIOLATION\0"					      \
	"b\026"	"INTERNAL_ERROR\0"					      \
	"b\027"	"MC_BLOCKED_TLP\0"					      \
	"b\030"	"ATOMIC_OP_EGRESS_BLOCKED\0"				      \
	"b\031"	"TLP_PREFIX_BLOCKED_ERROR\0"				      \
	"b\032"	"POISONTLP_EGRESS_BLOCKED\0"				      \
	"\0"

		/*
		 * If there are any hardware error status bits set,
		 * highlight them in particular, in three groups:
		 *
		 * - uncorrectable fatal (UC_STATUS and UC_SEVERITY)
		 * - uncorrectable nonfatal (UC_STATUS but not UC_SEVERITY)
		 * - corrected (COR_STATUS)
		 *
		 * And if there are any uncorrectable errors, show
		 * which one was reported first, according to
		 * CAP_CONTROL.
		 */
		uc_status = le32dec(&PE->AERInfo[PCI_AER_UC_STATUS]);
		uc_sev = le32dec(&PE->AERInfo[PCI_AER_UC_SEVERITY]);
		cor_status = le32dec(&PE->AERInfo[PCI_AER_COR_STATUS]);
		control = le32dec(&PE->AERInfo[PCI_AER_CAP_CONTROL]);

		if (uc_status & uc_sev) {
			snprintb(bitbuf, sizeof(bitbuf), PCI_AER_UC_STATUS_FMT,
			    uc_status & uc_sev);
			device_printf(sc->sc_dev, "%s:"
			    " AER hardware fatal uncorrectable errors: %s\n",
			    ctx, bitbuf);
		}
		if (uc_status & ~uc_sev) {
			snprintb(bitbuf, sizeof(bitbuf), PCI_AER_UC_STATUS_FMT,
			    uc_status & ~uc_sev);
			device_printf(sc->sc_dev, "%s:"
			    " AER hardware non-fatal uncorrectable errors:"
			    " %s\n",
			    ctx, bitbuf);
		}
		if (uc_status) {
			unsigned first = __SHIFTOUT(control,
			    PCI_AER_FIRST_ERROR_PTR);
			snprintb(bitbuf, sizeof(bitbuf), PCI_AER_UC_STATUS_FMT,
			    (uint32_t)1 << first);
			device_printf(sc->sc_dev, "%s:"
			    " AER hardware first uncorrectable error: %s\n",
			    ctx, bitbuf);
		}
		if (cor_status) {
			/* XXX move me to pcireg.h */
			snprintb(bitbuf, sizeof(bitbuf), "\177\020"
			    "b\000"	"RECEIVER_ERROR\0"
			    "b\006"	"BAD_TLP\0"
			    "b\007"	"BAD_DLLP\0"
			    "b\010"	"REPLAY_NUM_ROLLOVER\0"
			    "b\014"	"REPLAY_TIMER_TIMEOUT\0"
			    "b\015"	"ADVISORY_NF_ERROR\0"
			    "b\016"	"INTERNAL_ERROR\0"
			    "b\017"	"HEADER_LOG_OVERFLOW\0"
			    "\0", cor_status);
			device_printf(sc->sc_dev, "%s:"
			    " AER hardware corrected error: %s\n",
			    ctx, bitbuf);
		}
	}

out:	/*
	 * XXX pass this on to the PCI subsystem to handle
	 */
	return;
}

/*
 * apei_cper_reports
 *
 *	Table of known Common Platform Error Record types, symbolic
 *	names, minimum data lengths, and functions to report them.
 *
 *	The section types and corresponding section layouts are listed
 *	at:
 *
 *	https://uefi.org/specs/UEFI/2.10/Apx_N_Common_Platform_Error_Record.html
 */
static const struct apei_cper_report {
	const char *name;
	const struct uuid *type;
	size_t minlength;
	void (*func)(struct apei_softc *, const void *, size_t, const char *,
	    bool);
} apei_cper_reports[] = {
	{ "memory", &CPER_MEMORY_ERROR_SECTION,
	  sizeof(struct cper_memory_error),
	  apei_cper_memory_error_report },
	{ "PCIe", &CPER_PCIE_ERROR_SECTION,
	  sizeof(struct cper_pcie_error),
	  apei_cper_pcie_error_report },
};

/*
 * apei_gede_report_header(sc, gede, ctx, ratelimitok, &headerlen, &report)
 *
 *	Report the header of the ith Generic Error Data Entry in the
 *	given context, if ratelimitok is true.
 *
 *	Return the actual length of the header in headerlen, or 0 if
 *	not known because the revision isn't recognized.
 *
 *	Return the report type in report, or NULL if not known because
 *	the section type isn't recognized.
 */
static void
apei_gede_report_header(struct apei_softc *sc,
    const ACPI_HEST_GENERIC_DATA *gede, const char *ctx, bool ratelimitok,
    size_t *headerlenp, const struct apei_cper_report **reportp)
{
	const ACPI_HEST_GENERIC_DATA_V300 *const gede_v3 = (const void *)gede;
	struct uuid sectype;
	char guidstr[69];
	char buf[128];
	unsigned i;

	/*
	 * Print the section type as a C initializer.  It would be
	 * prettier to use standard hyphenated UUID notation, but that
	 * notation is slightly ambiguous here (two octets could be
	 * written either way, depending on Microsoft convention --
	 * which influenced ACPI and UEFI -- or internet convention),
	 * and the UEFI spec writes the C initializer notation, so this
	 * makes it easier to search for.
	 *
	 * Also print out a symbolic name, if we know it.
	 */
	apei_cper_guid_dec(gede->SectionType, &sectype);
	apei_format_guid(&sectype, guidstr);
	for (i = 0; i < __arraycount(apei_cper_reports); i++) {
		const struct apei_cper_report *const report =
		    &apei_cper_reports[i];

		if (memcmp(&sectype, report->type, sizeof(sectype)) != 0)
			continue;
		if (ratelimitok) {
			device_printf(sc->sc_dev, "%s:"
			    " SectionType=%s (%s error)\n",
			    ctx, guidstr, report->name);
		}
		*reportp = report;
		break;
	}
	if (i == __arraycount(apei_cper_reports)) {
		if (ratelimitok) {
			device_printf(sc->sc_dev, "%s: SectionType=%s\n", ctx,
			    guidstr);
		}
		*reportp = NULL;
	}

	/*
	 * Print the numeric severity and, if we have it, a symbolic
	 * name for it.
	 */
	if (ratelimitok) {
		device_printf(sc->sc_dev, "%s: ErrorSeverity=%"PRIu32" (%s)\n",
		    ctx,
		    gede->ErrorSeverity,
		    (gede->ErrorSeverity < __arraycount(apei_gede_severity)
			? apei_gede_severity[gede->ErrorSeverity]
			: "unknown"));
	}

	/*
	 * The Revision may not often be useful, but this is only ever
	 * shown at the time of a hardware error report, not something
	 * you can glean at your convenience with acpidump.  So print
	 * it anyway.
	 */
	if (ratelimitok) {
		device_printf(sc->sc_dev, "%s: Revision=0x%"PRIx16"\n", ctx,
		    gede->Revision);
	}

	/*
	 * Don't touch anything past the Revision until we've
	 * determined we understand it.  Return the header length to
	 * the caller, or return zero -- and stop here -- if we don't
	 * know what the actual header length is.
	 */
	if (gede->Revision < 0x0300) {
		*headerlenp = sizeof(*gede);
	} else if (gede->Revision < 0x0400) {
		*headerlenp = sizeof(*gede_v3);
	} else {
		*headerlenp = 0;
		return;
	}

	/*
	 * Print the validation bits at debug level.  Only really
	 * helpful if there are bits we _don't_ know about.
	 */
	if (ratelimitok) {
		/* XXX define this format somewhere */
		snprintb(buf, sizeof(buf), "\177\020"
		    "b\000"	"FRU_ID\0"
		    "b\001"	"FRU_TEXT\0" /* `FRU string', sometimes */
		    "b\002"	"TIMESTAMP\0"
		    "\0", gede->ValidationBits);
		aprint_debug_dev(sc->sc_dev, "%s: ValidationBits=%s\n", ctx,
		    buf);
	}

	/*
	 * Print the CPER section flags.
	 */
	if (ratelimitok) {
		snprintb(buf, sizeof(buf), CPER_SECTION_FLAGS_FMT,
		    gede->Flags);
		device_printf(sc->sc_dev, "%s: Flags=%s\n", ctx, buf);
	}

	/*
	 * The ErrorDataLength is unlikely to be useful for the log, so
	 * print it at debug level only.
	 */
	if (ratelimitok) {
		aprint_debug_dev(sc->sc_dev, "%s:"
		    " ErrorDataLength=0x%"PRIu32"\n",
		    ctx, gede->ErrorDataLength);
	}

	/*
	 * Print the FRU Id and text, if available.
	 */
	if (ratelimitok &&
	    (gede->ValidationBits & ACPI_HEST_GEN_VALID_FRU_ID) != 0) {
		struct uuid fruid;

		apei_cper_guid_dec(gede->FruId, &fruid);
		apei_format_guid(&fruid, guidstr);
		device_printf(sc->sc_dev, "%s: FruId=%s\n", ctx, guidstr);
	}
	if (ratelimitok &&
	    (gede->ValidationBits & ACPI_HEST_GEN_VALID_FRU_STRING) != 0) {
		device_printf(sc->sc_dev, "%s: FruText=%.20s\n",
		    ctx, gede->FruText);
	}

	/*
	 * Print the timestamp, if available by the revision number and
	 * the validation bits.
	 */
	if (ratelimitok &&
	    gede->Revision >= 0x0300 && gede->Revision < 0x0400 &&
	    gede->ValidationBits & ACPI_HEST_GEN_VALID_TIMESTAMP) {
		const uint8_t *const t = (const uint8_t *)&gede_v3->TimeStamp;
		const uint8_t s = t[0];
		const uint8_t m = t[1];
		const uint8_t h = t[2];
		const uint8_t f = t[3];
		const uint8_t D = t[4];
		const uint8_t M = t[5];
		const uint8_t Y = t[6];
		const uint8_t C = t[7];

		device_printf(sc->sc_dev, "%s: Timestamp=0x%"PRIx64
		    " (%02d%02d-%02d-%02dT%02d:%02d:%02d%s)\n",
		    ctx, gede_v3->TimeStamp,
		    C,Y, M, D, h,m,s,
		    f & __BIT(0) ? " (event time)" : " (collect time)");
	}
}

/*
 * apei_gesb_ratelimit
 *
 *	State to limit the rate of console log messages about hardware
 *	errors.  For each of the four severity levels in a Generic
 *	Error Status Block,
 *
 *	0 - Recoverable (uncorrectable),
 *	1 - Fatal (uncorrectable),
 *	2 - Corrected, and
 *	3 - None (including ill-formed errors),
 *
 *	we record the last time it happened, protected by a CPU simple
 *	lock that we only try-acquire so it is safe to use in any
 *	context, including non-maskable interrupt context.
 */

static struct {
	__cpu_simple_lock_t	lock;
	struct timeval		lasttime;
	volatile uint32_t	suppressed;
} __aligned(COHERENCY_UNIT) apei_gesb_ratelimit[4] __cacheline_aligned = {
	[ACPI_HEST_GEN_ERROR_RECOVERABLE] = { .lock = __SIMPLELOCK_UNLOCKED },
	[ACPI_HEST_GEN_ERROR_FATAL] = { .lock = __SIMPLELOCK_UNLOCKED },
	[ACPI_HEST_GEN_ERROR_CORRECTED] = { .lock = __SIMPLELOCK_UNLOCKED },
	[ACPI_HEST_GEN_ERROR_NONE] = { .lock = __SIMPLELOCK_UNLOCKED },
};

static void
atomic_incsat_32(volatile uint32_t *p)
{
	uint32_t o, n;

	do {
		o = atomic_load_relaxed(p);
		if (__predict_false(o == UINT_MAX))
			return;
		n = o + 1;
	} while (__predict_false(atomic_cas_32(p, o, n) != o));
}

/*
 * apei_gesb_ratecheck(sc, severity, suppressed)
 *
 *	Check for a rate limit on errors of the specified severity.
 *
 *	=> Return true if the error should be printed, and format into
 *	   the buffer suppressed a message saying how many errors were
 *	   previously suppressed.
 *
 *	=> Return false if the error should be suppressed because the
 *	   last one printed was too recent.
 */
static bool
apei_gesb_ratecheck(struct apei_softc *sc, uint32_t severity,
    char suppressed[static sizeof(" (4294967295 or more errors suppressed)")])
{
	/* one of each type per minute (XXX worth making configurable?) */
	const struct timeval mininterval = {60, 0};
	unsigned i = MIN(severity, ACPI_HEST_GEN_ERROR_NONE); /* paranoia */
	bool ok = false;

	/*
	 * If the lock is contended, the rate limit is probably
	 * exceeded, so it's not OK to print.
	 *
	 * Otherwise, with the lock held, ask ratecheck(9) whether it's
	 * OK to print.
	 */
	if (!__cpu_simple_lock_try(&apei_gesb_ratelimit[i].lock))
		goto out;
	ok = ratecheck(&apei_gesb_ratelimit[i].lasttime, &mininterval);
	__cpu_simple_unlock(&apei_gesb_ratelimit[i].lock);

out:	/*
	 * If it's OK to print, report the number of errors that were
	 * suppressed.  If it's not OK to print, count a suppressed
	 * error.
	 */
	if (ok) {
		const uint32_t n =
		    atomic_swap_32(&apei_gesb_ratelimit[i].suppressed, 0);

		if (n == 0) {
			suppressed[0] = '\0';
		} else {
			snprintf(suppressed,
			    sizeof(" (4294967295 or more errors suppressed)"),
			    " (%u%s error%s suppressed)",
			    n,
			    n == UINT32_MAX ? " or more" : "",
			    n == 1 ? "" : "s");
		}
	} else {
		atomic_incsat_32(&apei_gesb_ratelimit[i].suppressed);
		suppressed[0] = '\0';
	}
	return ok;
}

/*
 * apei_gesb_report(sc, gesb, size, ctx)
 *
 *	Check a Generic Error Status Block, of at most the specified
 *	size in bytes, and report any errors in it.  Return the 32-bit
 *	Block Status in case the caller needs it to acknowledge the
 *	report to firmware.
 */
uint32_t
apei_gesb_report(struct apei_softc *sc, const ACPI_HEST_GENERIC_STATUS *gesb,
    size_t size, const char *ctx, bool *fatalp)
{
	uint32_t status, unknownstatus, severity, nentries, i;
	uint32_t datalen, rawdatalen;
	const ACPI_HEST_GENERIC_DATA *gede0, *gede;
	const unsigned char *rawdata;
	bool ratelimitok = false;
	char suppressed[sizeof(" (4294967295 or more errors suppressed)")];
	bool fatal = false;

	/*
	 * Verify the buffer is large enough for a Generic Error Status
	 * Block before we try to touch anything in it.
	 */
	if (size < sizeof(*gesb)) {
		ratelimitok = apei_gesb_ratecheck(sc, ACPI_HEST_GEN_ERROR_NONE,
		    suppressed);
		if (ratelimitok) {
			device_printf(sc->sc_dev,
			    "%s: truncated GESB, %zu < %zu%s\n",
			    ctx, size, sizeof(*gesb), suppressed);
		}
		status = 0;
		goto out;
	}
	size -= sizeof(*gesb);

	/*
	 * Load the status.  Access ordering rules are unclear in the
	 * ACPI specification; I'm guessing that load-acquire of the
	 * block status is a good idea before any other access to the
	 * GESB.
	 */
	status = atomic_load_acquire(&gesb->BlockStatus);

	/*
	 * If there are no status bits set, the rest of the GESB is
	 * garbage, so stop here.
	 */
	if (status == 0) {
		/* XXX dtrace */
		/* XXX DPRINTF */
		goto out;
	}

	/*
	 * Read out the severity and get the number of entries in this
	 * status block.
	 */
	severity = gesb->ErrorSeverity;
	nentries = __SHIFTOUT(status, ACPI_HEST_ERROR_ENTRY_COUNT);

	/*
	 * Print a message to the console and dmesg about the severity
	 * of the error.
	 */
	ratelimitok = apei_gesb_ratecheck(sc, severity, suppressed);
	if (ratelimitok) {
		char statusbuf[128];

		/* XXX define this format somewhere */
		snprintb(statusbuf, sizeof(statusbuf), "\177\020"
		    "b\000"	"UE\0"
		    "b\001"	"CE\0"
		    "b\002"	"MULTI_UE\0"
		    "b\003"	"MULTI_CE\0"
		    "f\004\010"	"GEDE_COUNT\0"
		    "\0", status);

		if (severity < __arraycount(apei_gesb_severity)) {
			device_printf(sc->sc_dev, "%s"
			    " reported hardware error%s:"
			    " severity=%s nentries=%u status=%s\n",
			    ctx, suppressed,
			    apei_gesb_severity[severity], nentries, statusbuf);
		} else {
			device_printf(sc->sc_dev, "%s reported error%s:"
			    " severity=%"PRIu32" nentries=%u status=%s\n",
			    ctx, suppressed,
			    severity, nentries, statusbuf);
		}
	}

	/*
	 * Make a determination about whether the error is fatal.
	 *
	 * XXX Currently we don't have any mechanism to recover from
	 * uncorrectable but recoverable errors, so we treat those --
	 * and anything else we don't recognize -- as fatal.
	 */
	switch (severity) {
	case ACPI_HEST_GEN_ERROR_CORRECTED:
	case ACPI_HEST_GEN_ERROR_NONE:
		fatal = false;
		break;
	case ACPI_HEST_GEN_ERROR_FATAL:
	case ACPI_HEST_GEN_ERROR_RECOVERABLE: /* XXX */
	default:
		fatal = true;
		break;
	}

	/*
	 * Clear the bits we know about to warn if there's anything
	 * left we don't understand.
	 */
	unknownstatus = status;
	unknownstatus &= ~ACPI_HEST_UNCORRECTABLE;
	unknownstatus &= ~ACPI_HEST_MULTIPLE_UNCORRECTABLE;
	unknownstatus &= ~ACPI_HEST_CORRECTABLE;
	unknownstatus &= ~ACPI_HEST_MULTIPLE_CORRECTABLE;
	unknownstatus &= ~ACPI_HEST_ERROR_ENTRY_COUNT;
	if (ratelimitok && unknownstatus != 0) {
		/* XXX dtrace */
		device_printf(sc->sc_dev, "%s: unknown BlockStatus bits:"
		    " 0x%"PRIx32"\n", ctx, unknownstatus);
	}

	/*
	 * Advance past the Generic Error Status Block (GESB) header to
	 * the Generic Error Data Entries (GEDEs).
	 */
	gede0 = gede = (const ACPI_HEST_GENERIC_DATA *)(gesb + 1);

	/*
	 * Verify that the data length (GEDEs) fits within the size.
	 * If not, truncate the GEDEs.
	 */
	datalen = gesb->DataLength;
	if (size < datalen) {
		if (ratelimitok) {
			device_printf(sc->sc_dev, "%s:"
			    " GESB DataLength exceeds bounds:"
			    " %zu < %"PRIu32"\n",
			    ctx, size, datalen);
		}
		datalen = size;
	}
	size -= datalen;

	/*
	 * Report each of the Generic Error Data Entries.
	 */
	for (i = 0; i < nentries; i++) {
		size_t headerlen;
		const struct apei_cper_report *report;
		char subctx[128];

		/*
		 * Format a subcontext to show this numbered entry of
		 * the GESB.
		 */
		snprintf(subctx, sizeof(subctx), "%s entry %"PRIu32, ctx, i);

		/*
		 * If the remaining GESB data length isn't enough for a
		 * GEDE header, stop here.
		 */
		if (datalen < sizeof(*gede)) {
			if (ratelimitok) {
				device_printf(sc->sc_dev, "%s:"
				    " truncated GEDE: %"PRIu32" < %zu bytes\n",
				    subctx, datalen, sizeof(*gede));
			}
			break;
		}

		/*
		 * Print the GEDE header and get the full length (may
		 * vary from revision to revision of the GEDE) and the
		 * CPER report function if possible.
		 */
		apei_gede_report_header(sc, gede, subctx, ratelimitok,
		    &headerlen, &report);

		/*
		 * If we don't know the header length because of an
		 * unfamiliar revision, stop here.
		 */
		if (headerlen == 0) {
			if (ratelimitok) {
				device_printf(sc->sc_dev, "%s:"
				    " unknown revision: 0x%"PRIx16"\n",
				    subctx, gede->Revision);
			}
			break;
		}

		/*
		 * Stop here if what we mapped is too small for the
		 * error data length.
		 */
		datalen -= headerlen;
		if (datalen < gede->ErrorDataLength) {
			if (ratelimitok) {
				device_printf(sc->sc_dev, "%s:"
				    " truncated GEDE payload:"
				    " %"PRIu32" < %"PRIu32" bytes\n",
				    subctx, datalen, gede->ErrorDataLength);
			}
			break;
		}

		/*
		 * Report the Common Platform Error Record appendix to
		 * this Generic Error Data Entry.
		 */
		if (report == NULL) {
			if (ratelimitok) {
				device_printf(sc->sc_dev, "%s:"
				    " [unknown type]\n", ctx);
			}
		} else {
			/* XXX pass ratelimit through */
			(*report->func)(sc, (const char *)gede + headerlen,
			    gede->ErrorDataLength, subctx, ratelimitok);
		}

		/*
		 * Advance past the GEDE header and CPER data to the
		 * next GEDE.
		 */
		gede = (const ACPI_HEST_GENERIC_DATA *)((const char *)gede +
		    + headerlen + gede->ErrorDataLength);
	}

	/*
	 * Advance past the Generic Error Data Entries (GEDEs) to the
	 * raw error data.
	 *
	 * XXX Provide Max Raw Data Length as a parameter, as found in
	 * various HEST entry types.
	 */
	rawdata = (const unsigned char *)gede0 + datalen;

	/*
	 * Verify that the raw data length fits within the size.  If
	 * not, truncate the raw data.
	 */
	rawdatalen = gesb->RawDataLength;
	if (size < rawdatalen) {
		if (ratelimitok) {
			device_printf(sc->sc_dev, "%s:"
			    " GESB RawDataLength exceeds bounds:"
			    " %zu < %"PRIu32"\n",
			    ctx, size, rawdatalen);
		}
		rawdatalen = size;
	}
	size -= rawdatalen;

	/*
	 * Hexdump the raw data, if any.
	 */
	if (ratelimitok && rawdatalen > 0) {
		char devctx[128];

		snprintf(devctx, sizeof(devctx), "%s: %s: raw data",
		    device_xname(sc->sc_dev), ctx);
		hexdump(printf, devctx, rawdata, rawdatalen);
	}

	/*
	 * If there's anything left after the raw data, warn.
	 */
	if (ratelimitok && size > 0) {
		device_printf(sc->sc_dev, "%s: excess data: %zu bytes\n",
		    ctx, size);
	}

	/*
	 * Return the status so the caller can ack it, and tell the
	 * caller whether this error is fatal.
	 */
out:	*fatalp = fatal;
	return status;
}

MODULE(MODULE_CLASS_DRIVER, apei, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
apei_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_apei,
		    cfattach_ioconf_apei, cfdata_ioconf_apei);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_apei,
		    cfattach_ioconf_apei, cfdata_ioconf_apei);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
