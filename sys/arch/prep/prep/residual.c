/*	$Id: residual.c,v 1.1 2002/05/02 14:36:43 nonaka Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/residual.h>
#include <machine/pnp.h>

int dump_residual_data = 1;

static void make_pnp_device_tree(void *);
static void bustype_subr(DEVICE_ID *);

#define	NELEMS(array)	((size_t)(sizeof(array)/sizeof(array[0])))

static char *FirmwareSupplier[] = {
	"IBMFirmware",
	"MotoFirmware",
	"FirmWorks",
	"Bull",
};

static char *FirmwareSupports[] = {
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

static char *EndianSwitchMethod[] = {
	"Unknown",
	"UsePort92",
	"UsePCIConfigA8",
	"UseFF001030",
};

static char *SpreadIOMethod[] = {
	"UsePort850",
};

static char *CacheAttrib[] = {
	"None",
	"Split cache",
	"Combined cache",
};

static char *Usage[] = {
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

static char *BusId[] = {
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

static char *Flags[] = {
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

static char hextochr[] = "0123456789ABCDEF";

void
print_residual_device_info(void)
{
	VPD *vpd;
	PPC_CPU *ppc_cpu;
	MEM_MAP *mem_map;
	PPC_MEM *ppc_mem;
	PPC_DEVICE *ppc_dev;
	char *str;
	unsigned long l;
	unsigned short s;
	unsigned long nmseg;
	unsigned long nmem;
	unsigned long ndev;
	unsigned long page_size;
	int ncpu;
	int first;
	int i, j;
	unsigned char p[4];

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
	printf("\tPrintableModel = %-32s\n", vpd->PrintableModel);
	printf("\tSerial = %-16s\n", vpd->Serial);

	l = be32toh(vpd->FirmwareSupplier);
	printf("\tFirmwareSupplier = %s\n",
	    (l >= NELEMS(FirmwareSupplier)) ? "Unknown" : FirmwareSupplier[l]);
	l = be32toh(vpd->FirmwareSupports);
	printf("\tFirmwareSupports = 0x%08lx\n", l);
	for (first = 1, i = 0; i < sizeof(unsigned long) * 8; i++) {
		if ((l & (1UL << i)) != 0) {
			printf("\t\t: %s\n", i >= NELEMS(FirmwareSupports)
			    ? "Unknown" : FirmwareSupports[i]);
			   first = 0;
		}
	}
	if (first)
		printf("\t\t: None\n");

	printf("\tNvramSize = %ld\n", be32toh(vpd->NvramSize));
	printf("\tNumSIMMSlots = %ld\n", be32toh(vpd->NumSIMMSlots));
	s = be16toh(vpd->EndianSwitchMethod);
	printf("\tEndianSwitchMethod = %s\n",
	    (s >= NELEMS(EndianSwitchMethod))
	      ? "Unknown" : EndianSwitchMethod[s]);
	s = be16toh(vpd->SpreadIOMethod);
	printf("\tSpreadIOMethod = %s\n",
	    (s >= NELEMS(SpreadIOMethod)) ? "Unknown" : SpreadIOMethod[s]);
	printf("\tSmpIar = %ld\n", be32toh(vpd->SmpIar));
	printf("\tRAMErrLogOffset = %ld\n", be32toh(vpd->RAMErrLogOffset));
	printf("\tProcessorHz = %ld\n", be32toh(vpd->ProcessorHz));
	printf("\tProcessorBusHz = %ld\n", be32toh(vpd->ProcessorBusHz));
	printf("\tTimeBaseDivisor = %ld\n", be32toh(vpd->TimeBaseDivisor));
	printf("\tWordWidth = %ld\n", be32toh(vpd->WordWidth));
	page_size = be32toh(vpd->PageSize);
	printf("\tPageSize = %ld\n", page_size);
	printf("\tCoherenceBlockSize = %ld\n",be32toh(vpd->CoherenceBlockSize));
	printf("\tGranuleSize = %ld\n", be32toh(vpd->GranuleSize));

	printf("\tL1 Cache variables\n");
	printf("\t\tCacheSize = %ld\n", be32toh(vpd->CacheSize));
	l = be32toh(vpd->CacheAttrib);
	printf("\t\tCacheAttrib = %s\n",
	    (s >= NELEMS(CacheAttrib)) ? "Unknown" : CacheAttrib[s]);
	printf("\t\tCacheAssoc = %ld\n", be32toh(vpd->CacheAssoc));
	printf("\t\tCacheLineSize = %ld\n", be32toh(vpd->CacheLineSize));

	/*
	 * PPC_CPU
	 */
	printf("\n");
	printf("MaxNumCpus = %d\n", be16toh(res->MaxNumCpus));
	ncpu = be16toh(res->ActualNumCpus);
	printf("ActualNumCpus = %d\n", ncpu);
	ppc_cpu = res->Cpus;
	for (i = 0; i < ((ncpu > MAX_CPUS) ? MAX_CPUS : ncpu); i++) {
		printf("%d:\n", i);
		printf("\tCpuType = %08lx\n", be32toh(ppc_cpu[i].CpuType));
		printf("\tCpuNumber = %d\n", ppc_cpu[i].CpuNumber);
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
		printf("\tCpuState: %s (%d)\n", str, ppc_cpu[i].CpuState);
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
		printf("\tUsage = ");
		for (first = 1, j = 0; j < sizeof(unsigned long) * 8; j++) {
			if ((l & (1UL << j)) != 0) {
				printf("%s%s", first ? "" : ", ",
				    j >= NELEMS(Usage) ? "Unknown" : Usage[j]);
				first = 0;
			}
		}
		printf(" (0x%08lx)\n", l);
		printf("\tBasePage  = 0x%05lx000\n",
		    be32toh(mem_map[i].BasePage));
		pc = be32toh(mem_map[i].PageCount);
		printf("\tPageCount = 0x%08lx (%ld page%s)\n",
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
		printf("\tSIMMSize = %ld MB\n", be32toh(ppc_mem[i].SIMMSize));
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

		printf("\tDEVICE_ID\n");
		l = be32toh(id->BusId);
		printf("\t\tBusId = ");
		for (j = 0; j < sizeof(unsigned long) * 8; j++) {
			if ((l & (1UL << j)) != 0) {
				printf("%s",
				    j >= NELEMS(BusId) ? "Unknown" : BusId[j]);
				break;
			}
		}
		printf("\n");
		l = be32toh(id->DevId);
		p[0] = (l >> 24) & 0xff;
		p[1] = (l >> 16) & 0xff;
		p[2] = (l >> 8) & 0xff;
		p[3] = l & 0xff;
		printf("\t\tDevId = 0x%08lx (%c%c%c%c%c%c%c)\n", l,
		    ((p[0] >> 2) & 0x1f) + 'A' - 1,
		    (((p[0] & 0x03) << 3) | ((p[1] >> 5) & 0x07)) + 'A' - 1,
		    (p[1] & 0x1f) + 'A' - 1,
		    hextochr[(p[2] >> 4) & 0xf], hextochr[p[2] & 0xf],
		    hextochr[(p[3] >> 4) & 0xf], hextochr[p[3] & 0xf]);
		printf("\t\tSerialNum = 0x%08lx\n", be32toh(id->SerialNum));
		l = be32toh(id->Flags);
		printf("\t\tFlags = 0x%08lx\n", l);
		for (first = 1, j = 0; j < sizeof(unsigned long) * 8; j++) {
			if ((l & (1UL << j)) != 0) {
				printf("\t\t\t: %s\n",
				    j >= NELEMS(Flags) ? "Unknown" : Flags[j]);
				first = 0;
			}
		}
		if (first)
			printf("\t\t\t: None\n");

		bustype_subr(id);

		printf("\tBUS_ACCESS\n");
		printf("\t\tinfo0 = %d\n", bus->PnPAccess.CSN);
		printf("\t\tinfo1 = %d\n", bus->PnPAccess.LogicalDevNumber);

		l = be32toh(ppc_dev[i].AllocatedOffset);
		printf("\tAllocatedOffset  = 0x%08lx\n", l);
		make_pnp_device_tree(res->DevicePnPHeap + l);

		l = be32toh(ppc_dev[i].PossibleOffset);
		printf("\tPossibleOffset   = 0x%08lx\n", l);
		make_pnp_device_tree(res->DevicePnPHeap + l);

		l = be32toh(ppc_dev[i].CompatibleOffset);
		printf("\tCompatibleOffset = 0x%08lx\n", l);
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

		printf("\t\tCompatibleDevice = %c%c%c%c%c%c%c\n",
		    ((q[0] >> 2) & 0x1f) + 'A' - 1,
		    (((q[0] & 0x03) << 3) | ((q[1] >> 5) & 0x07)) + 'A' - 1,
		    (q[1] & 0x1f) + 'A' - 1,
		    hextochr[(q[2] >> 4) & 0xf], hextochr[q[2] & 0xf],
		    hextochr[(q[3] >> 4) & 0xf], hextochr[q[3] & 0xf]);
		}
		break;

	case IRQFormat: {
		struct _S4_Pack *p = v;

		printf("\t\tIRQ: ");
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
			static char *IRQInfo[] = {
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

		printf("\t\tDMA: ");
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
				printf("\t\tIOPort: 0x%x-0x%x",
				    iomin, iomin + len-1);
			else
				printf("\t\tIOPort: min 0x%x-0x%x,"
				    " max 0x%x-0x%x (%d byte%s align)",
				      iomin, iomin + len-1,
				      iomax, iomax + len-1,
				      align, align != 1 ? "s" : "");
		} else {
			if (iomin == iomax)
				printf("\t\tIOPort: 0x%x", iomin);
			else
				printf("\t\tIOPort: min 0x%x, max 0x%x"
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
			printf("\t\tFixedIOPort: 0x%x-0x%x",
			    ioport, ioport + len - 1);
		else
			printf("\t\tFixedIOPort: 0x%x", ioport);
		printf("\n");
		}
		break;

	case SmallVendorItem: {
		unsigned char *p = v;

		printf("\t\tSmallVendorItem: ");
		switch (p[1]) {
		case 1:
			printf("%c%c%c",
			    ((p[2] >> 2) & 0x1f) + 'A' - 1,
			    (((p[2] & 3) << 3) | ((p[3] >> 5) & 7)) + 'A' - 1,
			    (p[3] & 0x1f) + 'A' - 1);
			break;

		default:
			break;
		}

		printf("\n");
		for (i = 0; i < size - 1; i++) {
			if ((i % 16) == 0)
				printf("\t\t\t");
			printf("%02x ", p[i + 1]);
			if ((i % 16) == 15)
				printf("\n");
		}
		if ((i % 16) != 0)
			printf("\n");
		}
		break;

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

static int
pnp_large_pkt(void *v)
{
	int tag = *(unsigned char *)v;
	unsigned char *q = v;
	int item, size;
	int i;

	item = tag_large_item_name(tag);
	size = (q[1] | (q[2] << 8)) + 3 /* tag + length */;

	switch (item) {
	case LargeVendorItem: {
		unsigned char *p = v;

		printf("\t\tLargeVendorItem:\n");
		for (i = 0; i < size - 3; i++) {
			if ((i % 16) == 0)
				printf("\t\t\t");
			printf("%02x ", p[i + 3]);
			if ((i % 16) == 15)
				printf("\n");
		}
		if ((i % 16) != 0)
			printf("\n");
		}
		break;
	
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
		char	*str;
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

	printf("\t\tBaseType = %s (%d)\n", BaseType[type].str, id->BaseType);
	if (BaseType[type].func != NULL)
		(*BaseType[type].func)(id);
}

static void
mass_subr(DEVICE_ID *id)
{
	static char *IDEController_tabel[] = {
		"GeneralIDE",
		"ATACompatible",
	};
	static char *FloppyController_table[] = {
		"GeneralFloppy",
		"Compatible765",
		"NS398_Floppy",
		"NS26E_Floppy",
		"NS15C_Floppy",
		"NS2E_Floppy",
		"CHRP_Floppy",
	};
	char *p, *q = NULL;

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

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
nic_subr(DEVICE_ID *id)
{
	char *p, *q = NULL;

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

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
display_subr(DEVICE_ID *id)
{
	char *p, *q = NULL;

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

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
mm_subr(DEVICE_ID *id)
{
	static char *AudioController_table[] = {
		"GeneralAudio",
		"CS4232Audio",
	};
	char *p, *q = NULL;

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

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
mem_subr(DEVICE_ID *id)
{
	char *p, *q = NULL;

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

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
bridge_subr(DEVICE_ID *id)
{
	static char *PCIBridge_table[] = {
		"GeneralPCIBridge",
		"PCIBridgeIndirect",
		"PCIBridgeRS6K",
	};
	char *p, *q = NULL;

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

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
comm_subr(DEVICE_ID *id)
{
	static char *RS232Device_table[] = {
		"GeneralRS232",
		"COMx",
		"Compatible16450",
		"Compatible16550",
		"NS398SerPort",
		"NS26ESerPort",
		"NS15CSerPort",
		"NS2ESerPort",
	};
	static char *ATCompatibleParallelPort_table[] = {
		"GeneralParPort",
		"LPTx",
		"NS398ParPort",
		"NS26EParPort",
		"NS15CParPort",
		"NS2EParPort",
	};
	char *p, *q = NULL;

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

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
sys_subr(DEVICE_ID *id)
{
	static char *PIC_table[] = {
		"GeneralPIC",
		"ISA_PIC",
		"EISA_PIC",
		"MPIC",
		"RS6K_PIC",
	};
	static char *DMAController_table[] = {
		"GeneralDMA",
		"ISA_DMA",
		"EISA_DMA",
	};
	static char *SystemTimer_table[] = {
		"GeneralTimer",
		"ISA_Timer",
		"EISA_Timer",
	};
	static char *RealTimeClock_table[] = {
		"GeneralRTC",
		"ISA_RTC",
	};
	static char *L2Cache_table[] = {
		"None",
		"StoreThruOnly",
		"StoreInEnabled",
		"RS6KL2Cache",
	};
	static char *NVRAM_table[] = {
		"IndirectNVRAM",
		"DirectNVRAM",
		"IndirectNVRAM24",
	};
	static char *PowerManagement_table[] = {
		"GeneralPowerManagement",
		"EPOWPowerManagement",
		"PowerControl",
	};
	static char *GraphicAssist_table[] = {
		"Unknown",
		"TransferData",
		"IGMC32",
		"IGMC64",
	};
	static char *OperatorPanel_table[] = {
		"GeneralOPPanel",
		"HarddiskLight",
		"CDROMLight",
		"PowerLight",
		"KeyLock",
		"ANDisplay",
		"SystemStatusLED",
		"CHRP_SystemStatusLED",
	};
	char *p, *q = NULL;

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

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
input_subr(DEVICE_ID *id)
{
	char *p, *q = NULL;

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

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}

static void
service_subr(DEVICE_ID *id)
{
	char *p, *q = NULL;

	switch (id->SubType) {
	case GeneralMemoryController:
		p = "GeneralMemoryController";
		break;
	default:
		p = "UnknownMemoryController";
		break;
	}

	printf("\t\tSubType = %s (%d)\n", p, id->SubType);
	printf("\t\tInterface = %s (%d)\n", q ? q : "None", id->Interface);
}
