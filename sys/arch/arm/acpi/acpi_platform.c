/* $NetBSD: acpi_platform.c,v 1.12.2.3 2020/04/13 08:03:32 martin Exp $ */

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

#include "com.h"
#include "plcom.h"
#include "wsdisplay.h"
#include "genfb.h"
#include "opt_efi.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_platform.c,v 1.12.2.3 2020/04/13 08:03:32 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#include <uvm/uvm_extern.h>

#include <machine/bootconfig.h>
#include <arm/cpufunc.h>
#include <arm/locore.h>

#include <arm/cortex/gtmr_var.h>

#include <arm/arm/psci.h>
#include <arm/fdt/psci_fdtvar.h>

#include <evbarm/fdt/platform.h>

#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>
#include <dev/ic/ns16550reg.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#if NCOM > 0
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pucvar.h>
#endif

#if NWSDISPLAY > 0 && NGENFB > 0
#include <arm/acpi/acpi_simplefb.h>
#endif

#ifdef EFI_RUNTIME
#include <arm/arm/efi_runtime.h>
#endif

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <arm/acpi/acpi_table.h>

#include <libfdt.h>

#define	SPCR_BAUD_UNKNOWN			0
#define	SPCR_BAUD_9600				3
#define	SPCR_BAUD_19200				4
#define	SPCR_BAUD_57600				6
#define	SPCR_BAUD_115200			7

extern struct bus_space arm_generic_bs_tag;
extern struct bus_space arm_generic_a4x_bs_tag;

#if NPLCOM > 0
static struct plcom_instance plcom_console;
#endif

struct arm32_bus_dma_tag acpi_coherent_dma_tag;
static struct arm32_dma_range acpi_coherent_ranges[] = {
	[0] = {
		.dr_sysbase = 0,
		.dr_busbase = 0,
		.dr_len = UINTPTR_MAX,
	 	.dr_flags = _BUS_DMAMAP_COHERENT,
	}
};

static const struct pmap_devmap *
acpi_platform_devmap(void)
{
	static const struct pmap_devmap devmap[] = {
		DEVMAP_ENTRY_END
	};

	return devmap;
}

static void
acpi_platform_bootstrap(void)
{
	extern struct arm32_bus_dma_tag arm_generic_dma_tag;

	acpi_coherent_dma_tag = arm_generic_dma_tag;
	acpi_coherent_dma_tag._ranges = acpi_coherent_ranges;
	acpi_coherent_dma_tag._nranges = __arraycount(acpi_coherent_ranges);
}

static void
acpi_platform_startup(void)
{
	ACPI_TABLE_SPCR *spcr;
	ACPI_TABLE_FADT *fadt;
#ifdef MULTIPROCESSOR
	ACPI_TABLE_MADT *madt;
#endif
	int baud_rate;

	/*
	 * Setup serial console device
	 */
	if (ACPI_SUCCESS(acpi_table_find(ACPI_SIG_SPCR, (void **)&spcr))) {

		switch (spcr->BaudRate) {
		case SPCR_BAUD_9600:
			baud_rate = 9600;
			break;
		case SPCR_BAUD_19200:
			baud_rate = 19200;
			break;
		case SPCR_BAUD_57600:
			baud_rate = 57600;
			break;
		case SPCR_BAUD_115200:
		case SPCR_BAUD_UNKNOWN:
		default:
			baud_rate = 115200;
			break;
		}

		if (spcr->SerialPort.SpaceId == ACPI_ADR_SPACE_SYSTEM_MEMORY &&
		    spcr->SerialPort.Address != 0) {
			switch (spcr->InterfaceType) {
#if NPLCOM > 0
			case ACPI_DBG2_ARM_PL011:
			case ACPI_DBG2_ARM_SBSA_32BIT:
			case ACPI_DBG2_ARM_SBSA_GENERIC:
				plcom_console.pi_type = PLCOM_TYPE_PL011;
				plcom_console.pi_iot = &arm_generic_bs_tag;
				plcom_console.pi_iobase = spcr->SerialPort.Address;
				plcom_console.pi_size = PL011COM_UART_SIZE;
				plcom_console.pi_flags = PLC_FLAG_32BIT_ACCESS;

				plcomcnattach(&plcom_console, baud_rate, 0, TTYDEF_CFLAG, -1);
				break;
#endif
#if NCOM > 0
			case ACPI_DBG2_16550_COMPATIBLE:
			case ACPI_DBG2_16550_SUBSET:
				if (ACPI_ACCESS_BIT_WIDTH(spcr->SerialPort.AccessWidth) == 8) {
					comcnattach(&arm_generic_bs_tag, spcr->SerialPort.Address, baud_rate, -1,
					    COM_TYPE_NORMAL, TTYDEF_CFLAG);
				} else {
					comcnattach(&arm_generic_a4x_bs_tag, spcr->SerialPort.Address, baud_rate, -1,
					    COM_TYPE_NORMAL, TTYDEF_CFLAG);
				}
				break;
			case ACPI_DBG2_BCM2835:
				comcnattach(&arm_generic_a4x_bs_tag, spcr->SerialPort.Address + 0x40, baud_rate, -1,
				    COM_TYPE_BCMAUXUART, TTYDEF_CFLAG);
				cn_set_magic("+++++");
				break;
#endif
			default:
				printf("SPCR: kernel does not support interface type %#x\n", spcr->InterfaceType);
				break;
			}
		}
		acpi_table_unmap((ACPI_TABLE_HEADER *)spcr);
	}

	/*
	 * Initialize PSCI 0.2+ if implemented
	 */
	if (ACPI_SUCCESS(acpi_table_find(ACPI_SIG_FADT, (void **)&fadt))) {
		if (fadt->ArmBootFlags & ACPI_FADT_PSCI_COMPLIANT) {
			if (fadt->ArmBootFlags & ACPI_FADT_PSCI_USE_HVC) {
				psci_init(psci_call_hvc);
			} else {
				psci_init(psci_call_smc);
			}
		}
		acpi_table_unmap((ACPI_TABLE_HEADER *)fadt);
	}

#ifdef MULTIPROCESSOR
	/*
	 * Count CPUs
	 */
	if (ACPI_SUCCESS(acpi_table_find(ACPI_SIG_MADT, (void **)&madt))) {
		char *end = (char *)madt + madt->Header.Length;
		char *where = (char *)madt + sizeof(ACPI_TABLE_MADT);
		while (where < end) {
			ACPI_SUBTABLE_HEADER *subtable = (ACPI_SUBTABLE_HEADER *)where;
			if (subtable->Type == ACPI_MADT_TYPE_GENERIC_INTERRUPT)
				arm_cpu_max++;
			where += subtable->Length;
		}
		acpi_table_unmap((ACPI_TABLE_HEADER *)madt);
	}
#endif /* MULTIPROCESSOR */
}

static void
acpi_platform_init_attach_args(struct fdt_attach_args *faa)
{
	extern struct bus_space arm_generic_bs_tag;
	extern struct bus_space arm_generic_a4x_bs_tag;

	faa->faa_bst = &arm_generic_bs_tag;
	faa->faa_a4x_bst = &arm_generic_a4x_bs_tag;
	faa->faa_dmat = &acpi_coherent_dma_tag;
}

static void
acpi_platform_device_register(device_t self, void *aux)
{
#if NWSDISPLAY > 0 && NGENFB > 0
	if (device_is_a(self, "armfdt")) {
		/*
		 * Setup framebuffer console, if present.
		 */
		acpi_simplefb_preattach();
	}
#endif

#if NCOM > 0
	prop_dictionary_t prop = device_properties(self);

	if (device_is_a(self, "com")) {
		ACPI_TABLE_SPCR *spcr;

		if (ACPI_FAILURE(acpi_table_find(ACPI_SIG_SPCR, (void **)&spcr)))
			return;

		if (spcr->SerialPort.SpaceId != ACPI_ADR_SPACE_SYSTEM_MEMORY)
			goto spcr_unmap;
		if (spcr->SerialPort.Address == 0)
			goto spcr_unmap;
		if (spcr->InterfaceType != ACPI_DBG2_16550_COMPATIBLE &&
		    spcr->InterfaceType != ACPI_DBG2_16550_SUBSET)
			goto spcr_unmap;

		if (device_is_a(device_parent(self), "puc")) {
			const struct puc_attach_args * const paa = aux;
			int b, d, f;

			const int s = pci_get_segment(paa->pc);
			pci_decompose_tag(paa->pc, paa->tag, &b, &d, &f);

			if (spcr->PciSegment == s && spcr->PciBus == b &&
			    spcr->PciDevice == d && spcr->PciFunction == f)
				prop_dictionary_set_bool(prop, "force_console", true);
		}

		if (device_is_a(device_parent(self), "acpi")) {
			struct acpi_attach_args * const aa = aux;
			struct acpi_resources res;
			struct acpi_mem *mem;

			if (ACPI_FAILURE(acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
			    &res, &acpi_resource_parse_ops_quiet)))
				goto spcr_unmap;

			mem = acpi_res_mem(&res, 0);
			if (mem == NULL)
				goto crs_cleanup;

			if (mem->ar_base == spcr->SerialPort.Address)
				prop_dictionary_set_bool(prop, "force_console", true);

crs_cleanup:
			acpi_resource_cleanup(&res);
		}

spcr_unmap:
		acpi_table_unmap((ACPI_TABLE_HEADER *)spcr);
	}
#endif
}

static void
acpi_platform_reset(void)
{
#ifdef EFI_RUNTIME
	if (arm_efirt_reset(EFI_RESET_COLD) == 0)
		return;
#endif
	if (psci_available())
		psci_system_reset();
}

static u_int
acpi_platform_uart_freq(void)
{
	return 0;
}

static const struct arm_platform acpi_platform = {
	.ap_devmap = acpi_platform_devmap,
	.ap_bootstrap = acpi_platform_bootstrap,
	.ap_startup = acpi_platform_startup,
	.ap_init_attach_args = acpi_platform_init_attach_args,
	.ap_device_register = acpi_platform_device_register,
	.ap_reset = acpi_platform_reset,
	.ap_delay = gtmr_delay,
	.ap_uart_freq = acpi_platform_uart_freq,
};

ARM_PLATFORM(acpi, "netbsd,generic-acpi", &acpi_platform);
