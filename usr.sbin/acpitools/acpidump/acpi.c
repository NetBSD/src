/* $NetBSD: acpi.c,v 1.15.8.1 2017/11/22 15:54:09 martin Exp $ */

/*-
 * Copyright (c) 1998 Doug Rabson
 * Copyright (c) 2000 Mitsuru IWASAKI <iwasaki@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD: head/usr.sbin/acpi/acpidump/acpi.c 321299 2017-07-20 17:36:17Z emaste $
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: acpi.c,v 1.15.8.1 2017/11/22 15:54:09 martin Exp $");

#include <sys/param.h>
#include <sys/endian.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <uuid.h>

#include "acpidump.h"

#define BEGIN_COMMENT	"/*\n"
#define END_COMMENT	" */\n"

static void	acpi_print_string(char *s, size_t length);
static void	acpi_print_gas(ACPI_GENERIC_ADDRESS *gas);
static void	acpi_print_pci(uint16_t vendorid, uint16_t deviceid,
		    uint8_t seg, uint8_t bus, uint8_t device, uint8_t func);
static void	acpi_print_pci_sbdf(uint8_t seg, uint8_t bus, uint8_t device,
		    uint8_t func);
#ifdef notyet
static void	acpi_print_hest_generic_status(ACPI_HEST_GENERIC_STATUS *);
static void	acpi_print_hest_generic_data(ACPI_HEST_GENERIC_DATA *);
#endif
static void	acpi_print_whea(ACPI_WHEA_HEADER *whea,
		    void (*print_action)(ACPI_WHEA_HEADER *),
		    void (*print_ins)(ACPI_WHEA_HEADER *),
		    void (*print_flags)(ACPI_WHEA_HEADER *));
static uint64_t	acpi_select_address(uint32_t, uint64_t);
static void	acpi_handle_fadt(ACPI_TABLE_HEADER *fadt);
static void	acpi_print_cpu(u_char cpu_id);
static void	acpi_print_cpu_uid(uint32_t uid, char *uid_string);
static void	acpi_print_local_apic(uint32_t apic_id, uint32_t flags);
static void	acpi_print_io_apic(uint32_t apic_id, uint32_t int_base,
		    uint64_t apic_addr);
static void	acpi_print_mps_flags(uint16_t flags);
static void	acpi_print_intr(uint32_t intr, uint16_t mps_flags);
static void	acpi_print_local_nmi(u_int lint, uint16_t mps_flags);
static void	acpi_print_madt(ACPI_SUBTABLE_HEADER *mp);
static void	acpi_handle_bert(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_boot(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_cpep(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_dbgp(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_dbg2(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_einj(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_erst(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_hest(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_madt(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_msct(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_ecdt(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_hpet(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_mcfg(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_sbst(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_slit(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_spcr(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_spmi(ACPI_TABLE_HEADER *sdp);
static void	acpi_print_srat_cpu(uint32_t apic_id,
		    uint32_t proximity_domain,
		    uint32_t flags, uint32_t clockdomain);
static void	acpi_print_srat_memory(ACPI_SRAT_MEM_AFFINITY *mp);
static void	acpi_print_srat(ACPI_SUBTABLE_HEADER *srat);
static void	acpi_handle_srat(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_tcpa(ACPI_TABLE_HEADER *sdp);
static void	acpi_print_nfit(ACPI_NFIT_HEADER *nfit);
static void	acpi_handle_nfit(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_uefi(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_waet(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_wdat(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_wddt(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_wdrt(ACPI_TABLE_HEADER *sdp);
static void	acpi_print_sdt(ACPI_TABLE_HEADER *sdp);
static void	acpi_dump_bytes(ACPI_TABLE_HEADER *sdp);
static void	acpi_print_fadt(ACPI_TABLE_HEADER *sdp);
static void	acpi_print_facs(ACPI_TABLE_FACS *facs);
static void	acpi_print_dsdt(ACPI_TABLE_HEADER *dsdp);
static ACPI_TABLE_HEADER *acpi_map_sdt(vm_offset_t pa);
static void	acpi_print_rsd_ptr(ACPI_TABLE_RSDP *rp);
static void	acpi_handle_rsdt(ACPI_TABLE_HEADER *rsdp);
static void	acpi_walk_subtables(ACPI_TABLE_HEADER *table, void *first,
		    void (*action)(ACPI_SUBTABLE_HEADER *));
static void	acpi_walk_nfit(ACPI_TABLE_HEADER *table, void *first,
		    void (*action)(ACPI_NFIT_HEADER *));

/* Size of an address. 32-bit for ACPI 1.0, 64-bit for ACPI 2.0 and up. */
static int addr_size;

/* Strings used in the TCPA table */
static const char *tcpa_event_type_strings[] = {
	"PREBOOT Certificate",
	"POST Code",
	"Unused",
	"No Action",
	"Separator",
	"Action",
	"Event Tag",
	"S-CRTM Contents",
	"S-CRTM Version",
	"CPU Microcode",
	"Platform Config Flags",
	"Table of Devices",
	"Compact Hash",
	"IPL",
	"IPL Partition Data",
	"Non-Host Code",
	"Non-Host Config",
	"Non-Host Info"
};

static const char *TCPA_pcclient_strings[] = {
	"<undefined>",
	"SMBIOS",
	"BIS Certificate",
	"POST BIOS ROM Strings",
	"ESCD",
	"CMOS",
	"NVRAM",
	"Option ROM Execute",
	"Option ROM Configurateion",
	"<undefined>",
	"Option ROM Microcode Update ",
	"S-CRTM Version String",
	"S-CRTM Contents",
	"POST Contents",
	"Table of Devices",
};

#define	PRINTFLAG_END()		printflag_end()

static char pf_sep = '{';

static void
printflag_end(void)
{

	if (pf_sep == ',') {
		printf("}");
	} else if (pf_sep == '{') {
		printf("{}");
	}
	pf_sep = '{';
	printf("\n");
}

static void
printflag(uint64_t var, uint64_t mask, const char *name)
{

	if (var & mask) {
		printf("%c%s", pf_sep, name);
		pf_sep = ',';
	}
}

static void
acpi_print_string(char *s, size_t length)
{
	int	c;

	/* Trim trailing spaces and NULLs */
	while (length > 0 && (s[length - 1] == ' ' || s[length - 1] == '\0'))
		length--;

	while (length--) {
		c = *s++;
		putchar(c);
	}
}

static void
acpi_print_gas(ACPI_GENERIC_ADDRESS *gas)
{
	switch(gas->SpaceId) {
	case ACPI_ADR_SPACE_SYSTEM_MEMORY:
		if (gas->BitWidth <= 32)
			printf("0x%08x:%u[%u] (Memory)",
			    (u_int)gas->Address, gas->BitOffset,
			    gas->BitWidth);
		else
			printf("0x%016jx:%u[%u] (Memory)",
			    (uintmax_t)gas->Address, gas->BitOffset,
			    gas->BitWidth);
		break;
	case ACPI_ADR_SPACE_SYSTEM_IO:
		printf("0x%02x:%u[%u] (IO)", (u_int)gas->Address,
		    gas->BitOffset, gas->BitWidth);
		break;
	case ACPI_ADR_SPACE_PCI_CONFIG:
		printf("%x:%x+0x%x (PCI)", (uint16_t)(gas->Address >> 32),
		       (uint16_t)((gas->Address >> 16) & 0xffff),
		       (uint16_t)gas->Address);
		break;
	/* XXX How to handle these below? */
	case ACPI_ADR_SPACE_EC:
		printf("0x%x:%u[%u] (EC)", (uint16_t)gas->Address,
		       gas->BitOffset, gas->BitWidth);
		break;
	case ACPI_ADR_SPACE_SMBUS:
		printf("0x%x:%u[%u] (SMBus)", (uint16_t)gas->Address,
		       gas->BitOffset, gas->BitWidth);
		break;
	case ACPI_ADR_SPACE_CMOS:
	case ACPI_ADR_SPACE_PCI_BAR_TARGET:
	case ACPI_ADR_SPACE_IPMI:
	case ACPI_ADR_SPACE_GPIO:
	case ACPI_ADR_SPACE_GSBUS:
	case ACPI_ADR_SPACE_PLATFORM_COMM:
	case ACPI_ADR_SPACE_FIXED_HARDWARE:
	default:
		printf("0x%016jx (SpaceID=%hhu)", (uintmax_t)gas->Address,
		    gas->SpaceId);
		break;
	}
}

static void
acpi_print_pci(uint16_t vendorid, uint16_t deviceid,
    uint8_t seg, uint8_t bus, uint8_t device, uint8_t func)
{
	if (vendorid == 0xffff && deviceid == 0xffff) {
		printf("\tPCI Device=NONE\n");
		return;
	}

	printf("\tPCI device={\n");
	printf("\t\tVendor=0x%x\n", vendorid);
	printf("\t\tDevice=0x%x\n", deviceid);
	printf("\n");
	printf("\t\tSegment Group=%d\n", seg);
	printf("\t\tBus=%d\n", bus);
	printf("\t\tDevice=%d\n", device);
	printf("\t\tFunction=%d\n", func);
	printf("\t}\n");
}

static void
acpi_print_pci_sbdf(uint8_t seg, uint8_t bus, uint8_t device, uint8_t func)
{
	if (bus == 0xff && device == 0xff && func == 0xff) {
		printf("\tPCI Device=NONE\n");
		return;
	}

	printf("\tPCI device={\n");
	printf("\t\tSegment Group=%d\n", seg);
	printf("\t\tBus=%d\n", bus);
	printf("\t\tDevice=%d\n", device);
	printf("\t\tFunction=%d\n", func);
	printf("\t}\n");
}

#ifdef notyet
static void
acpi_print_hest_errorseverity(uint32_t error)
{
	printf("\tError Severity={ ");
	switch (error) {
	case 0:
		printf("Recoverable");
		break;
	case 1:
		printf("Fatal");
		break;
	case 2:
		printf("Corrected");
		break;
	case 3:
		printf("None");
		break;
	default:
		printf("%d (reserved)", error);
		break;
	}
	printf("}\n");
}
#endif

static void
acpi_print_hest_errorbank(ACPI_HEST_IA_ERROR_BANK *bank)
{
	printf("\n");
	printf("\tBank Number=%d\n", bank->BankNumber);
	printf("\tClear Status On Init={%s}\n",
		bank->ClearStatusOnInit ? "NO" : "YES");
	printf("\tStatus Data Format={ ");
	switch (bank->StatusFormat) {
	case 0:
		printf("IA32 MCA");
		break;
	case 1:
		printf("EMT64 MCA");
		break;
	case 2:
		printf("AMD64 MCA");
		break;
	}
	printf(" }\n");

	if (bank->ControlRegister)
		printf("\tControl Register=0x%x\n", bank->ControlRegister);
	printf("\tControl Init Data=0x%"PRIx64"\n", bank->ControlData);
	printf("\tStatus MSR=0x%x\n", bank->StatusRegister);
	printf("\tAddress MSR=0x%x\n", bank->AddressRegister);
	printf("\tMisc MSR=0x%x\n", bank->MiscRegister);
}

static void
acpi_print_hest_header(ACPI_HEST_HEADER *hest)
{
	printf("\tType={");
	switch (hest->Type) {
	case ACPI_HEST_TYPE_IA32_CHECK:
		printf("IA32 Machine Check Exception");
		break;
	case ACPI_HEST_TYPE_IA32_CORRECTED_CHECK:
		printf("IA32 Corrected Machine Check");
		break;
	case ACPI_HEST_TYPE_IA32_NMI:
		printf("IA32 Non-Maskable Interrupt");
		break;
	case ACPI_HEST_TYPE_NOT_USED3:
	case ACPI_HEST_TYPE_NOT_USED4:
	case ACPI_HEST_TYPE_NOT_USED5:
		printf("unused type: %d", hest->Type);
		break;
	case ACPI_HEST_TYPE_AER_ROOT_PORT:
		printf("PCI Express Root Port AER");
		break;
	case ACPI_HEST_TYPE_AER_ENDPOINT:
		printf("PCI Express Endpoint AER");
		break;
	case ACPI_HEST_TYPE_AER_BRIDGE:
		printf("PCI Express/PCI-X Bridge AER");
		break;
	case ACPI_HEST_TYPE_GENERIC_ERROR:
		printf("Generic Hardware Error Source");
		break;
	case ACPI_HEST_TYPE_GENERIC_ERROR_V2:
		printf("Generic Hardware Error Source version 2");
		break;
	case ACPI_HEST_TYPE_RESERVED:
	default:
		printf("Reserved (%d)", hest->Type);
		break;
	}
	printf("}\n");
	printf("\tSourceId=%d\n", hest->SourceId);
}

static void
acpi_print_hest_aer_common(ACPI_HEST_AER_COMMON *data)
{
	printf("\tFlags={ ");
	if (data->Flags & ACPI_HEST_FIRMWARE_FIRST)
		printf("FIRMWARE_FIRST");
	if (data->Flags & ACPI_HEST_GLOBAL)
		printf("GLOBAL");
	printf(" }\n");
	printf("\tEnabled={ %s ", data->Flags ? "YES" : "NO");
	if (data->Flags & ACPI_HEST_FIRMWARE_FIRST)
		printf("(ignored) ");
	printf("}\n");
	printf("\tNumber of Record to pre-allocate=%d\n",
		data->RecordsToPreallocate);
	printf("\tMax. Sections per Record=%d\n", data->MaxSectionsPerRecord);
	if (!(data->Flags & ACPI_HEST_GLOBAL))
		acpi_print_pci_sbdf(0, data->Bus, data->Device, data->Function);
	printf("\tDevice Control=0x%x\n", data->DeviceControl);
	printf("\tUncorrectable Error Mask Register=0x%x\n",
		data->UncorrectableMask);
	printf("\tUncorrectable Error Severity Register=0x%x\n",
		data->UncorrectableSeverity);
	printf("\tCorrectable Error Mask Register=0x%x\n",
		data->CorrectableMask);
	printf("\tAdvanced Capabilities Register=0x%x\n",
		data->AdvancedCapabilities);
}

static void
acpi_print_hest_notify(ACPI_HEST_NOTIFY *notify)
{
	printf("\tHW Error Notification={\n");
	printf("\t\tType={");
	switch (notify->Type) {
	case ACPI_HEST_NOTIFY_POLLED:
		printf("POLLED");
		break;
	case ACPI_HEST_NOTIFY_EXTERNAL:
		printf("EXTERN");
		break;
	case ACPI_HEST_NOTIFY_LOCAL:
		printf("LOCAL");
		break;
	case ACPI_HEST_NOTIFY_SCI:
		printf("SCI");
		break;
	case ACPI_HEST_NOTIFY_NMI:
		printf("NMI");
		break;
	case ACPI_HEST_NOTIFY_CMCI:
		printf("CMCI");
		break;
	case ACPI_HEST_NOTIFY_MCE:
		printf("MCE");
		break;
	case ACPI_HEST_NOTIFY_GPIO:
		printf("GPIO-Signal");
		break;
	case ACPI_HEST_NOTIFY_SEA:
		printf("ARMv8 SEA");
		break;
	case ACPI_HEST_NOTIFY_SEI:
		printf("ARMv8 SEI");
		break;
	case ACPI_HEST_NOTIFY_GSIV:
		printf("External Interrupt - GSIV");
		break;
	case ACPI_HEST_NOTIFY_RESERVED:
		printf("RESERVED");
		break;
	default:
		printf("%d (reserved)", notify->Type);
		break;
	}
	printf("}\n");

	printf("\t\tLength=%d\n", notify->Length);

#define PRINTFLAG(var, flag)	printflag((var), ACPI_HEST_## flag, #flag)

	printf("\t\tConfig Write Enable=");
	PRINTFLAG(notify->ConfigWriteEnable, TYPE);
	PRINTFLAG(notify->ConfigWriteEnable, POLL_INTERVAL);
	PRINTFLAG(notify->ConfigWriteEnable, POLL_THRESHOLD_VALUE);
	PRINTFLAG(notify->ConfigWriteEnable, POLL_THRESHOLD_WINDOW);
	PRINTFLAG(notify->ConfigWriteEnable, ERR_THRESHOLD_VALUE);
	PRINTFLAG(notify->ConfigWriteEnable, ERR_THRESHOLD_WINDOW);
	PRINTFLAG_END();

#undef PRINTFLAG

	printf("\t\tPoll Interval=%d msec\n", notify->PollInterval);
	printf("\t\tInterrupt Vector=%d\n", notify->Vector);
	printf("\t\tSwitch To Polling Threshold Value=%d\n",
		notify->PollingThresholdValue);
	printf("\t\tSwitch To Polling Threshold Window=%d msec\n",
		notify->PollingThresholdWindow);
	printf("\t\tError Threshold Value=%d\n",
		notify->ErrorThresholdValue);
	printf("\t\tError Threshold Window=%d msec\n",
		notify->ErrorThresholdWindow);
	printf("\t}\n");
}

#ifdef notyet
static void
acpi_print_hest_generic_status(ACPI_HEST_GENERIC_STATUS *data)
{
	uint32_t i, pos, entries;
	ACPI_HEST_GENERIC_DATA *gen;

	entries = data->BlockStatus & ACPI_HEST_ERROR_ENTRY_COUNT;

	printf("\tGeneric Error Status={\n");
	printf("\t\tBlock Status={ ");
	if (data->BlockStatus & ACPI_HEST_UNCORRECTABLE)
		printf("UNCORRECTABLE");
	if (data->BlockStatus & ACPI_HEST_CORRECTABLE)
		printf("CORRECTABLE");
	if (data->BlockStatus & ACPI_HEST_MULTIPLE_UNCORRECTABLE)
		printf("MULTIPLE UNCORRECTABLE");
	if (data->BlockStatus & ACPI_HEST_MULTIPLE_CORRECTABLE)
		printf("MULTIPLE CORRECTABLE");
	printf(" }\n");
	printf("\t\tEntry Count=%d\n", entries);
	printf("\t\tRaw Data Offset=%d\n", data->RawDataOffset);
	printf("\t\tRaw Data Length=%d\n", data->RawDataLength);
	printf("\t\tData Length=%d\n", data->DataLength);
	printf("\t");
	acpi_print_hest_errorseverity(data->ErrorSeverity);
	printf("\t}\n");

	pos = sizeof(ACPI_HEST_GENERIC_STATUS);
	for (i = 0; i < entries; i++) {
		gen = (ACPI_HEST_GENERIC_DATA *)((char *)data + pos);
		acpi_print_hest_generic_data(gen);
		pos += sizeof(ACPI_HEST_GENERIC_DATA);
	}
}
#endif

#ifdef notyet
static void
acpi_print_hest_generic_data(ACPI_HEST_GENERIC_DATA *data)
{
	printf("\tGeneric Error Data={\n");
	printf("\t\tSectionType=");
	acpi_print_string((char *)data->SectionType, sizeof(data->SectionType));
	printf("\n\t");
	acpi_print_hest_errorseverity(data->ErrorSeverity);
	printf("\t\tRevision=0x%x\n", data->Revision);
	printf("\t\tValidation Bits=0x%x\n", data->ValidationBits);
	printf("\t\tFlags=0x%x\n", data->Flags);
	printf("\t\tData Length=%d\n", data->ErrorDataLength);
	printf("\t\tField Replication Unit Id=");
	acpi_print_string((char *)data->FruId, sizeof(data->FruId));
	printf("\n");
	printf("\t\tField Replication Unit=");
	acpi_print_string((char *)data->FruText, sizeof(data->FruText));
	printf("\n");
	printf("\t}\n");
}
#endif

static void
acpi_print_whea(ACPI_WHEA_HEADER *whea,
    void (*print_action)(ACPI_WHEA_HEADER *),
    void (*print_ins)(ACPI_WHEA_HEADER *),
    void (*print_flags)(ACPI_WHEA_HEADER *))
{
	printf("\n");

	print_action(whea);
	print_ins(whea);
	if (print_flags)
		print_flags(whea);
	printf("\tRegisterRegion=");
	acpi_print_gas(&whea->RegisterRegion);
	printf("\n");
	printf("\tMASK=0x%08"PRIx64"\n", whea->Mask);
}

static void
acpi_print_hest_ia32_check(ACPI_HEST_IA_MACHINE_CHECK *data)
{
	uint32_t i, pos;
	ACPI_HEST_IA_ERROR_BANK *bank;

	acpi_print_hest_header(&data->Header);
	printf("\tFlags={ ");
	if (data->Flags & ACPI_HEST_FIRMWARE_FIRST)
		printf("FIRMWARE_FIRST");
	printf(" }\n");
	printf("\tEnabled={ %s }\n", data->Enabled ? "YES" : "NO");
	printf("\tNumber of Record to pre-allocate=%d\n",
		data->RecordsToPreallocate);
	printf("\tMax Sections per Record=%d\n",
		data->MaxSectionsPerRecord);
	printf("\tGlobal Capability Init Data=0x%"PRIx64"\n",
		data->GlobalCapabilityData);
	printf("\tGlobal Control Init Data=0x%"PRIx64"\n",
		data->GlobalControlData);
	printf("\tNumber of Hardware Error Reporting Banks=%d\n",
		data->NumHardwareBanks);

	pos = sizeof(ACPI_HEST_IA_MACHINE_CHECK);
	for (i = 0; i < data->NumHardwareBanks; i++) {
		bank = (ACPI_HEST_IA_ERROR_BANK *)((char *)data + pos);
		acpi_print_hest_errorbank(bank);
		pos += sizeof(ACPI_HEST_IA_ERROR_BANK);
	}
}

static void
acpi_print_hest_ia32_correctedcheck(ACPI_HEST_IA_CORRECTED *data)
{
	uint32_t i, pos;
	ACPI_HEST_IA_ERROR_BANK *bank;

	acpi_print_hest_header(&data->Header);
	printf("\tFlags={ ");
	if (data->Flags & ACPI_HEST_FIRMWARE_FIRST)
		printf("FIRMWARE_FIRST");
	printf(" }\n");
	printf("\tEnabled={ %s }\n", data->Enabled ? "YES" : "NO");
	printf("\tNumber of Record to pre-allocate=%d\n",
		data->RecordsToPreallocate);
	printf("\tMax Sections per Record=%d\n",
		data->MaxSectionsPerRecord);
	acpi_print_hest_notify(&data->Notify);

	printf("\tNumber of Hardware Error Reporting Banks=%d\n",
		data->NumHardwareBanks);

	pos = sizeof(ACPI_HEST_IA_MACHINE_CHECK);
	for (i = 0; i < data->NumHardwareBanks; i++) {
		bank = (ACPI_HEST_IA_ERROR_BANK *)((char *)data + pos);
		acpi_print_hest_errorbank(bank);
		pos += sizeof(ACPI_HEST_IA_ERROR_BANK);
	}
}

static void
acpi_print_hest_ia32_nmi(ACPI_HEST_IA_NMI *data)
{
	acpi_print_hest_header(&data->Header);
	printf("\tNumber of Record to pre-allocate=%d\n",
		data->RecordsToPreallocate);
	printf("\tMax Sections per Record=%d\n",
		data->MaxSectionsPerRecord);
	printf("\tMax Raw Data Length=%d\n",
		data->MaxRawDataLength);
}

static void
acpi_print_hest_aer_root(ACPI_HEST_AER_ROOT *data)
{
	acpi_print_hest_header(&data->Header);
	acpi_print_hest_aer_common(&data->Aer);
	printf("Root Error Command Register=0x%x\n", data->RootErrorCommand);
}

static void
acpi_print_hest_aer_endpoint(ACPI_HEST_AER *data)
{
	acpi_print_hest_header(&data->Header);
	acpi_print_hest_aer_common(&data->Aer);
}

static void
acpi_print_hest_aer_bridge(ACPI_HEST_AER_BRIDGE *data)
{
	acpi_print_hest_header(&data->Header);
	acpi_print_hest_aer_common(&data->Aer);

	printf("\tSecondary Uncorrectable Error Mask Register=0x%x\n",
		data->UncorrectableMask2);
	printf("\tSecondary Uncorrectable Error Severity Register=0x%x\n",
		data->UncorrectableSeverity2);
	printf("\tSecondory Advanced Capabilities Register=0x%x\n",
		data->AdvancedCapabilities2);
}

static void
acpi_print_hest_generic(ACPI_HEST_GENERIC *data)
{
	acpi_print_hest_header(&data->Header);
	if (data->RelatedSourceId != 0xffff)
		printf("\tReleated SourceId=%d\n", data->RelatedSourceId);
	printf("\tEnabled={%s}\n", data->Enabled ? "YES" : "NO");
	printf("\tNumber of Records to pre-allocate=%u\n",
		data->RecordsToPreallocate);
	printf("\tMax Sections per Record=%u\n", data->MaxSectionsPerRecord);
	printf("\tMax Raw Data Length=%u\n", data->MaxRawDataLength);
	printf("\tError Status Address=");
	acpi_print_gas(&data->ErrorStatusAddress);
	printf("\n");
	acpi_print_hest_notify(&data->Notify);
	printf("\tError Block Length=%u\n", data->ErrorBlockLength);
}

static void
acpi_print_hest_generic_v2(ACPI_HEST_GENERIC_V2 *data)
{

	/* The first 64 bytes are the same as ACPI_HEST_GENERIC */
	acpi_print_hest_generic((ACPI_HEST_GENERIC *)data);

	printf("\tError Status Address");
	acpi_print_gas(&data->ReadAckRegister);
	printf("\tRead Ack Preserve=0x%016jx\n",
	    (uintmax_t)data->ReadAckPreserve);
	printf("\tRead Ack Write=0x%016jx\n",
	    (uintmax_t)data->ReadAckWrite);
}

static void
acpi_handle_hest(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_HEST *hest;
	ACPI_HEST_HEADER *subhest;
	uint32_t i, pos;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	hest = (ACPI_TABLE_HEST *)sdp;

	printf("\tError Source Count=%d\n", hest->ErrorSourceCount);
	pos = sizeof(ACPI_TABLE_HEST);
	for (i = 0; i < hest->ErrorSourceCount; i++) {
		subhest = (ACPI_HEST_HEADER *)((char *)hest + pos);
		printf("\n");

		switch (subhest->Type) {
		case ACPI_HEST_TYPE_IA32_CHECK:
			acpi_print_hest_ia32_check(
				(ACPI_HEST_IA_MACHINE_CHECK *)subhest);
			pos += sizeof(ACPI_HEST_IA_MACHINE_CHECK);
			break;

		case ACPI_HEST_TYPE_IA32_CORRECTED_CHECK:
			acpi_print_hest_ia32_correctedcheck(
				(ACPI_HEST_IA_CORRECTED *)subhest);
			pos += sizeof(ACPI_HEST_IA_CORRECTED);
			break;

		case ACPI_HEST_TYPE_IA32_NMI:
			acpi_print_hest_ia32_nmi(
				(ACPI_HEST_IA_NMI *)subhest);
			pos += sizeof(ACPI_HEST_IA_NMI);
			break;

		case ACPI_HEST_TYPE_NOT_USED3:
		case ACPI_HEST_TYPE_NOT_USED4:
		case ACPI_HEST_TYPE_NOT_USED5:
			pos += sizeof(ACPI_HEST_HEADER);
			break;

		case ACPI_HEST_TYPE_AER_ROOT_PORT:
			acpi_print_hest_aer_root((ACPI_HEST_AER_ROOT *)subhest);
			pos += sizeof(ACPI_HEST_AER_ROOT);
			break;

		case ACPI_HEST_TYPE_AER_ENDPOINT:
			acpi_print_hest_aer_endpoint((ACPI_HEST_AER *)subhest);
			pos += sizeof(ACPI_HEST_AER);
			break;

		case ACPI_HEST_TYPE_AER_BRIDGE:
			acpi_print_hest_aer_bridge((ACPI_HEST_AER_BRIDGE *)subhest);
			pos += sizeof(ACPI_HEST_AER_BRIDGE);
			break;

		case ACPI_HEST_TYPE_GENERIC_ERROR:
			acpi_print_hest_generic((ACPI_HEST_GENERIC *)subhest);
			pos += sizeof(ACPI_HEST_GENERIC);
			break;

		case ACPI_HEST_TYPE_GENERIC_ERROR_V2:
			acpi_print_hest_generic_v2(
				(ACPI_HEST_GENERIC_V2 *)subhest);
			pos += sizeof(ACPI_HEST_GENERIC_V2);
			break;

		case ACPI_HEST_TYPE_RESERVED:
		default:
			pos += sizeof(ACPI_HEST_HEADER);
			break;
		}
	}

	printf(END_COMMENT);
}

static uint64_t
acpi_select_address(uint32_t addr32, uint64_t addr64)
{

	if (addr64 == 0)
		return addr32;

	if ((addr32 != 0) && ((addr64 & 0xfffffff) != addr32)) {
		/*
		 * A few systems (e.g., IBM T23) have an RSDP that claims
		 * revision 2 but the 64 bit addresses are invalid.  If
		 * revision 2 and the 32 bit address is non-zero but the
		 * 32 and 64 bit versions don't match, prefer the 32 bit
		 * version for all subsequent tables.
		 */
		return addr32;
	}

	return addr64;
}

static void
acpi_handle_fadt(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_HEADER *dsdp;
	ACPI_TABLE_FACS	*facs;
	ACPI_TABLE_FADT *fadt;

	fadt = (ACPI_TABLE_FADT *)sdp;
	acpi_print_fadt(sdp);

	facs = (ACPI_TABLE_FACS *)acpi_map_sdt(
		acpi_select_address(fadt->Facs, fadt->XFacs));
	if (memcmp(facs->Signature, ACPI_SIG_FACS, 4) != 0 || facs->Length < 64)
		errx(EXIT_FAILURE, "FACS is corrupt");
	acpi_print_facs(facs);

	dsdp = (ACPI_TABLE_HEADER *)acpi_map_sdt(
		acpi_select_address(fadt->Dsdt, fadt->XDsdt));
	if (memcmp(dsdp->Signature, ACPI_SIG_DSDT, 4) != 0)
		errx(EXIT_FAILURE, "DSDT signature mismatch");
	if (acpi_checksum(dsdp, dsdp->Length))
		errx(EXIT_FAILURE, "DSDT is corrupt");
	acpi_print_dsdt(dsdp);
}

static void
acpi_walk_subtables(ACPI_TABLE_HEADER *table, void *first,
    void (*action)(ACPI_SUBTABLE_HEADER *))
{
	ACPI_SUBTABLE_HEADER *subtable;
	char *end;

	subtable = first;
	end = (char *)table + table->Length;
	while ((char *)subtable < end) {
		printf("\n");
		if (subtable->Length < sizeof(ACPI_SUBTABLE_HEADER)) {
			warnx("invalid subtable length %u", subtable->Length);
			return;
		}
		action(subtable);
		subtable = (ACPI_SUBTABLE_HEADER *)((char *)subtable +
		    subtable->Length);
	}
}

static void
acpi_walk_nfit(ACPI_TABLE_HEADER *table, void *first,
    void (*action)(ACPI_NFIT_HEADER *))
{
	ACPI_NFIT_HEADER *subtable;
	char *end;

	subtable = first;
	end = (char *)table + table->Length;
	while ((char *)subtable < end) {
		printf("\n");
		if (subtable->Length < sizeof(ACPI_NFIT_HEADER)) {
			warnx("invalid subtable length %u", subtable->Length);
			return;
		}
		action(subtable);
		subtable = (ACPI_NFIT_HEADER *)((char *)subtable +
		    subtable->Length);
	}
}

static void
acpi_print_cpu(u_char cpu_id)
{

	printf("\tACPI CPU=");
	if (cpu_id == 0xff)
		printf("ALL\n");
	else
		printf("%d\n", (u_int)cpu_id);
}

static void
acpi_print_cpu_uid(uint32_t uid, char *uid_string)
{

	printf("\tUID=%d", uid);
	if (uid_string != NULL)
		printf(" (%s)", uid_string);
	printf("\n");
}

static void
acpi_print_local_apic(uint32_t apic_id, uint32_t flags)
{

	printf("\tFlags={");
	if (flags & ACPI_MADT_ENABLED)
		printf("ENABLED");
	else
		printf("DISABLED");
	printf("}\n");
	printf("\tAPIC ID=%d\n", apic_id);
}

static void
acpi_print_io_apic(uint32_t apic_id, uint32_t int_base, uint64_t apic_addr)
{

	printf("\tAPIC ID=%d\n", apic_id);
	printf("\tINT BASE=%d\n", int_base);
	printf("\tADDR=0x%016jx\n", (uintmax_t)apic_addr);
}

static void
acpi_print_mps_flags(uint16_t flags)
{

	printf("\tFlags={Polarity=");
	switch (flags & ACPI_MADT_POLARITY_MASK) {
	case ACPI_MADT_POLARITY_CONFORMS:
		printf("conforming");
		break;
	case ACPI_MADT_POLARITY_ACTIVE_HIGH:
		printf("active-hi");
		break;
	case ACPI_MADT_POLARITY_ACTIVE_LOW:
		printf("active-lo");
		break;
	default:
		printf("0x%x", flags & ACPI_MADT_POLARITY_MASK);
		break;
	}
	printf(", Trigger=");
	switch (flags & ACPI_MADT_TRIGGER_MASK) {
	case ACPI_MADT_TRIGGER_CONFORMS:
		printf("conforming");
		break;
	case ACPI_MADT_TRIGGER_EDGE:
		printf("edge");
		break;
	case ACPI_MADT_TRIGGER_LEVEL:
		printf("level");
		break;
	default:
		printf("0x%x", (flags & ACPI_MADT_TRIGGER_MASK) >> 2);
	}
	printf("}\n");
}

static void
acpi_print_gicc_flags(uint32_t flags)
{

	printf("\tFlags={Performance intr=");
	if (flags & ACPI_MADT_PERFORMANCE_IRQ_MODE)
		printf("edge");
	else
		printf("level");
	printf(", VGIC intr=");
	if (flags & ACPI_MADT_VGIC_IRQ_MODE)
		printf("edge");
	else
		printf("level");
	printf("}\n");
}

static void
acpi_print_intr(uint32_t intr, uint16_t mps_flags)
{

	printf("\tINTR=%d\n", intr);
	acpi_print_mps_flags(mps_flags);
}

static void
acpi_print_local_nmi(u_int lint, uint16_t mps_flags)
{

	printf("\tLINT Pin=%d\n", lint);
	acpi_print_mps_flags(mps_flags);
}

static const char *apic_types[] = {
    [ACPI_MADT_TYPE_LOCAL_APIC] = "Local APIC",
    [ACPI_MADT_TYPE_IO_APIC] = "IO APIC",
    [ACPI_MADT_TYPE_INTERRUPT_OVERRIDE] = "INT Override",
    [ACPI_MADT_TYPE_NMI_SOURCE] = "NMI",
    [ACPI_MADT_TYPE_LOCAL_APIC_NMI] = "Local APIC NMI",
    [ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE] = "Local APIC Override",
    [ACPI_MADT_TYPE_IO_SAPIC] = "IO SAPIC",
    [ACPI_MADT_TYPE_LOCAL_SAPIC] = "Local SAPIC",
    [ACPI_MADT_TYPE_INTERRUPT_SOURCE] = "Platform Interrupt",
    [ACPI_MADT_TYPE_LOCAL_X2APIC] = "Local X2APIC",
    [ACPI_MADT_TYPE_LOCAL_X2APIC_NMI] = "Local X2APIC NMI",
    [ACPI_MADT_TYPE_GENERIC_INTERRUPT] = "GIC CPU Interface Structure",
    [ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR] = "GIC Distributor Structure",
    [ACPI_MADT_TYPE_GENERIC_MSI_FRAME] = "GICv2m MSI Frame",
    [ACPI_MADT_TYPE_GENERIC_REDISTRIBUTOR] = "GIC Redistributor Structure",
    [ACPI_MADT_TYPE_GENERIC_TRANSLATOR] = "GIC ITS Structure"
};

static const char *platform_int_types[] = { "0 (unknown)", "PMI", "INIT",
					    "Corrected Platform Error" };

static void
acpi_print_gicm_flags(ACPI_MADT_GENERIC_MSI_FRAME *gicm)
{
	uint32_t flags = gicm->Flags;

	printf("\tFLAGS={");
	if (flags & ACPI_MADT_OVERRIDE_SPI_VALUES)
		printf("SPI Count/Base Select");
	printf("}\n");
}

static void
acpi_print_madt(ACPI_SUBTABLE_HEADER *mp)
{
	ACPI_MADT_LOCAL_APIC *lapic;
	ACPI_MADT_IO_APIC *ioapic;
	ACPI_MADT_INTERRUPT_OVERRIDE *over;
	ACPI_MADT_NMI_SOURCE *nmi;
	ACPI_MADT_LOCAL_APIC_NMI *lapic_nmi;
	ACPI_MADT_LOCAL_APIC_OVERRIDE *lapic_over;
	ACPI_MADT_IO_SAPIC *iosapic;
	ACPI_MADT_LOCAL_SAPIC *lsapic;
	ACPI_MADT_INTERRUPT_SOURCE *isrc;
	ACPI_MADT_LOCAL_X2APIC *x2apic;
	ACPI_MADT_LOCAL_X2APIC_NMI *x2apic_nmi;
	ACPI_MADT_GENERIC_INTERRUPT *gicc;
	ACPI_MADT_GENERIC_DISTRIBUTOR *gicd;
	ACPI_MADT_GENERIC_MSI_FRAME *gicm;
	ACPI_MADT_GENERIC_REDISTRIBUTOR *gicr;
	ACPI_MADT_GENERIC_TRANSLATOR *gict;

	if (mp->Type < __arraycount(apic_types))
		printf("\tType=%s\n", apic_types[mp->Type]);
	else
		printf("\tType=%d (unknown)\n", mp->Type);
	switch (mp->Type) {
	case ACPI_MADT_TYPE_LOCAL_APIC:
		lapic = (ACPI_MADT_LOCAL_APIC *)mp;
		acpi_print_cpu(lapic->ProcessorId);
		acpi_print_local_apic(lapic->Id, lapic->LapicFlags);
		break;
	case ACPI_MADT_TYPE_IO_APIC:
		ioapic = (ACPI_MADT_IO_APIC *)mp;
		acpi_print_io_apic(ioapic->Id, ioapic->GlobalIrqBase,
		    ioapic->Address);
		break;
	case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
		over = (ACPI_MADT_INTERRUPT_OVERRIDE *)mp;
		printf("\tBUS=%d\n", (u_int)over->Bus);
		printf("\tIRQ=%d\n", (u_int)over->SourceIrq);
		acpi_print_intr(over->GlobalIrq, over->IntiFlags);
		break;
	case ACPI_MADT_TYPE_NMI_SOURCE:
		nmi = (ACPI_MADT_NMI_SOURCE *)mp;
		acpi_print_intr(nmi->GlobalIrq, nmi->IntiFlags);
		break;
	case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
		lapic_nmi = (ACPI_MADT_LOCAL_APIC_NMI *)mp;
		acpi_print_cpu(lapic_nmi->ProcessorId);
		acpi_print_local_nmi(lapic_nmi->Lint, lapic_nmi->IntiFlags);
		break;
	case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE:
		lapic_over = (ACPI_MADT_LOCAL_APIC_OVERRIDE *)mp;
		printf("\tLocal APIC ADDR=0x%016jx\n",
		    (uintmax_t)lapic_over->Address);
		break;
	case ACPI_MADT_TYPE_IO_SAPIC:
		iosapic = (ACPI_MADT_IO_SAPIC *)mp;
		acpi_print_io_apic(iosapic->Id, iosapic->GlobalIrqBase,
		    iosapic->Address);
		break;
	case ACPI_MADT_TYPE_LOCAL_SAPIC:
		lsapic = (ACPI_MADT_LOCAL_SAPIC *)mp;
		acpi_print_cpu(lsapic->ProcessorId);
		acpi_print_local_apic(lsapic->Id, lsapic->LapicFlags);
		printf("\tAPIC EID=%d\n", (u_int)lsapic->Eid);
		if (mp->Length > offsetof(ACPI_MADT_LOCAL_SAPIC, Uid))
			acpi_print_cpu_uid(lsapic->Uid, lsapic->UidString);
		break;
	case ACPI_MADT_TYPE_INTERRUPT_SOURCE:
		isrc = (ACPI_MADT_INTERRUPT_SOURCE *)mp;
		if (isrc->Type < __arraycount(platform_int_types))
			printf("\tType=%s\n", platform_int_types[isrc->Type]);
		else
			printf("\tType=%d (unknown)\n", isrc->Type);
		printf("\tAPIC ID=%d\n", (u_int)isrc->Id);
		printf("\tAPIC EID=%d\n", (u_int)isrc->Eid);
		printf("\tSAPIC Vector=%d\n", (u_int)isrc->IoSapicVector);
		acpi_print_intr(isrc->GlobalIrq, isrc->IntiFlags);
		break;
	case ACPI_MADT_TYPE_LOCAL_X2APIC:
		x2apic = (ACPI_MADT_LOCAL_X2APIC *)mp;
		acpi_print_cpu_uid(x2apic->Uid, NULL);
		acpi_print_local_apic(x2apic->LocalApicId, x2apic->LapicFlags);
		break;
	case ACPI_MADT_TYPE_LOCAL_X2APIC_NMI:
		x2apic_nmi = (ACPI_MADT_LOCAL_X2APIC_NMI *)mp;
		acpi_print_cpu_uid(x2apic_nmi->Uid, NULL);
		acpi_print_local_nmi(x2apic_nmi->Lint, x2apic_nmi->IntiFlags);
		break;
	case ACPI_MADT_TYPE_GENERIC_INTERRUPT:
		gicc = (ACPI_MADT_GENERIC_INTERRUPT *)mp;
		acpi_print_cpu_uid(gicc->Uid, NULL);
		printf("\tCPU INTERFACE=%x\n", gicc->CpuInterfaceNumber);
		acpi_print_gicc_flags(gicc->Flags);
		printf("\tParking Protocol Version=%x\n", gicc->ParkingVersion);
		printf("\tPERF INTR=%d\n", gicc->PerformanceInterrupt);
		printf("\tParked ADDR=%016jx\n",
		    (uintmax_t)gicc->ParkedAddress);
		printf("\tBase ADDR=%016jx\n", (uintmax_t)gicc->BaseAddress);
		printf("\tGICV=%016jx\n", (uintmax_t)gicc->GicvBaseAddress);
		printf("\tGICH=%016jx\n", (uintmax_t)gicc->GichBaseAddress);
		printf("\tVGIC INTR=%d\n", gicc->VgicInterrupt);
		printf("\tGICR ADDR=%016jx\n",
		    (uintmax_t)gicc->GicrBaseAddress);
		printf("\tMPIDR=%jx\n", (uintmax_t)gicc->ArmMpidr);
		printf("\tEfficency Class=%d\n", (u_int)gicc->EfficiencyClass);
		break;
	case ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR:
		gicd = (ACPI_MADT_GENERIC_DISTRIBUTOR *)mp;
		printf("\tGIC ID=%d\n", (u_int)gicd->GicId);
		printf("\tBase ADDR=%016jx\n", (uintmax_t)gicd->BaseAddress);
		printf("\tVector Base=%d\n", gicd->GlobalIrqBase);
		printf("\tGIC VERSION=%d\n", (u_int)gicd->Version);
		break;
	case ACPI_MADT_TYPE_GENERIC_MSI_FRAME:
		gicm = (ACPI_MADT_GENERIC_MSI_FRAME*)mp;
		printf("\tBase ADDR=%016jx\n", (uintmax_t)gicm->BaseAddress);
		acpi_print_gicm_flags(gicm);
		printf("\tSPI Count=%u\n", gicm->SpiCount);
		printf("\tSPI Base=%u\n", gicm->SpiBase);
		break;
	case ACPI_MADT_TYPE_GENERIC_REDISTRIBUTOR:
		gicr = (ACPI_MADT_GENERIC_REDISTRIBUTOR *)mp;
		printf("\tBase ADDR=%016jx\n", (uintmax_t)gicr->BaseAddress);
		printf("\tLength=%08x\n", gicr->Length);
		break;
	case ACPI_MADT_TYPE_GENERIC_TRANSLATOR:
		gict = (ACPI_MADT_GENERIC_TRANSLATOR *)mp;
		printf("\tGIC ITS ID=%d\n", gict->TranslationId);
		printf("\tBase ADDR=%016jx\n", (uintmax_t)gict->BaseAddress);
		break;
	}
}

#ifdef notyet
static void
acpi_print_bert_region(ACPI_BERT_REGION *region)
{
	uint32_t i, pos, entries;
	ACPI_HEST_GENERIC_DATA *data;

	printf("\n");
	printf("\tBlockStatus={ ");

	if (region->BlockStatus & ACPI_BERT_UNCORRECTABLE)
		printf("Uncorrectable");
	if (region->BlockStatus & ACPI_BERT_CORRECTABLE)
		printf("Correctable");
	if (region->BlockStatus & ACPI_BERT_MULTIPLE_UNCORRECTABLE)
		printf("Multiple Uncorrectable");
	if (region->BlockStatus & ACPI_BERT_MULTIPLE_CORRECTABLE)
		printf("Multiple Correctable");
	entries = region->BlockStatus & ACPI_BERT_ERROR_ENTRY_COUNT;
	printf(", Error Entry Count=%d", entries);
	printf("}\n");

	printf("\tRaw Data Offset=0x%x\n", region->RawDataOffset);
	printf("\tRaw Data Length=0x%x\n", region->RawDataLength);
	printf("\tData Length=0x%x\n", region->DataLength);

	acpi_print_hest_errorseverity(region->ErrorSeverity);

	pos = sizeof(ACPI_BERT_REGION);
	for (i = 0; i < entries; i++) {
		data = (ACPI_HEST_GENERIC_DATA *)((char *)region + pos);
		acpi_print_hest_generic_data(data);
		pos += sizeof(ACPI_HEST_GENERIC_DATA);
	}
}
#endif

static void
acpi_handle_bert(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_BERT *bert;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	bert = (ACPI_TABLE_BERT *)sdp;

	printf("\tLength of Boot Error Region=%d bytes\n", bert->RegionLength);
	printf("\tPhysical Address of Region=0x%"PRIx64"\n", bert->Address);

	printf(END_COMMENT);
}

static void
acpi_handle_boot(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_BOOT *boot;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	boot = (ACPI_TABLE_BOOT *)sdp;
	printf("\tCMOS Index=0x%02x\n", boot->CmosIndex);
	printf(END_COMMENT);
}

static void
acpi_handle_cpep(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_CPEP *cpep;
	ACPI_CPEP_POLLING *poll;
	uint32_t cpep_pos;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	cpep = (ACPI_TABLE_CPEP *)sdp;

	cpep_pos = sizeof(ACPI_TABLE_CPEP);
	while (cpep_pos < sdp->Length) {
		poll = (ACPI_CPEP_POLLING *)((char *)cpep + cpep_pos);
		acpi_print_cpu(poll->Id);
		printf("\tACPI CPU EId=%d\n", poll->Eid);
		printf("\tPoll Interval=%d msec\n", poll->Interval);
		cpep_pos += sizeof(ACPI_CPEP_POLLING);
	}
	printf(END_COMMENT);
}

static void
acpi_handle_dbgp(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_DBGP *dbgp;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	dbgp = (ACPI_TABLE_DBGP *)sdp;
	printf("\tType={");
	switch (dbgp->Type) {
	case 0:
		printf("full 16550");
		break;
	case 1:
		printf("subset of 16550");
		break;
	}
	printf("}\n");
	printf("\tDebugPort=");
	acpi_print_gas(&dbgp->DebugPort);
	printf("\n");
	printf(END_COMMENT);
}

static void
acpi_print_dbg2_device(ACPI_DBG2_DEVICE *dev)
{

	printf("\t\tRevision=%u\n", dev->Revision);
	printf("\t\tLength=%u\n", dev->Length);
	printf("\t\tRegisterCount=%u\n", dev->RegisterCount);

	printf("\t\tNamepath=");
	acpi_print_string((char *)((vaddr_t)dev + dev->NamepathOffset),
	    dev->NamepathLength);
	printf("\n");

	if (dev->OemDataLength) {
		printf("\t\tOemDataLength=%u\n", dev->OemDataLength);
		printf("\t\tOemDataOffset=%u\n", dev->OemDataOffset);
		/* XXX need dump */
	}

	printf("\t\tPortType=");
	switch (dev->PortType) {
	case ACPI_DBG2_SERIAL_PORT:
		printf("Serial\n" "\t\tPortSubtype=");
		switch (dev->PortSubtype) {
		case ACPI_DBG2_16550_COMPATIBLE:
			printf("Fully 16550 compatible\n");
			break;
		case ACPI_DBG2_16550_SUBSET:
			printf("16550 subset with DBGP Rev. 1\n");
			break;
		case ACPI_DBG2_ARM_PL011:
			printf("ARM PL011\n");
			break;
		case ACPI_DBG2_ARM_SBSA_32BIT:
			printf("ARM SBSA 32bit only\n");
			break;
		case ACPI_DBG2_ARM_SBSA_GENERIC:
			printf("ARM SBSA Generic\n");
			break;
		case ACPI_DBG2_ARM_DCC:
			printf("ARM DCC\n");
			break;
		case ACPI_DBG2_BCM2835:
			printf("BCM2835\n");
			break;
		default:
			printf("reserved (%04hx)\n", dev->PortSubtype);
			break;
		}
		break;
	case ACPI_DBG2_1394_PORT:
		printf("IEEE1394\n" "\t\tPortSubtype=");
		if (dev->PortSubtype == ACPI_DBG2_1394_STANDARD)
			printf("Standard\n");
		else
			printf("reserved (%04hx)\n", dev->PortSubtype);
		break;
	case ACPI_DBG2_USB_PORT:
		printf("USB\n" "\t\tPortSubtype=");
		switch (dev->PortSubtype) {
		case ACPI_DBG2_USB_XHCI:
			printf("XHCIn");
			break;
		case ACPI_DBG2_USB_EHCI:
			printf("EHCI\n");
			break;
		default:
			printf("reserved (%04hx)\n", dev->PortSubtype);
			break;
		}
		break;
	case ACPI_DBG2_NET_PORT:
		printf("Net\n" "\t\tPciVendorID=%04x\n", dev->PortSubtype);
		break;
	default:
		printf("reserved (%04hx)\n", dev->PortType);
		printf("\t\tPortSubtype=reserved (%04hx)\n", dev->PortSubtype);
		break;
	}

	printf("\t\tBaseAddressOffset=0x%04x\n", dev->BaseAddressOffset);
	printf("\t\tAddressSizeOffset=0x%04x\n", dev->AddressSizeOffset);
}

static void
acpi_handle_dbg2(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_DBG2 *dbg2;
	ACPI_DBG2_DEVICE *device;
	unsigned int i;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	dbg2 = (ACPI_TABLE_DBG2 *)sdp;

	printf("\tCount=%u\n", dbg2->InfoCount);
	device = (ACPI_DBG2_DEVICE *)((vaddr_t)sdp + dbg2->InfoOffset);
	for (i = 0; i < dbg2->InfoCount; i++) {
		printf("\tDevice %u={\n", i);
		acpi_print_dbg2_device(device);
		printf("\t}\n");
		device++;
	}

	printf(END_COMMENT);
}

static void
acpi_print_einj_action(ACPI_WHEA_HEADER *whea)
{
	printf("\tACTION={");
	switch (whea->Action) {
	case ACPI_EINJ_BEGIN_OPERATION:
		printf("Begin Operation");
		break;
	case ACPI_EINJ_GET_TRIGGER_TABLE:
		printf("Get Trigger Table");
		break;
	case ACPI_EINJ_SET_ERROR_TYPE:
		printf("Set Error Type");
		break;
	case ACPI_EINJ_GET_ERROR_TYPE:
		printf("Get Error Type");
		break;
	case ACPI_EINJ_END_OPERATION:
		printf("End Operation");
		break;
	case ACPI_EINJ_EXECUTE_OPERATION:
		printf("Execute Operation");
		break;
	case ACPI_EINJ_CHECK_BUSY_STATUS:
		printf("Check Busy Status");
		break;
	case ACPI_EINJ_GET_COMMAND_STATUS:
		printf("Get Command Status");
		break;
	case ACPI_EINJ_SET_ERROR_TYPE_WITH_ADDRESS:
		printf("Set Error Type With Address");
		break;
	case ACPI_EINJ_GET_EXECUTE_TIMINGS:
		printf("Get Execute Operation Timings");
		break;
	case ACPI_EINJ_ACTION_RESERVED:
		printf("Preserved");
		break;
	case ACPI_EINJ_TRIGGER_ERROR:
		printf("Trigger Error");
		break;
	default:
		printf("%d", whea->Action);
		break;
	}
	printf("}\n");
}

static void
acpi_print_einj_instruction(ACPI_WHEA_HEADER *whea)
{
	uint32_t ins = whea->Instruction;

	printf("\tINSTRUCTION={");
	switch (ins) {
	case ACPI_EINJ_READ_REGISTER:
		printf("Read Register");
		break;
	case ACPI_EINJ_READ_REGISTER_VALUE:
		printf("Read Register Value");
		break;
	case ACPI_EINJ_WRITE_REGISTER:
		printf("Write Register");
		break;
	case ACPI_EINJ_WRITE_REGISTER_VALUE:
		printf("Write Register Value");
		break;
	case ACPI_EINJ_NOOP:
		printf("Noop");
		break;
	case ACPI_EINJ_INSTRUCTION_RESERVED:
		printf("Reserved");
		break;
	default:
		printf("%d", ins);
		break;
	}
	printf("}\n");
}

static void
acpi_print_einj_flags(ACPI_WHEA_HEADER *whea)
{
	uint32_t flags = whea->Flags;

	printf("\tFLAGS={");
	if (flags & ACPI_EINJ_PRESERVE)
		printf("PRESERVED");
	printf("}\n");
}

static void
acpi_handle_einj(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_EINJ *einj;
	ACPI_EINJ_ENTRY *einj_entry;
	uint32_t einj_pos;
	u_int i;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	einj = (ACPI_TABLE_EINJ *)sdp;

	printf("\tHeader Length=%d\n", einj->HeaderLength);
	printf("\tFlags=0x%x\n", einj->Flags);
	printf("\tEntries=%d\n", einj->Entries);

	einj_pos = sizeof(ACPI_TABLE_EINJ);
	for (i = 0; i < einj->Entries; i++) {
		einj_entry = (ACPI_EINJ_ENTRY *)((char *)einj + einj_pos);
		acpi_print_whea(&einj_entry->WheaHeader,
		    acpi_print_einj_action, acpi_print_einj_instruction,
		    acpi_print_einj_flags);
		einj_pos += sizeof(ACPI_EINJ_ENTRY);
	}
	printf(END_COMMENT);
}

static void
acpi_print_erst_action(ACPI_WHEA_HEADER *whea)
{
	printf("\tACTION={");
	switch (whea->Action) {
	case ACPI_ERST_BEGIN_WRITE:
		printf("Begin Write");
		break;
	case ACPI_ERST_BEGIN_READ:
		printf("Begin Read");
		break;
	case ACPI_ERST_BEGIN_CLEAR:
		printf("Begin Clear");
		break;
	case ACPI_ERST_END:
		printf("End");
		break;
	case ACPI_ERST_SET_RECORD_OFFSET:
		printf("Set Record Offset");
		break;
	case ACPI_ERST_EXECUTE_OPERATION:
		printf("Execute Operation");
		break;
	case ACPI_ERST_CHECK_BUSY_STATUS:
		printf("Check Busy Status");
		break;
	case ACPI_ERST_GET_COMMAND_STATUS:
		printf("Get Command Status");
		break;
	case ACPI_ERST_GET_RECORD_ID:
		printf("Get Record ID");
		break;
	case ACPI_ERST_SET_RECORD_ID:
		printf("Set Record ID");
		break;
	case ACPI_ERST_GET_RECORD_COUNT:
		printf("Get Record Count");
		break;
	case ACPI_ERST_BEGIN_DUMMY_WRIITE:
		printf("Begin Dummy Write");
		break;
	case ACPI_ERST_NOT_USED:
		printf("Unused");
		break;
	case ACPI_ERST_GET_ERROR_RANGE:
		printf("Get Error Range");
		break;
	case ACPI_ERST_GET_ERROR_LENGTH:
		printf("Get Error Length");
		break;
	case ACPI_ERST_GET_ERROR_ATTRIBUTES:
		printf("Get Error Attributes");
		break;
	case ACPI_ERST_EXECUTE_TIMINGS:
		printf("Execute Operation Timings");
		break;
	case ACPI_ERST_ACTION_RESERVED:
		printf("Reserved");
		break;
	default:
		printf("%d", whea->Action);
		break;
	}
	printf("}\n");
}

static void
acpi_print_erst_instruction(ACPI_WHEA_HEADER *whea)
{
	printf("\tINSTRUCTION={");
	switch (whea->Instruction) {
	case ACPI_ERST_READ_REGISTER:
		printf("Read Register");
		break;
	case ACPI_ERST_READ_REGISTER_VALUE:
		printf("Read Register Value");
		break;
	case ACPI_ERST_WRITE_REGISTER:
		printf("Write Register");
		break;
	case ACPI_ERST_WRITE_REGISTER_VALUE:
		printf("Write Register Value");
		break;
	case ACPI_ERST_NOOP:
		printf("Noop");
		break;
	case ACPI_ERST_LOAD_VAR1:
		printf("Load Var1");
		break;
	case ACPI_ERST_LOAD_VAR2:
		printf("Load Var2");
		break;
	case ACPI_ERST_STORE_VAR1:
		printf("Store Var1");
		break;
	case ACPI_ERST_ADD:
		printf("Add");
		break;
	case ACPI_ERST_SUBTRACT:
		printf("Subtract");
		break;
	case ACPI_ERST_ADD_VALUE:
		printf("Add Value");
		break;
	case ACPI_ERST_SUBTRACT_VALUE:
		printf("Subtract Value");
		break;
	case ACPI_ERST_STALL:
		printf("Stall");
		break;
	case ACPI_ERST_STALL_WHILE_TRUE:
		printf("Stall While True");
		break;
	case ACPI_ERST_SKIP_NEXT_IF_TRUE:
		printf("Skip Next If True");
		break;
	case ACPI_ERST_GOTO:
		printf("Goto");
		break;
	case ACPI_ERST_SET_SRC_ADDRESS_BASE:
		printf("Set Src Address Base");
		break;
	case ACPI_ERST_SET_DST_ADDRESS_BASE:
		printf("Set Dst Address Base");
		break;
	case ACPI_ERST_MOVE_DATA:
		printf("Move Data");
		break;
	case ACPI_ERST_INSTRUCTION_RESERVED:
		printf("Reserved");
		break;
	default:
		printf("%d (reserved)", whea->Instruction);
		break;
	}
	printf("}\n");
}

static void
acpi_print_erst_flags(ACPI_WHEA_HEADER *whea)
{
	uint32_t flags = whea->Flags;

	printf("\tFLAGS={");
	if (flags & ACPI_ERST_PRESERVE)
		printf("PRESERVED");
	printf("}\n");
}

static void
acpi_handle_erst(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_ERST *erst;
	ACPI_ERST_ENTRY *erst_entry;
	uint32_t erst_pos;
	u_int i;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	erst = (ACPI_TABLE_ERST *)sdp;

	printf("\tHeader Length=%d\n", erst->HeaderLength);
	printf("\tEntries=%d\n", erst->Entries);

	erst_pos = sizeof(ACPI_TABLE_ERST);
	for (i = 0; i < erst->Entries; i++) {
		erst_entry = (ACPI_ERST_ENTRY *)((char *)erst + erst_pos);
		acpi_print_whea(&erst_entry->WheaHeader,
		    acpi_print_erst_action, acpi_print_erst_instruction,
		    acpi_print_erst_flags);
		erst_pos += sizeof(ACPI_ERST_ENTRY);
	}
	printf(END_COMMENT);
}

static void
acpi_handle_madt(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_MADT *madt;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	madt = (ACPI_TABLE_MADT *)sdp;
	printf("\tLocal APIC ADDR=0x%08x\n", madt->Address);
	printf("\tFlags={");
	if (madt->Flags & ACPI_MADT_PCAT_COMPAT)
		printf("PC-AT");
	printf("}\n");
	acpi_walk_subtables(sdp, (madt + 1), acpi_print_madt);
	printf(END_COMMENT);
}

static void
acpi_handle_hpet(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_HPET *hpet;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	hpet = (ACPI_TABLE_HPET *)sdp;
	printf("\tHPET Number=%d\n", hpet->Sequence);
	printf("\tADDR=");
	acpi_print_gas(&hpet->Address);
	printf("\tHW Rev=0x%x\n", hpet->Id & ACPI_HPET_ID_HARDWARE_REV_ID);
	printf("\tComparators=%d\n", (hpet->Id & ACPI_HPET_ID_COMPARATORS) >>
	    8);
	printf("\tCounter Size=%d\n", hpet->Id & ACPI_HPET_ID_COUNT_SIZE_CAP ?
	    1 : 0);
	printf("\tLegacy IRQ routing capable={");
	if (hpet->Id & ACPI_HPET_ID_LEGACY_CAPABLE)
		printf("TRUE}\n");
	else
		printf("FALSE}\n");
	printf("\tPCI Vendor ID=0x%04x\n", hpet->Id >> 16);
	printf("\tMinimal Tick=%d\n", hpet->MinimumTick);
	printf("\tFlags=0x%02x\n", hpet->Flags);
	printf(END_COMMENT);
}

static void
acpi_handle_msct(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_MSCT *msct;
	ACPI_MSCT_PROXIMITY *msctentry;
	uint32_t pos;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	msct = (ACPI_TABLE_MSCT *)sdp;

	printf("\tProximity Offset=0x%x\n", msct->ProximityOffset);
	printf("\tMax Proximity Domains=%d\n", msct->MaxProximityDomains);
	printf("\tMax Clock Domains=%d\n", msct->MaxClockDomains);
	printf("\tMax Physical Address=0x%"PRIx64"\n", msct->MaxAddress);

	pos = msct->ProximityOffset;
	while (pos < msct->Header.Length) {
		msctentry = (ACPI_MSCT_PROXIMITY *)((char *)msct + pos);
		pos += msctentry->Length;

		printf("\n");
		printf("\tRevision=%d\n", msctentry->Revision);
		printf("\tLength=%d\n", msctentry->Length);
		printf("\tRange Start=%d\n", msctentry->RangeStart);
		printf("\tRange End=%d\n", msctentry->RangeEnd);
		printf("\tProcessor Capacity=%d\n",
		    msctentry->ProcessorCapacity);
		printf("\tMemory Capacity=0x%"PRIx64" byte\n",
		    msctentry->MemoryCapacity);
	}

	printf(END_COMMENT);
}

static void
acpi_handle_ecdt(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_ECDT *ecdt;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	ecdt = (ACPI_TABLE_ECDT *)sdp;
	printf("\tEC_CONTROL=");
	acpi_print_gas(&ecdt->Control);
	printf("\n\tEC_DATA=");
	acpi_print_gas(&ecdt->Data);
	printf("\n\tUID=%#x, ", ecdt->Uid);
	printf("GPE_BIT=%#x\n", ecdt->Gpe);
	printf("\tEC_ID=%s\n", ecdt->Id);
	printf(END_COMMENT);
}

static void
acpi_handle_mcfg(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_MCFG *mcfg;
	ACPI_MCFG_ALLOCATION *alloc;
	u_int i, entries;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	mcfg = (ACPI_TABLE_MCFG *)sdp;
	entries = (sdp->Length - sizeof(ACPI_TABLE_MCFG)) /
	    sizeof(ACPI_MCFG_ALLOCATION);
	alloc = (ACPI_MCFG_ALLOCATION *)(mcfg + 1);
	for (i = 0; i < entries; i++, alloc++) {
		printf("\n");
		printf("\tBase Address=0x%016jx\n", (uintmax_t)alloc->Address);
		printf("\tSegment Group=0x%04x\n", alloc->PciSegment);
		printf("\tStart Bus=%d\n", alloc->StartBusNumber);
		printf("\tEnd Bus=%d\n", alloc->EndBusNumber);
	}
	printf(END_COMMENT);
}

static void
acpi_handle_sbst(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_SBST *sbst;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	sbst = (ACPI_TABLE_SBST *)sdp;

	printf("\tWarning Level=%d mWh\n", sbst->WarningLevel);
	printf("\tLow Level=%d mWh\n", sbst->LowLevel);
	printf("\tCritical Level=%d mWh\n", sbst->CriticalLevel);

	printf(END_COMMENT);
}

static void
acpi_handle_slit(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_SLIT *slit;
	u_int idx;
	uint64_t cnt;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	slit = (ACPI_TABLE_SLIT *)sdp;

	cnt = slit->LocalityCount * slit->LocalityCount;
	printf("\tLocalityCount=%ju\n", (uintmax_t)slit->LocalityCount);
	printf("\tEntry=\n\t");
	for (idx = 0; idx < cnt; idx++) {
		printf("%u ", slit->Entry[idx]);
		if ((idx % slit->LocalityCount) == (slit->LocalityCount - 1)) {
			printf("\n");
			if (idx < cnt - 1)
				printf("\t");
		}
	}

	printf(END_COMMENT);
}

static void
acpi_handle_spcr(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_SPCR *spcr;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	spcr = (ACPI_TABLE_SPCR *)sdp;

	printf("\tSerial Port=");
	acpi_print_gas(&spcr->SerialPort);
	printf("\n\tInterrupt Type={");
	if (spcr->InterruptType & 0x1) {
		printf("\n\t\tdual-8259 IRQ=");
		switch (spcr->PcInterrupt) {
		case 2 ... 7:
		case 9 ... 12:
		case 14 ... 15:
			printf("%d", spcr->PcInterrupt);
			break;
		default:
			printf("%d (invalid entry)", spcr->PcInterrupt);
			break;
		}
	}
	if (spcr->InterruptType & 0x2) {
		printf("\n\t\tIO APIC={ GSI=%d }", spcr->Interrupt);
	}
	if (spcr->InterruptType & 0x4) {
		printf("\n\t\tIO SAPIC={ GSI=%d }", spcr->Interrupt);
	}
	printf("\n\t}\n");

	printf("\tBaud Rate=");
	switch (spcr->BaudRate) {
	case 3:
		printf("9600");
		break;
	case 4:
		printf("19200");
		break;
	case 6:
		printf("57600");
		break;
	case 7:
		printf("115200");
		break;
	default:
		printf("unknown speed index %d", spcr->BaudRate);
		break;
	}
	printf("\n\tParity={");
	switch (spcr->Parity) {
	case 0:
		printf("OFF");
		break;
	default:
		printf("ON");
		break;
	}
	printf("}\n");

	printf("\tStop Bits={");
	switch (spcr->StopBits) {
	case 1:
		printf("ON");
		break;
	default:
		printf("OFF");
		break;
	}
	printf("}\n");

	printf("\tFlow Control={");
	if (spcr->FlowControl & 0x1)
		printf("DCD, ");
	if (spcr->FlowControl & 0x2)
		printf("RTS/CTS hardware, ");
	if (spcr->FlowControl & 0x4)
		printf("XON/XOFF software");
	printf("}\n");

	printf("\tTerminal=");
	switch (spcr->TerminalType) {
	case 0:
		printf("VT100");
		break;
	case 1:
		printf("VT100+");
		break;
	case 2:
		printf("VT-UTF8");
		break;
	case 3:
		printf("ANSI");
		break;
	default:
		printf("unknown type %d", spcr->TerminalType);
		break;
	}
	printf("\n");

	acpi_print_pci(spcr->PciVendorId, spcr->PciDeviceId,
	    spcr->PciSegment, spcr->PciBus, spcr->PciDevice, spcr->PciFunction);

	printf("\tPCI Flags={");
	if (spcr->PciFlags & ACPI_SPCR_DO_NOT_DISABLE)
		printf("DONOT_DISABLE");
	printf("}\n");

	printf(END_COMMENT);
}

static void
acpi_handle_spmi(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_SPMI *spmi;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	spmi = (ACPI_TABLE_SPMI *)sdp;

	printf("\tInterface Type=");
	switch (spmi->InterfaceType) {
	case ACPI_SPMI_KEYBOARD:
		printf("Keyboard Controller Stype (KCS)");
		break;
	case ACPI_SPMI_SMI:
		printf("Server Management Interface Chip (SMIC)");
		break;
	case ACPI_SPMI_BLOCK_TRANSFER:
		printf("Block Transfer (BT)");
		break;
	case ACPI_SPMI_SMBUS:
		printf("SMBus System Interface (SSIF)");
		break;
	default:
		printf("Reserved(%d)", spmi->InterfaceType);
		break;
	}
	printf("\n\tSpecRevision=%d.%d\n", spmi->SpecRevision >> 8,
		spmi->SpecRevision & 0xff);

	printf("\n\tInterrupt Type={");
	if (spmi->InterruptType & 0x1) {
		printf("\n\t\tSCI triggered GPE=%d", spmi->GpeNumber);
	}
	if (spmi->InterruptType & 0x2) {
		printf("\n\t\tIO APIC/SAPIC={ GSI=%d }", spmi->Interrupt);
	}
	printf("\n\t}\n");

	printf("\tBase Address=");
	acpi_print_gas(&spmi->IpmiRegister);
	printf("\n");

	if ((spmi->PciDeviceFlag & 0x01) != 0)
		acpi_print_pci_sbdf(spmi->PciSegment, spmi->PciBus,
		    spmi->PciDevice, spmi->PciFunction);

	printf(END_COMMENT);
}

static void
acpi_print_srat_cpu(uint32_t apic_id, uint32_t proximity_domain,
    uint32_t flags, uint32_t clockdomain)
{

	printf("\tFlags={");
	if (flags & ACPI_SRAT_CPU_ENABLED)
		printf("ENABLED");
	else
		printf("DISABLED");
	printf("}\n");
	printf("\tAPIC ID=%d\n", apic_id);
	printf("\tProximity Domain=%d\n", proximity_domain);
	printf("\tClock Domain=%d\n", clockdomain);
}

static void
acpi_print_srat_memory(ACPI_SRAT_MEM_AFFINITY *mp)
{

	printf("\tFlags={");
	if (mp->Flags & ACPI_SRAT_MEM_ENABLED)
		printf("ENABLED");
	else
		printf("DISABLED");
	if (mp->Flags & ACPI_SRAT_MEM_HOT_PLUGGABLE)
		printf(",HOT_PLUGGABLE");
	if (mp->Flags & ACPI_SRAT_MEM_NON_VOLATILE)
		printf(",NON_VOLATILE");
	printf("}\n");
	printf("\tBase Address=0x%016jx\n", (uintmax_t)mp->BaseAddress);
	printf("\tLength=0x%016jx\n", (uintmax_t)mp->Length);
	printf("\tProximity Domain=%d\n", mp->ProximityDomain);
}

static const char *srat_types[] = {
    [ACPI_SRAT_TYPE_CPU_AFFINITY] = "CPU",
    [ACPI_SRAT_TYPE_MEMORY_AFFINITY] = "Memory",
    [ACPI_SRAT_TYPE_X2APIC_CPU_AFFINITY] = "X2APIC",
    [ACPI_SRAT_TYPE_GICC_AFFINITY] = "GICC",
};

static void
acpi_print_srat(ACPI_SUBTABLE_HEADER *srat)
{
	ACPI_SRAT_CPU_AFFINITY *cpu;
	ACPI_SRAT_X2APIC_CPU_AFFINITY *x2apic;
	ACPI_SRAT_GICC_AFFINITY *gic;

	if (srat->Type < __arraycount(srat_types))
		printf("\tType=%s\n", srat_types[srat->Type]);
	else
		printf("\tType=%d (unknown)\n", srat->Type);
	switch (srat->Type) {
	case ACPI_SRAT_TYPE_CPU_AFFINITY:
		cpu = (ACPI_SRAT_CPU_AFFINITY *)srat;
		acpi_print_srat_cpu(cpu->ApicId,
		    cpu->ProximityDomainHi[2] << 24 |
		    cpu->ProximityDomainHi[1] << 16 |
		    cpu->ProximityDomainHi[0] << 0 |
		    cpu->ProximityDomainLo,
		    cpu->Flags, cpu->ClockDomain);
		break;
	case ACPI_SRAT_TYPE_MEMORY_AFFINITY:
		acpi_print_srat_memory((ACPI_SRAT_MEM_AFFINITY *)srat);
		break;
	case ACPI_SRAT_TYPE_X2APIC_CPU_AFFINITY:
		x2apic = (ACPI_SRAT_X2APIC_CPU_AFFINITY *)srat;
		acpi_print_srat_cpu(x2apic->ApicId, x2apic->ProximityDomain,
		    x2apic->Flags, x2apic->ClockDomain);
		break;
	case ACPI_SRAT_TYPE_GICC_AFFINITY:
		gic = (ACPI_SRAT_GICC_AFFINITY *)srat;
		acpi_print_srat_cpu(gic->AcpiProcessorUid, gic->ProximityDomain,
		    gic->Flags, gic->ClockDomain);
		break;
	}
}

static void
acpi_handle_srat(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_SRAT *srat;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	srat = (ACPI_TABLE_SRAT *)sdp;
	printf("\tTable Revision=%d\n", srat->TableRevision);
	acpi_walk_subtables(sdp, (srat + 1), acpi_print_srat);
	printf(END_COMMENT);
}

static const char *nfit_types[] = {
    [ACPI_NFIT_TYPE_SYSTEM_ADDRESS] = "System Address",
    [ACPI_NFIT_TYPE_MEMORY_MAP] = "Memory Map",
    [ACPI_NFIT_TYPE_INTERLEAVE] = "Interleave",
    [ACPI_NFIT_TYPE_SMBIOS] = "SMBIOS",
    [ACPI_NFIT_TYPE_CONTROL_REGION] = "Control Region",
    [ACPI_NFIT_TYPE_DATA_REGION] = "Data Region",
    [ACPI_NFIT_TYPE_FLUSH_ADDRESS] = "Flush Address"
};


static void
acpi_print_nfit(ACPI_NFIT_HEADER *nfit)
{
	char *uuidstr;
	uint32_t status;

	ACPI_NFIT_SYSTEM_ADDRESS *sysaddr;
	ACPI_NFIT_MEMORY_MAP *mmap;
	ACPI_NFIT_INTERLEAVE *ileave;
	ACPI_NFIT_SMBIOS *smbios __unused;
	ACPI_NFIT_CONTROL_REGION *ctlreg;
	ACPI_NFIT_DATA_REGION *datareg;
	ACPI_NFIT_FLUSH_ADDRESS *fladdr;

	if (nfit->Type < __arraycount(nfit_types))
		printf("\tType=%s\n", nfit_types[nfit->Type]);
	else
		printf("\tType=%u (unknown)\n", nfit->Type);
	switch (nfit->Type) {
	case ACPI_NFIT_TYPE_SYSTEM_ADDRESS:
		sysaddr = (ACPI_NFIT_SYSTEM_ADDRESS *)nfit;
		printf("\tRangeIndex=%u\n", (u_int)sysaddr->RangeIndex);
		printf("\tProximityDomain=%u\n",
		    (u_int)sysaddr->ProximityDomain);
		uuid_to_string((uuid_t *)(sysaddr->RangeGuid),
		    &uuidstr, &status);
		if (status != uuid_s_ok)
			errx(1, "uuid_to_string: status=%u", status);
		printf("\tRangeGuid=%s\n", uuidstr);
		free(uuidstr);
		printf("\tAddress=0x%016jx\n", (uintmax_t)sysaddr->Address);
		printf("\tLength=0x%016jx\n", (uintmax_t)sysaddr->Length);
		printf("\tMemoryMapping=0x%016jx\n",
		    (uintmax_t)sysaddr->MemoryMapping);

#define PRINTFLAG(var, flag)	printflag((var), ACPI_NFIT_## flag, #flag)

		printf("\tFlags=");
		PRINTFLAG(sysaddr->Flags, ADD_ONLINE_ONLY);
		PRINTFLAG(sysaddr->Flags, PROXIMITY_VALID);
		PRINTFLAG_END();

#undef PRINTFLAG

		break;
	case ACPI_NFIT_TYPE_MEMORY_MAP:
		mmap = (ACPI_NFIT_MEMORY_MAP *)nfit;
		printf("\tDeviceHandle=%u\n", (u_int)mmap->DeviceHandle);
		printf("\tPhysicalId=%u\n", (u_int)mmap->PhysicalId);
		printf("\tRegionId=%u\n", (u_int)mmap->RegionId);
		printf("\tRangeIndex=%u\n", (u_int)mmap->RangeIndex);
		printf("\tRegionIndex=%u\n", (u_int)mmap->RegionIndex);
		printf("\tRegionSize=0x%016jx\n", (uintmax_t)mmap->RegionSize);
		printf("\tRegionOffset=0x%016jx\n",
		    (uintmax_t)mmap->RegionOffset);
		printf("\tAddress=0x%016jx\n", (uintmax_t)mmap->Address);
		printf("\tInterleaveIndex=%u\n", (u_int)mmap->InterleaveIndex);
		printf("\tInterleaveWays=%u\n", (u_int)mmap->InterleaveWays);

#define PRINTFLAG(var, flag)	printflag((var), ACPI_NFIT_MEM_## flag, #flag)

		printf("\tFlags=");
		PRINTFLAG(mmap->Flags, SAVE_FAILED);
		PRINTFLAG(mmap->Flags, RESTORE_FAILED);
		PRINTFLAG(mmap->Flags, FLUSH_FAILED);
		PRINTFLAG(mmap->Flags, NOT_ARMED);
		PRINTFLAG(mmap->Flags, HEALTH_OBSERVED);
		PRINTFLAG(mmap->Flags, HEALTH_ENABLED);
		PRINTFLAG(mmap->Flags, MAP_FAILED);
		PRINTFLAG_END();

#undef PRINTFLAG

		break;
	case ACPI_NFIT_TYPE_INTERLEAVE:
		ileave = (ACPI_NFIT_INTERLEAVE *)nfit;
		printf("\tInterleaveIndex=%u\n",
		    (u_int)ileave->InterleaveIndex);
		printf("\tLineCount=%u\n", (u_int)ileave->LineCount);
		printf("\tLineSize=%u\n", (u_int)ileave->LineSize);
		/* XXX ileave->LineOffset[i] output is not supported */
		break;
	case ACPI_NFIT_TYPE_SMBIOS:
		smbios = (ACPI_NFIT_SMBIOS *)nfit;
		/* XXX smbios->Data[x] output is not supported */
		break;
	case ACPI_NFIT_TYPE_CONTROL_REGION:
		ctlreg = (ACPI_NFIT_CONTROL_REGION *)nfit;
		printf("\tRegionIndex=%u\n", (u_int)ctlreg->RegionIndex);
		printf("\tVendorId=0x%04x\n", (u_int)ctlreg->VendorId);
		printf("\tDeviceId=0x%04x\n", (u_int)ctlreg->DeviceId);
		printf("\tRevisionId=%u\n", (u_int)ctlreg->RevisionId);
		printf("\tSubsystemVendorId=0x%04x\n",
		    (u_int)ctlreg->SubsystemVendorId);
		printf("\tSubsystemDeviceId=0x%04x\n",
		    (u_int)ctlreg->SubsystemDeviceId);
		printf("\tSubsystemRevisionId=%u\n",
		    (u_int)ctlreg->SubsystemRevisionId);
		printf("\tValidFields=%02x\n", (u_int)ctlreg->ValidFields);
		printf("\tManufacturingLocation=%u\n",
		    (u_int)ctlreg->ManufacturingLocation);
		printf("\tManufacturingDate=%u\n",
		    (u_int)ctlreg->ManufacturingDate);
		printf("\tSerialNumber=%u\n",
		    (u_int)ctlreg->SerialNumber);
		printf("\tCode=0x%04x\n", (u_int)ctlreg->Code);
		printf("\tWindows=%u\n", (u_int)ctlreg->Windows);
		printf("\tWindowSize=0x%016jx\n",
		    (uintmax_t)ctlreg->WindowSize);
		printf("\tCommandOffset=0x%016jx\n",
		    (uintmax_t)ctlreg->CommandOffset);
		printf("\tCommandSize=0x%016jx\n",
		    (uintmax_t)ctlreg->CommandSize);
		printf("\tStatusOffset=0x%016jx\n",
		    (uintmax_t)ctlreg->StatusOffset);
		printf("\tStatusSize=0x%016jx\n",
		    (uintmax_t)ctlreg->StatusSize);

#define PRINTFLAG(var, flag)	printflag((var), ACPI_NFIT_## flag, #flag)

		printf("\tFlags=");
		PRINTFLAG(ctlreg->Flags, CONTROL_BUFFERED);
		PRINTFLAG_END();

#undef PRINTFLAG

		break;
	case ACPI_NFIT_TYPE_DATA_REGION:
		datareg = (ACPI_NFIT_DATA_REGION *)nfit;
		printf("\tRegionIndex=%u\n", (u_int)datareg->RegionIndex);
		printf("\tWindows=%u\n", (u_int)datareg->Windows);
		printf("\tOffset=0x%016jx\n", (uintmax_t)datareg->Offset);
		printf("\tSize=0x%016jx\n", (uintmax_t)datareg->Size);
		printf("\tCapacity=0x%016jx\n", (uintmax_t)datareg->Capacity);
		printf("\tStartAddress=0x%016jx\n",
		    (uintmax_t)datareg->StartAddress);
		break;
	case ACPI_NFIT_TYPE_FLUSH_ADDRESS:
		fladdr = (ACPI_NFIT_FLUSH_ADDRESS *)nfit;
		printf("\tDeviceHandle=%u\n", (u_int)fladdr->DeviceHandle);
		printf("\tHintCount=%u\n", (u_int)fladdr->HintCount);
		/* XXX fladdr->HintAddress[i] output is not supported */
		break;
	}
}

static void
acpi_handle_nfit(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_NFIT *nfit;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	nfit = (ACPI_TABLE_NFIT *)sdp;
	acpi_walk_nfit(sdp, (nfit + 1), acpi_print_nfit);
	printf(END_COMMENT);
}

static char *
acpi_tcpa_evname(struct TCPAevent *event)
{
	struct TCPApc_event *pc_event;
	char *eventname = NULL;

	pc_event = (struct TCPApc_event *)(event + 1);

	switch(event->event_type) {
	case PREBOOT:
	case POST_CODE:
	case UNUSED:
	case NO_ACTION:
	case SEPARATOR:
	case SCRTM_CONTENTS:
	case SCRTM_VERSION:
	case CPU_MICROCODE:
	case PLATFORM_CONFIG_FLAGS:
	case TABLE_OF_DEVICES:
	case COMPACT_HASH:
	case IPL:
	case IPL_PARTITION_DATA:
	case NONHOST_CODE:
	case NONHOST_CONFIG:
	case NONHOST_INFO:
		asprintf(&eventname, "%s",
		    tcpa_event_type_strings[event->event_type]);
		break;

	case ACTION:
		eventname = calloc(event->event_size + 1, sizeof(char));
		memcpy(eventname, pc_event, event->event_size);
		break;

	case EVENT_TAG:
		switch (pc_event->event_id) {
		case SMBIOS:
		case BIS_CERT:
		case CMOS:
		case NVRAM:
		case OPTION_ROM_EXEC:
		case OPTION_ROM_CONFIG:
		case S_CRTM_VERSION:
		case POST_BIOS_ROM:
		case ESCD:
		case OPTION_ROM_MICROCODE:
		case S_CRTM_CONTENTS:
		case POST_CONTENTS:
			asprintf(&eventname, "%s",
			    TCPA_pcclient_strings[pc_event->event_id]);
			break;

		default:
			asprintf(&eventname, "<unknown tag 0x%02x>",
			    pc_event->event_id);
			break;
		}
		break;

	default:
		asprintf(&eventname, "<unknown 0x%02x>", event->event_type);
		break;
	}

	return eventname;
}

static void
acpi_print_tcpa(struct TCPAevent *event)
{
	int i;
	char *eventname;

	eventname = acpi_tcpa_evname(event);

	printf("\t%d", event->pcr_index);
	printf(" 0x");
	for (i = 0; i < 20; i++)
		printf("%02x", event->pcr_value[i]);
	printf(" [%s]\n", eventname ? eventname : "<unknown>");

	free(eventname);
}

static void
acpi_handle_tcpa(ACPI_TABLE_HEADER *sdp)
{
	struct TCPAbody *tcpa;
	struct TCPAevent *event;
	uintmax_t len, paddr;
	unsigned char *vaddr = NULL;
	unsigned char *vend = NULL;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	tcpa = (struct TCPAbody *) sdp;

	switch (tcpa->platform_class) {
	case ACPI_TCPA_BIOS_CLIENT:
		len = tcpa->client.log_max_len;
		paddr = tcpa->client.log_start_addr;
		break;

	case ACPI_TCPA_BIOS_SERVER:
		len = tcpa->server.log_max_len;
		paddr = tcpa->server.log_start_addr;
		break;

	default:
		printf("XXX");
		printf(END_COMMENT);
		return;
	}
	printf("\tClass %u Base Address 0x%jx Length %ju\n\n",
	    tcpa->platform_class, paddr, len);

	if (len == 0) {
		printf("\tEmpty TCPA table\n");
		printf(END_COMMENT);
		return;
	}
	if(sdp->Revision == 1) {
		printf("\tOLD TCPA spec log found. Dumping not supported.\n");
		printf(END_COMMENT);
		return;
	}

	vaddr = (unsigned char *)acpi_map_physical(paddr, len);
	vend = vaddr + len;

	while (vaddr != NULL) {
		if ((vaddr + sizeof(struct TCPAevent) >= vend)||
		    (vaddr + sizeof(struct TCPAevent) < vaddr))
			break;
		event = (struct TCPAevent *)(void *)vaddr;
		if (vaddr + event->event_size >= vend)
			break;
		if (vaddr + event->event_size < vaddr)
			break;
		if (event->event_type == 0 && event->event_size == 0)
			break;
#if 0
		{
		unsigned int i, j, k;

		printf("\n\tsize %d\n\t\t%p ", event->event_size, vaddr);
		for (j = 0, i = 0; i <
		    sizeof(struct TCPAevent) + event->event_size; i++) {
			printf("%02x ", vaddr[i]);
			if ((i+1) % 8 == 0) {
				for (k = 0; k < 8; k++)
					printf("%c", isprint(vaddr[j+k]) ?
					    vaddr[j+k] : '.');
				printf("\n\t\t%p ", &vaddr[i + 1]);
				j = i + 1;
			}
		}
		printf("\n"); }
#endif
		acpi_print_tcpa(event);

		vaddr += sizeof(struct TCPAevent) + event->event_size;
	}

	printf(END_COMMENT);
}

static const char *
devscope_type2str(int type)
{
	static char typebuf[16];

	switch (type) {
	case 1:
		return ("PCI Endpoint Device");
	case 2:
		return ("PCI Sub-Hierarchy");
	case 3:
		return ("IOAPIC");
	case 4:
		return ("HPET");
	default:
		snprintf(typebuf, sizeof(typebuf), "%d", type);
		return (typebuf);
	}
}

static int
acpi_handle_dmar_devscope(void *addr, int remaining)
{
	char sep;
	int pathlen;
	ACPI_DMAR_PCI_PATH *path, *pathend;
	ACPI_DMAR_DEVICE_SCOPE *devscope = addr;

	if (remaining < (int)sizeof(ACPI_DMAR_DEVICE_SCOPE))
		return (-1);

	if (remaining < devscope->Length)
		return (-1);

	printf("\n");
	printf("\t\tType=%s\n", devscope_type2str(devscope->EntryType));
	printf("\t\tLength=%d\n", devscope->Length);
	printf("\t\tEnumerationId=%d\n", devscope->EnumerationId);
	printf("\t\tStartBusNumber=%d\n", devscope->Bus);

	path = (ACPI_DMAR_PCI_PATH *)(devscope + 1);
	pathlen = devscope->Length - sizeof(ACPI_DMAR_DEVICE_SCOPE);
	pathend = path + pathlen / sizeof(ACPI_DMAR_PCI_PATH);
	if (path < pathend) {
		sep = '{';
		printf("\t\tPath=");
		do {
			printf("%c%d:%d", sep, path->Device, path->Function);
			sep=',';
			path++;
		} while (path < pathend);
		printf("}\n");
	}

	return (devscope->Length);
}

static void
acpi_handle_dmar_drhd(ACPI_DMAR_HARDWARE_UNIT *drhd)
{
	char *cp;
	int remaining, consumed;

	printf("\n");
	printf("\tType=DRHD\n");
	printf("\tLength=%d\n", drhd->Header.Length);

#define	PRINTFLAG(var, flag)	printflag((var), ACPI_DMAR_## flag, #flag)

	printf("\tFlags=");
	PRINTFLAG(drhd->Flags, INCLUDE_ALL);
	PRINTFLAG_END();

#undef PRINTFLAG

	printf("\tSegment=%d\n", drhd->Segment);
	printf("\tAddress=0x%016jx\n", (uintmax_t)drhd->Address);

	remaining = drhd->Header.Length - sizeof(ACPI_DMAR_HARDWARE_UNIT);
	if (remaining > 0)
		printf("\tDevice Scope:");
	while (remaining > 0) {
		cp = (char *)drhd + drhd->Header.Length - remaining;
		consumed = acpi_handle_dmar_devscope(cp, remaining);
		if (consumed <= 0)
			break;
		else
			remaining -= consumed;
	}
}

static void
acpi_handle_dmar_rmrr(ACPI_DMAR_RESERVED_MEMORY *rmrr)
{
	char *cp;
	int remaining, consumed;

	printf("\n");
	printf("\tType=RMRR\n");
	printf("\tLength=%d\n", rmrr->Header.Length);
	printf("\tSegment=%d\n", rmrr->Segment);
	printf("\tBaseAddress=0x%016jx\n", (uintmax_t)rmrr->BaseAddress);
	printf("\tLimitAddress=0x%016jx\n", (uintmax_t)rmrr->EndAddress);

	remaining = rmrr->Header.Length - sizeof(ACPI_DMAR_RESERVED_MEMORY);
	if (remaining > 0)
		printf("\tDevice Scope:");
	while (remaining > 0) {
		cp = (char *)rmrr + rmrr->Header.Length - remaining;
		consumed = acpi_handle_dmar_devscope(cp, remaining);
		if (consumed <= 0)
			break;
		else
			remaining -= consumed;
	}
}

static void
acpi_handle_dmar_atsr(ACPI_DMAR_ATSR *atsr)
{
	char *cp;
	int remaining, consumed;

	printf("\n");
	printf("\tType=ATSR\n");
	printf("\tLength=%d\n", atsr->Header.Length);

#define	PRINTFLAG(var, flag)	printflag((var), ACPI_DMAR_## flag, #flag)

	printf("\tFlags=");
	PRINTFLAG(atsr->Flags, ALL_PORTS);
	PRINTFLAG_END();

#undef PRINTFLAG

	printf("\tSegment=%d\n", atsr->Segment);

	remaining = atsr->Header.Length - sizeof(ACPI_DMAR_ATSR);
	if (remaining > 0)
		printf("\tDevice Scope:");
	while (remaining > 0) {
		cp = (char *)atsr + atsr->Header.Length - remaining;
		consumed = acpi_handle_dmar_devscope(cp, remaining);
		if (consumed <= 0)
			break;
		else
			remaining -= consumed;
	}
}

static void
acpi_handle_dmar_rhsa(ACPI_DMAR_RHSA *rhsa)
{

	printf("\n");
	printf("\tType=RHSA\n");
	printf("\tLength=%d\n", rhsa->Header.Length);
	printf("\tBaseAddress=0x%016jx\n", (uintmax_t)rhsa->BaseAddress);
	printf("\tProximityDomain=0x%08x\n", rhsa->ProximityDomain);
}

static int
acpi_handle_dmar_remapping_structure(void *addr, int remaining)
{
	ACPI_DMAR_HEADER *hdr = addr;

	if (remaining < (int)sizeof(ACPI_DMAR_HEADER))
		return (-1);

	if (remaining < hdr->Length)
		return (-1);

	switch (hdr->Type) {
	case ACPI_DMAR_TYPE_HARDWARE_UNIT:
		acpi_handle_dmar_drhd(addr);
		break;
	case ACPI_DMAR_TYPE_RESERVED_MEMORY:
		acpi_handle_dmar_rmrr(addr);
		break;
	case ACPI_DMAR_TYPE_ROOT_ATS:
		acpi_handle_dmar_atsr(addr);
		break;
	case ACPI_DMAR_TYPE_HARDWARE_AFFINITY:
		acpi_handle_dmar_rhsa(addr);
		break;
	default:
		printf("\n");
		printf("\tType=%d\n", hdr->Type);
		printf("\tLength=%d\n", hdr->Length);
		break;
	}
	return (hdr->Length);
}

#ifndef ACPI_DMAR_X2APIC_OPT_OUT
#define	ACPI_DMAR_X2APIC_OPT_OUT	(0x2)
#endif

static void
acpi_handle_dmar(ACPI_TABLE_HEADER *sdp)
{
	char *cp;
	int remaining, consumed;
	ACPI_TABLE_DMAR *dmar;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	dmar = (ACPI_TABLE_DMAR *)sdp;
	printf("\tHost Address Width=%d\n", dmar->Width + 1);

#define PRINTFLAG(var, flag)	printflag((var), ACPI_DMAR_## flag, #flag)

	printf("\tFlags=");
	PRINTFLAG(dmar->Flags, INTR_REMAP);
	PRINTFLAG(dmar->Flags, X2APIC_OPT_OUT);
	PRINTFLAG_END();

#undef PRINTFLAG

	remaining = sdp->Length - sizeof(ACPI_TABLE_DMAR);
	while (remaining > 0) {
		cp = (char *)sdp + sdp->Length - remaining;
		consumed = acpi_handle_dmar_remapping_structure(cp, remaining);
		if (consumed <= 0)
			break;
		else
			remaining -= consumed;
	}

	printf(END_COMMENT);
}

static void
acpi_handle_uefi(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_UEFI *uefi;
	char *uuidstr;
	uint32_t status;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	uefi = (ACPI_TABLE_UEFI *)sdp;

	uuid_to_string((uuid_t *)(uefi->Identifier),
	    &uuidstr, &status);
	if (status != uuid_s_ok)
		errx(1, "uuid_to_string: status=%u", status);
	printf("\tUUID=%s\n", uuidstr);
	free(uuidstr);

	printf("\tDataOffset=%04hx\n", uefi->DataOffset);
	/* XXX need write */

	printf(END_COMMENT);
}

static void
acpi_handle_waet(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_WAET *waet;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	waet = (ACPI_TABLE_WAET *)sdp;

	printf("\tRTC Timer={");
	if (waet->Flags & ACPI_WAET_RTC_NO_ACK)
		printf("No ACK required");
	else
		printf("default behaviour");
	printf("}\n");
	printf("\t ACPI PM Timer={");
	if (waet->Flags & ACPI_WAET_TIMER_ONE_READ)
		printf("One Read sufficient");
	else
		printf("default behaviour");
	printf("}\n");

	printf(END_COMMENT);
}

static void
acpi_print_wdat_action(ACPI_WHEA_HEADER *whea)
{
	printf("\tACTION={");
	switch (whea->Action) {
	case ACPI_WDAT_RESET:
		printf("RESET");
		break;
	case ACPI_WDAT_GET_CURRENT_COUNTDOWN:
		printf("GET_CURRENT_COUNTDOWN");
		break;
	case ACPI_WDAT_GET_COUNTDOWN:
		printf("GET_COUNTDOWN");
		break;
	case ACPI_WDAT_SET_COUNTDOWN:
		printf("SET_COUNTDOWN");
		break;
	case ACPI_WDAT_GET_RUNNING_STATE:
		printf("GET_RUNNING_STATE");
		break;
	case ACPI_WDAT_SET_RUNNING_STATE:
		printf("SET_RUNNING_STATE");
		break;
	case ACPI_WDAT_GET_STOPPED_STATE:
		printf("GET_STOPPED_STATE");
		break;
	case ACPI_WDAT_SET_STOPPED_STATE:
		printf("SET_STOPPED_STATE");
		break;
	case ACPI_WDAT_GET_REBOOT:
		printf("GET_REBOOT");
		break;
	case ACPI_WDAT_SET_REBOOT:
		printf("SET_REBOOT");
		break;
	case ACPI_WDAT_GET_SHUTDOWN:
		printf("GET_SHUTDOWN");
		break;
	case ACPI_WDAT_SET_SHUTDOWN:
		printf("SET_SHUTDOWN");
		break;
	case ACPI_WDAT_GET_STATUS:
		printf("GET_STATUS");
		break;
	case ACPI_WDAT_SET_STATUS:
		printf("SET_STATUS");
		break;
	case ACPI_WDAT_ACTION_RESERVED:
		printf("ACTION_RESERVED");
		break;
	default:
		printf("%d", whea->Action);
		break;
	}
	printf("}\n");
}

static void
acpi_print_wdat_instruction(ACPI_WHEA_HEADER *whea)
{
	uint32_t ins;

	ins = whea->Instruction & ~ACPI_WDAT_PRESERVE_REGISTER;

	printf("\tINSTRUCTION={");
	switch (ins) {
	case ACPI_WDAT_READ_VALUE:
		printf("READ_VALUE");
		break;
	case ACPI_WDAT_READ_COUNTDOWN:
		printf("READ_COUNTDOWN");
		break;
	case ACPI_WDAT_WRITE_VALUE:
		printf("WRITE_VALUE");
		break;
	case ACPI_WDAT_WRITE_COUNTDOWN:
		printf("WRITE_COUNTDOWN");
		break;
	case ACPI_WDAT_INSTRUCTION_RESERVED:
		printf("INSTRUCTION_RESERVED");
		break;
	default:
		printf("%d", ins);
		break;
	}

	if (whea->Instruction & ACPI_WDAT_PRESERVE_REGISTER)
		printf(", Preserve Register");

	printf("}\n");
}

static void
acpi_handle_wdat(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_WDAT *wdat;
	ACPI_WHEA_HEADER *whea;
	ACPI_WDAT_ENTRY *wdat_pos;
	u_int i;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	wdat = (ACPI_TABLE_WDAT *)sdp;

	printf("\tHeader Length=%d\n", wdat->HeaderLength);

	acpi_print_pci_sbdf(wdat->PciSegment, wdat->PciBus, wdat->PciDevice,
	    wdat->PciFunction);
	printf("\n\tTimer Counter Period=%d msec\n", wdat->TimerPeriod);
	printf("\tTimer Maximum Counter Value=%d\n", wdat->MaxCount);
	printf("\tTimer Minimum Counter Value=%d\n", wdat->MinCount); 

	printf("\tFlags={");
	if (wdat->Flags & ACPI_WDAT_ENABLED)
		printf("ENABLED");
	if (wdat->Flags & ACPI_WDAT_STOPPED)
		printf(", STOPPED");
	printf("}\n");

	wdat_pos = (ACPI_WDAT_ENTRY *)((char *)wdat + sizeof(ACPI_TABLE_WDAT));

	for (i = 0; i < wdat->Entries; i++) {
		whea = (ACPI_WHEA_HEADER *)wdat_pos;
		acpi_print_whea(whea,
		    acpi_print_wdat_action, acpi_print_wdat_instruction,
		    NULL);
		wdat_pos++;
	}
	printf(END_COMMENT);
}

static void
acpi_handle_wddt(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_WDDT *wddt;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	wddt = (ACPI_TABLE_WDDT *)sdp;

	printf("\tSpecVersion=%04hx\n", wddt->SpecVersion);
	printf("\tTableVersion=%04hx\n", wddt->TableVersion);
	printf("\tPciVendorID=%04hx\n", wddt->PciVendorId);
	printf("\tAddress=");
	acpi_print_gas(&wddt->Address);
	printf("\n\tTimer Maximum Counter Value=%d\n", wddt->MaxCount);
	printf("\tTimer Minimum Counter Value=%d\n", wddt->MinCount); 
	printf("\tTimer Counter Period=%d\n", wddt->Period);

#define PRINTFLAG(var, flag)	printflag((var), ACPI_WDDT_## flag, #flag)

	printf("\tStatus=");
	PRINTFLAG(wddt->Status, AVAILABLE);
	PRINTFLAG(wddt->Status, ACTIVE);
	PRINTFLAG(wddt->Status, TCO_OS_OWNED);
	PRINTFLAG(wddt->Status, USER_RESET);
	PRINTFLAG(wddt->Status, WDT_RESET);
	PRINTFLAG(wddt->Status, POWER_FAIL);
	PRINTFLAG(wddt->Status, UNKNOWN_RESET);
	PRINTFLAG_END();

	printf("\tCapability=");
	PRINTFLAG(wddt->Capability, AUTO_RESET);
	PRINTFLAG(wddt->Capability, ALERT_SUPPORT);
	PRINTFLAG_END();

#undef PRINTFLAG

	printf(END_COMMENT);
}

static void
acpi_handle_wdrt(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_WDRT *wdrt;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	wdrt = (ACPI_TABLE_WDRT *)sdp;

	printf("\tControl Register=");
	acpi_print_gas(&wdrt->ControlRegister);
	printf("\tCount Register=");
	acpi_print_gas(&wdrt->CountRegister);
	acpi_print_pci(wdrt->PciVendorId, wdrt->PciDeviceId,
	    wdrt->PciSegment, wdrt->PciBus, wdrt->PciDevice, wdrt->PciFunction);

	/* Value must be >= 511 and < 65535 */
	printf("\tMaxCount=%d", wdrt->MaxCount);
	if (wdrt->MaxCount < 511)
		printf(" (Out of Range. Valid range: 511 <= maxcount < 65535)");
	printf("\n");

	printf("\tUnit={");
	switch (wdrt->Units) {
	case 0:
		printf("1 seconds/count");
		break;
	case 1:
		printf("100 milliseconds/count");
		break;
	case 2:
		printf("10 milliseconds/count");
		break;
	default:
		printf("%d", wdrt->Units);
		break;
	}
	printf("}\n");

	printf(END_COMMENT);
}

static void
acpi_print_sdt(ACPI_TABLE_HEADER *sdp)
{
	printf("  ");
	acpi_print_string(sdp->Signature, ACPI_NAME_SIZE);
	printf(": Length=%d, Revision=%d, Checksum=%d",
	       sdp->Length, sdp->Revision, sdp->Checksum);
	if (acpi_checksum(sdp, sdp->Length))
		printf(" (Incorrect)");
	printf(",\n\tOEMID=");
	acpi_print_string(sdp->OemId, ACPI_OEM_ID_SIZE);
	printf(", OEM Table ID=");
	acpi_print_string(sdp->OemTableId, ACPI_OEM_TABLE_ID_SIZE);
	printf(", OEM Revision=0x%x,\n", sdp->OemRevision);
	printf("\tCreator ID=");
	acpi_print_string(sdp->AslCompilerId, ACPI_NAME_SIZE);
	printf(", Creator Revision=0x%x\n", sdp->AslCompilerRevision);
}

static void
acpi_dump_bytes(ACPI_TABLE_HEADER *sdp)
{
	unsigned int i;
	uint8_t *p;

	p = (uint8_t *)sdp;
	printf("\n\tData={");
	for (i = 0; i < sdp->Length; i++) {
		if (cflag) {
			if (i % 64 == 0)
				printf("\n\t ");
			else if (i % 16 == 0)
				printf(" ");
			printf("%c", (p[i] >= ' ' && p[i] <= '~') ? p[i] : '.');
		} else {
			if (i % 16 == 0)
				printf("\n\t\t");
			else if (i % 8 == 0)
				printf("   ");
			printf(" %02x", p[i]);
		}
	}
	printf("\n\t}\n");
}

static void
acpi_print_rsdt(ACPI_TABLE_HEADER *rsdp)
{
	ACPI_TABLE_RSDT *rsdt;
	ACPI_TABLE_XSDT *xsdt;
	int	i, entries;

	rsdt = (ACPI_TABLE_RSDT *)rsdp;
	xsdt = (ACPI_TABLE_XSDT *)rsdp;
	printf(BEGIN_COMMENT);
	acpi_print_sdt(rsdp);
	entries = (rsdp->Length - sizeof(ACPI_TABLE_HEADER)) / addr_size;
	printf("\tEntries={ ");
	for (i = 0; i < entries; i++) {
		if (i > 0)
			printf(", ");
		if (addr_size == 4)
			printf("0x%08x", le32toh(rsdt->TableOffsetEntry[i]));
		else
			printf("0x%016jx",
			    (uintmax_t)le64toh(xsdt->TableOffsetEntry[i]));
	}
	printf(" }\n");
	printf(END_COMMENT);
}

static const char *acpi_pm_profiles[] = {
	"Unspecified", "Desktop", "Mobile", "Workstation",
	"Enterprise Server", "SOHO Server", "Appliance PC",
	"Performance Server", "Tablet"
};

static void
acpi_print_fadt(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_FADT *fadt;
	const char *pm;

	fadt = (ACPI_TABLE_FADT *)sdp;
	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	printf(" \tFACS=0x%x, DSDT=0x%x\n", fadt->Facs,
	       fadt->Dsdt);
	/* XXX ACPI 2.0 eliminated this */
	printf("\tINT_MODEL=%s\n", fadt->Model ? "APIC" : "PIC");
	if (fadt->PreferredProfile >= sizeof(acpi_pm_profiles) / sizeof(char *))
		pm = "Reserved";
	else
		pm = acpi_pm_profiles[fadt->PreferredProfile];
	printf("\tPreferred_PM_Profile=%s (%d)\n", pm, fadt->PreferredProfile);
	printf("\tSCI_INT=%d\n", fadt->SciInterrupt);
	printf("\tSMI_CMD=0x%x, ", fadt->SmiCommand);
	printf("ACPI_ENABLE=0x%x, ", fadt->AcpiEnable);
	printf("ACPI_DISABLE=0x%x, ", fadt->AcpiDisable);
	printf("S4BIOS_REQ=0x%x\n", fadt->S4BiosRequest);
	printf("\tPSTATE_CNT=0x%x\n", fadt->PstateControl);
	printf("\tPM1a_EVT_BLK=0x%x-0x%x\n",
	       fadt->Pm1aEventBlock,
	       fadt->Pm1aEventBlock + fadt->Pm1EventLength - 1);
	if (fadt->Pm1bEventBlock != 0)
		printf("\tPM1b_EVT_BLK=0x%x-0x%x\n",
		       fadt->Pm1bEventBlock,
		       fadt->Pm1bEventBlock + fadt->Pm1EventLength - 1);
	printf("\tPM1a_CNT_BLK=0x%x-0x%x\n",
	       fadt->Pm1aControlBlock,
	       fadt->Pm1aControlBlock + fadt->Pm1ControlLength - 1);
	if (fadt->Pm1bControlBlock != 0)
		printf("\tPM1b_CNT_BLK=0x%x-0x%x\n",
		       fadt->Pm1bControlBlock,
		       fadt->Pm1bControlBlock + fadt->Pm1ControlLength - 1);
	if (fadt->Pm2ControlBlock != 0)
		printf("\tPM2_CNT_BLK=0x%x-0x%x\n",
		       fadt->Pm2ControlBlock,
		       fadt->Pm2ControlBlock + fadt->Pm2ControlLength - 1);
	printf("\tPM_TMR_BLK=0x%x-0x%x\n",
	       fadt->PmTimerBlock,
	       fadt->PmTimerBlock + fadt->PmTimerLength - 1);
	if (fadt->Gpe0Block != 0)
		printf("\tGPE0_BLK=0x%x-0x%x\n",
		       fadt->Gpe0Block,
		       fadt->Gpe0Block + fadt->Gpe0BlockLength - 1);
	if (fadt->Gpe1Block != 0)
		printf("\tGPE1_BLK=0x%x-0x%x, GPE1_BASE=%d\n",
		       fadt->Gpe1Block,
		       fadt->Gpe1Block + fadt->Gpe1BlockLength - 1,
		       fadt->Gpe1Base);
	if (fadt->CstControl != 0)
		printf("\tCST_CNT=0x%x\n", fadt->CstControl);
	printf("\tP_LVL2_LAT=%d us, P_LVL3_LAT=%d us\n",
	       fadt->C2Latency, fadt->C3Latency);
	printf("\tFLUSH_SIZE=%d, FLUSH_STRIDE=%d\n",
	       fadt->FlushSize, fadt->FlushStride);
	printf("\tDUTY_OFFSET=%d, DUTY_WIDTH=%d\n",
	       fadt->DutyOffset, fadt->DutyWidth);
	printf("\tDAY_ALRM=%d, MON_ALRM=%d, CENTURY=%d\n",
	       fadt->DayAlarm, fadt->MonthAlarm, fadt->Century);

#define PRINTFLAG(var, flag)	printflag((var), ACPI_FADT_## flag, #flag)

	printf("\tIAPC_BOOT_ARCH=");
	PRINTFLAG(fadt->BootFlags, LEGACY_DEVICES);
	PRINTFLAG(fadt->BootFlags, 8042);
	PRINTFLAG(fadt->BootFlags, NO_VGA);
	PRINTFLAG(fadt->BootFlags, NO_MSI);
	PRINTFLAG(fadt->BootFlags, NO_ASPM);
	PRINTFLAG(fadt->BootFlags, NO_CMOS_RTC);
	PRINTFLAG_END();

	printf("\tFlags=");
	PRINTFLAG(fadt->Flags, WBINVD);
	PRINTFLAG(fadt->Flags, WBINVD_FLUSH);
	PRINTFLAG(fadt->Flags, C1_SUPPORTED);
	PRINTFLAG(fadt->Flags, C2_MP_SUPPORTED);
	PRINTFLAG(fadt->Flags, POWER_BUTTON);
	PRINTFLAG(fadt->Flags, SLEEP_BUTTON);
	PRINTFLAG(fadt->Flags, FIXED_RTC);
	PRINTFLAG(fadt->Flags, S4_RTC_WAKE);
	PRINTFLAG(fadt->Flags, 32BIT_TIMER);
	PRINTFLAG(fadt->Flags, DOCKING_SUPPORTED);
	PRINTFLAG(fadt->Flags, RESET_REGISTER);
	PRINTFLAG(fadt->Flags, SEALED_CASE);
	PRINTFLAG(fadt->Flags, HEADLESS);
	PRINTFLAG(fadt->Flags, SLEEP_TYPE);
	PRINTFLAG(fadt->Flags, PCI_EXPRESS_WAKE);
	PRINTFLAG(fadt->Flags, PLATFORM_CLOCK);
	PRINTFLAG(fadt->Flags, S4_RTC_VALID);
	PRINTFLAG(fadt->Flags, REMOTE_POWER_ON);
	PRINTFLAG(fadt->Flags, APIC_CLUSTER);
	PRINTFLAG(fadt->Flags, APIC_PHYSICAL);
	PRINTFLAG(fadt->Flags, HW_REDUCED);
	PRINTFLAG(fadt->Flags, LOW_POWER_S0);
	PRINTFLAG_END();

	if (sdp->Length < ACPI_FADT_V2_SIZE)
		goto out;

	if (fadt->Flags & ACPI_FADT_RESET_REGISTER) {
		printf("\tRESET_REG=");
		acpi_print_gas(&fadt->ResetRegister);
		printf(", RESET_VALUE=%#x\n", fadt->ResetValue);
	}

	printf("\tArmBootFlags=");
	PRINTFLAG(fadt->ArmBootFlags, PSCI_COMPLIANT);
	PRINTFLAG(fadt->ArmBootFlags, PSCI_USE_HVC);
	PRINTFLAG_END();

#undef PRINTFLAG

	printf("\tMinorRevision=%u\n", fadt->MinorRevision);

	if (sdp->Length < ACPI_FADT_V3_SIZE)
		goto out;

	printf("\tX_FACS=0x%016jx, ", (uintmax_t)fadt->XFacs);
	printf("X_DSDT=0x%016jx\n", (uintmax_t)fadt->XDsdt);
	printf("\tX_PM1a_EVT_BLK=");
	acpi_print_gas(&fadt->XPm1aEventBlock);
	if (fadt->XPm1bEventBlock.Address != 0) {
		printf("\n\tX_PM1b_EVT_BLK=");
		acpi_print_gas(&fadt->XPm1bEventBlock);
	}
	printf("\n\tX_PM1a_CNT_BLK=");
	acpi_print_gas(&fadt->XPm1aControlBlock);
	if (fadt->XPm1bControlBlock.Address != 0) {
		printf("\n\tX_PM1b_CNT_BLK=");
		acpi_print_gas(&fadt->XPm1bControlBlock);
	}
	if (fadt->XPm2ControlBlock.Address != 0) {
		printf("\n\tX_PM2_CNT_BLK=");
		acpi_print_gas(&fadt->XPm2ControlBlock);
	}
	printf("\n\tX_PM_TMR_BLK=");
	acpi_print_gas(&fadt->XPmTimerBlock);
	if (fadt->XGpe0Block.Address != 0) {
		printf("\n\tX_GPE0_BLK=");
		acpi_print_gas(&fadt->XGpe0Block);
	}
	if (fadt->XGpe1Block.Address != 0) {
		printf("\n\tX_GPE1_BLK=");
		acpi_print_gas(&fadt->XGpe1Block);
	}
	printf("\n");

	if (sdp->Length < ACPI_FADT_V5_SIZE)
		goto out;

	if (fadt->SleepControl.Address != 0) {
		printf("\tSleepControl=");
		acpi_print_gas(&fadt->SleepControl);
		printf("\n");
	}
	if (fadt->SleepStatus.Address != 0) {
		printf("\n\tSleepStatus=");
		acpi_print_gas(&fadt->SleepStatus);
		printf("\n");
	}

	if (sdp->Length < ACPI_FADT_V6_SIZE)
		goto out;

	printf("\tHypervisorId=0x%016"PRIx64"\n", fadt->HypervisorId);

out:
	printf(END_COMMENT);
}

static void
acpi_print_facs(ACPI_TABLE_FACS *facs)
{
	printf(BEGIN_COMMENT);
	printf("  FACS:\tLength=%u, ", facs->Length);
	printf("HwSig=0x%08x, ", facs->HardwareSignature);
	printf("Firm_Wake_Vec=0x%08x\n", facs->FirmwareWakingVector);

#define PRINTFLAG(var, flag)	printflag((var), ACPI_GLOCK_## flag, #flag)

	printf("\tGlobal_Lock=");
	PRINTFLAG(facs->GlobalLock, PENDING);
	PRINTFLAG(facs->GlobalLock, OWNED);
	PRINTFLAG_END();

#undef PRINTFLAG

#define PRINTFLAG(var, flag)	printflag((var), ACPI_FACS_## flag, #flag)

	printf("\tFlags=");
	PRINTFLAG(facs->Flags, S4_BIOS_PRESENT);
	PRINTFLAG(facs->Flags, 64BIT_WAKE);
	PRINTFLAG_END();

#undef PRINTFLAG

	if (facs->XFirmwareWakingVector != 0)
		printf("\tX_Firm_Wake_Vec=%016jx\n",
		    (uintmax_t)facs->XFirmwareWakingVector);
	printf("\tVersion=%u\n", facs->Version);

	printf("\tOspmFlags={");
	if (facs->OspmFlags & ACPI_FACS_64BIT_ENVIRONMENT)
		printf("64BIT_WAKE");
	printf("}\n");

	printf(END_COMMENT);
}

static void
acpi_print_dsdt(ACPI_TABLE_HEADER *dsdp)
{
	printf(BEGIN_COMMENT);
	acpi_print_sdt(dsdp);
	printf(END_COMMENT);
}

int
acpi_checksum(void *p, size_t length)
{
	uint8_t *bp;
	uint8_t sum;

	bp = p;
	sum = 0;
	while (length--)
		sum += *bp++;

	return (sum);
}

static ACPI_TABLE_HEADER *
acpi_map_sdt(vm_offset_t pa)
{
	ACPI_TABLE_HEADER *sp;

	sp = acpi_map_physical(pa, sizeof(ACPI_TABLE_HEADER));
	sp = acpi_map_physical(pa, sp->Length);
	return (sp);
}

static void
acpi_print_rsd_ptr(ACPI_TABLE_RSDP *rp)
{
	printf(BEGIN_COMMENT);
	printf("  RSD PTR: OEM=");
	acpi_print_string(rp->OemId, ACPI_OEM_ID_SIZE);
	printf(", ACPI_Rev=%s (%d)\n", rp->Revision < 2 ? "1.0x" : "2.0x",
	       rp->Revision);
	if (rp->Revision < 2) {
		printf("\tRSDT=0x%08x, cksum=%u\n", rp->RsdtPhysicalAddress,
		    rp->Checksum);
	} else {
		printf("\tXSDT=0x%016jx, length=%u, cksum=%u\n",
		    (uintmax_t)rp->XsdtPhysicalAddress, rp->Length,
		    rp->ExtendedChecksum);
	}
	printf(END_COMMENT);
}

static void
acpi_handle_rsdt(ACPI_TABLE_HEADER *rsdp)
{
	ACPI_TABLE_HEADER *sdp;
	ACPI_TABLE_RSDT *rsdt;
	ACPI_TABLE_XSDT *xsdt;
	vm_offset_t addr = 0;
	int entries, i;

	acpi_print_rsdt(rsdp);
	rsdt = (ACPI_TABLE_RSDT *)rsdp;
	xsdt = (ACPI_TABLE_XSDT *)rsdp;
	entries = (rsdp->Length - sizeof(ACPI_TABLE_HEADER)) / addr_size;
	for (i = 0; i < entries; i++) {
		if (addr_size == 4)
			addr = le32toh(rsdt->TableOffsetEntry[i]);
		else
			addr = le64toh(xsdt->TableOffsetEntry[i]);
		if (addr == 0)
			continue;
		sdp = (ACPI_TABLE_HEADER *)acpi_map_sdt(addr);
		if (acpi_checksum(sdp, sdp->Length)) {
			warnx("RSDT entry %d (sig %.4s) is corrupt", i,
			    sdp->Signature);
			if (sflag)
				continue;
		}
		if (!memcmp(sdp->Signature, ACPI_SIG_FADT, 4))
			acpi_handle_fadt(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_BERT, 4))
			acpi_handle_bert(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_BOOT, 4))
			acpi_handle_boot(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_CPEP, 4))
			acpi_handle_cpep(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_DBGP, 4))
			acpi_handle_dbgp(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_DBG2, 4))
			acpi_handle_dbg2(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_DMAR, 4))
			acpi_handle_dmar(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_EINJ, 4))
			acpi_handle_einj(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_ERST, 4))
			acpi_handle_erst(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_MADT, 4))
			acpi_handle_madt(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_MSCT, 4))
			acpi_handle_msct(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_HEST, 4))
			acpi_handle_hest(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_HPET, 4))
			acpi_handle_hpet(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_ECDT, 4))
			acpi_handle_ecdt(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_MCFG, 4))
			acpi_handle_mcfg(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_SBST, 4))
			acpi_handle_sbst(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_SLIT, 4))
			acpi_handle_slit(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_SPCR, 4))
			acpi_handle_spcr(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_SPMI, 4))
			acpi_handle_spmi(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_SRAT, 4))
			acpi_handle_srat(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_TCPA, 4))
			acpi_handle_tcpa(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_NFIT, 4))
			acpi_handle_nfit(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_UEFI, 4))
			acpi_handle_uefi(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_WAET, 4))
			acpi_handle_waet(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_WDAT, 4))
			acpi_handle_wdat(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_WDDT, 4))
			acpi_handle_wddt(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_WDRT, 4))
			acpi_handle_wdrt(sdp);
		else {
			printf(BEGIN_COMMENT);
			acpi_print_sdt(sdp);
			acpi_dump_bytes(sdp);
			printf(END_COMMENT);
		}
	}
}

ACPI_TABLE_HEADER *
sdt_load_devmem(void)
{
	ACPI_TABLE_RSDP *rp;
	ACPI_TABLE_HEADER *rsdp;

	rp = acpi_find_rsd_ptr();
	if (!rp)
		errx(EXIT_FAILURE, "Can't find ACPI information");

	if (tflag)
		acpi_print_rsd_ptr(rp);
	if (rp->Revision < 2) {
		rsdp = (ACPI_TABLE_HEADER *)acpi_map_sdt(rp->RsdtPhysicalAddress);
		if (memcmp(rsdp->Signature, "RSDT", 4) != 0 ||
		    acpi_checksum(rsdp, rsdp->Length) != 0)
			errx(EXIT_FAILURE, "RSDT is corrupted");
		addr_size = sizeof(uint32_t);
	} else {
		rsdp = (ACPI_TABLE_HEADER *)acpi_map_sdt(rp->XsdtPhysicalAddress);
		if (memcmp(rsdp->Signature, "XSDT", 4) != 0 ||
		    acpi_checksum(rsdp, rsdp->Length) != 0)
			errx(EXIT_FAILURE, "XSDT is corrupted");
		addr_size = sizeof(uint64_t);
	}
	return (rsdp);
}

/* Write the DSDT to a file, concatenating any SSDTs (if present). */
static int
write_dsdt(int fd, ACPI_TABLE_HEADER *rsdt, ACPI_TABLE_HEADER *dsdt)
{
	ACPI_TABLE_HEADER sdt;
	ACPI_TABLE_HEADER *ssdt;
	uint8_t sum;

	/* Create a new checksum to account for the DSDT and any SSDTs. */
	sdt = *dsdt;
	if (rsdt != NULL) {
		sdt.Checksum = 0;
		sum = acpi_checksum(dsdt + 1, dsdt->Length -
		    sizeof(ACPI_TABLE_HEADER));
		ssdt = sdt_from_rsdt(rsdt, ACPI_SIG_SSDT, NULL);
		while (ssdt != NULL) {
			sdt.Length += ssdt->Length - sizeof(ACPI_TABLE_HEADER);
			sum += acpi_checksum(ssdt + 1,
			    ssdt->Length - sizeof(ACPI_TABLE_HEADER));
			ssdt = sdt_from_rsdt(rsdt, ACPI_SIG_SSDT, ssdt);
		}
		sum += acpi_checksum(&sdt, sizeof(ACPI_TABLE_HEADER));
		sdt.Checksum -= sum;
	}

	/* Write out the DSDT header and body. */
	write(fd, &sdt, sizeof(ACPI_TABLE_HEADER));
	write(fd, dsdt + 1, dsdt->Length - sizeof(ACPI_TABLE_HEADER));

	/* Write out any SSDTs (if present.) */
	if (rsdt != NULL) {
		ssdt = sdt_from_rsdt(rsdt, "SSDT", NULL);
		while (ssdt != NULL) {
			write(fd, ssdt + 1, ssdt->Length -
			    sizeof(ACPI_TABLE_HEADER));
			ssdt = sdt_from_rsdt(rsdt, "SSDT", ssdt);
		}
	}
	return (0);
}

void
dsdt_save_file(char *outfile, ACPI_TABLE_HEADER *rsdt, ACPI_TABLE_HEADER *dsdp)
{
	int	fd;
	mode_t	mode;

	assert(outfile != NULL);
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, mode);
	if (fd == -1) {
		perror("dsdt_save_file");
		return;
	}
	write_dsdt(fd, rsdt, dsdp);
	close(fd);
}

void
aml_disassemble(ACPI_TABLE_HEADER *rsdt, ACPI_TABLE_HEADER *dsdp)
{
	char buf[MAXPATHLEN], tmpstr[MAXPATHLEN], wrkdir[MAXPATHLEN];
	const char *iname = "/acpdump.din";
	const char *oname = "/acpdump.dsl";
	const char *tmpdir;
	FILE *fp;
	size_t len;
	int fd, status;
	pid_t pid;

	if (rsdt == NULL)
		errx(EXIT_FAILURE, "aml_disassemble: invalid rsdt");
	if (dsdp == NULL)
		errx(EXIT_FAILURE, "aml_disassemble: invalid dsdp");

	tmpdir = getenv("TMPDIR");
	if (tmpdir == NULL)
		tmpdir = _PATH_TMP;
	if (realpath(tmpdir, buf) == NULL) {
		perror("realpath tmp dir");
		return;
	}
	len = sizeof(wrkdir) - strlen(iname);
	if ((size_t)snprintf(wrkdir, len, "%s/acpidump.XXXXXX", buf) > len-1 ) {
		fprintf(stderr, "$TMPDIR too long\n");
		return;
	}
	if  (mkdtemp(wrkdir) == NULL) {
		perror("mkdtemp tmp working dir");
		return;
	}
	len = (size_t)snprintf(tmpstr, sizeof(tmpstr), "%s%s", wrkdir, iname);
	assert(len <= sizeof(tmpstr) - 1);
	fd = open(tmpstr, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		perror("iasl tmp file");
		return;
	}
	write_dsdt(fd, rsdt, dsdp);
	close(fd);

	/* Run iasl -d on the temp file */
	if ((pid = fork()) == 0) {
		close(STDOUT_FILENO);
		if (vflag == 0)
			close(STDERR_FILENO);
		execl("/usr/bin/iasl", "iasl", "-d", tmpstr, NULL);
		err(EXIT_FAILURE, "exec");
	}
	if (pid > 0)
		wait(&status);
	if (unlink(tmpstr) < 0) {
		perror("unlink");
		goto out;
	}
	if (pid < 0) {
		perror("fork");
		goto out;
	}
	if (status != 0) {
		fprintf(stderr, "iast exit status = %d\n", status);
	}

	/* Dump iasl's output to stdout */
	len = (size_t)snprintf(tmpstr, sizeof(tmpstr), "%s%s", wrkdir, oname);
	assert(len <= sizeof(tmpstr) - 1);
	fp = fopen(tmpstr, "r");
	if (unlink(tmpstr) < 0) {
		perror("unlink");
		goto out;
	}
	if (fp == NULL) {
		perror("iasl tmp file (read)");
		goto out;
	}
	while ((len = fread(buf, 1, sizeof(buf), fp)) > 0)
		fwrite(buf, 1, len, stdout);
	fclose(fp);

    out:
	if (rmdir(wrkdir) < 0)
		perror("rmdir");
}

void
sdt_print_all(ACPI_TABLE_HEADER *rsdp)
{
	acpi_handle_rsdt(rsdp);
}

/* Fetch a table matching the given signature via the RSDT. */
ACPI_TABLE_HEADER *
sdt_from_rsdt(ACPI_TABLE_HEADER *rsdp, const char *sig, ACPI_TABLE_HEADER *last)
{
	ACPI_TABLE_HEADER *sdt;
	ACPI_TABLE_RSDT *rsdt;
	ACPI_TABLE_XSDT *xsdt;
	vm_offset_t addr = 0;
	int entries, i;

	rsdt = (ACPI_TABLE_RSDT *)rsdp;
	xsdt = (ACPI_TABLE_XSDT *)rsdp;
	entries = (rsdp->Length - sizeof(ACPI_TABLE_HEADER)) / addr_size;
	for (i = 0; i < entries; i++) {
		if (addr_size == 4)
			addr = le32toh(rsdt->TableOffsetEntry[i]);
		else
			addr = le64toh(xsdt->TableOffsetEntry[i]);
		if (addr == 0)
			continue;
		sdt = (ACPI_TABLE_HEADER *)acpi_map_sdt(addr);
		if (last != NULL) {
			if (sdt == last)
				last = NULL;
			continue;
		}
		if (memcmp(sdt->Signature, sig, strlen(sig)))
			continue;
		if (acpi_checksum(sdt, sdt->Length))
			errx(EXIT_FAILURE, "RSDT entry %d is corrupt", i);
		return (sdt);
	}

	return (NULL);
}

ACPI_TABLE_HEADER *
dsdt_from_fadt(ACPI_TABLE_FADT *fadt)
{
	ACPI_TABLE_HEADER	*sdt;

	/* Use the DSDT address if it is version 1, otherwise use XDSDT. */
	sdt = (ACPI_TABLE_HEADER *)acpi_map_sdt(
		acpi_select_address(fadt->Dsdt, fadt->XDsdt));
	if (acpi_checksum(sdt, sdt->Length))
		errx(EXIT_FAILURE, "DSDT is corrupt");
	return (sdt);
}
