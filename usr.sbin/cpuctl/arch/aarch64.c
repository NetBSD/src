/*	$NetBSD: aarch64.c,v 1.21 2022/04/30 14:06:10 ryo Exp $	*/

/*
 * Copyright (c) 2018 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: aarch64.c,v 1.21 2022/04/30 14:06:10 ryo Exp $");
#endif /* no lint */

#include <sys/types.h>
#include <sys/cpuio.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <err.h>

#include <arm/cputypes.h>
#include <aarch64/armreg.h>

#include "../cpuctl.h"

struct cpuidtab {
	uint32_t cpu_partnum;
	const char *cpu_name;
	const char *cpu_class;
	const char *cpu_architecture;
};

struct impltab {
	uint32_t impl_id;
	const char *impl_name;
};

struct fieldinfo {
	unsigned int flags;
#define FIELDINFO_FLAGS_DEC	0x0001
#define FIELDINFO_FLAGS_4LOG2	0x0002
	unsigned char bitpos;
	unsigned char bitwidth;
	const char *name;
	const char * const *info;
};


#define CPU_PARTMASK	(CPU_ID_IMPLEMENTOR_MASK | CPU_ID_PARTNO_MASK)
const struct cpuidtab cpuids[] = {
	{ CPU_ID_CORTEXA35R0 & CPU_PARTMASK, "Cortex-A35", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA53R0 & CPU_PARTMASK, "Cortex-A53", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA57R0 & CPU_PARTMASK, "Cortex-A57", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA55R1 & CPU_PARTMASK, "Cortex-A55", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA65R0 & CPU_PARTMASK, "Cortex-A65", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA72R0 & CPU_PARTMASK, "Cortex-A72", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA73R0 & CPU_PARTMASK, "Cortex-A73", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA75R2 & CPU_PARTMASK, "Cortex-A75", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA76R3 & CPU_PARTMASK, "Cortex-A76", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA76AER1 & CPU_PARTMASK, "Cortex-A76AE", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA77R0 & CPU_PARTMASK, "Cortex-A77", "Arm", "v8.2-A+" },
	{ CPU_ID_NVIDIADENVER2 & CPU_PARTMASK, "Denver2", "NVIDIA", "v8-A" },
	{ CPU_ID_EMAG8180 & CPU_PARTMASK, "eMAG", "Ampere", "v8-A" },
	{ CPU_ID_NEOVERSEE1R1 & CPU_PARTMASK, "Neoverse E1", "Arm", "v8.2-A+" },
	{ CPU_ID_NEOVERSEN1R3 & CPU_PARTMASK, "Neoverse N1", "Arm", "v8.2-A+" },
	{ CPU_ID_THUNDERXRX, "ThunderX", "Cavium", "v8-A" },
	{ CPU_ID_THUNDERX81XXRX, "ThunderX CN81XX", "Cavium", "v8-A" },
	{ CPU_ID_THUNDERX83XXRX, "ThunderX CN83XX", "Cavium", "v8-A" },
	{ CPU_ID_THUNDERX2RX, "ThunderX2", "Marvell", "v8.1-A" },
	{ CPU_ID_APPLE_M1_ICESTORM & CPU_PARTMASK, "M1 Icestorm", "Apple", "Apple Silicon" },
	{ CPU_ID_APPLE_M1_FIRESTORM & CPU_PARTMASK, "M1 Firestorm", "Apple", "Apple Silicon" },
};

const struct impltab implids[] = {
	{ CPU_ID_ARM_LTD,	"ARM Limited"				},
	{ CPU_ID_BROADCOM,	"Broadcom Corporation"			},
	{ CPU_ID_CAVIUM,	"Cavium Inc."				},
	{ CPU_ID_DEC,		"Digital Equipment Corporation"		},
	{ CPU_ID_INFINEON,	"Infineon Technologies AG"		},
	{ CPU_ID_MOTOROLA,	"Motorola or Freescale Semiconductor Inc." },
	{ CPU_ID_NVIDIA,	"NVIDIA Corporation"			},
	{ CPU_ID_APM,		"Applied Micro Circuits Corporation"	},
	{ CPU_ID_QUALCOMM,	"Qualcomm Inc."				},
	{ CPU_ID_SAMSUNG,	"SAMSUNG"				},
	{ CPU_ID_TI,		"Texas Instruments"			},
	{ CPU_ID_MARVELL,	"Marvell International Ltd."		},
	{ CPU_ID_APPLE,		"Apple Inc."				},
	{ CPU_ID_FARADAY,	"Faraday Technology Corporation"	},
	{ CPU_ID_INTEL,		"Intel Corporation"			}
};

#define FIELDNAME(_bitpos, _bitwidth, _name)	\
	.bitpos = _bitpos,			\
	.bitwidth = _bitwidth,			\
	.name = _name

#define FIELDINFO(_bitpos, _bitwidth, _name)	\
	FIELDNAME(_bitpos, _bitwidth, _name),	\
	.info = (const char *[1 << _bitwidth])


/* ID_AA64PFR0_EL1 - AArch64 Processor Feature Register 0 */
struct fieldinfo id_aa64pfr0_fieldinfo[] = {
	{
		FIELDINFO(0, 4, "EL0") {
			[0] = "No EL0",
			[1] = "AArch64",
			[2] = "AArch64/AArch32"
		}
	},
	{
		FIELDINFO(4, 4, "EL1") {
			[0] = "No EL1",
			[1] = "AArch64",
			[2] = "AArch64/AArch32"
		}
	},
	{
		FIELDINFO(8, 4, "EL2") {
			[0] = "No EL2",
			[1] = "AArch64",
			[2] = "AArch64/AArch32"
		}
	},
	{
		FIELDINFO(12, 4, "EL3") {
			[0] = "No EL3",
			[1] = "AArch64",
			[2] = "AArch64/AArch32"
		}
	},
	{
		FIELDINFO(16, 4, "FP") {
			[0] = "Floating Point",
			[1] = "Floating Point including half-precision support",
			[15] = "No Floating Point"
		}
	},
	{
		FIELDINFO(20, 4, "AdvSIMD") {
			[0] = "Advanced SIMD",
			[1] = "Advanced SIMD including half-precision support",
			[15] = "No Advanced SIMD"
		}
	},
	{
		FIELDINFO(24, 4, "GIC") {
			[0] = "GIC CPU interface sysregs not implemented",
			[1] = "GIC CPU interface sysregs v3.0/4.0 supported",
			[3] = "GIC CPU interface sysregs v4.1 supported"
		}
	},
	{
		FIELDINFO(28, 4, "RAS") {
			[0] = "Reliability/Availability/Serviceability not supported",
			[1] = "Reliability/Availability/Serviceability supported",
			[2] = "Reliability/Availability/Serviceability ARMv8.4 supported",
		},
	},
	{
		FIELDINFO(32, 4, "SVE") {
			[0] = "Scalable Vector Extensions not implemented",
			[1] = "Scalable Vector Extensions implemented",
		},
	},
	{
		FIELDINFO(36, 4, "SEL2") {
			[0] = "Secure EL2 not implemented",
			[1] = "Secure EL2 implemented",
		},
	},
	{
		FIELDINFO(40, 4, "MPAM") {
			[0] = "Memory Partitioning and Monitoring not implemented",
			[1] = "Memory Partitioning and Monitoring implemented",
		},
	},
	{
		FIELDINFO(44, 4, "AMU") {
			[0] = "Activity Monitors Extension not implemented",
			[1] = "Activity Monitors Extension v1 ARMv8.4",
			[2] = "Activity Monitors Extension v1 ARMv8.6",
		},
	},
	{
		FIELDINFO(48, 4, "DIT") {
			[0] = "No Data-Independent Timing guarantees",
			[1] = "Data-Independent Timing guaranteed by PSTATE.DIT",
		},
	},
	{
		FIELDINFO(56, 4, "CSV2") {
			[0] = "Branch prediction might be Spectred",
			[1] = "Branch prediction maybe not Spectred",
			[2] = "Branch prediction probably not Spectred",
		},
	},
	{
		FIELDINFO(60, 4, "CSV3") {
			[0] = "Faults might be Spectred",
			[1] = "Faults maybe not Spectred",
			[2] = "Faults probably not Spectred",
		},
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* ID_AA64PFR1_EL1 - AArch64 Processor Feature Register 1 */
struct fieldinfo id_aa64pfr1_fieldinfo[] = {
	{
		FIELDINFO(0, 4, "BT") {
			[0] = "Branch Target Identification not implemented",
			[1] = "Branch Target Identification implemented",
		}
	},
	{
		FIELDINFO(4, 4, "SSBS") {
			[0] = "Speculative Store Bypassing control not implemented",
			[1] = "Speculative Store Bypassing control implemented",
			[2] = "Speculative Store Bypassing control implemented, plus MSR/MRS"
		}
	},
	{
		FIELDINFO(8, 4, "MTE") {
			[0] = "Memory Tagging Extension not implemented",
			[1] = "Instruction-only Memory Taggined Extension"
			    " implemented",
			[2] = "Full Memory Tagging Extension implemented",
			[3] = "Memory Tagging Extension implemented"
			    " with Tag Check Fault handling"
		}
	},
	{
		FIELDINFO(12, 4, "RAS_frac") {
			[0] = "Regular RAS",
			[1] = "RAS plus registers"
		}
	},
	{
		FIELDINFO(16, 4, "MPAM_frac") {
			[0] = "MPAM not implemented, or v1.0",
			[1] = "MPAM v0.1 or v1.1"
		}
	},
	{
		FIELDINFO(32, 4, "CSV2_frac") {
			[0] = "not disclosed",
			[1] = "SCXTNUM_ELx registers not supported",
			[2] = "SCXTNUM_ELx registers supported"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* ID_AA64ISAR0_EL1 - AArch64 Instruction Set Attribute Register 0 */
struct fieldinfo id_aa64isar0_fieldinfo[] = {
	{
		FIELDINFO(4, 4, "AES") {
			[0] = "No AES",
			[1] = "AESE/AESD/AESMC/AESIMC",
			[2] = "AESE/AESD/AESMC/AESIMC+PMULL/PMULL2"
		}
	},
	{
		FIELDINFO(8, 4, "SHA1") {
			[0] = "No SHA1",
			[1] = "SHA1C/SHA1P/SHA1M/SHA1H/SHA1SU0/SHA1SU1"
		}
	},
	{
		FIELDINFO(12, 4, "SHA2") {
			[0] = "No SHA2",
			[1] = "SHA256H/SHA256H2/SHA256SU0/SHA256SU1",
			[2] = "SHA256H/SHA256H2/SHA256SU0/SHA256SU1"
			     "/SHA512H/SHA512H2/SHA512SU0/SHA512SU1"
		}
	},
	{
		FIELDINFO(16, 4, "CRC32") {
			[0] = "No CRC32",
			[1] = "CRC32B/CRC32H/CRC32W/CRC32X"
			    "/CRC32CB/CRC32CH/CRC32CW/CRC32CX"
		}
	},
	{
		FIELDINFO(20, 4, "Atomic") {
			[0] = "No Atomic",
			[2] = "LDADD/LDCLR/LDEOR/LDSET/LDSMAX/LDSMIN"
			    "/LDUMAX/LDUMIN/CAS/CASP/SWP",
		}
	},
	{
		FIELDINFO(28, 4, "RDM") {
			[0] = "No RDMA",
			[1] = "SQRDMLAH/SQRDMLSH",
		}
	},
	{
		FIELDINFO(32, 4, "SHA3") {
			[0] = "No SHA3",
			[1] = "EOR3/RAX1/XAR/BCAX",
		}
	},
	{
		FIELDINFO(36, 4, "SM3") {
			[0] = "No SM3",
			[1] = "SM3SS1/SM3TT1A/SM3TT1B/SM3TT2A/SM3TT2B"
			    "/SM3PARTW1/SM3PARTW2",
		}
	},
	{
		FIELDINFO(40, 4, "SM4") {
			[0] = "No SM4",
			[1] = "SM4E/SM4EKEY",
		}
	},
	{
		FIELDINFO(44, 4, "DP") {
			[0] = "No Dot Product",
			[1] = "UDOT/SDOT",
		}
	},
	{
		FIELDINFO(48, 4, "FHM") {
			[0] = "No FHM",
			[1] = "FMLAL/FMLSL",
		}
	},
	{
		FIELDINFO(52, 4, "TS") {
			[0] = "No TS",
			[1] = "CFINV/RMIF/SETF16/SETF8",
			[2] = "CFINV/RMIF/SETF16/SETF8/AXFLAG/XAFLAG",
		}
	},
	{
		FIELDINFO(56, 4, "TLBI") {
			[0] = "No outer shareable and TLB range maintenance"
			    " instructions",
			[1] = "Outer shareable TLB maintenance instructions",
			[2] = "Outer shareable and TLB range maintenance"
			    " instructions",
		}
	},
	{
		FIELDINFO(60, 4, "RNDR") {
			[0] = "No RNDR/RNDRRS",
			[1] = "RNDR/RNDRRS",
		},
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* ID_AA64ISAR0_EL1 - AArch64 Instruction Set Attribute Register 0 */
struct fieldinfo id_aa64isar1_fieldinfo[] = {
	{
		FIELDINFO(0, 4, "DPB") {
			[0] = "No DC CVAP",
			[1] = "DC CVAP",
			[2] = "DC CVAP/DC CVADP"
		}
	},
	{
		FIELDINFO(4, 4, "APA") {
			[0] = "No Architected Address Authentication algorithm",
			[1] = "QARMA with PAC",
			[2] = "QARMA with EnhancedPAC",
			[3] = "QARMA with EnhancedPAC2",
			[4] = "QARMA with EnhancedPAC/PAC2",
			[5] = "QARMA with EnhancedPAC/PAC2/FPACCombined"
		}
	},
	{
		FIELDINFO(8, 4, "API") {
			[0] = "No Address Authentication algorithm",
			[1] = "Address Authentication algorithm implemented",
			[2] = "EnhancedPAC",
			[3] = "EnhancedPAC2",
			[4] = "EnhancedPAC2/FPAC",
			[5] = "EnhancedPAC2/FPAC/FPACCombined"
		}
	},
	{
		FIELDINFO(12, 4, "JSCVT") {
			[0] = "No FJCVTZS",
			[1] = "FJCVTZS"
		}
	},
	{
		FIELDINFO(16, 4, "FCMA") {
			[0] = "No FCMA",
			[1] = "FCMLA/FCADD"
		}
	},
	{
		FIELDINFO(20, 4, "LRCPC") {
			[0] = "no LRCPC",
			[1] = "LDAPR",
			[2] = "LDAPR/LDAPUR/STLUR"
		}
	},
	{
		FIELDINFO(24, 4, "GPA") {
			[0] = "No Architected Generic Authentication algorithm",
			[1] = "QARMA with PACGA"
		}
	},
	{
		FIELDINFO(28, 4, "GPI") {
			[0] = "No Generic Authentication algorithm",
			[1] = "Generic Authentication algorithm implemented"
		}
	},
	{
		FIELDINFO(32, 4, "FRINTTS") {
			[0] = "No FRINTTS",
			[1] = "FRINT32Z/FRINT32X/FRINT64Z/FRINT64X"
		}
	},
	{
		FIELDINFO(36, 4, "SB") {
			[0] = "No SB",
			[1] = "SB"
		}
	},
	{
		FIELDINFO(40, 4, "SPECRES") {
			[0] = "No SPECRES",
			[1] = "CFP RCTX/DVP RCTX/CPP RCTX"
		}
	},
	{
		FIELDINFO(44, 4, "BF16") {
			[0] = "No BFloat16",
			[1] = "BFCVT/BFCVTN/BFCVTN2/BFDOT"
			    "/BFMLALB/BFMLALT/BFMMLA"
		}
	},
	{
		FIELDINFO(48, 4, "DGH") {
			[0] = "Data Gathering Hint not implemented",
			[1] = "Data Gathering Hint implemented"
		}
	},
	{
		FIELDINFO(52, 4, "I8MM") {
			[0] = "No Int8 matrix",
			[1] = "SMMLA/SUDOT/UMMLA/USMMLA/USDOT"
		}
	},
	{
		FIELDINFO(56, 4, "XS") {
			[0] = "No XS/nXS qualifier",
			[1] = "XS attribute, TLBI and DSB"
			    " with nXS qualifier supported"
		}
	},
	{
		FIELDINFO(60, 4, "LS64") {
			[0] = "No LS64",
			[1] = "LD64B/ST64B",
			[2] = "LD64B/ST64B/ST64BV",
			[3] = "LD64B/ST64B/ST64BV/ST64BV0/ACCDATA_EL1",
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* ID_AA64MMFR0_EL1 - AArch64 Memory Model Feature Register 0 */
struct fieldinfo id_aa64mmfr0_fieldinfo[] = {
	{
		FIELDINFO(0, 4, "PARange") {
			[0] = "32bits/4GB",
			[1] = "36bits/64GB",
			[2] = "40bits/1TB",
			[3] = "42bits/4TB",
			[4] = "44bits/16TB",
			[5] = "48bits/256TB",
			[6] = "52bits/4PB"
		}
	},
	{
		FIELDINFO(4, 4, "ASIDBit") {
			[0] = "8bits",
			[2] = "16bits"
		}
	},
	{
		FIELDINFO(8, 4, "BigEnd") {
			[0] = "No mixed-endian",
			[1] = "Mixed-endian"
		}
	},
	{
		FIELDINFO(12, 4, "SNSMem") {
			[0] = "No distinction B/W Secure and Non-secure Memory",
			[1] = "Distinction B/W Secure and Non-secure Memory"
		}
	},
	{
		FIELDINFO(16, 4, "BigEndEL0") {
			[0] = "No mixed-endian at EL0",
			[1] = "Mixed-endian at EL0"
		}
	},
	{
		FIELDINFO(20, 4, "TGran16") {
			[0] = "No 16KB granule",
			[1] = "16KB granule"
		}
	},
	{
		FIELDINFO(24, 4, "TGran64") {
			[0] = "64KB granule",
			[15] = "No 64KB granule"
		}
	},
	{
		FIELDINFO(28, 4, "TGran4") {
			[0] = "4KB granule",
			[15] = "No 4KB granule"
		}
	},
	{
		FIELDINFO(32, 4, "TGran16_2") {
			[0] = "same as TGran16",
			[1] = "No 16KB granule at stage2",
			[2] = "16KB granule at stage2",
			[3] = "16KB granule at stage2/52bit"
		}
	},
	{
		FIELDINFO(36, 4, "TGran64_2") {
			[0] = "same as TGran64",
			[1] = "No 64KB granule at stage2",
			[2] = "64KB granule at stage2"
		}
	},
	{
		FIELDINFO(40, 4, "TGran4_2") {
			[0] = "same as TGran4",
			[1] = "No 4KB granule at stage2",
			[2] = "4KB granule at stage2"
		}
	},
	{
		FIELDINFO(44, 4, "ExS") {
			[0] = "All Exception entries and exits are context"
			    " synchronization events",
			[1] = "Non-context synchronizing exception entry and"
			    " exit are supported"
		}
	},
	{
		FIELDINFO(56, 4, "FGT") {
			[0] = "fine-grained trap controls not implemented",
			[1] = "fine-grained trap controls implemented"
		}
	},
	{
		FIELDINFO(60, 4, "ECV") {
			[0] = "Enhanced Counter Virtualization not implemented",
			[1] = "Enhanced Counter Virtualization implemented",
			[2] = "Enhanced Counter Virtualization"
			    " + CNTHCTL_EL2.ECV/CNTPOFF_EL2 implemented"
		}
	},

	{ .bitwidth = 0 }	/* end of table */
};

/* ID_AA64MMFR1_EL1 - AArch64 Memory Model Feature Register 1 */
struct fieldinfo id_aa64mmfr1_fieldinfo[] = {
	{
		FIELDINFO(0, 4, "HAFDBS") {
			[0] = "Access and Dirty flags not supported",
			[1] = "Access flag supported",
			[2] = "Access and Dirty flags supported",
		}
	},
	{
		FIELDINFO(4, 4, "VMIDBits") {
			[0] = "8bits",
			[2] = "16bits"
		}
	},
	{
		FIELDINFO(8, 4, "VH") {
			[0] = "Virtualization Host Extensions not supported",
			[1] = "Virtualization Host Extensions supported",
		}
	},
	{
		FIELDINFO(12, 4, "HPDS") {
			[0] = "Disabling of hierarchical controls not supported",
			[1] = "Disabling of hierarchical controls supported",
			[2] = "Disabling of hierarchical controls supported, plus PTD"
		}
	},
	{
		FIELDINFO(16, 4, "LO") {
			[0] = "LORegions not supported",
			[1] = "LORegions supported"
		}
	},
	{
		FIELDINFO(20, 4, "PAN") {
			[0] = "PAN not supported",
			[1] = "PAN supported",
			[2] = "PAN supported, and instructions supported",
			[3] = "PAN supported, instructions supported"
			    ", and SCTLR_EL[12].EPAN bits supported"
		}
	},
	{
		FIELDINFO(24, 4, "SpecSEI") {
			[0] = "SError interrupt not supported",
			[1] = "SError interrupt supported"
		}
	},
	{
		FIELDINFO(28, 4, "XNX") {
			[0] = "Distinction between EL0 and EL1 XN control"
			    " at stage2 not supported",
			[1] = "Distinction between EL0 and EL1 XN control"
			    " at stage2 supported"
		}
	},
	{
		FIELDINFO(32, 4, "TWED") {
			[0] = "Configurable delayed trapping of WFE is not"
			    " supported",
			[1] = "Configurable delayed trapping of WFE supported"
		}
	},
	{
		FIELDINFO(36, 4, "ETS") {
			[0] = "Enhanced Translation Synchronization not"
			    " supported",
			[1] = "Enhanced Translation Synchronization supported"
		}
	},
	{
		FIELDINFO(40, 4, "HCX") {
			[0] = "HCRX_EL2 not supported",
			[1] = "HCRX_EL2 supported"
		}
	},
	{
		FIELDINFO(44, 4, "AFP") {
			[0] = "FPCR.{AH,FIZ,NEP} fields not supported",
			[1] = "FPCR.{AH,FIZ,NEP} fields supported"
		}
	},
	{
		FIELDINFO(48, 4, "nTLBPA") {
			[0] = "might include non-coherent caches",
			[1] = "does not include non-coherent caches"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* ID_AA64DFR0_EL1 - AArch64 Debug Feature Register 0 */
struct fieldinfo id_aa64dfr0_fieldinfo[] = {
	{
		FIELDINFO(0, 4, "DebugVer") {
			[6] = "ARMv8 debug architecture",
			[7] = "ARMv8 debug architecture"
			    " with Virtualization Host Extensions",
			[8] = "ARMv8.2 debug architecture",
			[9] = "ARMv8.4 debug architecture"
		}
	},
	{
		FIELDINFO(4, 4, "TraceVer") {
			[0] = "Trace supported",
			[1] = "Trace not supported"
		}
	},
	{
		FIELDINFO(8, 4, "PMUVer") {
			[0] = "No Performance monitor",
			[1] = "Performance monitor unit v3",
			[4] = "Performance monitor unit v3 for ARMv8.1",
			[5] = "Performance monitor unit v3 for ARMv8.4",
			[6] = "Performance monitor unit v3 for ARMv8.5",
			[7] = "Performance monitor unit v3 for ARMv8.7",
			[15] = "implementation defined"
		}
	},
	{
		FIELDINFO(32, 4, "PMSVer") {
			[0] = "Statistical Profiling Extension not implemented",
			[1] = "Statistical Profiling Extension implemented",
			[2] = "Statistical Profiling Extension and "
			    "Event packet alignment flag implemented",
			[3] = "Statistical Profiling Extension, "
			    "Event packet alignment flag, and "
			    "Branch target address packet, etc."
		}
	},
	{
		FIELDINFO(36, 4, "DoubleLock") {
			[0] = "OS Double Lock implemented",
			[1] = "OS Double Lock not implemented"
		}
	},
	{
		FIELDINFO(40, 4, "TraceFilt") {
			[0] = "ARMv8.4 Self-hosted Trace Extension not "
			    "implemented",
			[1] = "ARMv8.4 Self-hosted Trace Extension implemented"
		}
	},
	{
		FIELDINFO(48, 4, "MTPMU") {
			[0] = "Multi-threaded PMU extension not implemented,"
			    " or implementation defined",
			[1] = "Multi-threaded PMU extension implemented",
			[15] = "Multi-threaded PMU extension not implemented"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};


/* MVFR0_EL1 - Media and VFP Feature Register 0 */
struct fieldinfo mvfr0_fieldinfo[] = {
	{
		FIELDINFO(0, 4, "SIMDreg") {
			[0] = "No SIMD",
			[1] = "16x64-bit SIMD",
			[2] = "32x64-bit SIMD"
		}
	},
	{
		FIELDINFO(4, 4, "FPSP") {
			[0] = "No VFP support single precision",
			[1] = "VFPv2 support single precision",
			[2] = "VFPv2/VFPv3/VFPv4 support single precision"
		}
	},
	{
		FIELDINFO(8, 4, "FPDP") {
			[0] = "No VFP support double precision",
			[1] = "VFPv2 support double precision",
			[2] = "VFPv2/VFPv3/VFPv4 support double precision"
		}
	},
	{
		FIELDINFO(12, 4, "FPTrap") {
			[0] = "No floating point exception trapping support",
			[1] = "VFPv2/VFPv3/VFPv4 support exception trapping"
		}
	},
	{
		FIELDINFO(16, 4, "FPDivide") {
			[0] = "VDIV not supported",
			[1] = "VDIV supported"
		}
	},
	{
		FIELDINFO(20, 4, "FPSqrt") {
			[0] = "VSQRT not supported",
			[1] = "VSQRT supported"
		}
	},
	{
		FIELDINFO(24, 4, "FPShVec") {
			[0] = "Short Vectors not supported",
			[1] = "Short Vectors supported"
		}
	},
	{
		FIELDINFO(28, 4, "FPRound") {
			[0] = "Only Round to Nearest mode",
			[1] = "All rounding modes"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* MVFR1_EL1 - Media and VFP Feature Register 1 */
struct fieldinfo mvfr1_fieldinfo[] = {
	{
		FIELDINFO(0, 4, "FPFtZ") {
			[0] = "only the Flush-to-Zero",
			[1] = "full Denormalized number arithmetic"
		}
	},
	{
		FIELDINFO(4, 4, "FPDNan") {
			[0] = "Default NaN",
			[1] = "Propagation of NaN"
		}
	},
	{
		FIELDINFO(8, 4, "SIMDLS") {
			[0] = "No Advanced SIMD Load/Store",
			[1] = "Advanced SIMD Load/Store"
		}
	},
	{
		FIELDINFO(12, 4, "SIMDInt") {
			[0] = "No Advanced SIMD Integer",
			[1] = "Advanced SIMD Integer"
		}
	},
	{
		FIELDINFO(16, 4, "SIMDSP") {
			[0] = "No Advanced SIMD single precision",
			[1] = "Advanced SIMD single precision"
		}
	},
	{
		FIELDINFO(20, 4, "SIMDHP") {
			[0] = "No Advanced SIMD half precision",
			[1] = "Advanced SIMD half precision conversion",
			[2] = "Advanced SIMD half precision conversion"
			    " and arithmetic"
		}
	},
	{
		FIELDINFO(24, 4, "FPHP") {
			[0] = "No half precision conversion",
			[1] = "half/single precision conversion",
			[2] = "half/single/double precision conversion",
			[3] = "half/single/double precision conversion, and "
			    "half precision arithmetic"
		}
	},
	{
		FIELDINFO(28, 4, "SIMDFMAC") {
			[0] = "No Fused Multiply-Accumulate",
			[1] = "Fused Multiply-Accumulate"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* MVFR2_EL1 - Media and VFP Feature Register 2 */
struct fieldinfo mvfr2_fieldinfo[] = {
	{
		FIELDINFO(0, 4, "SIMDMisc") {
			[0] = "No miscellaneous features",
			[1] = "Conversion to Integer w/Directed Rounding modes",
			[2] = "Conversion to Integer w/Directed Rounding modes"
			    ", Round to Integral floating point",
			[3] = "Conversion to Integer w/Directed Rounding modes"
			    ", Round to Integral floating point"
			    ", MaxNum and MinNum"
		}
	},
	{
		FIELDINFO(4, 4, "FPMisc") {
			[0] = "No miscellaneous features",
			[1] = "Floating point selection",
			[2] = "Floating point selection"
			    ", Conversion to Integer w/Directed Rounding modes",
			[3] = "Floating point selection"
			    ", Conversion to Integer w/Directed Rounding modes"
			    ", Round to Integral floating point",
			[4] = "Floating point selection"
			    ", Conversion to Integer w/Directed Rounding modes"
			    ", Round to Integral floating point"
			    ", MaxNum and MinNum"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* CLIDR_EL1 - Cache Level ID Register */
const char * const clidr_cachetype[8] = { /* 8=3bit */
	[0] = "None",
	[1] = "Instruction cache",
	[2] = "Data cache",
	[3] = "Instruction and Data cache",
	[4] = "Unified cache"
};

struct fieldinfo clidr_fieldinfo[] = {
	{
		FIELDNAME(0, 3, "L1"),
		.info = clidr_cachetype
	},
	{
		FIELDNAME(3, 3, "L2"),
		.info = clidr_cachetype
	},
	{
		FIELDNAME(6, 3, "L3"),
		.info = clidr_cachetype
	},
	{
		FIELDNAME(9, 3, "L4"),
		.info = clidr_cachetype
	},
	{
		FIELDNAME(12, 3, "L5"),
		.info = clidr_cachetype
	},
	{
		FIELDNAME(15, 3, "L6"),
		.info = clidr_cachetype
	},
	{
		FIELDNAME(18, 3, "L7"),
		.info = clidr_cachetype
	},
	{
		FIELDNAME(21, 3, "LoUU"),
		.flags = FIELDINFO_FLAGS_DEC
	},
	{
		FIELDNAME(24, 3, "LoC"),
		.flags = FIELDINFO_FLAGS_DEC
	},
	{
		FIELDNAME(27, 3, "LoUIS"),
		.flags = FIELDINFO_FLAGS_DEC
	},
	{
		FIELDNAME(30, 3, "ICB"),
		.flags = FIELDINFO_FLAGS_DEC
	},
	{ .bitwidth = 0 }	/* end of table */
};

struct fieldinfo ctr_fieldinfo[] = {
	{
		FIELDNAME(0, 4, "IminLine"),
		.flags = FIELDINFO_FLAGS_DEC | FIELDINFO_FLAGS_4LOG2
	},
	{
		FIELDNAME(16, 4, "DminLine"),
		.flags = FIELDINFO_FLAGS_DEC | FIELDINFO_FLAGS_4LOG2
	},
	{
		FIELDINFO(14, 2, "L1 Icache policy") {
			[0] = "VMID aware PIPT (VPIPT)",
			[1] = "ASID-tagged VIVT (AIVIVT)",
			[2] = "VIPT",
			[3] = "PIPT"
		},
	},
	{
		FIELDNAME(20, 4, "ERG"),
		.flags = FIELDINFO_FLAGS_DEC | FIELDINFO_FLAGS_4LOG2
	},
	{
		FIELDNAME(24, 4, "CWG"),
		.flags = FIELDINFO_FLAGS_DEC | FIELDINFO_FLAGS_4LOG2
	},
	{
		FIELDNAME(28, 1, "DIC"),
		.flags = FIELDINFO_FLAGS_DEC
	},
	{
		FIELDNAME(29, 1, "IDC"),
		.flags = FIELDINFO_FLAGS_DEC
	},
	{ .bitwidth = 0 }	/* end of table */
};


static void
print_fieldinfo(const char *cpuname, const char *setname,
    struct fieldinfo *fieldinfo, uint64_t data)
{
	uint64_t v;
	const char *info;
	int i, flags;

#define WIDTHMASK(w)	(0xffffffffffffffffULL >> (64 - (w)))

	for (i = 0; fieldinfo[i].bitwidth != 0; i++) {
		v = (data >> fieldinfo[i].bitpos) &
		    WIDTHMASK(fieldinfo[i].bitwidth);

		flags = fieldinfo[i].flags;
		info = NULL;
		if (fieldinfo[i].info != NULL)
			info = fieldinfo[i].info[v];

		printf("%s: %s: %s: ",
		    cpuname, setname, fieldinfo[i].name);

		if (verbose)
			printf("0x%"PRIx64": ", v);

		if (info == NULL) {
			if (flags & FIELDINFO_FLAGS_4LOG2)
				v = 4 * (1 << v);
			if (flags & FIELDINFO_FLAGS_DEC)
				printf("%"PRIu64"\n", v);
			else
				printf("0x%"PRIx64"\n", v);
		} else {
			printf("%s\n", info);
		}
	}
}

/* MIDR_EL1 - Main ID Register */
static void
identify_midr(const char *cpuname, uint32_t cpuid)
{
	unsigned int i;
	uint32_t implid, cpupart, variant, revision;
	const char *implementer = NULL;
	static char implbuf[128];

	implid = cpuid & CPU_ID_IMPLEMENTOR_MASK;
	cpupart = cpuid & CPU_PARTMASK;
	variant = __SHIFTOUT(cpuid, CPU_ID_VARIANT_MASK);
	revision = __SHIFTOUT(cpuid, CPU_ID_REVISION_MASK);

	for (i = 0; i < __arraycount(implids); i++) {
		if (implid == implids[i].impl_id) {
			implementer = implids[i].impl_name;
		}
	}
	if (implementer == NULL) {
		snprintf(implbuf, sizeof(implbuf), "unknown implementer: 0x%02x",
		    implid >> 24);
		implementer = implbuf;
	}

	for (i = 0; i < __arraycount(cpuids); i++) {
		if (cpupart == cpuids[i].cpu_partnum) {
			printf("%s: %s, %s r%dp%d (%s %s core)\n",
			    cpuname, implementer,
			    cpuids[i].cpu_name, variant, revision,
			    cpuids[i].cpu_class,
			    cpuids[i].cpu_architecture);
			return;
		}
	}
	printf("%s: unknown CPU ID: 0x%08x\n", cpuname, cpuid);
}

/* REVIDR_EL1 - Revision ID Register */
static void
identify_revidr(const char *cpuname, uint32_t revidr)
{
	printf("%s: revision: 0x%08x\n", cpuname, revidr);
}

/* MPIDR_EL1 - Multiprocessor Affinity Register */
static void
identify_mpidr(const char *cpuname, uint32_t mpidr)
{
	const char *setname = "multiprocessor affinity";

	printf("%s: %s: Affinity-Level: %"PRIu64"-%"PRIu64"-%"PRIu64"-%"PRIu64"\n",
	    cpuname, setname,
	    __SHIFTOUT(mpidr, MPIDR_AFF3),
	    __SHIFTOUT(mpidr, MPIDR_AFF2),
	    __SHIFTOUT(mpidr, MPIDR_AFF1),
	    __SHIFTOUT(mpidr, MPIDR_AFF0));

	if ((mpidr & MPIDR_U) == 0)
		printf("%s: %s: Multiprocessor system\n", cpuname, setname);
	else
		printf("%s: %s: Uniprocessor system\n", cpuname, setname);

	if ((mpidr & MPIDR_MT) == 0)
		printf("%s: %s: Core Independent\n", cpuname, setname);
	else
		printf("%s: %s: Multi-Threading\n", cpuname, setname);

}

/* AA64DFR0 - Debug feature register 0 */
static void
identify_dfr0(const char *cpuname, uint64_t dfr0)
{
	const char *setname = "debug feature 0";

	printf("%s: %s: CTX_CMPs: %lu context-aware breakpoints\n",
	    cpuname, setname, __SHIFTOUT(dfr0, ID_AA64DFR0_EL1_CTX_CMPS) + 1);
	printf("%s: %s: WRPs: %lu watchpoints\n",
	    cpuname, setname, __SHIFTOUT(dfr0, ID_AA64DFR0_EL1_WRPS) + 1);
	printf("%s: %s: BRPs: %lu breakpoints\n",
	    cpuname, setname, __SHIFTOUT(dfr0, ID_AA64DFR0_EL1_BRPS) + 1);
	print_fieldinfo(cpuname, setname,
	    id_aa64dfr0_fieldinfo, dfr0);
}

void
identifycpu(int fd, const char *cpuname)
{
	char path[128];
	size_t len;
#define SYSCTL_CPU_ID_MAXSIZE	64
	uint64_t sysctlbuf[SYSCTL_CPU_ID_MAXSIZE];
	struct aarch64_sysctl_cpu_id *id =
	    (struct aarch64_sysctl_cpu_id *)sysctlbuf;

	snprintf(path, sizeof path, "machdep.%s.cpu_id", cpuname);
	len = sizeof(sysctlbuf);
	memset(sysctlbuf, 0, len);
	if (sysctlbyname(path, id, &len, 0, 0) == -1)
		err(1, "couldn't get %s", path);
	if (len != sizeof(struct aarch64_sysctl_cpu_id))
		fprintf(stderr, "Warning: kernel version bumped?\n");

	if (verbose) {
		printf("%s: MIDR_EL1: 0x%08"PRIx64"\n",
		    cpuname, id->ac_midr);
		printf("%s: MPIDR_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_mpidr);
		printf("%s: ID_AA64DFR0_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_aa64dfr0);
		printf("%s: ID_AA64DFR1_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_aa64dfr1);
		printf("%s: ID_AA64ISAR0_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_aa64isar0);
		printf("%s: ID_AA64ISAR1_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_aa64isar1);
		printf("%s: ID_AA64MMFR0_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_aa64mmfr0);
		printf("%s: ID_AA64MMFR1_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_aa64mmfr1);
		printf("%s: ID_AA64MMFR2_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_aa64mmfr2);
		printf("%s: ID_AA64PFR0_EL1: 0x%08"PRIx64"\n",
		    cpuname, id->ac_aa64pfr0);
		printf("%s: ID_AA64PFR1_EL1: 0x%08"PRIx64"\n",
		    cpuname, id->ac_aa64pfr1);
		printf("%s: ID_AA64ZFR0_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_aa64zfr0);
		printf("%s: MVFR0_EL1: 0x%08"PRIx32"\n",
		    cpuname, id->ac_mvfr0);
		printf("%s: MVFR1_EL1: 0x%08"PRIx32"\n",
		    cpuname, id->ac_mvfr1);
		printf("%s: MVFR2_EL1: 0x%08"PRIx32"\n",
		    cpuname, id->ac_mvfr2);
		printf("%s: CLIDR_EL1: 0x%016"PRIx64"\n",
		    cpuname, id->ac_clidr);
		printf("%s: CTR_EL0: 0x%016"PRIx64"\n",
		    cpuname, id->ac_ctr);
	}

	identify_midr(cpuname, id->ac_midr);
	identify_revidr(cpuname, id->ac_revidr);
	identify_mpidr(cpuname, id->ac_mpidr);
	print_fieldinfo(cpuname, "isa features 0",
	    id_aa64isar0_fieldinfo, id->ac_aa64isar0);
	print_fieldinfo(cpuname, "isa features 1",
	    id_aa64isar1_fieldinfo, id->ac_aa64isar1);
	print_fieldinfo(cpuname, "memory model 0",
	    id_aa64mmfr0_fieldinfo, id->ac_aa64mmfr0);
	print_fieldinfo(cpuname, "memory model 1",
	    id_aa64mmfr1_fieldinfo, id->ac_aa64mmfr1);
	print_fieldinfo(cpuname, "processor feature 0",
	    id_aa64pfr0_fieldinfo, id->ac_aa64pfr0);
	print_fieldinfo(cpuname, "processor feature 1",
	    id_aa64pfr1_fieldinfo, id->ac_aa64pfr1);
	identify_dfr0(cpuname, id->ac_aa64dfr0);

	print_fieldinfo(cpuname, "media and VFP features 0",
	    mvfr0_fieldinfo, id->ac_mvfr0);
	print_fieldinfo(cpuname, "media and VFP features 1",
	    mvfr1_fieldinfo, id->ac_mvfr1);
	print_fieldinfo(cpuname, "media and VFP features 2",
	    mvfr2_fieldinfo, id->ac_mvfr2);

	if (len <= offsetof(struct aarch64_sysctl_cpu_id, ac_clidr))
		return;
	print_fieldinfo(cpuname, "cache level",
	    clidr_fieldinfo, id->ac_clidr);
	print_fieldinfo(cpuname, "cache type",
	    ctr_fieldinfo, id->ac_ctr);
}

bool
identifycpu_bind(void)
{
	return false;
}

int
ucodeupdate_check(int fd, struct cpu_ucode *uc)
{
	return 0;
}
