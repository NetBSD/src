/* $NetBSD: acpi.c,v 1.5 2009/12/22 08:44:03 cegger Exp $ */

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
 *	$FreeBSD: src/usr.sbin/acpi/acpidump/acpi.c,v 1.37 2009/08/25 20:35:57 jhb Exp $
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: acpi.c,v 1.5 2009/12/22 08:44:03 cegger Exp $");

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

#include "acpidump.h"

#define BEGIN_COMMENT	"/*\n"
#define END_COMMENT	" */\n"

static void	acpi_print_string(char *s, size_t length);
static void	acpi_print_gas(ACPI_GENERIC_ADDRESS *gas);
static void	acpi_print_pci(uint16_t vendorid, uint16_t deviceid,
		    uint8_t seg, uint8_t bus, uint8_t device, uint8_t func);
static void	acpi_print_pci_sbfd(uint8_t seg, uint8_t bus, uint8_t device,
		    uint8_t func);
#ifdef notyet
static void	acpi_print_hest_generic_status(ACPI_HEST_GENERIC_STATUS *);
static void	acpi_print_hest_generic_data(ACPI_HEST_GENERIC_DATA *);
#endif
static void	acpi_print_whea(ACPI_WHEA_HEADER *whea,
		    void (*print_action)(ACPI_WHEA_HEADER *),
		    void (*print_ins)(ACPI_WHEA_HEADER *),
		    void (*print_flags)(ACPI_WHEA_HEADER *));
static int	acpi_get_fadt_revision(ACPI_TABLE_FADT *fadt);
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
static void	acpi_print_srat_cpu(uint32_t apic_id,
		    uint32_t proximity_domain,
		    uint32_t flags, uint32_t clockdomain);
static void	acpi_print_srat_memory(ACPI_SRAT_MEM_AFFINITY *mp);
static void	acpi_print_srat(ACPI_SUBTABLE_HEADER *srat);
static void	acpi_handle_srat(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_tcpa(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_waet(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_wdat(ACPI_TABLE_HEADER *sdp);
static void	acpi_handle_wdrt(ACPI_TABLE_HEADER *sdp);
static void	acpi_print_sdt(ACPI_TABLE_HEADER *sdp);
static void	acpi_print_fadt(ACPI_TABLE_HEADER *sdp);
static void	acpi_print_facs(ACPI_TABLE_FACS *facs);
static void	acpi_print_dsdt(ACPI_TABLE_HEADER *dsdp);
static ACPI_TABLE_HEADER *acpi_map_sdt(vm_offset_t pa);
static void	acpi_print_rsd_ptr(ACPI_TABLE_RSDP *rp);
static void	acpi_handle_rsdt(ACPI_TABLE_HEADER *rsdp);
static void	acpi_walk_subtables(ACPI_TABLE_HEADER *table, void *first,
		    void (*action)(ACPI_SUBTABLE_HEADER *));

/* Size of an address. 32-bit for ACPI 1.0, 64-bit for ACPI 2.0 and up. */
static int addr_size;

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
	case ACPI_GAS_MEMORY:
		printf("0x%08lx:%u[%u] (Memory)", (u_long)gas->Address,
		       gas->BitOffset, gas->BitWidth);
		break;
	case ACPI_GAS_IO:
		printf("0x%02lx:%u[%u] (IO)", (u_long)gas->Address,
		       gas->BitOffset, gas->BitWidth);
		break;
	case ACPI_GAS_PCI:
		printf("%x:%x+0x%x (PCI)", (uint16_t)(gas->Address >> 32),
		       (uint16_t)((gas->Address >> 16) & 0xffff),
		       (uint16_t)gas->Address);
		break;
	/* XXX How to handle these below? */
	case ACPI_GAS_EMBEDDED:
		printf("0x%x:%u[%u] (EC)", (uint16_t)gas->Address,
		       gas->BitOffset, gas->BitWidth);
		break;
	case ACPI_GAS_SMBUS:
		printf("0x%x:%u[%u] (SMBus)", (uint16_t)gas->Address,
		       gas->BitOffset, gas->BitWidth);
		break;
	case ACPI_GAS_CMOS:
	case ACPI_GAS_PCIBAR:
	case ACPI_GAS_DATATABLE:
	case ACPI_GAS_FIXED:
	default:
		printf("0x%08lx (?)", (u_long)gas->Address);
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
acpi_print_pci_sbfd(uint8_t seg, uint8_t bus, uint8_t device, uint8_t func)
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
	printf("\tClear Status On Init={ %s }\n",
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
	printf("\tType={ ");
	switch (hest->Type) {
	case ACPI_HEST_TYPE_IA32_CHECK:
		printf("IA32 Machine Check Exception");
		break;
	case ACPI_HEST_TYPE_IA32_CORRECTED_CHECK:
		printf("IA32 Corrected Machine Check\n");
		break;
	case ACPI_HEST_TYPE_IA32_NMI:
		printf("IA32 Non-Maskable Interrupt\n");
		break;
	case ACPI_HEST_TYPE_NOT_USED3:
	case ACPI_HEST_TYPE_NOT_USED4:
	case ACPI_HEST_TYPE_NOT_USED5:
		printf("unused type: %d\n", hest->Type);
		break;
	case ACPI_HEST_TYPE_AER_ROOT_PORT:
		printf("PCI Express Root Port AER\n");
		break;
	case ACPI_HEST_TYPE_AER_ENDPOINT:
		printf("PCI Express Endpoint AER\n");
		break;
	case ACPI_HEST_TYPE_AER_BRIDGE:
		printf("PCI Express/PCI-X Bridge AER\n");
		break;
	case ACPI_HEST_TYPE_GENERIC_ERROR:
		printf("Generic Hardware Error Source\n");
		break;
	case ACPI_HEST_TYPE_RESERVED:
	default:
		printf("Reserved\n");
		break;
	}
	printf(" }\n");
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
		acpi_print_pci_sbfd(0, data->Bus, data->Device, data->Function);
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
	printf("\t\tType={ ");
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
	case ACPI_HEST_NOTIFY_RESERVED:
		printf("RESERVED");
		break;
	default:
		printf(" %d (reserved)", notify->Type);
		break;
	}
	printf(" }\n");

	printf("\t\tLength=%d\n", notify->Length);
	printf("\t\tConfig Write Enable={\n");
	if (notify->ConfigWriteEnable & ACPI_HEST_TYPE)
		printf("TYPE");
	if (notify->ConfigWriteEnable & ACPI_HEST_POLL_INTERVAL)
		printf("POLL INTERVAL");
	if (notify->ConfigWriteEnable & ACPI_HEST_POLL_THRESHOLD_VALUE)
		printf("THRESHOLD VALUE");
	if (notify->ConfigWriteEnable & ACPI_HEST_POLL_THRESHOLD_WINDOW)
		printf("THRESHOLD WINDOW");
	if (notify->ConfigWriteEnable & ACPI_HEST_ERR_THRESHOLD_VALUE)
		printf("THRESHOLD VALUE");
	if (notify->ConfigWriteEnable & ACPI_HEST_ERR_THRESHOLD_WINDOW)
		printf("THRESHOLD WINDOW");
	printf("}\n");

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
	printf("\tEnabled={ %s }\n", data->Enabled ? "YES" : "NO");
	printf("\tNumber of Records to pre-allocate=%d\n",
		data->RecordsToPreallocate);
	printf("\tMax Sections per Record=%d\n",
		data->MaxSectionsPerRecord);
	printf("\tMax Raw Data Length=%d\n", data->MaxRawDataLength);
	printf("\tError Status Address=");
	acpi_print_gas(&data->ErrorStatusAddress);
	acpi_print_hest_notify(&data->Notify);
	printf("\tError Block Length=%d\n", data->ErrorBlockLength);
}

static void
acpi_handle_hest(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_HEST *hest;
	ACPI_HEST_HEADER *subhest;
	uint32_t i, pos;
	void *subtable;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	hest = (ACPI_TABLE_HEST *)sdp;

	printf("\tError Source Count=%d\n", hest->ErrorSourceCount);
	pos = sizeof(ACPI_TABLE_HEST);
	for (i = 0; i < hest->ErrorSourceCount; i++) {
		subhest = (ACPI_HEST_HEADER *)((char *)hest + pos);
		subtable = (void *)((char *)subhest + sizeof(ACPI_HEST_HEADER));
		printf("\n");

		printf("\tType={ ");
		switch (subhest->Type) {
		case ACPI_HEST_TYPE_IA32_CHECK:
			acpi_print_hest_ia32_check(subtable);
			pos += sizeof(ACPI_HEST_IA_MACHINE_CHECK);
			break;

		case ACPI_HEST_TYPE_IA32_CORRECTED_CHECK:
			acpi_print_hest_ia32_correctedcheck(subtable);
			pos += sizeof(ACPI_HEST_IA_CORRECTED);
			break;

		case ACPI_HEST_TYPE_IA32_NMI:
			acpi_print_hest_ia32_nmi(subtable);
			pos += sizeof(ACPI_HEST_IA_NMI);
			break;

		case ACPI_HEST_TYPE_NOT_USED3:
		case ACPI_HEST_TYPE_NOT_USED4:
		case ACPI_HEST_TYPE_NOT_USED5:
			pos += sizeof(ACPI_HEST_HEADER);
			break;

		case ACPI_HEST_TYPE_AER_ROOT_PORT:
			acpi_print_hest_aer_root(subtable);
			pos += sizeof(ACPI_HEST_AER_ROOT);
			break;

		case ACPI_HEST_TYPE_AER_ENDPOINT:
			acpi_print_hest_aer_endpoint(subtable);
			pos += sizeof(ACPI_HEST_AER_ROOT);
			break;

		case ACPI_HEST_TYPE_AER_BRIDGE:
			acpi_print_hest_aer_bridge(subtable);
			pos += sizeof(ACPI_HEST_AER_BRIDGE);
			break;

		case ACPI_HEST_TYPE_GENERIC_ERROR:
			acpi_print_hest_generic(subtable);
			pos += sizeof(ACPI_HEST_GENERIC);
			break;

		case ACPI_HEST_TYPE_RESERVED:
		default:
			pos += sizeof(ACPI_HEST_HEADER);
			break;
		}
	}

	printf(END_COMMENT);
}

/* The FADT revision indicates whether we use the DSDT or X_DSDT addresses. */
static int
acpi_get_fadt_revision(ACPI_TABLE_FADT *fadt)
{
	int fadt_revision;

	/* Set the FADT revision separately from the RSDP version. */
	if (addr_size == 8) {
		fadt_revision = 2;

		/*
		 * A few systems (e.g., IBM T23) have an RSDP that claims
		 * revision 2 but the 64 bit addresses are invalid.  If
		 * revision 2 and the 32 bit address is non-zero but the
		 * 32 and 64 bit versions don't match, prefer the 32 bit
		 * version for all subsequent tables.
		 */
		if (fadt->Facs != 0 &&
		    (fadt->XFacs & 0xffffffff) != fadt->Facs)
			fadt_revision = 1;
	} else
		fadt_revision = 1;
	return (fadt_revision);
}

static void
acpi_handle_fadt(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_HEADER *dsdp;
	ACPI_TABLE_FACS	*facs;
	ACPI_TABLE_FADT *fadt;
	int		fadt_revision;

	fadt = (ACPI_TABLE_FADT *)sdp;
	acpi_print_fadt(sdp);

	fadt_revision = acpi_get_fadt_revision(fadt);
	if (fadt_revision == 1)
		facs = (ACPI_TABLE_FACS *)acpi_map_sdt(fadt->Facs);
	else
		facs = (ACPI_TABLE_FACS *)acpi_map_sdt(fadt->XFacs);
	if (memcmp(facs->Signature, ACPI_SIG_FACS, 4) != 0 || facs->Length < 64)
		errx(EXIT_FAILURE, "FACS is corrupt");
	acpi_print_facs(facs);

	if (fadt_revision == 1)
		dsdp = (ACPI_TABLE_HEADER *)acpi_map_sdt(fadt->Dsdt);
	else
		dsdp = (ACPI_TABLE_HEADER *)acpi_map_sdt(fadt->XDsdt);
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
		action(subtable);
		subtable = (ACPI_SUBTABLE_HEADER *)((char *)subtable +
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

const char *apic_types[] = { "Local APIC", "IO APIC", "INT Override", "NMI",
			     "Local APIC NMI", "Local APIC Override",
			     "IO SAPIC", "Local SAPIC", "Platform Interrupt",
			     "Local X2APIC", "Local X2APIC NMI" };
const char *platform_int_types[] = { "0 (unknown)", "PMI", "INIT",
				     "Corrected Platform Error" };

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

	if (mp->Type < sizeof(apic_types) / sizeof(apic_types[0]))
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
		if (isrc->Type < sizeof(platform_int_types) /
		    sizeof(platform_int_types[0]))
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
	printf("DebugPort=");
	acpi_print_gas(&dbgp->DebugPort);
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

	printf("\tFLAGS={ ");
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

	printf("\tFLAGS={ ");
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
		printf("\tBase Address=0x%016jx\n", alloc->Address);
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
	printf("\tLocalityCount=%"PRIu64"\n", slit->LocalityCount);
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

const char *srat_types[] = { "CPU", "Memory", "X2APIC" };

static void
acpi_print_srat(ACPI_SUBTABLE_HEADER *srat)
{
	ACPI_SRAT_CPU_AFFINITY *cpu;
	ACPI_SRAT_X2APIC_CPU_AFFINITY *x2apic;

	if (srat->Type < sizeof(srat_types) / sizeof(srat_types[0]))
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

static void
acpi_handle_tcpa(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_TCPA *tcpa;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	tcpa = (ACPI_TABLE_TCPA *)sdp;

	printf("\tMaximum Length of Event Log Area=%d\n", tcpa->MaxLogLength);
	printf("\tPhysical Address of Log Area=0x%08"PRIx64"\n",
	    tcpa->LogAddress);

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
		printf(", Preserve Register ");

	printf("}\n");
}

static void
acpi_handle_wdat(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_WDAT *wdat;
	ACPI_WHEA_HEADER *whea;
	char *wdat_pos;
	u_int i;

	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	wdat = (ACPI_TABLE_WDAT *)sdp;

	printf("\tHeader Length=%d\n", wdat->HeaderLength);

	acpi_print_pci_sbfd(wdat->PciSegment, wdat->PciBus, wdat->PciDevice,
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

	wdat_pos = ((char *)wdat + sizeof(ACPI_TABLE_HEADER)
	    + wdat->HeaderLength);

	for (i = 0; i < wdat->Entries; i++) {
		whea = (ACPI_WHEA_HEADER *)wdat_pos;
		acpi_print_whea(whea,
		    acpi_print_wdat_action, acpi_print_wdat_instruction,
		    NULL);
		wdat_pos += sizeof(ACPI_WDAT_ENTRY);
	}
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
	printf(": Length=%d, Revision=%d, Checksum=%d,\n",
	       sdp->Length, sdp->Revision, sdp->Checksum);
	printf("\tOEMID=");
	acpi_print_string(sdp->OemId, ACPI_OEM_ID_SIZE);
	printf(", OEM Table ID=");
	acpi_print_string(sdp->OemTableId, ACPI_OEM_TABLE_ID_SIZE);
	printf(", OEM Revision=0x%x,\n", sdp->OemRevision);
	printf("\tCreator ID=");
	acpi_print_string(sdp->AslCompilerId, ACPI_NAME_SIZE);
	printf(", Creator Revision=0x%x\n", sdp->AslCompilerRevision);
}

static void
acpi_print_rsdt(ACPI_TABLE_HEADER *rsdp)
{
	ACPI_TABLE_RSDT *rsdt;
	ACPI_TABLE_XSDT *xsdt;
	int	i, entries;
	u_long	addr;

	rsdt = (ACPI_TABLE_RSDT *)rsdp;
	xsdt = (ACPI_TABLE_XSDT *)rsdp;
	printf(BEGIN_COMMENT);
	acpi_print_sdt(rsdp);
	entries = (rsdp->Length - sizeof(ACPI_TABLE_HEADER)) / addr_size;
	printf("\tEntries={ ");
	for (i = 0; i < entries; i++) {
		if (i > 0)
			printf(", ");
		switch (addr_size) {
		case 4:
			addr = le32toh(rsdt->TableOffsetEntry[i]);
			break;
		case 8:
			addr = le64toh(xsdt->TableOffsetEntry[i]);
			break;
		default:
			addr = 0;
		}
		assert(addr != 0);
		printf("0x%08lx", addr);
	}
	printf(" }\n");
	printf(END_COMMENT);
}

static const char *acpi_pm_profiles[] = {
	"Unspecified", "Desktop", "Mobile", "Workstation",
	"Enterprise Server", "SOHO Server", "Appliance PC"
};

static void
acpi_print_fadt(ACPI_TABLE_HEADER *sdp)
{
	ACPI_TABLE_FADT *fadt;
	const char *pm;
	char	    sep;

	fadt = (ACPI_TABLE_FADT *)sdp;
	printf(BEGIN_COMMENT);
	acpi_print_sdt(sdp);
	printf(" \tFACS=0x%x, DSDT=0x%x\n", fadt->Facs,
	       fadt->Dsdt);
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

#define PRINTFLAG(var, flag) do {			\
	if ((var) & ACPI_FADT_## flag) {		\
		printf("%c%s", sep, #flag); sep = ',';	\
	}						\
} while (0)

	printf("\tIAPC_BOOT_ARCH=");
	sep = '{';
	PRINTFLAG(fadt->BootFlags, LEGACY_DEVICES);
	PRINTFLAG(fadt->BootFlags, 8042);
	PRINTFLAG(fadt->BootFlags, NO_VGA);
	PRINTFLAG(fadt->BootFlags, NO_MSI);
	PRINTFLAG(fadt->BootFlags, NO_ASPM);
	if (fadt->BootFlags != 0)
		printf("}");
	printf("\n");

	printf("\tFlags=");
	sep = '{';
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
	if (fadt->Flags != 0)
		printf("}\n");

#undef PRINTFLAG

	if (fadt->Flags & ACPI_FADT_RESET_REGISTER) {
		printf("\tRESET_REG=");
		acpi_print_gas(&fadt->ResetRegister);
		printf(", RESET_VALUE=%#x\n", fadt->ResetValue);
	}
	if (acpi_get_fadt_revision(fadt) > 1) {
		printf("\tX_FACS=0x%08lx, ", (u_long)fadt->XFacs);
		printf("X_DSDT=0x%08lx\n", (u_long)fadt->XDsdt);
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
	}

	printf(END_COMMENT);
}

static void
acpi_print_facs(ACPI_TABLE_FACS *facs)
{
	printf(BEGIN_COMMENT);
	printf("  FACS:\tLength=%u, ", facs->Length);
	printf("HwSig=0x%08x, ", facs->HardwareSignature);
	printf("Firm_Wake_Vec=0x%08x\n", facs->FirmwareWakingVector);

	printf("\tGlobal_Lock=");
	if (facs->GlobalLock != 0) {
		if (facs->GlobalLock & ACPI_GLOCK_PENDING)
			printf("PENDING,");
		if (facs->GlobalLock & ACPI_GLOCK_OWNED)
			printf("OWNED");
	}
	printf("\n");

	printf("\tFlags=");
	if (facs->Flags & ACPI_FACS_S4_BIOS_PRESENT)
		printf("S4BIOS");
	printf("\n");

	if (facs->XFirmwareWakingVector != 0) {
		printf("\tX_Firm_Wake_Vec=%08lx\n",
		       (u_long)facs->XFirmwareWakingVector);
	}
	printf("\tVersion=%u\n", facs->Version);

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
		printf("\tXSDT=0x%08lx, length=%u, cksum=%u\n",
		    (u_long)rp->XsdtPhysicalAddress, rp->Length,
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
	vm_offset_t addr;
	int entries, i;

	acpi_print_rsdt(rsdp);
	rsdt = (ACPI_TABLE_RSDT *)rsdp;
	xsdt = (ACPI_TABLE_XSDT *)rsdp;
	entries = (rsdp->Length - sizeof(ACPI_TABLE_HEADER)) / addr_size;
	for (i = 0; i < entries; i++) {
		switch (addr_size) {
		case 4:
			addr = le32toh(rsdt->TableOffsetEntry[i]);
			break;
		case 8:
			addr = le64toh(xsdt->TableOffsetEntry[i]);
			break;
		default:
			assert((addr = 0));
		}

		sdp = (ACPI_TABLE_HEADER *)acpi_map_sdt(addr);
		if (acpi_checksum(sdp, sdp->Length)) {
			warnx("RSDT entry %d (sig %.4s) is corrupt", i,
			    sdp->Signature);
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
		else if (!memcmp(sdp->Signature, ACPI_SIG_SRAT, 4))
			acpi_handle_srat(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_TCPA, 4))
			acpi_handle_tcpa(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_WAET, 4))
			acpi_handle_waet(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_WDAT, 4))
			acpi_handle_wdat(sdp);
		else if (!memcmp(sdp->Signature, ACPI_SIG_WDRT, 4))
			acpi_handle_wdrt(sdp);
		else {
			printf(BEGIN_COMMENT);
			acpi_print_sdt(sdp);
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
	char buf[PATH_MAX], tmpstr[PATH_MAX];
	const char *tmpdir;
	char *tmpext;
	FILE *fp;
	size_t len;
	int fd;

	if (rsdt == NULL)
		errx(EXIT_FAILURE, "aml_disassemble: invalid rsdt");
	if (dsdp == NULL)
		errx(EXIT_FAILURE, "aml_disassemble: invalid dsdp");

	tmpdir = getenv("TMPDIR");
	if (tmpdir == NULL)
		tmpdir = _PATH_TMP;
	strlcpy(tmpstr, tmpdir, sizeof(tmpstr));
	if (realpath(tmpstr, buf) == NULL) {
		perror("realpath tmp file");
		return;
	}
	strlcpy(tmpstr, buf, sizeof(tmpstr));
	strlcat(tmpstr, "/acpidump.", sizeof(tmpstr));
	len = strlen(tmpstr);
	tmpext = tmpstr + len;
	strlcpy(tmpext, "XXXXXX", sizeof(tmpstr) - len);
	fd = mkstemp(tmpstr);
	if (fd < 0) {
		perror("iasl tmp file");
		return;
	}
	write_dsdt(fd, rsdt, dsdp);
	close(fd);

	/* Run iasl -d on the temp file */
	if (fork() == 0) {
		close(STDOUT_FILENO);
		if (vflag == 0)
			close(STDERR_FILENO);
		execl("/usr/bin/iasl", "iasl", "-d", tmpstr, NULL);
		err(EXIT_FAILURE, "exec");
	}

	wait(NULL);
	unlink(tmpstr);

	/* Dump iasl's output to stdout */
	strlcpy(tmpext, "dsl", sizeof(tmpstr) - len);
	fp = fopen(tmpstr, "r");
	unlink(tmpstr);
	if (fp == NULL) {
		perror("iasl tmp file (read)");
		return;
	}
	while ((len = fread(buf, 1, sizeof(buf), fp)) > 0)
		fwrite(buf, 1, len, stdout);
	fclose(fp);
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
	vm_offset_t addr;
	int entries, i;

	rsdt = (ACPI_TABLE_RSDT *)rsdp;
	xsdt = (ACPI_TABLE_XSDT *)rsdp;
	entries = (rsdp->Length - sizeof(ACPI_TABLE_HEADER)) / addr_size;
	for (i = 0; i < entries; i++) {
		switch (addr_size) {
		case 4:
			addr = le32toh(rsdt->TableOffsetEntry[i]);
			break;
		case 8:
			addr = le64toh(xsdt->TableOffsetEntry[i]);
			break;
		default:
			assert((addr = 0));
		}
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
	if (acpi_get_fadt_revision(fadt) == 1)
		sdt = (ACPI_TABLE_HEADER *)acpi_map_sdt(fadt->Dsdt);
	else
		sdt = (ACPI_TABLE_HEADER *)acpi_map_sdt(fadt->XDsdt);
	if (acpi_checksum(sdt, sdt->Length))
		errx(EXIT_FAILURE, "DSDT is corrupt\n");
	return (sdt);
}
