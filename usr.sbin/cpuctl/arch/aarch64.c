/*	$NetBSD: aarch64.c,v 1.2.2.2 2018/05/21 04:36:19 pgoyette Exp $	*/

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
__RCSID("$NetBSD: aarch64.c,v 1.2.2.2 2018/05/21 04:36:19 pgoyette Exp $");
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
	int bitpos;
	int bitwidth;
	const char *name;
	const char * const *info;
};


#define CPU_PARTMASK	(CPU_ID_IMPLEMENTOR_MASK | CPU_ID_PARTNO_MASK)
const struct cpuidtab cpuids[] = {
	{ CPU_ID_CORTEXA53R0 & CPU_PARTMASK, "Cortex-A53", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA57R0 & CPU_PARTMASK, "Cortex-A57", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA72R0 & CPU_PARTMASK, "Cortex-A72", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA73R0 & CPU_PARTMASK, "Cortex-A73", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA55R1 & CPU_PARTMASK, "Cortex-A55", "Cortex", "V8.2-A" },
	{ CPU_ID_CORTEXA75R2 & CPU_PARTMASK, "Cortex-A75", "Cortex", "V8.2-A" }
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

/* ID_AA64PFR0_EL1 - AArch64 Processor Feature Register 0 */
struct fieldinfo id_aa64pfr0_fieldinfo[] = {
	{
		.bitpos = 0, .bitwidth = 4, .name = "EL0",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No EL0",
			[1] = "AArch64",
			[2] = "AArch64/AArch32"
		}
	},
	{
		.bitpos = 4, .bitwidth = 4, .name = "EL1",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No EL1",
			[1] = "AArch64",
			[2] = "AArch64/AArch32"
		}
	},
	{
		.bitpos = 8, .bitwidth = 4, .name = "EL2",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No EL2",
			[1] = "AArch64",
			[2] = "AArch64/AArch32"
		}
	},
	{
		.bitpos = 12, .bitwidth = 4, .name = "EL3",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No EL3",
			[1] = "AArch64",
			[2] = "AArch64/AArch32"
		}
	},
	{
		.bitpos = 16, .bitwidth = 4, .name = "FP",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "Floating Point",
			[15] = "No Floating Point"
		}
	},
	{
		.bitpos = 20, .bitwidth = 4, .name = "AdvSIMD",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "Advanced SIMD",
			[15] = "No Advanced SIMD"
		}
	},
	{
		.bitpos = 24, .bitwidth = 4, .name = "GIC",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No GIC",
			[1] = "GICv3"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* ID_AA64ISAR0_EL1 - AArch64 Instruction Set Attribute Register 0 */
struct fieldinfo id_aa64isar0_fieldinfo[] = {
	{
		.bitpos = 4, .bitwidth = 4, .name = "AES",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No AES",
			[1] = "AESE/AESD/AESMC/AESIMC",
			[2] = "AESE/AESD/AESMC/AESIMC+PMULL/PMULL2"
		}
	},
	{
		.bitpos = 8, .bitwidth = 4, .name = "SHA1",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No SHA1",
			[1] = "SHA1C/SHA1P/SHA1M/SHA1H/SHA1SU0/SHA1SU1"
		}
	},
	{
		.bitpos = 12, .bitwidth = 4, .name = "SHA2",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No SHA2",
			[1] = "SHA256H/SHA256H2/SHA256SU0/SHA256U1"
		}
	},
	{
		.bitpos = 16, .bitwidth = 4, .name = "CRC32",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No CRC32",
			[1] = "CRC32B/CRC32H/CRC32W/CRC32X"
			    "/CRC32CB/CRC32CH/CRC32CW/CRC32CX"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* ID_AA64MMFR0_EL1 - AArch64 Memory Model Feature Register 0 */
struct fieldinfo id_aa64mmfr0_fieldinfo[] = {
	{
		.bitpos = 0, .bitwidth = 4, .name = "PARange",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "32bits/4GB",
			[1] = "36bits/64GB",
			[2] = "40bits/1TB",
			[3] = "42bits/4TB",
			[4] = "44bits/16TB",
			[5] = "48bits/256TB"
		}
	},
	{
		.bitpos = 4, .bitwidth = 4, .name = "ASIDBit",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "8bits",
			[2] = "16bits"
		}
	},
	{
		.bitpos = 8, .bitwidth = 4, .name = "BigEnd",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No mixed-endian",
			[1] = "Mixed-endian"
		}
	},
	{
		.bitpos = 12, .bitwidth = 4, .name = "SNSMem",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No distinction B/W Secure and Non-secure Memory",
			[1] = "Distinction B/W Secure and Non-secure Memory"
		}
	},
	{
		.bitpos = 16, .bitwidth = 4, .name = "BigEndEL0",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No mixed-endian at EL0",
			[1] = "Mixed-endian at EL0"
		}
	},
	{
		.bitpos = 20, .bitwidth = 4, .name = "TGran16",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No 16KB granule",
			[1] = "16KB granule"
		}
	},
	{
		.bitpos = 24, .bitwidth = 4, .name = "TGran64",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "64KB granule",
			[15] = "No 64KB granule"
		}
	},
	{
		.bitpos = 28, .bitwidth = 4, .name = "TGran4",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "4KB granule",
			[15] = "No 4KB granule"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* MVFR0_EL1 - Media and VFP Feature Register 0 */
struct fieldinfo mvfr0_fieldinfo[] = {
	{
		.bitpos = 0, .bitwidth = 4, .name = "SIMDreg",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No SIMD",
			[1] = "16x64-bit SIMD",
			[2] = "32x64-bit SIMD"
		}
	},
	{
		.bitpos = 4, .bitwidth = 4, .name = "FPSP",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No VFP support single precision",
			[1] = "VFPv2 support single precision",
			[2] = "VFPv2/VFPv3/VFPv4 support single precision"
		}
	},
	{
		.bitpos = 8, .bitwidth = 4, .name = "FPDP",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No VFP support double precision",
			[1] = "VFPv2 support double precision",
			[2] = "VFPv2/VFPv3/VFPv4 support double precision"
		}
	},
	{
		.bitpos = 12, .bitwidth = 4, .name = "FPTrap",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "VFPv2 support exception trapping",
			[1] = "VFPv2/VFPv3/VFPv4 support exception trapping"
		}
	},
	{
		.bitpos = 16, .bitwidth = 4, .name = "FPDivide",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "VDIV not supported",
			[1] = "VDIV supported"
		}
	},
	{
		.bitpos = 20, .bitwidth = 4, .name = "FPSqrt",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "VSQRT not supported",
			[1] = "VSQRT supported"
		}
	},
	{
		.bitpos = 24, .bitwidth = 4, .name = "FPShVec",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "Short Vectors not supported",
			[1] = "Short Vectors supported"
		}
	},
	{
		.bitpos = 28, .bitwidth = 4, .name = "FPRound",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "Only Round to Nearest mode",
			[1] = "All rounding modes"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* MVFR1_EL1 - Media and VFP Feature Register 1 */
struct fieldinfo mvfr1_fieldinfo[] = {
	{
		.bitpos = 0, .bitwidth = 4, .name = "FPFtZ",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "only the Flush-to-Zero",
			[1] = "full Denormalized number arithmetic"
		}
	},
	{
		.bitpos = 4, .bitwidth = 4, .name = "FPDNan",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "Default NaN",
			[1] = "Propagation of NaN"
		}
	},
	{
		.bitpos = 8, .bitwidth = 4, .name = "SIMDLS",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No Advanced SIMD Load/Store",
			[1] = "Advanced SIMD Load/Store"
		}
	},
	{
		.bitpos = 12, .bitwidth = 4, .name = "SIMDInt",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No Advanced SIMD Integer",
			[1] = "Advanced SIMD Integer"
		}
	},
	{
		.bitpos = 16, .bitwidth = 4, .name = "SIMDSP",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No Advanced SIMD single precision",
			[1] = "Advanced SIMD single precision"
		}
	},
	{
		.bitpos = 20, .bitwidth = 4, .name = "SIMDHP",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No Advanced SIMD half precision",
			[1] = "Advanced SIMD half precision"
		}
	},
	{
		.bitpos = 24, .bitwidth = 4, .name = "FPHP",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No half precision conversion",
			[1] = "half/single precision conversion",
			[2] = "half/single/double precision conversion"
		}
	},
	{
		.bitpos = 28, .bitwidth = 4, .name = "SIMDFMAC",
		.info = (const char *[16]) { /* 16=4bit */
			[0] = "No Fused Multiply-Accumulate",
			[1] = "Fused Multiply-Accumulate"
		}
	},
	{ .bitwidth = 0 }	/* end of table */
};

/* MVFR2_EL1 - Media and VFP Feature Register 2 */
struct fieldinfo mvfr2_fieldinfo[] = {
	{
		.bitpos = 0, .bitwidth = 4, .name = "SIMDMisc",
		.info = (const char *[16]) { /* 16=4bit */
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
		.bitpos = 4, .bitwidth = 4, .name = "FPMisc",
		.info = (const char *[16]) { /* 16=4bit */
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

static void
print_fieldinfo(const char *cpuname, const char *setname,
    struct fieldinfo *fieldinfo, uint64_t data)
{
	uint64_t v;
	const char *info;
	int i;

#define WIDTHMASK(w)	(0xffffffffffffffffULL >> (64 - (w)))

	for (i = 0; fieldinfo[i].bitwidth != 0; i++) {
		v = (data >> fieldinfo[i].bitpos) &
		    WIDTHMASK(fieldinfo[i].bitwidth);

		info = fieldinfo[i].info[v];
		if (info == NULL)
			printf("%s: %s: %s: 0x%"PRIx64"\n",
			    cpuname, setname, fieldinfo[i].name, v);
		else
			printf("%s: %s: %s: %s\n",
			    cpuname, setname, fieldinfo[i].name, info);
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

static void *
sysctlfetch(const char *sname, size_t *lenp)
{
	size_t len;
	void *data;

	if (sysctlbyname(sname, NULL, &len, NULL, 0) != 0) {
		warn("sysctlbyname: %s", sname);
		return NULL;
	}

	data = malloc(len);
	if (data == NULL) {
		warn("malloc");
		return NULL;
	}

	if (sysctlbyname(sname, data, &len, NULL, 0) != 0) {
		warn("sysctlbyname: %s", sname);
		free(data);
		return NULL;
	}

	*lenp = len;
	return data;
}

void
identifycpu(int fd, const char *cpuname)
{
	void *regs;
	size_t len;

	/* MIDR_EL1 */
	regs = sysctlfetch("machdep.cpu_id", &len);
	if (regs != NULL) {
		if (len >= sizeof(uint32_t))
			identify_midr(cpuname, ((uint32_t *)regs)[0]);
		free(regs);
	}

	/* REVIDR_EL1 */
	regs = sysctlfetch("machdep.id_revidr", &len);
	if (regs != NULL) {
		if (len >= sizeof(uint32_t))
			identify_revidr(cpuname, ((uint32_t *)regs)[0]);
		free(regs);
	}

	/* MPIDR_EL1 */
	regs = sysctlfetch("machdep.id_mpidr", &len);
	if (regs != NULL) {
		if (len >= sizeof(uint64_t))
			identify_mpidr(cpuname, ((uint64_t *)regs)[0]);
		free(regs);
	}

	/* ID_AA64ISAR0_EL1 */
	regs = sysctlfetch("machdep.id_aa64isar", &len);
	if (regs != NULL) {
		if (len >= sizeof(uint64_t))
			print_fieldinfo(cpuname, "isa features 0",
			    id_aa64isar0_fieldinfo, ((uint64_t *)regs)[0]);
		free(regs);
	}

	/* ID_AA64MMFR0_EL1 */
	regs = sysctlfetch("machdep.id_aa64mmfr", &len);
	if (regs != NULL) {
		if (len >= sizeof(uint64_t))
			print_fieldinfo(cpuname, "memory model 0",
			    id_aa64mmfr0_fieldinfo, ((uint64_t *)regs)[0]);
		free(regs);
	}

	/* ID_AA64PFR0_EL1 */
	regs = sysctlfetch("machdep.id_aa64pfr", &len);
	if (regs != NULL) {
		if (len >= sizeof(uint64_t))
			print_fieldinfo(cpuname, "processor feature 0",
			    id_aa64pfr0_fieldinfo, ((uint64_t *)regs)[0]);
		free(regs);
	}

	/* MVFR[012]_EL1 */
	regs = sysctlfetch("machdep.id_mvfr", &len);
	if (regs != NULL) {
		if (len >= sizeof(uint32_t))
			print_fieldinfo(cpuname, "media and VFP features 0",
			    mvfr0_fieldinfo, ((uint32_t *)regs)[0]);
		if (len >= sizeof(uint32_t) * 2)
			print_fieldinfo(cpuname, "media and VFP features 1",
			    mvfr1_fieldinfo, ((uint32_t *)regs)[1]);
		if (len >= sizeof(uint32_t) * 3)
			print_fieldinfo(cpuname, "media and VFP features 2",
			    mvfr2_fieldinfo, ((uint32_t *)regs)[2]);
		free(regs);
	}
}

bool
identifycpu_bind(void)
{
	return true;
}

int
ucodeupdate_check(int fd, struct cpu_ucode *uc)
{
	return 0;
}
