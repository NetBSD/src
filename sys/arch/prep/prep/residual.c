/*      $NetBSD: residual.c,v 1.16.44.1 2014/08/20 00:03:21 tls Exp $     */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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
__KERNEL_RCSID(0, "$NetBSD: residual.c,v 1.16.44.1 2014/08/20 00:03:21 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/inttypes.h>

#include <machine/residual.h>
#include <machine/pnp.h>

#include "opt_residual.h"

#ifdef RESIDUAL_DATA_DUMP

#include <machine/chpidpnp.h>
#include <machine/pcipnp.h>

static void make_pnp_device_tree(void *);
static void bustype_subr(DEVICE_ID *);

int dump_residual_data = 1;

#define	NELEMS(array)	((size_t)(sizeof(array)/sizeof(array[0])))

static const char *FirmwareSupplier[] = {
	"IBMFirmware",
	"MotoFirmware",
	"FirmWorks",
	"Bull",
};

static const char *FirmwareSupports[] = {
	"Conventional",
	"OpenFirmware",
	"Diagnostics",
	"LowDebug",
	"Multiboot",
	"LowClient",
	"Hex41",
	"FAT",
	"ISO9660",
	"SCSI_InitiatorID_Override",
	"Tape_Boot",
	"FW_Boot_Path",
};

static const char *EndianSwitchMethod[] = {
	"Unknown",
	"UsePort92",
	"UsePCIConfigA8",
	"UseFF001030",
};

static const char *SpreadIOMethod[] = {
	"UsePort850",
};

static const char *CacheAttrib[] = {
	"No cache",
	"Split cache",
	"Combined cache",
};

static const char *TLBAttrib[] = {
	"No TLB",
	"Split TLB",
	"Combined TLB",
};

static const char *Usage[] = {
	"FirmwareStack",
	"FirmwareHeap",
	"FirmwareCode",
	"BootImage",
	"Free",
	"Unpopulated",
	"ISAAddr",
	"PCIConfig",
	"PCIAddr",
	"SystemRegs",
	"SystemIO",
	"IOMemory",
	"UnPopSystemROM",
	"SystemROM",
	"ResumeBlock",
	"Other",
};

static const char *BusId[] = {
	"ISA",
	"EISA",
	"PCI",
	"PCMCIA",
	"ISAPNP",
	"MCA",
	"MX",
	"PROCESSOR",
	"VME",
};

static const char *Flags[] = {
	"Output",
	"Input",
	"ConsoleOut",
	"ConsoleIn",
	"Removable",
	"ReadOnly",
	"PowerManaged",
	"Disableable",
	"Configurable",
	"Boot",
	"Dock",
	"Static",
	"Failed",
	"Integrated",
	"Enabled",
};

#endif /* RESIDUAL_DATA_DUMP */


/*
 * Convert a pnp DevId to a string.
 */

void
pnp_devid_to_string(uint32_t devid, char *s)
{
	uint8_t p[4];
	uint32_t l;

	if (res->Revision == 0)
		l = le32toh(devid);
	else
		l = be32toh(devid);
	p[0] = (l >> 24) & 0xff;
	p[1] = (l >> 16) & 0xff;
	p[2] = (l >> 8) & 0xff;
	p[3] = l & 0xff;

	*s++ = ((p[0] >> 2) & 0x1f) + 'A' - 1;
	*s++ = (((p[0] & 0x03) << 3) | ((p[1] >> 5) & 0x07)) + 'A' - 1;
	*s++ = (p[1] & 0x1f) + 'A' - 1;
	*s++ = HEXDIGITS[(p[2] >> 4) & 0xf];
	*s++ = HEXDIGITS[p[2] & 0xf];
	*s++ = HEXDIGITS[(p[3] >> 4) & 0xf];
	*s++ = HEXDIGITS[p[3] & 0xf];
	*s = '\0';
}

/*
 * Count the number of a specific deviceid on the pnp tree.
 */

int
count_pnp_devices(const char *devid)
{
	PPC_DEVICE *ppc_dev;
	int i, found=0;
	uint32_t ndev;
	char deviceid[8];

	ndev = be32toh(res->ActualNumDevices);
	ppc_dev = res->Devices;

	for (i = 0; i < ((ndev > MAX_DEVICES) ? MAX_DEVICES : ndev); i++) {
		DEVICE_ID *id = &ppc_dev[i].DeviceId;

		pnp_devid_to_string(id->DevId, deviceid);
		if (strcmp(deviceid, devid) == 0)
			found++;
	}
	return found;
}

/*
 * find and return a pointer to the N'th device of a given type.
 */

PPC_DEVICE *
find_nth_pnp_device(const char *devid, int busid, int n)
{
	PPC_DEVICE *ppc_dev;
	int i, found=0;
	uint32_t ndev, l, bid = 0;
	char deviceid[8];

	ndev = be32toh(res->ActualNumDevices);
	ppc_dev = res->Devices;
	n++;

	if (busid != 0)
		bid = 1UL << busid;

	for (i = 0; i < ((ndev > MAX_DEVICES) ? MAX_DEVICES : ndev); i++) {
		DEVICE_ID *id = &ppc_dev[i].DeviceId;

		if (bid) {
			printf("???\n");
			l = be32toh(id->BusId);
			if ((l & bid) == 0)
				continue;
		}
		pnp_devid_to_string(id->DevId, deviceid);
		if (strcmp(deviceid, devid) == 0) {
			found++;
			if (found == n)
				return &ppc_dev[i];
		}
	}
	return NULL;
}

int
pnp_pci_busno(void *v, int *bus)
{
	struct _L4_Pack *pack = v;
	struct _L4_PPCPack *p = &pack->L4_Data.L4_PPCPack;
	int item, size, tag = *(unsigned char *)v;
	unsigned char *q = v;

	item = tag_large_item_name(tag);
	size = (q[1] | (q[2] << 8)) + 3 /* tag + length */;
	*bus = -1;

	if (res->Revision == 0)
                return size;
	if (item != LargeVendorItem)
		return size;
	if (p->Type != LV_PCIBridge) /* PCI Bridge type */
		return size;	

	*bus = p->PPCData[16];
	return size;
}

/* Get the PCI config base and addr from PNP */

int
pnp_pci_configbase(void *v, uint32_t *addr, uint32_t *data)
{
	struct _L4_Pack *pack = v;
	struct _L4_PPCPack *p = &pack->L4_Data.L4_PPCPack;
	int item, size, tag = *(unsigned char *)v;
	unsigned char *q = v;

	item = tag_large_item_name(tag);
	size = (q[1] | (q[2] << 8)) + 3 /* tag + length */;
	/* init to zero so we don't return garbage values */
	*addr = 0;
	*data = 0;

	if (res->Revision == 0)
		return size;
	if (item != LargeVendorItem)
		return size;
	if (p->Type != LV_PCIBridge) /* PCI Bridge type */
		return size;

	*addr = (uint32_t)le64dec(&p->PPCData[0]);
	*data = (uint32_t)le64dec(&p->PPCData[8]);
	return size;
}

#ifdef RESIDUAL_DATA_DUMP

void
print_residual_device_info(void)
{
	VPD *vpd;
	PPC_CPU *ppc_cpu;
	MEM_MAP *mem_map;
	PPC_MEM *ppc_mem;
	PPC_DEVICE *ppc_dev;
	const char *str;
	unsigned long l;
	unsigned short s;
	unsigned long nmseg;
	unsigned long nmem;
	unsigned long ndev;
	unsigned long page_size;
	int ncpus;
	int first;
	int i, j;
	char deviceid[9];

	if (!dump_residual_data)
		return;

	if (be32toh(res->ResidualLength) == 0)
		return;

	printf("ResidualLength = %ld\n", be32toh(res->ResidualLength));
	printf("Version = %d\n", res->Version);
	printf("Revision = %d\n", res->Revision);
	printf("EC = %d\n", be16toh(res->EC));

	/*
	 * VPD
	 */
	vpd = &res->VitalProductData;
	printf("\nVPD\n");
	printf("  PrintableModel = %-32s\n", vpd->PrintableModel);
	printf("  Serial = %-16s\n", vpd->Serial);

	l = be32toh(vpd->FirmwareSupplier);
	printf("  FirmwareSupplier = %s\n",
	    (l >= NELEMS(FirmwareSupplier)) ? "Unknown" : FirmwareSupplier[l]);
	l = be32toh(vpd->FirmwareSupports);
	printf("  FirmwareSupports = 0x%08lx\n", l);
	for (first = 1, i = 0; i < sizeof(unsigned long) * 8; i++) {
		if ((l & (1UL << i)) != 0) {
			printf("    : %s\n", i >= NELEMS(FirmwareSupports)
			    ? "Unknown" : FirmwareSupports[i]);
			   first = 0;
		}
	}
	if (first)
		printf("    : None\n");

	printf("  NvramSize = %ld\n", be32toh(vpd->NvramSize));
	printf("  NumSIMMSlots = %ld\n", be32toh(vpd->NumSIMMSlots));
	s = be16toh(vpd->EndianSwitchMethod);
	printf("  EndianSwitchMethod = %s\n",
	    (s >= NELEMS(EndianSwitchMethod))
	      ? "Unknown" : EndianSwitchMethod[s]);
	s = be16toh(vpd->SpreadIOMethod);
	printf("  SpreadIOMethod = %s\n",
	    (s >= NELEMS(SpreadIOMethod)) ? "Unknown" : SpreadIOMethod[s]);
	printf("  SmpIar = %ld\n", be32toh(vpd->SmpIar));
	printf("  RAMErrLogOffset = %ld\n", be32toh(vpd->RAMErrLogOffset));
	printf("  ProcessorHz = %ld\n", be32toh(vpd->ProcessorHz));
	printf("  ProcessorBusHz = %ld\n", be32toh(vpd->ProcessorBusHz));
	printf("  TimeBaseDivisor = %ld\n", be32toh(vpd->TimeBaseDivisor));
	printf("  WordWidth = %ld\n", be32toh(vpd->WordWidth));
	page_size = be32toh(vpd->PageSize);
	printf("  PageSize = %ld\n", page_size);
	printf("  CoherenceBlockSize = %ld\n",be32toh(vpd->CoherenceBlockSize));
	printf("  GranuleSize = %ld\n", be32toh(vpd->GranuleSize));

	printf("  L1 Cache variables\n");
	printf("    CacheSize = %ld\n", be32toh(vpd->CacheSize));
	l = be32toh(vpd->CacheAttrib);
	printf("    CacheAttrib = %s\n",
	    (l >= NELEMS(CacheAttrib)) ? "Unknown" : CacheAttrib[l]);
	printf("    CacheAssoc = %ld\n", be32toh(vpd->CacheAssoc));
	printf("    CacheLineSize = %ld\n", be32toh(vpd->CacheLineSize));
	printf("    I-CacheSize = %ld\n", be32toh(vpd->I_CacheSize));
	printf("    I-CacheAssoc = %ld\n", be32toh(vpd->I_CacheAssoc));
	printf("    I-CacheLineSize = %ld\n", be32toh(vpd->I_CacheLineSize));
	printf("    D-CacheSize = %ld\n", be32toh(vpd->D_CacheSize));
	printf("    D-CacheAssoc = %ld\n", be32toh(vpd->D_CacheAssoc));
	printf("    D-CacheLineSize = %ld\n", be32toh(vpd->D_CacheLineSize));
	printf("  Translation Lookaside Buffer variables\n");
	printf("    Number of TLB entries = %ld\n", be32toh(vpd->TLBSize));
	l = be32toh(vpd->TLBAttrib);
	printf("    TLBAttrib = %s\n",
	    (l >= NELEMS(TLBAttrib)) ? "Unknown" : TLBAttrib[l]);
	printf("    TLBAssoc = %ld\n", be32toh(vpd->TLBAssoc));
	printf("    I-TLBSize = %ld\n", be32toh(vpd->I_TLBSize));
	printf("    I-TLBAssoc = %ld\n", be32toh(vpd->I_TLBAssoc));
	printf("    D-TLBSize = %ld\n", be32toh(vpd->D_TLBSize));
	printf("    D-TLBAssoc = %ld\n", be32toh(vpd->D_TLBAssoc));
	printf("  ExtendedVPD = 0x%lx\n", be32toh(vpd->ExtendedVPD));

	/*
	 * PPC_CPU
	 */
	printf("\n");
	printf("MaxNumCpus = %d\n", be16toh(res->MaxNumCpus));
	ncpus = be16toh(res->ActualNumCpus);
	printf("ActualNumCpus = %d\n", ncpus);
	ppc_cpu = res->Cpus;
	for (i = 0; i < ((ncpus > MAX_CPUS) ? MAX_CPUS : ncpus); i++) {
		printf("%d:\n", i);
		printf("  CpuType = %08lx\n", be32toh(ppc_cpu[i].CpuType));
		printf("  CpuNumber = %d\n", ppc_cpu[i].CpuNumber);
		switch (ppc_cpu[i].CpuState) {
		case CPU_GOOD:
			str = "CPU is present, and active";
			break;
		case CPU_GOOD_FW:
			str = "CPU is present, and in firmware";
			break;
		case CPU_OFF:
			str = "CPU is present, but inactive";
			break;
		case CPU_FAILED:
			str = "CPU is present, but failed POST";
			break;
		case CPU_NOT_PRESENT:
			str = "CPU not present";
			break;
		default:
			str = "Unknown state";
			break;
		}
		printf("  CpuState: %s (%d)\n", str, ppc_cpu[i].CpuState);
	}

	printf("\n");
	printf("TotalMemory = %ld (0x%08lx)\n",
	    be32toh(res->TotalMemory) ,be32toh(res->TotalMemory));
	printf("GoodMemory  = %ld (0x%08lx)\n",
	    be32toh(res->GoodMemory), be32toh(res->GoodMemory));

	/*
	 * MEM_MAP
	 */
	printf("\n");
	nmseg = be32toh(res->ActualNumMemSegs);
	printf("ActualNumMemSegs = %ld\n", nmseg);
	mem_map = res->Segs;
	for (i = 0; i < ((nmseg > MAX_MEM_SEGS) ? MAX_MEM_SEGS : nmseg); i++) {
		unsigned long pc;

		printf("%d:\n", i);
		l = be32toh(mem_map[i].Usage);
		printf("  Usage = ");
		for (first = 1, j = 0; j < sizeof(unsigned long) * 8; j++) {
			if ((l & (1UL << j)) != 0) {
				printf("%s%s", first ? "" : ", ",
				    j >= NELEMS(Usage) ? "Unknown" : Usage[j]);
				first = 0;
			}
		}
		printf(" (0x%08lx)\n", l);
		printf("  BasePage  = 0x%05lx000\n",
		    be32toh(mem_map[i].BasePage));
		pc = be32toh(mem_map[i].PageCount);
		printf("  PageCount = 0x%08lx (%ld page%s)\n",
		    page_size * pc, pc, pc == 1 ? "" : "s");
	}

	/*
	 * PPC_MEM
	 */
	printf("\n");
	nmem = be32toh(res->ActualNumMemories);
	printf("ActualNumMemories = %ld\n", nmem);
	ppc_mem = res->Memories;
	for (i = 0; i < ((nmem > MAX_MEMS) ? MAX_MEMS : nmem); i++) {
		printf("%d:\n", i);
		printf("  SIMMSize = %ld MB\n", be32toh(ppc_mem[i].SIMMSize));
	}

	/*
	 * PPC_DEVICE
	 */
	printf("\n");
	ndev = be32toh(res->ActualNumDevices);
	printf("ActualNumDevices = %ld\n", ndev);
	ppc_dev = res->Devices;
	for (i = 0; i < ((ndev > MAX_DEVICES) ? MAX_DEVICES : ndev); i++) {
		DEVICE_ID *id = &ppc_dev[i].DeviceId;
		BUS_ACCESS *bus = &ppc_dev[i].BusAccess;

		printf("\n%d:\n", i);

		printf("  DEVICE_ID\n");
		l = be32toh(id->BusId);
		printf("    BusId = ");
		for (j = 0; j < sizeof(unsigned long) * 8; j++) {
			if ((l & (1UL << j)) != 0) {
				printf("%s",
				    j >= NELEMS(BusId) ? "Unknown" : BusId[j]);
				break;
			}
		}
		printf("\n");
		pnp_devid_to_string(id->DevId, deviceid);
		printf("    DevId = 0x%08lx (%s)\n", id->DevId, deviceid);
		printf("    SerialNum = 0x%08lx\n", be32toh(id->SerialNum));
		l = be32toh(id->Flags);
		printf("    Flags = 0x%08lx\n", l);
		for (first = 1, j = 0; j < sizeof(unsigned long) * 8; j++) {
			if ((l & (1UL << j)) != 0) {
				printf("      : %s\n",
				    j >= NELEMS(Flags) ? "Unknown" : Flags[j]);
				first = 0;
			}
		}
		if (first)
			printf("      : None\n");

		bustype_subr(id);

		printf("  BUS_ACCESS\n");
		printf("    info0 = %d\n", bus->PnPAccess.CSN);
		printf("    info1 = %d\n", bus->PnPAccess.LogicalDevNumber);

		l = be32toh(ppc_dev[i].AllocatedOffset);
		printf("  AllocatedOffset  = 0x%08lx\n", l);
		make_pnp_device_tree(res->DevicePnPHeap + l);

		l = be32toh(ppc_dev[i].PossibleOffset);
		printf("  PossibleOffset   = 0x%08lx\n", l);
		make_pnp_device_tree(res->DevicePnPHeap + l);

		l = be32toh(ppc_dev[i].CompatibleOffset);
		printf("  CompatibleOffset = 0x%08lx\n", l);
		make_pnp_device_tree(res->DevicePnPHeap + l);
	}
}

enum {
	TAG_SMALL = 0,
	TAG_LARGE
};

/*--
 * pnp device
 */
static int pnp_small_pkt(void *);
static int pnp_large_pkt(void *);

static void
make_pnp_device_tree(void *v)
{
	unsigned char *p = v;
	int size;

	if (p == NULL)
		return;

	for (; p[0] != END_TAG; p += size) {
		if (tag_type(p[0]) == TAG_SMALL)
			size = pnp_small_pkt(p);
		else
			size = pnp_large_pkt(p);
	}
}

static void small_vendor_chipid(void *v)
{
	ChipIDPack *p = v;
	char chipid[8];
	int16_t id;
	int i, j;
	struct _svid {
		int16_t id;
		const char *name;
		int type;
	} svid[] = {
		{ Dakota,	"IBM North/South Dakota", Chip_MemCont },
		{ Idaho,	"IBM Idaho", Chip_MemCont },
		{ Eagle,	"Motorola Eagle", Chip_MemCont },
		{ Kauai_Lanai,	"IBM Kauai/Lanai", Chip_MemCont },
		{ Montana_Nevada,"IBM Montana/Nevada", Chip_MemCont },
		{ Union,	"IBM Union", Chip_MemCont },
		{ Cobra_Viper,	"IBM Cobra/Viper", Chip_MemCont },
		{ Grackle,	"Motorola Grackle", Chip_MemCont },
		{ SIO_ZB,	"Intel 82378ZB", Chip_ISABridge },
		{ FireCoral,	"IBM FireCoral", Chip_ISABridge },
		{ Python,	"IBM Python", Chip_PCIBridge },
		{ DEC21050,	"PCI-PCI (DEC 21050)", Chip_PCIBridge },
		{ IBM2782351,	"PCI-PCI (IBM 2782351)", Chip_PCIBridge },
		{ IBM2782352,	"PCI-PCI (IBM 2782352)", Chip_PCIBridge },
		{ INTEL_8236SL,	"Intel 8236SL", Chip_PCMCIABridge },
		{ RICOH_RF5C366C,"RICOH RF5C366C", Chip_PCMCIABridge },
		{ INTEL_82374,	"Intel 82374/82375", Chip_EISABridge },
		{ MCACoral,	"MCA Coral", Chip_MCABridge },
		{ Cheyenne,	"IBM Cheyenne", Chip_L2Cache },
		{ IDT,		"IDT", Chip_L2Cache },
		{ Sony1PB,	"Sony1PB", Chip_L2Cache },
		{ Mamba,	"IBM Mamba", Chip_L2Cache },
		{ Alaska,	"IBM Alaska", Chip_L2Cache },
		{ Glance,	"IBM Glance", Chip_L2Cache },
		{ Ocelot,	"IBM Ocelot", Chip_L2Cache },
		{ Carrera,	"IBM Carrera", Chip_PM },
		{ Sig750,	"Signetics 87C750", Chip_PM },
		{ MPIC_2,	"IBM MPIC-2", Chip_IntrCont },
		{ DallasRTC,	"Dallas 1385 compatible", Chip_MiscPlanar },
		{ Dallas1585,	"Dallas 1585 compatible", Chip_MiscPlanar },
		{ Timer8254,	"8254-compatible timer", Chip_MiscPlanar },
		{ HarddiskLt,	"Op Panel HD light", Chip_MiscPlanar },
		{ 0,		NULL, -1 },
	};
	const char *chiptype[] = {
		"Memory Controller",
		"ISA Bridge",
		"PCI Bridge",
		"PCMCIA Bridge",
		"EISA Bridge",
		"MCA Bridge",
		"L2 Cache Controller",
		"Power Management Controller",
		"Interrupt Controller",
		"Misc. Planar Device"
	};

	id = le16dec(&p->Name[0]);
	snprintf(chipid, 8, "%c%c%c%0hX",
	    ((p->VendorID0 >> 2) & 0x1f) + 'A' - 1,
	    (((p->VendorID0 & 3) << 3) | ((p->VendorID1 >> 5) & 7)) + 'A' - 1,
	    (p->VendorID1 & 0x1f) + 'A' - 1, id);
	for (i = 0, j = -1; svid[i].name != NULL; i++) {
		if (id == svid[i].id) {
			j = i;
			break;
		}
	}
	printf("Chip ID: %s\n", chipid);
	if (j == -1) {
		printf("      Unknown Chip Type\n");
		return;
	}
	printf("      %s: %s\n", chiptype[svid[j].type], svid[j].name);
	return;
}

static int
pnp_small_pkt(void *v)
{
	int tag = *(unsigned char *)v;
	int item, size;
	int i, j;
	int first;

	item = tag_small_item_name(tag);
	size = tag_small_count(tag) + 1 /* tag */;

	switch (item) {
	case CompatibleDevice: {
		struct _S3_Pack *p = v;
		unsigned char *q = p->CompatId;

		printf("    CompatibleDevice = %c%c%c%c%c%c%c\n",
		    ((q[0] >> 2) & 0x1f) + 'A' - 1,
		    (((q[0] & 0x03) << 3) | ((q[1] >> 5) & 0x07)) + 'A' - 1,
		    (q[1] & 0x1f) + 'A' - 1,
		    HEXDIGITS[(q[2] >> 4) & 0xf], HEXDIGITS[q[2] & 0xf],
		    HEXDIGITS[(q[3] >> 4) & 0xf], HEXDIGITS[q[3] & 0xf]);
		}
		break;

	case IRQFormat: {
		struct _S4_Pack *p = v;

		printf("    IRQ: ");
		for (first = 1, j = 0; j < 2; j++) {
			for (i = 0; i < 8; i++) {
				if (p->IRQMask[j] & (1 << i)) {
					printf("%s%d",
					    first ? "" : ", ", j * 8 + i);
					first = 0;
				}
			}
		}
		if (first)
			printf("None ");

		if (size == 3) {
			static const char *IRQInfo[] = {
				"high true edge sensitive",
				"low true edge sensitive",
				"high true level sensitive",
				"low true level sensitive",
			};

			if (p->IRQInfo & 0xf0)
				goto IRQout;

			for (first = 1, i = 0; i < NELEMS(IRQInfo); i++) {
				if (p->IRQInfo & (1 << i)) {
					printf("%s%s", first ? " (" : ", ",
					    IRQInfo[i]);
					first = 0;
				}
			}
			if (!first)
				printf(")");
		}
IRQout:
		printf("\n");
		}
		break;

	case DMAFormat: {
		struct _S5_Pack *p = v;

		printf("    DMA: ");
		for (first = 1, i = 0; i < 8; i++) {
			if (p->DMAMask & (1 << i)) {
				printf("%s%d", first ? "" : ", ", i);
				first = 0;
			}
		}
		printf("%s", first ? "None" : "");

		printf("\n");
		}
		break;

	case StartDepFunc:
	case EndDepFunc:
		break;

	case IOPort: {
		struct _S8_Pack *p = v;
		unsigned short mask;
		unsigned short iomin, iomax;
		int align, len;

		mask = p->IOInfo & ISAAddr16bit ? 0xffff : 0x03ff;
		iomin = (p->RangeMin[0] | (p->RangeMin[1] << 8)) & mask;
		iomax = (p->RangeMax[0] | (p->RangeMax[1] << 8)) & mask;
		align = p->IOAlign;
		len = p->IONum;

		if (len != 1) {
			if (iomin == iomax)
				printf("    IOPort: 0x%x-0x%x",
				    iomin, iomin + len-1);
			else
				printf("    IOPort: min 0x%x-0x%x,"
				    " max 0x%x-0x%x (%d byte%s align)",
				      iomin, iomin + len-1,
				      iomax, iomax + len-1,
				      align, align != 1 ? "s" : "");
		} else {
			if (iomin == iomax)
				printf("    IOPort: 0x%x", iomin);
			else
				printf("    IOPort: min 0x%x, max 0x%x"
				    " (%d byte%s align)",
				      iomin, iomax,
				      align, align != 1 ? "s" : "");
		}
		printf("\n");
		}
		break;

	case FixedIOPort: {
		struct _S9_Pack *p = v;
		unsigned short ioport;
		int len;

		ioport = (p->Range[0] | (p->Range[1] << 8)) & 0x3ff;
		len = p->IONum;

		if (len != 1)
			printf("    FixedIOPort: 0x%x-0x%x",
			    ioport, ioport + len - 1);
		else
			printf("    FixedIOPort: 0x%x", ioport);
		printf("\n");
		}
		break;

	case SmallVendorItem: {
		unsigned char *p = v;

		printf("    SmallVendorItem: ");
		switch (p[1]) {
		case 1:
			small_vendor_chipid(v);
			break;

		case 3:
			printf("Processor Number: %d\n", p[2]);
			break;

		default:
			printf("\n");
			for (i = 0; i < size - 1; i++) {
				if ((i % 16) == 0)
					printf("      ");
				printf("%02x ", p[i + 1]);
				if ((i % 16) == 15)
					printf("\n");
			}
			if ((i % 16) != 0)
				printf("\n");
			break;
		}
		break;
	}
	default: {
		unsigned char *p = v;

		printf("small\n");
		printf("item = %d\n", item);
		printf("size = %d\n", size);

		for (i = 1; i < size; i++)
			printf("%02x ", p[i]);
		printf("\n");
		}
		break;
	}

	return size;
}

/*
 * large vendor items
 */

static void large_vendor_default_subr(struct _L4_PPCPack *p, void *v, int size);
static void large_vendor_floppy_subr(struct _L4_PPCPack *p, void *v, int size);
static void large_vendor_l2cache_subr(struct _L4_PPCPack *p, void *v, int size);
static void large_vendor_pcibridge_subr(struct _L4_PPCPack *p, void *v,
    int size);
static void large_vendor_bat_subr(struct _L4_PPCPack *p, void *v, int size);
static void large_vendor_bba_subr(struct _L4_PPCPack *p, void *v, int size);
static void large_vendor_scsi_subr(struct _L4_PPCPack *p, void *v, int size);
static void large_vendor_pms_subr(struct _L4_PPCPack *p, void *v, int size);
static void large_vendor_gaddr_subr(struct _L4_PPCPack *p, void *v, int size);
static void large_vendor_isaintr_subr(struct _L4_PPCPack *p, void *v, int size);

static void
large_vendor_default_subr(struct _L4_PPCPack *p, void *v, int size)
{
	int i;

	for (i = 0; i < size - 3; i++) {
		if ((i % 16) == 0)
			printf("      ");
		printf("%02x ", p->PPCData[i]);
		if ((i % 16) == 15)
			printf("\n");
	}
	if ((i % 16) != 0)
		printf("\n");
}

static void
large_vendor_floppy_subr(struct _L4_PPCPack *p, void *v, int size)
{
	int i;
	const char *str;

	for (i = 0; i < (size - 4) / 2; i++) {
		switch (p->PPCData[i*2]) {
		case 0:
			str = "Not present";
			break;
		case 1:
			str = "3.5\" 2MiB";
			break;
		case 2:
			str = "3.5\" 4MiB";
			break;
		case 3:
			str = "5.25\" 1.6MiB";
			break;
		default:
			str = "Unknown type";
			break;
		}
		printf("      Floppy drive %d, %s", i, str);
		if (p->PPCData[i*2 + 1] & 0x01)
			printf(", Media sense");
		if (p->PPCData[i*2 + 1] & 0x02)
			printf(", Auto eject");
		if (p->PPCData[i*2 + 1] & 0x04)
			printf(", Alt speed");
		printf("\n");
	}
}

static void
large_vendor_l2cache_subr(struct _L4_PPCPack *p, void *v, int size)
{
	static const unsigned char *L2type[] =
	    { "None", "WriteThru", "CopyBack" };
	static const unsigned char *L2assoc[] =
	    { "None", "DirectMapped", "2-way set" };
	static const unsigned char *L2hw[] =
	    { "None", "Invalidate", "Flush", "Unknown" };

	printf("      %u K %s %s %s L2 cache\n"
	    "\t%hd/%hd bytes line/sector size\n",
	    le32dec(&p->PPCData[0]), L2type[p->PPCData[10]],
	    L2assoc[le16dec(&p->PPCData[4])],
	    L2hw[p->PPCData[11]], le16dec(&p->PPCData[6]),
	    le16dec(&p->PPCData[8]));
}

static void
large_vendor_pcibridge_subr(struct _L4_PPCPack *p, void *v, int size)
{
	int i, numslots;
	char tmpstr[30];
	PCIInfoPack *pi = v;
	static const unsigned char *intrtype[] =
	    { "8259", "MPIC", "RS6k BUID %d" };

	/* rev 0 residual has no valid pcibridge data */
	if (res->Revision == 0) {
		large_vendor_default_subr(p, v, size);
		return;
	}
	numslots = (le16dec(&pi->count0)-21)/sizeof(IntrMap);

	printf("    PCI Bridge parameters\n"
	    "      ConfigBaseAddress 0x%0" PRIx64" \n"
	    "      ConfigBaseData 0x%0" PRIx64 "\n"
	    "      Bus number %d\n",
	    le64dec(&pi->configbaseaddr), le64dec(&pi->configbasedata),
	    pi->busnum);

	printf("    PCI Bridge Slot Data\n");
	for (i = 0; i < numslots; i++) {
		int j, first, l;

		if (pi->map[i].slotnum)
			printf("      PCI Slot %d", pi->map[i].slotnum);
		else
			printf("      Integrated PCI device");
		printf(" DevFunc 0x%02x\n", pi->map[i].devfunc);
		for (j = 0, first = 1, t = tmpstr; j < MAX_PCI_INTRS; j++) {
			if (pi->map[i].intr[j] != 0xFFFF) {
				if (first)
					first = 0;
				else
					*t++ = '/';
				*t++ = 'A' + j;
			}
		}
		*t = '\0';
		if (first)
			continue; /* there were no valid intrs */
		printf("        interrupt line(s) %s routed to", tmpstr);
		snprintf(tmpstr, sizeof(tmpstr),
		    intrtype[pi->map[i].intrctrltype - 1],
		    pi->map[i].intrctrlnum);
		printf(" %s line(s) ", tmpstr);
		for (j = 0, first = 1, l = 0; j < MAX_PCI_INTRS; j++) {
			int line = bswap16(pi->map[i].intr[j]);

			if (pi->map[i].intr[j] != 0xFFFF) {
				l += snprintf(tmpstr + l, sizeof(tmpstr) - l, 
				    "%s%d(%c)", l == 0 ? "/" : "",
				    line & 0x7fff, line & 0x8000 ? 'E' : 'L');
				if (l > sizeof(tmpstr))
					break;
			}
		}
		printf("%s\n", tmpstr);
	}
}

static void
large_vendor_bat_subr(struct _L4_PPCPack *p, void *v, int size)
{
	static const unsigned char *convtype[] =
	    { "Bus Memory", "Bus I/O", "DMA" };
	static const unsigned char *transtype[] =
	    { "direct", "mapped", "PPC special storage segment" };

	printf("      Bridge address translation, %s decoding:\n"
	    "      Parent Base\tBus Base\tRange\t   Conversion\tTranslation\n"
	    "      0x%8.8" PRIx64 "\t0x%8.8" PRIx64 "\t0x%8.8" PRIx64
	    " %s%s%s\n",
	    p->PPCData[0] & 1 ? "positive" : "subtractive",
	    le64dec(&p->PPCData[4]), le64dec(&p->PPCData[12]),
	    le64dec(&p->PPCData[20]), convtype[p->PPCData[2] - 1],
	    p->PPCData[2] == 3 ? "\t\t" : "\t",
	    transtype[p->PPCData[1] - 1]);
}

static void
large_vendor_bba_subr(struct _L4_PPCPack *p, void *v, int size)
{
	printf("      Bus speed %ld Hz, %d slot(s)\n",
	    (long)le32dec(&p->PPCData), p->PPCData[4]);
}

static void
large_vendor_scsi_subr(struct _L4_PPCPack *p, void *v, int size)
{
	int i;

	printf("    SCSI buses: %d id(s):", p->PPCData[0]);
	for (i = 1; i <= p->PPCData[0]; i++)
		printf("\t\t\t%d%c", p->PPCData[i],
		    i == p->PPCData[0] ? '\n' : ',');
}

static void
large_vendor_pms_subr(struct _L4_PPCPack *p, void *v, int size)
{
	unsigned int flags;
	int i;
	static const unsigned char *power[] = {
	    "Hibernation", "Suspend", "Laptop lid events", "Laptop battery",
	    "Modem-triggered resume from hibernation",
	    "Modem-triggered resume from suspend",
	    "Timer-triggered resume from hibernation",
	    "Timer-triggered resume from suspend",
	    "Timer-triggered hibernation from suspend",
	    "Software-controlled power switch", "External resume trigger",
	    "Software main power switch can be overridden by hardware",
	    "Resume button", "Automatic transition between states is inhibited"
	};

	printf("      Power management attributes:");
	flags = le32dec(&p->PPCData);
	if (!flags) {
		printf(" (none)\n");
		return;
	}
	printf("\n");
	for (i = 0; i < 14; i++)
		if (flags & 1 << i)
			printf("\t%s\n", power[i]);
	if (flags & 0xffffc000)
		printf("\tunknown flags (0x%8.8x)\n", flags);
}

static void
large_vendor_gaddr_subr(struct _L4_PPCPack *p, void *v, int size)
{
	static const unsigned char *addrtype[] = { "I/O", "Memory", "System" };

	printf("      %s address (%d bits), at 0x%" PRIx64 " size 0x%"
	    PRIx64 " bytes\n", addrtype[p->PPCData[0] - 1], p->PPCData[1],
	    le64dec(&p->PPCData[4]), le64dec(&p->PPCData[12]));
}

static void
large_vendor_isaintr_subr(struct _L4_PPCPack *p, void *v, int size)
{
	int i;
	char tmpstr[30];
	static const unsigned char *inttype[] =
	    { "8259", "MPIC", "RS6k BUID %d" };

	snprintf(tmpstr, sizeof(tmpstr), inttype[p->PPCData[0] - 1],
	    p->PPCData[1]);
	printf("      ISA interrupts routed to %s lines\n\t", tmpstr);

	for (i = 0; i < 16; i++) {
		int line = le16dec(&p->PPCData[2 + 2*i]);

		if (line != 0xffff)
			printf(" %d(IRQ%d)", line, i);
		if (i == 8)
			printf("\n\t");
	}
	printf("\n");
}

static int
pnp_large_pkt(void *v)
{
	int tag = *(unsigned char *)v;
	unsigned char *q = v;
	int item, size;
	int i;
	static struct large_vendor_type {
		const char	*str;
		void	(*func)(struct _L4_PPCPack *p, void *vv, int sz);
	} Large_Vendor_Type[] = {
		{ "None",			NULL },
		{ "Diskette Drive",		large_vendor_floppy_subr },
		{ "L2 Cache",			large_vendor_l2cache_subr },
		{ "PCI Bridge",			large_vendor_pcibridge_subr },
		{ "Display",			large_vendor_default_subr },
		{ "Bridge Address Translation",	large_vendor_bat_subr },
		{ "Bus Bridge Attributes",	large_vendor_bba_subr },
		{ "SCSI Controller Information",large_vendor_scsi_subr },
		{ "Power Management Support",	large_vendor_pms_subr },
		{ "Generic Address",		large_vendor_gaddr_subr },
		{ "ISA Bridge Information",	large_vendor_isaintr_subr },
		{ "Video Channels",		large_vendor_default_subr },
		{ "Power Control",		large_vendor_default_subr },
		{ "Memory SIMM PD Data",	large_vendor_default_subr },
		{ "System Interrupts",		large_vendor_default_subr },
		{ "Error Log",			large_vendor_default_subr },
		{ "Extended VPD",		large_vendor_default_subr },
		{ "Timebase Control",		large_vendor_default_subr },
	};

	item = tag_large_item_name(tag);
	size = (q[1] | (q[2] << 8)) + 3 /* tag + length */;

	switch (item) {
	case LargeVendorItem: {
		struct _L4_Pack *pack = v;
		struct _L4_PPCPack *p =  &pack->L4_Data.L4_PPCPack;

		printf("    LargeVendorItem: %s\n",
		    Large_Vendor_Type[p->Type].str);
		if (p->Type <= 17 && Large_Vendor_Type[p->Type].func != NULL)
			(*Large_Vendor_Type[p->Type].func)(p, v, size);
		break;
	}
	case MemoryRange: {
		struct _L1_Pack *pack = v;

		printf("    Memory Range:\n");
		if (pack->Data[0] & L1_Shadow)
			printf("      Memory is shadowable\n");
		if (pack->Data[0] & L1_32bit_mem)
			printf("      32-bit memory only\n");
		if (pack->Data[0] & L1_8_16bit_mem)
			printf("      8-bit and 16-bit supported\n");
		if (pack->Data[0] & L1_Decode_Hi)
			printf("      decode supports high address\n");
		if (pack->Data[0] & L1_Cache)
			printf("      read cacheable, write-through\n");
		if (pack->Data[0] & L1_Writable)
			printf("      Memory is writable\n");

		if (pack->Count0 >= 0x9) {
			printf("      minbase : 0x%x\n",
			    (pack->Data[2] << 16) | (pack->Data[1] << 8));
			printf("      maxbase : 0x%x\n",
			    (pack->Data[4] << 16) | (pack->Data[3] << 8));
			printf("      align   : 0x%x\n",
			    (pack->Data[6] << 8) | pack->Data[5]);
			printf("      length  : 0x%x\n",
			    (pack->Data[8] << 16) | (pack->Data[7] << 8));
		}
		break;
	}
	
	default: {
		unsigned char *p = v;

		printf("large\n");
		printf("item = %d\n", item);
		printf("size = %d\n", size);

		for (i = 3; i < size; i++)
			printf("%02x ", p[i]);
		printf("\n");
		}
		break;
	}

	return size;
}

/*--
 * bus type
 */
static void mass_subr(DEVICE_ID *);
static void nic_subr(DEVICE_ID *);
static void display_subr(DEVICE_ID *);
static void mm_subr(DEVICE_ID *);
static void mem_subr(DEVICE_ID *);
static void bridge_subr(DEVICE_ID *);
static void comm_subr(DEVICE_ID *);
static void sys_subr(DEVICE_ID *);
static void input_subr(DEVICE_ID *);
static void service_subr(DEVICE_ID *);

static void
bustype_subr(DEVICE_ID *id)
{
	static struct bustype {
		const char	*str;
		void	(*func)(DEVICE_ID *);
	} BaseType[] = {
		{ "Reserved"			, NULL },
		{ "MassStorageDevice"		, mass_subr },
		{ "NetworkInterfaceController"	, nic_subr },
		{ "DisplayController"		, display_subr },
		{ "MultimediaController"	, mm_subr },
		{ "MemoryController"		, mem_subr },
		{ "BridgeController"		, bridge_subr },
		{ "CommunicationsDevice"	, comm_subr },
		{ "SystemPeripheral"		, sys_subr },
		{ "InputDevice"			, input_subr },
		{ "ServiceProcessor"		, service_subr },
	};
	int type;

	type = (id->BaseType >= NELEMS(BaseType)) ? 0 : id->BaseType;

	printf("    BaseType = %s (%d)\n", BaseType[type].str, id->BaseType);
	if (BaseType[type].func != NULL)
		(*BaseType[type].func)(id);
}

static void
mass_subr(DEVICE_ID *id)
{
	static const char *IDEController_tabel[] = {
		"GeneralIDE",
		"ATACompatible",
	};
	static const char *FloppyController_table[] = {
		"GeneralFloppy",
		"Compatible765",
		"NS398_Floppy",
		"NS26E_Floppy",
		"NS15C_Floppy",
		"NS2E_Floppy",
		"CHRP_Floppy",
	};
	const char *p, *q = NULL;

	switch (id->SubType) {
	case SCSIController:
		p = "SCSIController";
		q = "GeneralSCSI";
		break;
	case IDEController:
		p = "IDEController";
		q = id->Interface >= NELEMS(IDEController_tabel)
		    ? NULL : IDEController_tabel[id->Interface];
		break;
	case FloppyController:
		p = "FloppyController";
		q = id->Interface >= NELEMS(FloppyController_table)
		    ? NULL : FloppyController_table[id->Interface];
		break;
	case IPIController:
		p = "IPIController";
		q = "GeneralIPI";
		break;
	case OtherMassStorageController:
		p = "OtherMassStorageController";
		break;
	default:
		p = "UnknownStorageController";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
nic_subr(DEVICE_ID *id)
{
	const char *p, *q = NULL;

	switch (id->SubType) {
	case EthernetController:
		p = "EthernetController";
		q = "GeneralEther";
		break;
	case TokenRingController:
		p = "TokenRingController";
		q = "GeneralToken";
		break;
	case FDDIController:
		p = "FDDIController";
		q = "GeneralFDDI";
		break;
	case OtherNetworkController:
		p = "OtherNetworkController";
		break;
	default:
		p = "UnknownNetworkController";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
display_subr(DEVICE_ID *id)
{
	const char *p, *q = NULL;

	switch (id->SubType) {
	case VGAController:
		p = "VGAController";
		q = "GeneralVGA";
		break;
	case SVGAController:
		p = "SVGAController";
		q = "GeneralSVGA";
		break;
	case XGAController:
		p = "XGAController";
		q = "GeneralXGA";
		break;
	case OtherDisplayController:
		p = "OtherDisplayController";
		break;
	default:
		p = "UnknownDisplayController";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
mm_subr(DEVICE_ID *id)
{
	static const char *AudioController_table[] = {
		"GeneralAudio",
		"CS4232Audio",
	};
	const char *p, *q = NULL;

	switch (id->SubType) {
	case VideoController:
		p = "VideoController";
		q = "GeneralVideo";
		break;
	case AudioController:
		p = "AudioController";
		q = id->Interface >= NELEMS(AudioController_table)
		    ? NULL : AudioController_table[id->Interface];
		break;
	case OtherMultimediaController:
		p = "OtherMultimediaController";
		break;
	default:
		p = "UnknownMultimediaController";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
mem_subr(DEVICE_ID *id)
{
	const char *p, *q = NULL;

	switch (id->SubType) {
	case RAM:
		p = "RAM";
		q = "GeneralRAM";
		break;
	case FLASH:
		p = "FLASH";
		q = "GeneralFLASH";
		break;
	case OtherMemoryDevice:
		p = "OtherMemoryDevice";
		break;
	default:
		p = "UnknownMemoryDevice";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
bridge_subr(DEVICE_ID *id)
{
	static const char *PCIBridge_table[] = {
		"GeneralPCIBridge",
		"PCIBridgeIndirect",
		"PCIBridgeRS6K",
	};
	const char *p, *q = NULL;

	switch (id->SubType) {
	case HostProcessorBridge:
		p = "HostProcessorBridge";
		q = "GeneralHostBridge";
		break;
	case ISABridge:
		p = "ISABridge";
		q = "GeneralISABridge";
		break;
	case EISABridge:
		p = "EISABridge";
		q = "GeneralEISABridge";
		break;
	case MicroChannelBridge:
		p = "MicroChannelBridge";
		q = "GeneralMCABridge";
		break;
	case PCIBridge:
		p = "PCIBridge";
		q = id->Interface >= NELEMS(PCIBridge_table)
		    ? NULL : PCIBridge_table[id->Interface];
		break;
	case PCMCIABridge:
		p = "PCMCIABridge";
		q = "GeneralPCMCIABridge";
		break;
	case VMEBridge:
		p = "VMEBridge";
		q = "GeneralVMEBridge";
		break;
	case OtherBridgeDevice:
		p = "OtherBridgeDevice";
		break;
	default:
		p = "UnknownBridgeDevice";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
comm_subr(DEVICE_ID *id)
{
	static const char *RS232Device_table[] = {
		"GeneralRS232",
		"COMx",
		"Compatible16450",
		"Compatible16550",
		"NS398SerPort",
		"NS26ESerPort",
		"NS15CSerPort",
		"NS2ESerPort",
	};
	static const char *ATCompatibleParallelPort_table[] = {
		"GeneralParPort",
		"LPTx",
		"NS398ParPort",
		"NS26EParPort",
		"NS15CParPort",
		"NS2EParPort",
	};
	const char *p, *q = NULL;

	switch (id->SubType) {
	case RS232Device:
		p = "RS232Device";
		q = id->Interface >= NELEMS(RS232Device_table)
		    ? NULL : RS232Device_table[id->Interface];
		break;
	case ATCompatibleParallelPort:
		p = "ATCompatibleParallelPort";
		q = id->Interface >= NELEMS(ATCompatibleParallelPort_table)
		    ? NULL : ATCompatibleParallelPort_table[id->Interface];
		break;
	case OtherCommunicationsDevice:
		p = "OtherCommunicationsDevice";
		break;
	default:
		p = "UnknownCommunicationsDevice";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
sys_subr(DEVICE_ID *id)
{
	static const char *PIC_table[] = {
		"GeneralPIC",
		"ISA_PIC",
		"EISA_PIC",
		"MPIC",
		"RS6K_PIC",
	};
	static const char *DMAController_table[] = {
		"GeneralDMA",
		"ISA_DMA",
		"EISA_DMA",
	};
	static const char *SystemTimer_table[] = {
		"GeneralTimer",
		"ISA_Timer",
		"EISA_Timer",
	};
	static const char *RealTimeClock_table[] = {
		"GeneralRTC",
		"ISA_RTC",
	};
	static const char *L2Cache_table[] = {
		"None",
		"StoreThruOnly",
		"StoreInEnabled",
		"RS6KL2Cache",
	};
	static const char *NVRAM_table[] = {
		"IndirectNVRAM",
		"DirectNVRAM",
		"IndirectNVRAM24",
	};
	static const char *PowerManagement_table[] = {
		"GeneralPowerManagement",
		"EPOWPowerManagement",
		"PowerControl",
	};
	static const char *GraphicAssist_table[] = {
		"Unknown",
		"TransferData",
		"IGMC32",
		"IGMC64",
	};
	static const char *OperatorPanel_table[] = {
		"GeneralOPPanel",
		"HarddiskLight",
		"CDROMLight",
		"PowerLight",
		"KeyLock",
		"ANDisplay",
		"SystemStatusLED",
		"CHRP_SystemStatusLED",
	};
	const char *p, *q = NULL;

	switch (id->SubType) {
	case ProgrammableInterruptController:
		p = "ProgrammableInterruptController";
		q = id->Interface >= NELEMS(PIC_table)
		    ? NULL : PIC_table[id->Interface];
		break;
	case DMAController:
		p = "DMAController";
		q = id->Interface >= NELEMS(DMAController_table)
		    ? NULL : DMAController_table[id->Interface];
		break;
	case SystemTimer:
		p = "SystemTimer";
		q = id->Interface >= NELEMS(SystemTimer_table)
		    ? NULL : SystemTimer_table[id->Interface];
		break;
	case RealTimeClock:
		p = "RealTimeClock";
		q = id->Interface >= NELEMS(RealTimeClock_table)
		    ? NULL : RealTimeClock_table[id->Interface];
		break;
	case L2Cache:
		p = "L2Cache";
		q = id->Interface >= NELEMS(L2Cache_table)
		    ? NULL : L2Cache_table[id->Interface];
		break;
	case NVRAM:
		p = "NVRAM";
		q = id->Interface >= NELEMS(NVRAM_table)
		    ? NULL : NVRAM_table[id->Interface];
		break;
	case PowerManagement:
		p = "PowerManagement";
		q = id->Interface >= NELEMS(PowerManagement_table)
		    ? NULL : PowerManagement_table[id->Interface];
		break;
	case CMOS:
		p = "CMOS";
		q = "GeneralCMOS";
		break;
	case OperatorPanel:
		p = "OperatorPanel";
		q = id->Interface >= NELEMS(OperatorPanel_table)
		    ? NULL : OperatorPanel_table[id->Interface];
		break;
	case ServiceProcessorClass1:
		p = "ServiceProcessorClass1";
		q = "GeneralServiceProcessor";
		break;
	case ServiceProcessorClass2:
		p = "ServiceProcessorClass2";
		q = "GeneralServiceProcessor";
		break;
	case ServiceProcessorClass3:
		p = "ServiceProcessorClass3";
		q = "GeneralServiceProcessor";
		break;
	case GraphicAssist:
		p = "GraphicAssist";
		q = id->Interface >= NELEMS(GraphicAssist_table)
		    ? NULL : GraphicAssist_table[id->Interface];
		break;
	case SystemPlanar:
		p = "SystemPlanar";
		q = "GeneralSystemPlanar";
		break;
	case OtherSystemPeripheral:
		p = "OtherSystemPeripheral";
		break;
	default:
		p = "UnknownSystemPeripheral";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
input_subr(DEVICE_ID *id)
{
	const char *p, *q = NULL;

	switch (id->SubType) {
	case KeyboardController:
		p = "KeyboardController";
		break;
	case Digitizer:
		p = "Digitizer";
		break;
	case MouseController:
		p = "MouseController";
		break;
	case TabletController:
		p = "TabletController";
		break;
	case OtherInputController:
		p = "OtherInputController";
		break;
	default:
		p = "UnknownInputController";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
service_subr(DEVICE_ID *id)
{
	const char *p, *q = NULL;

	switch (id->SubType) {
	case ServiceProcessorClass1:
		p = "ServiceProcessorClass1";
		break;
	case ServiceProcessorClass2:
		p = "ServiceProcessorClass2";
		break;
	case ServiceProcessorClass3:
		p = "ServiceProcessorClass3";
		break;
	default:
		p = "UnknownServiceProcessor";
		break;
	}

	printf("    SubType = %s (%d)\n", p, id->SubType);
	printf("    Interface = %s (%d)\n", q ? q : "None", id->Interface);
}

#endif /* RESIDUAL_DATA_DUMP */
