/*	$NetBSD: specialreg.h,v 1.98.2.11 2019/02/12 09:27:17 martin Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)specialreg.h	7.1 (Berkeley) 5/9/91
 */

/*
 * Bits in 386 special registers:
 */
#define CR0_PE	0x00000001	/* Protected mode Enable */
#define CR0_MP	0x00000002	/* "Math" Present (NPX or NPX emulator) */
#define CR0_EM	0x00000004	/* EMulate non-NPX coproc. (trap ESC only) */
#define CR0_TS	0x00000008	/* Task Switched (if MP, trap ESC and WAIT) */
#define CR0_ET	0x00000010	/* Extension Type (387 (if set) vs 287) */
#define CR0_PG	0x80000000	/* PaGing enable */

/*
 * Bits in 486 special registers:
 */
#define CR0_NE	0x00000020	/* Numeric Error enable (EX16 vs IRQ13) */
#define CR0_WP	0x00010000	/* Write Protect (honor PG_RW in all modes) */
#define CR0_AM	0x00040000	/* Alignment Mask (set to enable AC flag) */
#define CR0_NW	0x20000000	/* Not Write-through */
#define CR0_CD	0x40000000	/* Cache Disable */

/*
 * Cyrix 486 DLC special registers, accessible as IO ports.
 */
#define CCR0	0xc0		/* configuration control register 0 */
#define CCR0_NC0	0x01	/* first 64K of each 1M memory region is non-cacheable */
#define CCR0_NC1	0x02	/* 640K-1M region is non-cacheable */
#define CCR0_A20M	0x04	/* enables A20M# input pin */
#define CCR0_KEN	0x08	/* enables KEN# input pin */
#define CCR0_FLUSH	0x10	/* enables FLUSH# input pin */
#define CCR0_BARB	0x20	/* flushes internal cache when entering hold state */
#define CCR0_CO		0x40	/* cache org: 1=direct mapped, 0=2x set assoc */
#define CCR0_SUSPEND	0x80	/* enables SUSP# and SUSPA# pins */

#define CCR1	0xc1		/* configuration control register 1 */
#define CCR1_RPL	0x01	/* enables RPLSET and RPLVAL# pins */
/* the remaining 7 bits of this register are reserved */

/*
 * bits in the %cr4 control register:
 */
#define CR4_VME		0x00000001 /* virtual 8086 mode extension enable */
#define CR4_PVI		0x00000002 /* protected mode virtual interrupt enable */
#define CR4_TSD		0x00000004 /* restrict RDTSC instruction to cpl 0 */
#define CR4_DE		0x00000008 /* debugging extension */
#define CR4_PSE		0x00000010 /* large (4MB) page size enable */
#define CR4_PAE		0x00000020 /* physical address extension enable */
#define CR4_MCE		0x00000040 /* machine check enable */
#define CR4_PGE		0x00000080 /* page global enable */
#define CR4_PCE		0x00000100 /* enable RDPMC instruction for all cpls */
#define CR4_OSFXSR	0x00000200 /* enable fxsave/fxrestor and SSE */
#define CR4_OSXMMEXCPT	0x00000400 /* enable unmasked SSE exceptions */
#define CR4_UMIP	0x00000800 /* user-mode instruction prevention */
#define CR4_VMXE	0x00002000 /* enable VMX operations */
#define CR4_SMXE	0x00004000 /* enable SMX operations */
#define CR4_FSGSBASE	0x00010000 /* enable *FSBASE and *GSBASE instructions */
#define CR4_PCIDE	0x00020000 /* enable Process Context IDentifiers */
#define CR4_OSXSAVE	0x00040000 /* enable xsave and xrestore */
#define CR4_SMEP	0x00100000 /* enable SMEP support */
#define CR4_SMAP	0x00200000 /* enable SMAP support */
#define CR4_PKE		0x00400000 /* protection key enable */

/*
 * Extended Control Register XCR0
 */
#define XCR0_X87	0x00000001	/* x87 FPU/MMX state */
#define XCR0_SSE	0x00000002	/* SSE state */
#define XCR0_YMM_Hi128	0x00000004	/* AVX-256 (ymmn registers) */
#define XCR0_BNDREGS	0x00000008	/* Memory protection ext bounds */
#define XCR0_BNDCSR	0x00000010	/* Memory protection ext state */
#define XCR0_Opmask	0x00000020	/* AVX-512 Opmask */
#define XCR0_ZMM_Hi256	0x00000040	/* AVX-512 upper 256 bits low regs */
#define XCR0_Hi16_ZMM	0x00000080	/* AVX-512 512 bits upper registers */

/*
 * Known fpu bits - only these get enabled. The save area is sized for all the
 * fields below (max 2680 bytes).
 */
#define XCR0_FPU	(XCR0_X87 | XCR0_SSE | XCR0_YMM_Hi128 | \
			XCR0_Opmask | XCR0_ZMM_Hi256 | XCR0_Hi16_ZMM)

#define XCR0_BND	(XCR0_BNDREGS | XCR0_BNDCSR)

#define XCR0_FLAGS1	"\20" \
	"\1" "x87"	"\2" "SSE"	"\3" "AVX" \
	"\4" "BNDREGS"	"\5" "BNDCSR" \
	"\6" "Opmask"	"\7" "ZMM_Hi256" "\10" "Hi16_ZMM"


/*
 * CPUID "features" bits
 */

/* Fn00000001 %edx features */
#define CPUID_FPU	0x00000001	/* processor has an FPU? */
#define CPUID_VME	0x00000002	/* has virtual mode (%cr4's VME/PVI) */
#define CPUID_DE	0x00000004	/* has debugging extension */
#define CPUID_PSE	0x00000008	/* has 4MB page size extension */
#define CPUID_TSC	0x00000010	/* has time stamp counter */
#define CPUID_MSR	0x00000020	/* has model specific registers */
#define CPUID_PAE	0x00000040	/* has phys address extension */
#define CPUID_MCE	0x00000080	/* has machine check exception */
#define CPUID_CX8	0x00000100	/* has CMPXCHG8B instruction */
#define CPUID_APIC	0x00000200	/* has enabled APIC */
#define CPUID_B10	0x00000400	/* reserved, MTRR */
#define CPUID_SEP	0x00000800	/* has SYSENTER/SYSEXIT extension */
#define CPUID_MTRR	0x00001000	/* has memory type range register */
#define CPUID_PGE	0x00002000	/* has page global extension */
#define CPUID_MCA	0x00004000	/* has machine check architecture */
#define CPUID_CMOV	0x00008000	/* has CMOVcc instruction */
#define CPUID_PAT	0x00010000	/* Page Attribute Table */
#define CPUID_PSE36	0x00020000	/* 36-bit PSE */
#define CPUID_PN	0x00040000	/* processor serial number */
#define CPUID_CFLUSH	0x00080000	/* CLFLUSH insn supported */
#define CPUID_B20	0x00100000	/* reserved */
#define CPUID_DS	0x00200000	/* Debug Store */
#define CPUID_ACPI	0x00400000	/* ACPI performance modulation regs */
#define CPUID_MMX	0x00800000	/* MMX supported */
#define CPUID_FXSR	0x01000000	/* fast FP/MMX save/restore */
#define CPUID_SSE	0x02000000	/* streaming SIMD extensions */
#define CPUID_SSE2	0x04000000	/* streaming SIMD extensions #2 */
#define CPUID_SS	0x08000000	/* self-snoop */
#define CPUID_HTT	0x10000000	/* Hyper-Threading Technology */
#define CPUID_TM	0x20000000	/* thermal monitor (TCC) */
#define CPUID_IA64	0x40000000	/* IA-64 architecture */
#define CPUID_SBF	0x80000000	/* signal break on FERR */

#define CPUID_FLAGS1	"\20" \
	"\1" "FPU"	"\2" "VME"	"\3" "DE"	"\4" "PSE" \
	"\5" "TSC"	"\6" "MSR"	"\7" "PAE"	"\10" "MCE" \
	"\11" "CX8"	"\12" "APIC"	"\13" "B10"	"\14" "SEP" \
	"\15" "MTRR"	"\16" "PGE"	"\17" "MCA"	"\20" "CMOV" \
	"\21" "PAT"	"\22" "PSE36"	"\23" "PN"	"\24" "CLFLUSH" \
	"\25" "B20"	"\26" "DS"	"\27" "ACPI"	"\30" "MMX" \
	"\31" "FXSR"	"\32" "SSE"	"\33" "SSE2"	"\34" "SS" \
	"\35" "HTT"	"\36" "TM"	"\37" "IA64"	"\40" "SBF"

/* Blacklists of CPUID flags - used to mask certain features */
#ifdef XEN
/* Not on Xen */
#define CPUID_FEAT_BLACKLIST	 (CPUID_PGE|CPUID_PSE|CPUID_MTRR)
#else
#define CPUID_FEAT_BLACKLIST	 0
#endif /* XEN */

/*
 * CPUID "features" bits in Fn00000001 %ecx
 */

#define CPUID2_SSE3	0x00000001	/* Streaming SIMD Extensions 3 */
#define CPUID2_PCLMUL	0x00000002	/* PCLMULQDQ instructions */
#define CPUID2_DTES64	0x00000004	/* 64-bit Debug Trace */
#define CPUID2_MONITOR	0x00000008	/* MONITOR/MWAIT instructions */
#define CPUID2_DS_CPL	0x00000010	/* CPL Qualified Debug Store */
#define CPUID2_VMX	0x00000020	/* Virtual Machine Extensions */
#define CPUID2_SMX	0x00000040	/* Safer Mode Extensions */
#define CPUID2_EST	0x00000080	/* Enhanced SpeedStep Technology */
#define CPUID2_TM2	0x00000100	/* Thermal Monitor 2 */
#define CPUID2_SSSE3	0x00000200	/* Supplemental SSE3 */
#define CPUID2_CID	0x00000400	/* Context ID */
#define CPUID2_SDBG	0x00000800	/* Silicon Debug */
#define CPUID2_FMA	0x00001000	/* has Fused Multiply Add */
#define CPUID2_CX16	0x00002000	/* has CMPXCHG16B instruction */
#define CPUID2_xTPR	0x00004000	/* Task Priority Messages disabled? */
#define CPUID2_PDCM	0x00008000	/* Perf/Debug Capability MSR */
/* bit 16 unused	0x00010000 */
#define CPUID2_PCID	0x00020000	/* Process Context ID */
#define CPUID2_DCA	0x00040000	/* Direct Cache Access */
#define CPUID2_SSE41	0x00080000	/* Streaming SIMD Extensions 4.1 */
#define CPUID2_SSE42	0x00100000	/* Streaming SIMD Extensions 4.2 */
#define CPUID2_X2APIC	0x00200000	/* xAPIC Extensions */
#define CPUID2_MOVBE	0x00400000	/* MOVBE (move after byteswap) */
#define CPUID2_POPCNT	0x00800000	/* popcount instruction available */
#define CPUID2_DEADLINE	0x01000000	/* APIC Timer supports TSC Deadline */
#define CPUID2_AES	0x02000000	/* AES instructions */
#define CPUID2_XSAVE	0x04000000	/* XSAVE instructions */
#define CPUID2_OSXSAVE	0x08000000	/* XGETBV/XSETBV instructions */
#define CPUID2_AVX	0x10000000	/* AVX instructions */
#define CPUID2_F16C	0x20000000	/* half precision conversion */
#define CPUID2_RDRAND	0x40000000	/* RDRAND (hardware random number) */
#define CPUID2_RAZ	0x80000000	/* RAZ. Indicates guest state. */

#define CPUID2_FLAGS1	"\20" \
	"\1" "SSE3"	"\2" "PCLMULQDQ" "\3" "DTES64"	"\4" "MONITOR" \
	"\5" "DS-CPL"	"\6" "VMX"	"\7" "SMX"	"\10" "EST" \
	"\11" "TM2"	"\12" "SSSE3"	"\13" "CID"	"\14" "SDBG" \
	"\15" "FMA"	"\16" "CX16"	"\17" "xTPR"	"\20" "PDCM" \
	"\21" "B16"	"\22" "PCID"	"\23" "DCA"	"\24" "SSE41" \
	"\25" "SSE42"	"\26" "X2APIC"	"\27" "MOVBE"	"\30" "POPCNT" \
	"\31" "DEADLINE" "\32" "AES"	"\33" "XSAVE"	"\34" "OSXSAVE" \
	"\35" "AVX"	"\36" "F16C"	"\37" "RDRAND"	"\40" "RAZ"

/* CPUID Fn00000001 %eax */

#define CPUID_TO_BASEFAMILY(cpuid)	(((cpuid) >> 8) & 0xf)
#define CPUID_TO_BASEMODEL(cpuid)	(((cpuid) >> 4) & 0xf)
#define CPUID_TO_STEPPING(cpuid)	((cpuid) & 0xf)

/*
 * The Extended family bits should only be inspected when CPUID_TO_BASEFAMILY()
 * returns 15. They are use to encode family value 16 to 270 (add 15).
 * The Extended model bits are the high 4 bits of the model.
 * They are only valid for family >= 15 or family 6 (intel, but all amd
 * family 6 are documented to return zero bits for them).
 */
#define CPUID_TO_EXTFAMILY(cpuid)	(((cpuid) >> 20) & 0xff)
#define CPUID_TO_EXTMODEL(cpuid)	(((cpuid) >> 16) & 0xf)

/* The macros for the Display Family and the Display Model */
#define CPUID_TO_FAMILY(cpuid)	(CPUID_TO_BASEFAMILY(cpuid)	\
	    + ((CPUID_TO_BASEFAMILY(cpuid) != 0x0f)		\
		? 0 : CPUID_TO_EXTFAMILY(cpuid)))
#define CPUID_TO_MODEL(cpuid)	(CPUID_TO_BASEMODEL(cpuid)	\
	    | ((CPUID_TO_BASEFAMILY(cpuid) != 0x0f)		\
		&& (CPUID_TO_BASEFAMILY(cpuid) != 0x06)		\
		? 0 : (CPUID_TO_EXTMODEL(cpuid) << 4)))

/* CPUID Fn00000001 %ebx */
#define	CPUID_BRAND_INDEX	__BITS(7,0)
#define	CPUID_CLFLUSH_SIZE	__BITS(15,8)
#define	CPUID_HTT_CORES		__BITS(23,16)
#define	CPUID_LOCAL_APIC_ID	__BITS(31,24)

/*
 * Intel Deterministic Cache Parameter Leaf
 * Fn0000_0004
 */

/* %eax */
#define CPUID_DCP_CACHETYPE	__BITS(4, 0)	/* Cache type */
#define CPUID_DCP_CACHETYPE_N	0		/*   NULL */
#define CPUID_DCP_CACHETYPE_D	1		/*   Data cache */
#define CPUID_DCP_CACHETYPE_I	2		/*   Instruction cache */
#define CPUID_DCP_CACHETYPE_U	3		/*   Unified cache */
#define CPUID_DCP_CACHELEVEL	__BITS(7, 5)	/* Cache level (start at 1) */
#define CPUID_DCP_SELFINITCL	__BIT(8)	/* Self initializing cachelvl*/
#define CPUID_DCP_FULLASSOC	__BIT(9)	/* Full associative */
#define CPUID_DCP_SHAREING	__BITS(25, 14)	/* shareing */
#define CPUID_DCP_CORE_P_PKG	__BITS(31, 26)	/* Cores/package */

/* %ebx */
#define CPUID_DCP_LINESIZE	__BITS(11, 0)	/* System coherency linesize */
#define CPUID_DCP_PARTITIONS	__BITS(21, 12)	/* Physical line partitions */
#define CPUID_DCP_WAYS		__BITS(31, 22)	/* Ways of associativity */

/* Number of sets: %ecx */

/* %edx */
#define CPUID_DCP_INVALIDATE	__BIT(0)	/* WB invalidate/invalidate */
#define CPUID_DCP_INCLUSIVE	__BIT(1)	/* Cache inclusiveness */
#define CPUID_DCP_COMPLEX	__BIT(2)	/* Complex cache indexing */

/*
 * Intel/AMD MONITOR/MWAIT
 * Fn0000_0005
 */
/* %eax */
#define CPUID_MON_MINSIZE	__BITS(15, 0)  /* Smallest monitor-line size */
/* %ebx */
#define CPUID_MON_MAXSIZE	__BITS(15, 0)  /* Largest monitor-line size */
/* %ecx */
#define CPUID_MON_EMX		__BIT(0)       /* MONITOR/MWAIT Extensions */
#define CPUID_MON_IBE		__BIT(1)       /* Interrupt as Break Event */

#define CPUID_MON_FLAGS	"\20" \
	"\1" "EMX"	"\2" "IBE"

/* %edx: number of substates for specific C-state */
#define CPUID_MON_SUBSTATE(edx, cstate) (((edx) >> (cstate * 4)) & 0x0000000f)

/*
 * Intel/AMD Digital Thermal Sensor and
 * Power Management, Fn0000_0006 - %eax.
 */
#define CPUID_DSPM_DTS	__BIT(0)	/* Digital Thermal Sensor */
#define CPUID_DSPM_IDA	__BIT(1)	/* Intel Dynamic Acceleration */
#define CPUID_DSPM_ARAT	__BIT(2)	/* Always Running APIC Timer */
#define CPUID_DSPM_PLN	__BIT(4)	/* Power Limit Notification */
#define CPUID_DSPM_ECMD	__BIT(5)	/* Clock Modulation Extension */
#define CPUID_DSPM_PTM	__BIT(6)	/* Package Level Thermal Management */
#define CPUID_DSPM_HWP	__BIT(7)	/* HWP */
#define CPUID_DSPM_HWP_NOTIFY __BIT(8)	/* HWP Notification */
#define CPUID_DSPM_HWP_ACTWIN  __BIT(9)	/* HWP Activity Window */
#define CPUID_DSPM_HWP_EPP __BIT(10)	/* HWP Energy Performance Preference */
#define CPUID_DSPM_HWP_PLR __BIT(11)	/* HWP Package Level Request */
#define CPUID_DSPM_HDC	__BIT(13)	/* Hardware Duty Cycling */
#define CPUID_DSPM_TBMT3 __BIT(14)	/* Turbo Boost Max Technology 3.0 */
#define CPUID_DSPM_HWP_CAP    __BIT(15)	/* HWP Capabilities */
#define CPUID_DSPM_HWP_PECI   __BIT(16)	/* HWP PECI override */
#define CPUID_DSPM_HWP_FLEX   __BIT(17)	/* Flexible HWP */
#define CPUID_DSPM_HWP_FAST   __BIT(18)	/* Fast access for IA32_HWP_REQUEST */
#define CPUID_DSPM_HWP_IGNIDL __BIT(20)	/* Ignore Idle Logical Processor HWP */

#define CPUID_DSPM_FLAGS	"\20" \
	"\1" "DTS"	"\2" "IDA"	"\3" "ARAT" 			\
	"\5" "PLN"	"\6" "ECMD"	"\7" "PTM"	"\10" "HWP"	\
	"\11" "HWP_NOTIFY" "\12" "HWP_ACTWIN" "\13" "HWP_EPP" "\14" "HWP_PLR" \
			"\16" "HDC"	"\17" "TBM3"	"\20" "HWP_CAP" \
	"\21" "HWP_PECI" "\22" "HWP_FLEX" "\23" "HWP_FAST"		\
	"25" "HWP_IGNIDL"

/*
 * Intel/AMD Digital Thermal Sensor and
 * Power Management, Fn0000_0006 - %ecx.
 */
#define CPUID_DSPM_HWF	0x00000001	/* MSR_APERF/MSR_MPERF available */
#define CPUID_DSPM_EPB	0x00000008	/* Energy Performance Bias */

#define CPUID_DSPM_FLAGS1	"\20" "\1" "HWF" "\4" "EPB"

/*
 * Intel/AMD Structured Extended Feature leaf Fn0000_0007
 * %eax == 0: Subleaf 0
 *	%eax: The Maximum input value for supported subleaf.
 *	%ebx: Feature bits.
 *	%ecx: Feature bits.
 *	%edx: Feature bits.
 */

/* %ebx */
#define CPUID_SEF_FSGSBASE	__BIT(0)  /* {RD,WR}{FS,GS}BASE */
#define CPUID_SEF_TSC_ADJUST	__BIT(1)  /* IA32_TSC_ADJUST MSR support */
#define CPUID_SEF_SGX		__BIT(2)  /* Software Guard Extentions */
#define CPUID_SEF_BMI1		__BIT(3)  /* advanced bit manipulation ext. 1st grp */
#define CPUID_SEF_HLE		__BIT(4)  /* Hardware Lock Elision */
#define CPUID_SEF_AVX2		__BIT(5)  /* Advanced Vector Extensions 2 */
#define CPUID_SEF_FDPEXONLY	__BIT(6)  /* x87FPU Data ptr updated only on x87exp */
#define CPUID_SEF_SMEP		__BIT(7)  /* Supervisor-Mode Excecution Prevention */
#define CPUID_SEF_BMI2		__BIT(8)  /* advanced bit manipulation ext. 2nd grp */
#define CPUID_SEF_ERMS		__BIT(9)  /* Enhanced REP MOVSB/STOSB */
#define CPUID_SEF_INVPCID	__BIT(10) /* INVPCID instruction */
#define CPUID_SEF_RTM		__BIT(11) /* Restricted Transactional Memory */
#define CPUID_SEF_QM		__BIT(12) /* Resource Director Technology Monitoring */
#define CPUID_SEF_FPUCSDS	__BIT(13) /* Deprecate FPU CS and FPU DS values */
#define CPUID_SEF_MPX		__BIT(14) /* Memory Protection Extensions */
#define CPUID_SEF_PQE		__BIT(15) /* Resource Director Technology Allocation */
#define CPUID_SEF_AVX512F	__BIT(16) /* AVX-512 Foundation */
#define CPUID_SEF_AVX512DQ	__BIT(17) /* AVX-512 Double/Quadword */
#define CPUID_SEF_RDSEED	__BIT(18) /* RDSEED instruction */
#define CPUID_SEF_ADX		__BIT(19) /* ADCX/ADOX instructions */
#define CPUID_SEF_SMAP		__BIT(20) /* Supervisor-Mode Access Prevention */
#define CPUID_SEF_AVX512_IFMA	__BIT(21) /* AVX-512 Integer Fused Multiply Add */
/* Bit 22 was PCOMMIT */
#define CPUID_SEF_CLFLUSHOPT	__BIT(23) /* Cache Line FLUSH OPTimized */
#define CPUID_SEF_CLWB		__BIT(24) /* Cache Line Write Back */
#define CPUID_SEF_PT		__BIT(25) /* Processor Trace */
#define CPUID_SEF_AVX512PF	__BIT(26) /* AVX-512 PreFetch */
#define CPUID_SEF_AVX512ER	__BIT(27) /* AVX-512 Exponential and Reciprocal */
#define CPUID_SEF_AVX512CD	__BIT(28) /* AVX-512 Conflict Detection */
#define CPUID_SEF_SHA		__BIT(29) /* SHA Extensions */
#define CPUID_SEF_AVX512BW	__BIT(30) /* AVX-512 Byte and Word */
#define CPUID_SEF_AVX512VL	__BIT(31) /* AVX-512 Vector Length */

#define CPUID_SEF_FLAGS	"\20" \
	"\1" "FSGSBASE"	"\2" "TSCADJUST" "\3" "SGX"	"\4" "BMI1"	\
	"\5" "HLE"	"\6" "AVX2"	"\7" "FDPEXONLY" "\10" "SMEP"	\
	"\11" "BMI2"	"\12" "ERMS"	"\13" "INVPCID"	"\14" "RTM"	\
	"\15" "QM"	"\16" "FPUCSDS"	"\17" "MPX"    	"\20" "PQE"	\
	"\21" "AVX512F"	"\22" "AVX512DQ" "\23" "RDSEED"	"\24" "ADX"	\
	"\25" "SMAP"	"\26" "AVX512_IFMA"		"\30" "CLFLUSHOPT" \
	"\31" "CLWB"	"\32" "PT"	"\33" "AVX512PF" "\34" "AVX512ER" \
	"\35" "AVX512CD""\36" "SHA"	"\37" "AVX512BW" "\40" "AVX512VL"

/* %ecx */
#define CPUID_SEF_PREFETCHWT1	__BIT(0)  /* PREFETCHWT1 instruction */
#define CPUID_SEF_AVX512_VBMI	__BIT(1)  /* AVX-512 Vector Byte Manipulation */
#define CPUID_SEF_UMIP		__BIT(2)  /* User-Mode Instruction prevention */
#define CPUID_SEF_PKU		__BIT(3)  /* Protection Keys for User-mode pages */
#define CPUID_SEF_OSPKE		__BIT(4)  /* OS has set CR4.PKE to ena. protec. keys */
#define CPUID_SEF_WAITPKG	__BIT(5)  /* TPAUSE,UMONITOR,UMWAIT */
#define CPUID_SEF_AVX512_VBMI2	__BIT(6)  /* AVX-512 Vector Byte Manipulation 2 */
#define CPUID_SEF_GFNI		__BIT(8)
#define CPUID_SEF_VAES		__BIT(9)
#define CPUID_SEF_VPCLMULQDQ	__BIT(10)
#define CPUID_SEF_AVX512_VNNI	__BIT(11) /* Vector neural Network Instruction */
#define CPUID_SEF_AVX512_BITALG	__BIT(12)
#define CPUID_SEF_AVX512_VPOPCNTDQ __BIT(14)
#define CPUID_SEF_MAWAU		__BITS(21, 17) /* MAWAU for BND{LD,ST}X */
#define CPUID_SEF_RDPID		__BIT(22) /* RDPID and IA32_TSC_AUX */
#define CPUID_SEF_CLDEMOTE	__BIT(25) /* Cache line demote */
#define CPUID_SEF_MOVDIRI	__BIT(27) /* MOVDIRI instruction */
#define CPUID_SEF_MOVDIR64B	__BIT(28) /* MOVDIR64B instruction */
#define CPUID_SEF_SGXLC		__BIT(30) /* SGX Launch Configuration */

#define CPUID_SEF_FLAGS1	"\177\20" \
	"b\0PREFETCHWT1\0" "b\1AVX512_VBMI\0" "b\2UMIP\0" "b\3PKU\0"	\
	"b\4OSPKE\0"	"b\5WAITPKG\0"	"b\6AVX512_VBMI2\0"		      \
	"b\10GFNI\0"	"b\11VAES\0"	"b\12VPCLMULQDQ\0" "b\13AVX512_VNNI\0"\
	"b\14AVX512_BITALG\0"		"b\16AVX512_VPOPCNTDQ\0"	\
	"f\21\5MAWAU\0"							\
					"b\26RDPID\0"			\
			"b\31CLDEMOTE\0"		"b\33MOVDIRI\0"	\
	"b\34MOVDIR64B\0"		"b\36SGXLC\0"

/* %edx */
#define CPUID_SEF_AVX512_4VNNIW	__BIT(2)
#define CPUID_SEF_AVX512_4FMAPS	__BIT(3)
#define CPUID_SEF_IBRS		__BIT(26) /* IBRS / IBPB Speculation Control */
#define CPUID_SEF_STIBP		__BIT(27) /* STIBP Speculation Control */
#define CPUID_SEF_L1D_FLUSH	__BIT(28) /* IA32_FLUSH_CMD MSR */
#define CPUID_SEF_ARCH_CAP	__BIT(29) /* IA32_ARCH_CAPABILITIES */
#define CPUID_SEF_CORE_CAP	__BIT(30) /* IA32_CORE_CAPABILITIES */
#define CPUID_SEF_SSBD		__BIT(31) /* Speculative Store Bypass Disable */

#define CPUID_SEF_FLAGS2	"\20" \
				"\3" "AVX512_4VNNIW" "\4" "AVX512_4FMAPS" \
					"\33" "IBRS"	"\34" "STIBP"	\
	"\35" "L1D_FLUSH" "\36" "ARCH_CAP" "\37CORE_CAP"	"\40" "SSBD"

/*
 * Intel CPUID Architectural Performance Monitoring Fn0000000a
 *
 * See also src/usr.sbin/tprof/arch/tprof_x86.c
 */

/* %eax */
#define CPUID_PERF_VERSION	__BITS(7, 0)   /* Version ID */
#define CPUID_PERF_NGPPC	__BITS(15, 8)  /* Num of G.P. perf counter */
#define CPUID_PERF_NBWGPPC	__BITS(23, 16) /* Bit width of G.P. perfcnt */
#define CPUID_PERF_BVECLEN	__BITS(31, 24) /* Length of EBX bit vector */

#define CPUID_PERF_FLAGS0	"\177\20"	\
	"f\0\10VERSION\0" "f\10\10GPCounter\0"	\
	"f\20\10GPBitwidth\0" "f\30\10Vectorlen\0"

/* %ebx */
#define CPUID_PERF_CORECYCL	__BIT(0)       /* No core cycle */
#define CPUID_PERF_INSTRETRY	__BIT(1)       /* No instruction retried */
#define CPUID_PERF_REFCYCL	__BIT(2)       /* No reference cycles */
#define CPUID_PERF_LLCREF	__BIT(3)       /* No LLCache reference */
#define CPUID_PERF_LLCMISS	__BIT(4)       /* No LLCache miss */
#define CPUID_PERF_BRINSRETR	__BIT(5)       /* No branch inst. retried */
#define CPUID_PERF_BRMISPRRETR	__BIT(6)       /* No branch mispredict retry */

#define CPUID_PERF_FLAGS1	"\177\20"				      \
	"b\0CORECYCL\0" "b\1INSTRETRY\0" "b\2REFCYCL\0" "b\3LLCREF\0" \
	"b\4LLCMISS\0" "b\5BRINSRETR\0" "b\6BRMISPRRETR\0"

/* %edx */
#define CPUID_PERF_NFFPC	__BITS(4, 0)   /* Num of fixed-funct perfcnt */
#define CPUID_PERF_NBWFFPC	__BITS(12, 5)  /* Bit width of fixed-func pc */
#define CPUID_PERF_ANYTHREADDEPR __BIT(15)      /* Any Thread deprecation */

#define CPUID_PERF_FLAGS3	"\177\20"				\
	"f\0\5FixedFunc\0" "f\5\10FFBitwidth\0" "b\17ANYTHREADDEPR\0"

/*
 * Intel CPUID Extended Topology Enumeration Fn0000000b
 * %ecx == level number
 *	%eax: See below.
 *	%ebx: Number of logical processors at this level.
 *	%ecx: See below.
 *	%edx: x2APIC ID of the current logical processor.
 */
/* %eax */
#define CPUID_TOP_SHIFTNUM	__BITS(4, 0) /* Topology ID shift value */
/* %ecx */
#define CPUID_TOP_LVLNUM	__BITS(7, 0) /* Level number */
#define CPUID_TOP_LVLTYPE	__BITS(15, 8) /* Level type */
#define CPUID_TOP_LVLTYPE_INVAL	0	 	/* Invalid */
#define CPUID_TOP_LVLTYPE_SMT	1	 	/* SMT */
#define CPUID_TOP_LVLTYPE_CORE	2	 	/* Core */

/*
 * Intel/AMD CPUID Processor extended state Enumeration Fn0000000d
 *
 * %ecx == 0: supported features info:
 *	%eax: Valid bits of lower 32bits of XCR0
 *	%ebx: Maximum save area size for features enabled in XCR0
 *	%ecx: Maximum save area size for all cpu features
 *	%edx: Valid bits of upper 32bits of XCR0
 *
 * %ecx == 1:
 *	%eax: Bit 0 => xsaveopt instruction available (sandy bridge onwards)
 *	%ebx: Save area size for features enabled by XCR0 | IA32_XSS
 *	%ecx: Valid bits of lower 32bits of IA32_XSS
 *	%edx: Valid bits of upper 32bits of IA32_XSS
 *
 * %ecx >= 2: Save area details for XCR0 bit n
 *	%eax: size of save area for this feature
 *	%ebx: offset of save area for this feature
 *	%ecx, %edx: reserved
 *	All of %eax, %ebx, %ecx and %edx are zero for unsupported features.
 */

/* %ecx=1 %eax */
#define CPUID_PES1_XSAVEOPT	0x00000001	/* xsaveopt instruction */
#define CPUID_PES1_XSAVEC	0x00000002	/* xsavec & compacted XRSTOR */
#define CPUID_PES1_XGETBV	0x00000004	/* xgetbv with ECX = 1 */
#define CPUID_PES1_XSAVES	0x00000008	/* xsaves/xrstors, IA32_XSS */

#define CPUID_PES1_FLAGS	"\20" \
	"\1" "XSAVEOPT"	"\2" "XSAVEC"	"\3" "XGETBV"	"\4" "XSAVES"

/*
 * Intel Deterministic Address Translation Parameter Leaf
 * Fn0000_0018
 */

/* %ecx=0 %eax __BITS(31, 0): the maximum input value of supported sub-leaf */

/* %ebx */
#define CPUID_DATP_PGSIZE	__BITS(3, 0)	/* page size */
#define CPUID_DATP_PGSIZE_4KB	__BIT(0)	/* 4KB page support */
#define CPUID_DATP_PGSIZE_2MB	__BIT(1)	/* 2MB page support */
#define CPUID_DATP_PGSIZE_4MB	__BIT(2)	/* 4MB page support */
#define CPUID_DATP_PGSIZE_1GB	__BIT(3)	/* 1GB page support */
#define CPUID_DATP_PARTITIONING	__BITS(10, 8)	/* Partitioning */
#define CPUID_DATP_WAYS		__BITS(31, 16)	/* Ways of associativity */

/* Number of sets: %ecx */

/* %edx */
#define CPUID_DATP_TCTYPE	__BITS(4, 0)	/* Translation Cache type */
#define CPUID_DATP_TCTYPE_N	0		/*   NULL (not valid) */
#define CPUID_DATP_TCTYPE_D	1		/*   Data TLB */
#define CPUID_DATP_TCTYPE_I	2		/*   Instruction TLB */
#define CPUID_DATP_TCTYPE_U	3		/*   Unified TLB */
#define CPUID_DATP_TCLEVEL	__BITS(7, 5)	/* TLB level (start at 1) */
#define CPUID_DATP_FULLASSOC	__BIT(8)	/* Full associative */
#define CPUID_DATP_SHAREING	__BITS(25, 14)	/* shareing */


/* Intel Fn80000001 extended features - %edx */
#define CPUID_SYSCALL	0x00000800	/* SYSCALL/SYSRET */
#define CPUID_XD	0x00100000	/* Execute Disable (like CPUID_NOX) */
#define CPUID_P1GB	0x04000000	/* 1GB Large Page Support */
#define CPUID_RDTSCP	0x08000000	/* Read TSC Pair Instruction */
#define CPUID_EM64T	0x20000000	/* Intel EM64T */

#define CPUID_INTEL_EXT_FLAGS	"\20" \
	"\14" "SYSCALL/SYSRET"	"\25" "XD"	"\33" "P1GB" \
	"\34" "RDTSCP"	"\36" "EM64T"

/* Intel Fn80000001 extended features - %ecx */
#define CPUID_LAHF	0x00000001	/* LAHF/SAHF in IA-32e mode, 64bit sub*/
		/*	0x00000020 */	/* LZCNT. Same as AMD's CPUID_LZCNT */
#define CPUID_PREFETCHW	0x00000100	/* PREFETCHW */

#define CPUID_INTEL_FLAGS4	"\20"				\
	"\1" "LAHF"	"\02" "B01"	"\03" "B02"		\
			"\06" "LZCNT"				\
	"\11" "PREFETCHW"


/* AMD/VIA Fn80000001 extended features - %edx */
/*	CPUID_SYSCALL			   SYSCALL/SYSRET */
#define CPUID_MPC	0x00080000	/* Multiprocessing Capable */
#define CPUID_NOX	0x00100000	/* No Execute Page Protection */
#define CPUID_MMXX	0x00400000	/* AMD MMX Extensions */
/*	CPUID_MMX			   MMX supported */
/*	CPUID_FXSR			   fast FP/MMX save/restore */
#define CPUID_FFXSR	0x02000000	/* FXSAVE/FXSTOR Extensions */
/*	CPUID_P1GB			   1GB Large Page Support */
/*	CPUID_RDTSCP			   Read TSC Pair Instruction */
/*	CPUID_EM64T			   Long mode */
#define CPUID_3DNOW2	0x40000000	/* 3DNow! Instruction Extension */
#define CPUID_3DNOW	0x80000000	/* 3DNow! Instructions */

#define CPUID_EXT_FLAGS	"\20" \
						"\14" "SYSCALL/SYSRET"	\
							"\24" "MPC"	\
	"\25" "NOX"			"\27" "MMXX"	"\30" "MMX"	\
	"\31" "FXSR"	"\32" "FFXSR"	"\33" "P1GB"	"\34" "RDTSCP"	\
			"\36" "LONG"	"\37" "3DNOW2"	"\40" "3DNOW"

/* AMD Fn80000001 extended features - %ecx */
/* 	CPUID_LAHF			   LAHF/SAHF instruction */
#define CPUID_CMPLEGACY	0x00000002	/* Compare Legacy */
#define CPUID_SVM	0x00000004	/* Secure Virtual Machine */
#define CPUID_EAPIC	0x00000008	/* Extended APIC space */
#define CPUID_ALTMOVCR0	0x00000010	/* Lock Mov Cr0 */
#define CPUID_LZCNT	0x00000020	/* LZCNT instruction */
#define CPUID_SSE4A	0x00000040	/* SSE4A instruction set */
#define CPUID_MISALIGNSSE 0x00000080	/* Misaligned SSE */
#define CPUID_3DNOWPF	0x00000100	/* 3DNow Prefetch */
#define CPUID_OSVW	0x00000200	/* OS visible workarounds */
#define CPUID_IBS	0x00000400	/* Instruction Based Sampling */
#define CPUID_XOP	0x00000800	/* XOP instruction set */
#define CPUID_SKINIT	0x00001000	/* SKINIT */
#define CPUID_WDT	0x00002000	/* watchdog timer support */
#define CPUID_LWP	0x00008000	/* Light Weight Profiling */
#define CPUID_FMA4	0x00010000	/* FMA4 instructions */
#define CPUID_TCE	0x00020000	/* Translation cache Extension */
#define CPUID_NODEID	0x00080000	/* NodeID MSR available*/
#define CPUID_TBM	0x00200000	/* TBM instructions */
#define CPUID_TOPOEXT	0x00400000	/* cpuid Topology Extension */
#define CPUID_PCEC	0x00800000	/* Perf Ctr Ext Core */
#define CPUID_PCENB	0x01000000	/* Perf Ctr Ext NB */
#define CPUID_SPM	0x02000000	/* Stream Perf Mon */
#define CPUID_DBE	0x04000000	/* Data Breakpoint Extension */
#define CPUID_PTSC	0x08000000	/* PerfTsc */
#define CPUID_L2IPERFC	0x10000000	/* L2I performance counter Extension */
#define CPUID_MWAITX	0x20000000	/* MWAITX/MONITORX support */

#define CPUID_AMD_FLAGS4	"\20" \
	"\1" "LAHF"	"\2" "CMPLEGACY" "\3" "SVM"	"\4" "EAPIC" \
	"\5" "ALTMOVCR0" "\6" "LZCNT"	"\7" "SSE4A"	"\10" "MISALIGNSSE" \
	"\11" "3DNOWPREFETCH" \
			"\12" "OSVW"	"\13" "IBS"	"\14" "XOP" \
	"\15" "SKINIT"	"\16" "WDT"	"\17" "B14"	"\20" "LWP" \
	"\21" "FMA4"	"\22" "TCE"	"\23" "B18"	"\24" "NodeID" \
	"\25" "B20"	"\26" "TBM"	"\27" "TopoExt"	"\30" "PCExtC" \
	"\31" "PCExtNB"	"\32" "StrmPM"	"\33" "DBExt"	"\34" "PerfTsc" \
	"\35" "L2IPERFC" "\36" "MWAITX"	"\37" "B30"	"\40" "B31"

/*
 * AMD Advanced Power Management
 * CPUID Fn8000_0007 %edx
 */
#define CPUID_APM_TS	0x00000001	/* Temperature Sensor */
#define CPUID_APM_FID	0x00000002	/* Frequency ID control */
#define CPUID_APM_VID	0x00000004	/* Voltage ID control */
#define CPUID_APM_TTP	0x00000008	/* THERMTRIP (PCI F3xE4 register) */
#define CPUID_APM_HTC	0x00000010	/* Hardware thermal control (HTC) */
#define CPUID_APM_STC	0x00000020	/* Software thermal control (STC) */
#define CPUID_APM_100	0x00000040	/* 100MHz multiplier control */
#define CPUID_APM_HWP	0x00000080	/* HW P-State control */
#define CPUID_APM_TSC	0x00000100	/* TSC invariant */
#define CPUID_APM_CPB	0x00000200	/* Core performance boost */
#define CPUID_APM_EFF	0x00000400	/* Effective Frequency (read-only) */

#define CPUID_APM_FLAGS		"\20" \
	"\1" "TS"	"\2" "FID"	"\3" "VID"	"\4" "TTP" \
	"\5" "HTC"	"\6" "STC"	"\7" "100"	"\10" "HWP" \
	"\11" "TSC"	"\12" "CPB"	"\13" "EffFreq"	"\14" "B11" \
	"\15" "B12"

/* AMD Fn8000000a %edx features (SVM features) */
#define CPUID_AMD_SVM_NP		0x00000001
#define CPUID_AMD_SVM_LbrVirt		0x00000002
#define CPUID_AMD_SVM_SVML		0x00000004
#define CPUID_AMD_SVM_NRIPS		0x00000008
#define CPUID_AMD_SVM_TSCRateCtrl	0x00000010
#define CPUID_AMD_SVM_VMCBCleanBits	0x00000020
#define CPUID_AMD_SVM_FlushByASID	0x00000040
#define CPUID_AMD_SVM_DecodeAssist	0x00000080
#define CPUID_AMD_SVM_PauseFilter	0x00000400
#define CPUID_AMD_SVM_PFThreshold	0x0x001000 /* PAUSE filter threshold */
#define CPUID_AMD_SVM_AVIC		0x00002000 /* AMD Virtual intr. ctrl */
#define CPUID_AMD_SVM_V_VMSAVE_VMLOAD	0x00008000 /* Virtual VM{SAVE/LOAD} */
#define CPUID_AMD_SVM_vGIF		0x00010000 /* Virtualized GIF */
#define CPUID_AMD_SVM_FLAGS	 "\20" \
	"\1" "NP"	"\2" "LbrVirt"	"\3" "SVML"	"\4" "NRIPS"	\
	"\5" "TSCRate"	"\6" "VMCBCleanBits" 				\
			        "\7" "FlushByASID" "\10" "DecodeAssist"	\
	"\11" "B08"	"\12" "B09"	"\13" "PauseFilter" "\14" "B11" \
	"\15" "PFThreshold" "\16" "AVIC" "\17" "B14"			\
						"\20" "V_VMSAVE_VMLOAD"	\
	"\21" "VGIF"

/*
 * Centaur Extended Feature flags
 */
#define CPUID_VIA_HAS_RNG	0x00000004	/* Random number generator */
#define CPUID_VIA_DO_RNG	0x00000008
#define CPUID_VIA_HAS_ACE	0x00000040	/* AES Encryption */
#define CPUID_VIA_DO_ACE	0x00000080
#define CPUID_VIA_HAS_ACE2	0x00000100	/* AES+CTR instructions */
#define CPUID_VIA_DO_ACE2	0x00000200
#define CPUID_VIA_HAS_PHE	0x00000400	/* SHA1+SHA256 HMAC */
#define CPUID_VIA_DO_PHE	0x00000800
#define CPUID_VIA_HAS_PMM	0x00001000	/* RSA Instructions */
#define CPUID_VIA_DO_PMM	0x00002000

#define CPUID_FLAGS_PADLOCK	"\20" \
	"\3" "RNG"	"\7" "AES"	"\11" "AES/CTR"	"\13" "SHA1/SHA256" \
	"\15" "RSA"

/*
 * Model-specific registers for the i386 family
 */
#define MSR_P5_MC_ADDR		0x000	/* P5 only */
#define MSR_P5_MC_TYPE		0x001	/* P5 only */
#define MSR_TSC			0x010
#define MSR_CESR		0x011	/* P5 only (trap on P6) */
#define MSR_CTR0		0x012	/* P5 only (trap on P6) */
#define MSR_CTR1		0x013	/* P5 only (trap on P6) */
#define MSR_IA32_PLATFORM_ID	0x017
#define MSR_APICBASE		0x01b
#define 	APICBASE_BSP		0x00000100	/* boot processor */
#define 	APICBASE_EXTD		0x00000400	/* x2APIC mode */
#define 	APICBASE_EN		0x00000800	/* software enable */
/*
 * APICBASE_PHYSADDR is actually variable-sized on some CPUs. But we're
 * only interested in the initial value, which is guaranteed to fit the
 * first 32 bits. So this macro is fine.
 */
#define 	APICBASE_PHYSADDR	0xfffff000	/* physical address */
#define MSR_EBL_CR_POWERON	0x02a
#define MSR_EBC_FREQUENCY_ID	0x02c	/* PIV only */
#define MSR_TEST_CTL		0x033
#define MSR_IA32_SPEC_CTRL	0x048
#define 	IA32_SPEC_CTRL_IBRS	0x01
#define 	IA32_SPEC_CTRL_STIBP	0x02
#define 	IA32_SPEC_CTRL_SSBD	0x04
#define MSR_IA32_PRED_CMD	0x049
#define 	IA32_PRED_CMD_IBPB	0x01
#define MSR_BIOS_UPDT_TRIG	0x079
#define MSR_BBL_CR_D0		0x088	/* PII+ only */
#define MSR_BBL_CR_D1		0x089	/* PII+ only */
#define MSR_BBL_CR_D2		0x08a	/* PII+ only */
#define MSR_BIOS_SIGN		0x08b
#define MSR_PERFCTR0		0x0c1
#define MSR_PERFCTR1		0x0c2
#define MSR_FSB_FREQ		0x0cd	/* Core Duo/Solo only */
#define MSR_MPERF		0x0e7
#define MSR_APERF		0x0e8
#define MSR_IA32_EXT_CONFIG	0x0ee	/* Undocumented. Core Solo/Duo only */
#define MSR_MTRRcap		0x0fe
#define MSR_IA32_ARCH_CAPABILITIES 0x10a
#define 	IA32_ARCH_RDCL_NO	0x01
#define 	IA32_ARCH_IBRS_ALL	0x02
#define 	IA32_ARCH_RSBA		0x04
#define 	IA32_ARCH_SKIP_L1DFL_VMENTRY 0x08
#define 	IA32_ARCH_SSB_NO	0x10
#define MSR_IA32_FLUSH_CMD 0x10b
#define 	IA32_FLUSH_CMD_L1D_FLUSH 0x01
#define MSR_BBL_CR_ADDR		0x116	/* PII+ only */
#define MSR_BBL_CR_DECC		0x118	/* PII+ only */
#define MSR_BBL_CR_CTL		0x119	/* PII+ only */
#define MSR_BBL_CR_TRIG		0x11a	/* PII+ only */
#define MSR_BBL_CR_BUSY		0x11b	/* PII+ only */
#define MSR_BBL_CR_CTR3		0x11e	/* PII+ only */
#define MSR_SYSENTER_CS		0x174	/* PII+ only */
#define MSR_SYSENTER_ESP	0x175	/* PII+ only */
#define MSR_SYSENTER_EIP	0x176	/* PII+ only */
#define MSR_MCG_CAP		0x179
#define MSR_MCG_STATUS		0x17a
#define MSR_MCG_CTL		0x17b
#define MSR_EVNTSEL0		0x186
#define MSR_EVNTSEL1		0x187
#define MSR_PERF_STATUS		0x198	/* Pentium M */
#define MSR_PERF_CTL		0x199	/* Pentium M */
#define MSR_THERM_CONTROL	0x19a
#define MSR_THERM_INTERRUPT	0x19b
#define MSR_THERM_STATUS	0x19c
#define MSR_THERM2_CTL		0x19d	/* Pentium M */
#define MSR_MISC_ENABLE		0x1a0
#define 	IA32_MISC_MWAIT_EN	0x40000
#define MSR_TEMPERATURE_TARGET	0x1a2
#define MSR_DEBUGCTLMSR		0x1d9
#define MSR_LASTBRANCHFROMIP	0x1db
#define MSR_LASTBRANCHTOIP	0x1dc
#define MSR_LASTINTFROMIP	0x1dd
#define MSR_LASTINTTOIP		0x1de
#define MSR_ROB_CR_BKUPTMPDR6	0x1e0
#define MSR_MTRRphysBase0	0x200
#define MSR_MTRRphysMask0	0x201
#define MSR_MTRRphysBase1	0x202
#define MSR_MTRRphysMask1	0x203
#define MSR_MTRRphysBase2	0x204
#define MSR_MTRRphysMask2	0x205
#define MSR_MTRRphysBase3	0x206
#define MSR_MTRRphysMask3	0x207
#define MSR_MTRRphysBase4	0x208
#define MSR_MTRRphysMask4	0x209
#define MSR_MTRRphysBase5	0x20a
#define MSR_MTRRphysMask5	0x20b
#define MSR_MTRRphysBase6	0x20c
#define MSR_MTRRphysMask6	0x20d
#define MSR_MTRRphysBase7	0x20e
#define MSR_MTRRphysMask7	0x20f
#define MSR_MTRRphysBase8	0x210
#define MSR_MTRRphysMask8	0x211
#define MSR_MTRRphysBase9	0x212
#define MSR_MTRRphysMask9	0x213
#define MSR_MTRRphysBase10	0x214
#define MSR_MTRRphysMask10	0x215
#define MSR_MTRRphysBase11	0x216
#define MSR_MTRRphysMask11	0x217
#define MSR_MTRRphysBase12	0x218
#define MSR_MTRRphysMask12	0x219
#define MSR_MTRRphysBase13	0x21a
#define MSR_MTRRphysMask13	0x21b
#define MSR_MTRRphysBase14	0x21c
#define MSR_MTRRphysMask14	0x21d
#define MSR_MTRRphysBase15	0x21e
#define MSR_MTRRphysMask15	0x21f
#define MSR_MTRRfix64K_00000	0x250
#define MSR_MTRRfix16K_80000	0x258
#define MSR_MTRRfix16K_A0000	0x259
#define MSR_MTRRfix4K_C0000	0x268
#define MSR_MTRRfix4K_C8000	0x269
#define MSR_MTRRfix4K_D0000	0x26a
#define MSR_MTRRfix4K_D8000	0x26b
#define MSR_MTRRfix4K_E0000	0x26c
#define MSR_MTRRfix4K_E8000	0x26d
#define MSR_MTRRfix4K_F0000	0x26e
#define MSR_MTRRfix4K_F8000	0x26f
#define MSR_CR_PAT		0x277
#define MSR_MTRRdefType		0x2ff
#define MSR_MC0_CTL		0x400
#define MSR_MC0_STATUS		0x401
#define MSR_MC0_ADDR		0x402
#define MSR_MC0_MISC		0x403
#define MSR_MC1_CTL		0x404
#define MSR_MC1_STATUS		0x405
#define MSR_MC1_ADDR		0x406
#define MSR_MC1_MISC		0x407
#define MSR_MC2_CTL		0x408
#define MSR_MC2_STATUS		0x409
#define MSR_MC2_ADDR		0x40a
#define MSR_MC2_MISC		0x40b
#define MSR_MC3_CTL		0x40c
#define MSR_MC3_STATUS		0x40d
#define MSR_MC3_ADDR		0x40e
#define MSR_MC3_MISC		0x40f
#define MSR_MC4_CTL		0x410
#define MSR_MC4_STATUS		0x411
#define MSR_MC4_ADDR		0x412
#define MSR_MC4_MISC		0x413
				/* 0x480 - 0x490 VMX */
#define MSR_X2APIC_BASE		0x800	/* 0x800 - 0xBFF */
#define  MSR_X2APIC_ID			0x002	/* x2APIC ID. (RO) */
#define  MSR_X2APIC_VERS		0x003	/* Version. (RO) */
#define  MSR_X2APIC_TPRI		0x008	/* Task Prio. (RW) */
#define  MSR_X2APIC_PPRI		0x00a	/* Processor prio. (RO) */
#define  MSR_X2APIC_EOI			0x00b	/* End Int. (W) */
#define  MSR_X2APIC_LDR			0x00d	/* Logical dest. (RO) */
#define  MSR_X2APIC_SVR			0x00f	/* Spurious intvec (RW) */
#define  MSR_X2APIC_ISR			0x010	/* In-Service Status (RO) */
#define  MSR_X2APIC_TMR			0x018	/* Trigger Mode (RO) */
#define  MSR_X2APIC_IRR			0x020	/* Interrupt Req (RO) */
#define  MSR_X2APIC_ESR			0x028	/* Err status. (RW) */
#define  MSR_X2APIC_LVT_CMCI		0x02f	/* LVT CMCI (RW) */
#define  MSR_X2APIC_ICRLO		0x030	/* Int. cmd. (RW64) */
#define  MSR_X2APIC_LVTT		0x032	/* Loc.vec.(timer) (RW) */
#define  MSR_X2APIC_TMINT		0x033	/* Loc.vec (Thermal) (RW) */
#define  MSR_X2APIC_PCINT		0x034	/* Loc.vec (Perf Mon) (RW) */
#define  MSR_X2APIC_LVINT0		0x035	/* Loc.vec (LINT0) (RW) */
#define  MSR_X2APIC_LVINT1		0x036	/* Loc.vec (LINT1) (RW) */
#define  MSR_X2APIC_LVERR		0x037	/* Loc.vec (ERROR) (RW) */
#define  MSR_X2APIC_ICR_TIMER		0x038	/* Initial count (RW) */
#define  MSR_X2APIC_CCR_TIMER		0x039	/* Current count (RO) */
#define  MSR_X2APIC_DCR_TIMER		0x03e	/* Divisor config (RW) */
#define  MSR_X2APIC_SELF_IPI		0x03f	/* SELF IPI (W) */

/*
 * VIA "Nehemiah" MSRs
 */
#define MSR_VIA_RNG		0x0000110b
#define MSR_VIA_RNG_ENABLE	0x00000040
#define MSR_VIA_RNG_NOISE_MASK	0x00000300
#define MSR_VIA_RNG_NOISE_A	0x00000000
#define MSR_VIA_RNG_NOISE_B	0x00000100
#define MSR_VIA_RNG_2NOISE	0x00000300
#define MSR_VIA_ACE		0x00001107
#define 	VIA_ACE_ALTINST	0x00000001
#define 	VIA_ACE_ECX8	0x00000002
#define 	VIA_ACE_ENABLE	0x10000000

/*
 * VIA "Eden" MSRs
 */
#define MSR_VIA_FCR		MSR_VIA_ACE

/*
 * AMD K6/K7 MSRs.
 */
#define MSR_K6_UWCCR		0xc0000085
#define MSR_K7_EVNTSEL0		0xc0010000
#define MSR_K7_EVNTSEL1		0xc0010001
#define MSR_K7_EVNTSEL2		0xc0010002
#define MSR_K7_EVNTSEL3		0xc0010003
#define MSR_K7_PERFCTR0		0xc0010004
#define MSR_K7_PERFCTR1		0xc0010005
#define MSR_K7_PERFCTR2		0xc0010006
#define MSR_K7_PERFCTR3		0xc0010007

/*
 * AMD K8 (Opteron) MSRs.
 */
#define MSR_SYSCFG	0xc0010010

#define MSR_EFER	0xc0000080		/* Extended feature enable */
#define 	EFER_SCE	0x00000001	/* SYSCALL extension */
#define 	EFER_LME	0x00000100	/* Long Mode Enable */
#define 	EFER_LMA	0x00000400	/* Long Mode Active */
#define 	EFER_NXE	0x00000800	/* No-Execute Enabled */
#define 	EFER_SVME	0x00001000	/* Secure Virtual Machine En. */
#define 	EFER_LMSLE	0x00002000	/* Long Mode Segment Limit E. */
#define 	EFER_FFXSR	0x00004000	/* Fast FXSAVE/FXRSTOR En. */
#define 	EFER_TCE	0x00008000	/* Translation Cache Ext. */

#define MSR_STAR	0xc0000081		/* 32 bit syscall gate addr */
#define MSR_LSTAR	0xc0000082		/* 64 bit syscall gate addr */
#define MSR_CSTAR	0xc0000083		/* compat syscall gate addr */
#define MSR_SFMASK	0xc0000084		/* flags to clear on syscall */

#define MSR_FSBASE	0xc0000100		/* 64bit offset for fs: */
#define MSR_GSBASE	0xc0000101		/* 64bit offset for gs: */
#define MSR_KERNELGSBASE 0xc0000102		/* storage for swapgs ins */

#define MSR_VMCR	0xc0010114	/* Virtual Machine Control Register */
#define 	VMCR_DPD	0x00000001	/* Debug port disable */
#define 	VMCR_RINIT	0x00000002	/* intercept init */
#define 	VMCR_DISA20	0x00000004	/* Disable A20 masking */
#define 	VMCR_LOCK	0x00000008	/* SVM Lock */
#define 	VMCR_SVMED	0x00000010	/* SVME Disable */
#define MSR_SVMLOCK	0xc0010118	/* SVM Lock key */

/*
 * These require a 'passcode' for access.  See cpufunc.h.
 */
#define MSR_HWCR	0xc0010015
#define 	HWCR_TLBCACHEDIS	0x00000008
#define 	HWCR_FFDIS		0x00000040

#define MSR_NB_CFG	0xc001001f
#define 	NB_CFG_DISIOREQLOCK	0x0000000000000008ULL
#define 	NB_CFG_DISDATMSK	0x0000001000000000ULL
#define 	NB_CFG_INITAPICCPUIDLO	(1ULL << 54)

#define MSR_LS_CFG	0xc0011020
#define 	LS_CFG_DIS_LS2_SQUISH	0x02000000
#define 	LS_CFG_DIS_SSB_F15H	0x0040000000000000ULL
#define 	LS_CFG_DIS_SSB_F16H	0x0000000200000000ULL
#define 	LS_CFG_DIS_SSB_F17H	0x0000000000000400ULL

#define MSR_IC_CFG	0xc0011021
#define 	IC_CFG_DIS_SEQ_PREFETCH	0x00000800
#define 	IC_CFG_DIS_IND		0x00004000

#define MSR_DC_CFG	0xc0011022
#define 	DC_CFG_DIS_CNV_WC_SSO	0x00000008
#define 	DC_CFG_DIS_SMC_CHK_BUF	0x00000400
#define 	DC_CFG_ERRATA_261	0x01000000

#define MSR_BU_CFG	0xc0011023
#define 	BU_CFG_ERRATA_298	0x0000000000000002ULL
#define 	BU_CFG_ERRATA_254	0x0000000000200000ULL
#define 	BU_CFG_ERRATA_309	0x0000000000800000ULL
#define 	BU_CFG_THRL2IDXCMPDIS	0x0000080000000000ULL
#define 	BU_CFG_WBPFSMCCHKDIS	0x0000200000000000ULL
#define 	BU_CFG_WBENHWSBDIS	0x0001000000000000ULL

#define MSR_DE_CFG	0xc0011029
#define 	DE_CFG_ERRATA_721	0x00000001

/* AMD Family10h MSRs */
#define MSR_OSVW_ID_LENGTH		0xc0010140
#define MSR_OSVW_STATUS			0xc0010141
#define MSR_UCODE_AMD_PATCHLEVEL	0x0000008b
#define MSR_UCODE_AMD_PATCHLOADER	0xc0010020

/* X86 MSRs */
#define MSR_RDTSCP_AUX			0xc0000103

/*
 * Constants related to MTRRs
 */
#define MTRR_N64K		8	/* numbers of fixed-size entries */
#define MTRR_N16K		16
#define MTRR_N4K		64

/*
 * the following four 3-byte registers control the non-cacheable regions.
 * These registers must be written as three separate bytes.
 *
 * NCRx+0: A31-A24 of starting address
 * NCRx+1: A23-A16 of starting address
 * NCRx+2: A15-A12 of starting address | NCR_SIZE_xx.
 *
 * The non-cacheable region's starting address must be aligned to the
 * size indicated by the NCR_SIZE_xx field.
 */
#define NCR1	0xc4
#define NCR2	0xc7
#define NCR3	0xca
#define NCR4	0xcd

#define NCR_SIZE_0K	0
#define NCR_SIZE_4K	1
#define NCR_SIZE_8K	2
#define NCR_SIZE_16K	3
#define NCR_SIZE_32K	4
#define NCR_SIZE_64K	5
#define NCR_SIZE_128K	6
#define NCR_SIZE_256K	7
#define NCR_SIZE_512K	8
#define NCR_SIZE_1M	9
#define NCR_SIZE_2M	10
#define NCR_SIZE_4M	11
#define NCR_SIZE_8M	12
#define NCR_SIZE_16M	13
#define NCR_SIZE_32M	14
#define NCR_SIZE_4G	15

/*
 * Performance monitor events.
 *
 * Note that 586-class and 686-class CPUs have different performance
 * monitors available, and they are accessed differently:
 *
 *	686-class: `rdpmc' instruction
 *	586-class: `rdmsr' instruction, CESR MSR
 *
 * The descriptions of these events are too lengthy to include here.
 * See Appendix A of "Intel Architecture Software Developer's
 * Manual, Volume 3: System Programming" for more information.
 */

/*
 * 586-class CESR MSR format.  Lower 16 bits is CTR0, upper 16 bits
 * is CTR1.
 */

#define PMC5_CESR_EVENT			0x003f
#define PMC5_CESR_OS			0x0040
#define PMC5_CESR_USR			0x0080
#define PMC5_CESR_E			0x0100
#define PMC5_CESR_P			0x0200

#define PMC5_DATA_READ			0x00
#define PMC5_DATA_WRITE			0x01
#define PMC5_DATA_TLB_MISS		0x02
#define PMC5_DATA_READ_MISS		0x03
#define PMC5_DATA_WRITE_MISS		0x04
#define PMC5_WRITE_M_E			0x05
#define PMC5_DATA_LINES_WBACK		0x06
#define PMC5_DATA_CACHE_SNOOP		0x07
#define PMC5_DATA_CACHE_SNOOP_HIT	0x08
#define PMC5_MEM_ACCESS_BOTH_PIPES	0x09
#define PMC5_BANK_CONFLICTS		0x0a
#define PMC5_MISALIGNED_DATA		0x0b
#define PMC5_INST_READ			0x0c
#define PMC5_INST_TLB_MISS		0x0d
#define PMC5_INST_CACHE_MISS		0x0e
#define PMC5_SEGMENT_REG_LOAD		0x0f
#define PMC5_BRANCHES			0x12
#define PMC5_BTB_HITS			0x13
#define PMC5_BRANCH_TAKEN		0x14
#define PMC5_PIPELINE_FLUSH		0x15
#define PMC5_INST_EXECUTED		0x16
#define PMC5_INST_EXECUTED_V_PIPE	0x17
#define PMC5_BUS_UTILIZATION		0x18
#define PMC5_WRITE_BACKUP_STALL		0x19
#define PMC5_DATA_READ_STALL		0x1a
#define PMC5_WRITE_E_M_STALL		0x1b
#define PMC5_LOCKED_BUS			0x1c
#define PMC5_IO_CYCLE			0x1d
#define PMC5_NONCACHE_MEM_READ		0x1e
#define PMC5_AGI_STALL			0x1f
#define PMC5_FLOPS			0x22
#define PMC5_BP0_MATCH			0x23
#define PMC5_BP1_MATCH			0x24
#define PMC5_BP2_MATCH			0x25
#define PMC5_BP3_MATCH			0x26
#define PMC5_HARDWARE_INTR		0x27
#define PMC5_DATA_RW			0x28
#define PMC5_DATA_RW_MISS		0x29

/*
 * 686-class Event Selector MSR format.
 */

#define PMC6_EVTSEL_EVENT		0x000000ff
#define PMC6_EVTSEL_UNIT		0x0000ff00
#define PMC6_EVTSEL_UNIT_SHIFT		8
#define PMC6_EVTSEL_USR			(1 << 16)
#define PMC6_EVTSEL_OS			(1 << 17)
#define PMC6_EVTSEL_E			(1 << 18)
#define PMC6_EVTSEL_PC			(1 << 19)
#define PMC6_EVTSEL_INT			(1 << 20)
#define PMC6_EVTSEL_EN			(1 << 22)	/* PerfEvtSel0 only */
#define PMC6_EVTSEL_INV			(1 << 23)
#define PMC6_EVTSEL_COUNTER_MASK	0xff000000
#define PMC6_EVTSEL_COUNTER_MASK_SHIFT	24

/* Data Cache Unit */
#define PMC6_DATA_MEM_REFS		0x43
#define PMC6_DCU_LINES_IN		0x45
#define PMC6_DCU_M_LINES_IN		0x46
#define PMC6_DCU_M_LINES_OUT		0x47
#define PMC6_DCU_MISS_OUTSTANDING	0x48

/* Instruction Fetch Unit */
#define PMC6_IFU_IFETCH			0x80
#define PMC6_IFU_IFETCH_MISS		0x81
#define PMC6_ITLB_MISS			0x85
#define PMC6_IFU_MEM_STALL		0x86
#define PMC6_ILD_STALL			0x87

/* L2 Cache */
#define PMC6_L2_IFETCH			0x28
#define PMC6_L2_LD			0x29
#define PMC6_L2_ST			0x2a
#define PMC6_L2_LINES_IN		0x24
#define PMC6_L2_LINES_OUT		0x26
#define PMC6_L2_M_LINES_INM		0x25
#define PMC6_L2_M_LINES_OUTM		0x27
#define PMC6_L2_RQSTS			0x2e
#define PMC6_L2_ADS			0x21
#define PMC6_L2_DBUS_BUSY		0x22
#define PMC6_L2_DBUS_BUSY_RD		0x23

/* External Bus Logic */
#define PMC6_BUS_DRDY_CLOCKS		0x62
#define PMC6_BUS_LOCK_CLOCKS		0x63
#define PMC6_BUS_REQ_OUTSTANDING	0x60
#define PMC6_BUS_TRAN_BRD		0x65
#define PMC6_BUS_TRAN_RFO		0x66
#define PMC6_BUS_TRANS_WB		0x67
#define PMC6_BUS_TRAN_IFETCH		0x68
#define PMC6_BUS_TRAN_INVAL		0x69
#define PMC6_BUS_TRAN_PWR		0x6a
#define PMC6_BUS_TRANS_P		0x6b
#define PMC6_BUS_TRANS_IO		0x6c
#define PMC6_BUS_TRAN_DEF		0x6d
#define PMC6_BUS_TRAN_BURST		0x6e
#define PMC6_BUS_TRAN_ANY		0x70
#define PMC6_BUS_TRAN_MEM		0x6f
#define PMC6_BUS_DATA_RCV		0x64
#define PMC6_BUS_BNR_DRV		0x61
#define PMC6_BUS_HIT_DRV		0x7a
#define PMC6_BUS_HITM_DRDV		0x7b
#define PMC6_BUS_SNOOP_STALL		0x7e

/* Floating Point Unit */
#define PMC6_FLOPS			0xc1
#define PMC6_FP_COMP_OPS_EXE		0x10
#define PMC6_FP_ASSIST			0x11
#define PMC6_MUL			0x12
#define PMC6_DIV			0x12
#define PMC6_CYCLES_DIV_BUSY		0x14

/* Memory Ordering */
#define PMC6_LD_BLOCKS			0x03
#define PMC6_SB_DRAINS			0x04
#define PMC6_MISALIGN_MEM_REF		0x05
#define PMC6_EMON_KNI_PREF_DISPATCHED	0x07	/* P-III only */
#define PMC6_EMON_KNI_PREF_MISS		0x4b	/* P-III only */

/* Instruction Decoding and Retirement */
#define PMC6_INST_RETIRED		0xc0
#define PMC6_UOPS_RETIRED		0xc2
#define PMC6_INST_DECODED		0xd0
#define PMC6_EMON_KNI_INST_RETIRED	0xd8
#define PMC6_EMON_KNI_COMP_INST_RET	0xd9

/* Interrupts */
#define PMC6_HW_INT_RX			0xc8
#define PMC6_CYCLES_INT_MASKED		0xc6
#define PMC6_CYCLES_INT_PENDING_AND_MASKED 0xc7

/* Branches */
#define PMC6_BR_INST_RETIRED		0xc4
#define PMC6_BR_MISS_PRED_RETIRED	0xc5
#define PMC6_BR_TAKEN_RETIRED		0xc9
#define PMC6_BR_MISS_PRED_TAKEN_RET	0xca
#define PMC6_BR_INST_DECODED		0xe0
#define PMC6_BTB_MISSES			0xe2
#define PMC6_BR_BOGUS			0xe4
#define PMC6_BACLEARS			0xe6

/* Stalls */
#define PMC6_RESOURCE_STALLS		0xa2
#define PMC6_PARTIAL_RAT_STALLS		0xd2

/* Segment Register Loads */
#define PMC6_SEGMENT_REG_LOADS		0x06

/* Clocks */
#define PMC6_CPU_CLK_UNHALTED		0x79

/* MMX Unit */
#define PMC6_MMX_INSTR_EXEC		0xb0	/* Celeron, P-II, P-IIX only */
#define PMC6_MMX_SAT_INSTR_EXEC		0xb1	/* P-II and P-III only */
#define PMC6_MMX_UOPS_EXEC		0xb2	/* P-II and P-III only */
#define PMC6_MMX_INSTR_TYPE_EXEC	0xb3	/* P-II and P-III only */
#define PMC6_FP_MMX_TRANS		0xcc	/* P-II and P-III only */
#define PMC6_MMX_ASSIST			0xcd	/* P-II and P-III only */
#define PMC6_MMX_INSTR_RET		0xc3	/* P-II only */

/* Segment Register Renaming */
#define PMC6_SEG_RENAME_STALLS		0xd4	/* P-II and P-III only */
#define PMC6_SEG_REG_RENAMES		0xd5	/* P-II and P-III only */
#define PMC6_RET_SEG_RENAMES		0xd6	/* P-II and P-III only */

/*
 * AMD K7. [Doc: 22007K.pdf, Feb 2002]
 */
/* Event Selector MSR format */
#define K7_EVTSEL_EVENT			0x000000ff
#define K7_EVTSEL_UNIT			0x0000ff00
#define K7_EVTSEL_UNIT_SHIFT		8
#define K7_EVTSEL_USR			__BIT(16)
#define K7_EVTSEL_OS			__BIT(17)
#define K7_EVTSEL_E			__BIT(18)
#define K7_EVTSEL_PC			__BIT(19)
#define K7_EVTSEL_INT			__BIT(20)
#define K7_EVTSEL_EN			__BIT(22)
#define K7_EVTSEL_INV			__BIT(23)
#define K7_EVTSEL_COUNTER_MASK		0xff000000
#define K7_EVTSEL_COUNTER_MASK_SHIFT	24
/* Data Cache Unit */
#define K7_DATA_CACHE_ACCESS		0x40
#define K7_DATA_CACHE_MISS		0x41
#define K7_DATA_CACHE_REFILL		0x42
#define K7_DATA_CACHE_REFILL_SYSTEM	0x43
#define K7_DATA_CACHE_WBACK		0x44
#define K7_L1_DTLB_MISS			0x45
#define K7_L2_DTLB_MISS			0x46
#define K7_MISALIGNED_DATA_REF		0x47
/* Instruction Fetch Unit */
#define K7_IFU_IFETCH			0x80
#define K7_IFU_IFETCH_MISS		0x81
#define K7_IFU_REFILL_FROM_L2		0x82
#define K7_IFU_REFILL_FROM_SYSTEM	0x83
#define K7_L1_ITLB_MISS			0x84
#define K7_L2_ITLB_MISS			0x85
/* Retired */
#define K7_RETIRED_INST			0xc0
#define K7_RETIRED_OPS			0xc1
#define K7_RETIRED_BRANCH		0xc2
#define K7_RETIRED_BRANCH_MISPREDICTED	0xc3
#define K7_RETIRED_TAKEN_BRANCH		0xc4
#define K7_RETIRED_TAKEN_BRANCH_MISPREDICTED	0xc5
#define K7_RETIRED_FAR_CONTROL_TRANSFER	0xc6
#define K7_RETIRED_RESYNC_BRANCH	0xc7
/* Interrupts */
#define K7_CYCLES_INT_MASKED		0xcd
#define K7_CYCLES_INT_PENDING_AND_MASKED	0xce
#define K7_HW_INTR_RECV			0xcf

/*
 * AMD 10h family PMCs. [Doc: 31116.pdf, Jan 2013]
 */
/*	Register MSRs			*/
#define MSR_F10H_EVNTSEL0			0xc0010000
#define MSR_F10H_EVNTSEL1			0xc0010001
#define MSR_F10H_EVNTSEL2			0xc0010002
#define MSR_F10H_EVNTSEL3			0xc0010003
#define MSR_F10H_PERFCTR0			0xc0010004
#define MSR_F10H_PERFCTR1			0xc0010005
#define MSR_F10H_PERFCTR2			0xc0010006
#define MSR_F10H_PERFCTR3			0xc0010007
/*	Event Selector MSR format	*/
#define F10H_EVTSEL_EVENT_MASK			0x000F000000FF
#define F10H_EVTSEL_EVENT_SHIFT_LOW		0
#define F10H_EVTSEL_EVENT_SHIFT_HIGH		32
#define F10H_EVTSEL_UNIT_MASK			0x0000FF00
#define F10H_EVTSEL_UNIT_SHIFT			8
#define F10H_EVTSEL_USR				__BIT(16)
#define F10H_EVTSEL_OS				__BIT(17)
#define F10H_EVTSEL_EDGE			__BIT(18)
#define F10H_EVTSEL_RSVD1			__BIT(19)
#define F10H_EVTSEL_INT				__BIT(20)
#define F10H_EVTSEL_RSVD2			__BIT(21)
#define F10H_EVTSEL_EN				__BIT(22)
#define F10H_EVTSEL_INV				__BIT(23)
#define F10H_EVTSEL_COUNTER_MASK		0xFF000000
#define F10H_EVTSEL_COUNTER_MASK_SHIFT		24
/*	Floating Point Events		*/
#define F10H_FP_DISPATCHED_FPU_OPS		0x00
#define F10H_FP_CYCLES_EMPTY_FPU_OPS		0x01
#define F10H_FP_DISPATCHED_FASTFLAG_OPS		0x02
#define F10H_FP_RETIRED_SSE_OPS			0x03
#define F10H_FP_RETIRED_MOVE_OPS		0x04
#define F10H_FP_RETIRED_SERIALIZING_OPS		0x05
#define F10H_FP_CYCLES_SERIALIZING_OP_SCHEDULER	0x06
/*	Load/Store and TLB Events	*/
#define F10H_SEGMENT_REG_LOADS			0x20
#define	F10H_PIPELINE_RESTART_SELFMOD_CODE	0x21
#define F10H_PIPELINE_RESTART_PROBE_HIT		0x22
#define F10H_LS_BUFFER_2_FILL			0x23
#define F10H_LOCKED_OPERATIONS			0x24
#define F10H_RETIRED_CLFLUSH_INSTRUCTIONS	0x26
#define F10H_RETIRED_CPUID_INSTRUCTIONS		0x27
#define F10H_CANCELLED_STORE_LOAD_FORWARD_OPS	0x2A
#define F10H_SMI_RECEIVED			0x2B
/*	Data Cache Events		*/
#define F10H_DATA_CACHE_ACCESS			0x40
#define F10H_DATA_CACHE_MISS			0x41
#define F10H_DATA_CACHE_REFILL_FROM_L2		0x42
#define F10H_DATA_CACHE_REFILL_FROM_NORTHBRIDGE	0x43
#define F10H_CACHE_LINES_EVICTED		0x44
#define F10H_L1_DTLB_MISS			0x45
#define F10H_L2_DTLB_MISS			0x46
#define F10H_MISALIGNED_ACCESS			0x47
#define F10H_MICROARCH_LATE_CANCEL_OF_ACCESS	0x48
#define F10H_MICROARCH_EARLY_CANCEL_OF_ACCESS	0x49
#define F10H_SINGLE_BIT_ECC_ERRORS_RECORDED	0x4A
#define F10H_PREFETCH_INSTRUCTIONS_DISPATCHED	0x4B
#define F10H_DCACHE_MISSES_LOCKED_INSTRUCTIONS	0x4C
#define F10H_L1_DTLB_HIT			0x4D
#define F10H_INEFFECTIVE_SOFTWARE_PREFETCHS	0x52
#define F10H_GLOBAL_TLB_FLUSHES			0x54
#define F10H_MEMORY_REQUESTS_BY_TYPE		0x65
#define F10H_DATA_PREFETCHER			0x67
#define F10H_MAB_REQUESTS			0x68
#define F10H_MAB_WAIT_CYCLES			0x69
#define F10H_NORTHBRIDGE_READ_RESP_BY_COH_STATE	0x6C
#define F10H_OCTWORDS_WRITTEN_TO_SYSTEM		0x6D
#define F10H_CPU_CLOCKS_NOT_HALTED		0x76
#define F10H_REQUESTS_TO_L2_CACHE		0x7D
#define F10H_L2_CACHE_MISSES			0x7E
#define F10H_L2_FILL				0x7F
/* F10H_PAGE_SIZE_MISMATCHES (0x01C0): reserved on some revisions */
/*	Instruction Cache Events	*/
#define F10H_INSTRUCTION_CACHE_FETCH		0x80
#define F10H_INSTRUCTION_CACHE_MISS		0x81
#define F10H_INSTRUCTION_CACHE_REFILL_FROM_L2	0x82
#define F10H_INSTRUCTION_CACHE_REFILL_FROM_SYS	0x83
#define F10H_L1_ITLB_MISS			0x84
#define F10H_L2_ITLB_MISS			0x85
#define F10H_PIPELINE_RESTART_INSTR_STREAM_PROBE	0x86
#define F10H_INSTRUCTION_FETCH_STALL		0x87
#define F10H_RETURN_STACK_HITS			0x88
#define F10H_RETURN_STACK_OVERFLOWS		0x89
#define F10H_INSTRUCTION_CACHE_VICTIMS		0x8B
#define F10H_INSTRUCTION_CACHE_LINES_INVALIDATED	0x8C
#define F10H_ITLD_RELOADS			0x99
#define F10H_ITLD_RELOADS_ABORTED		0x9A
/*	Execution Unit Events		*/
#define F10H_RETIRED_INSTRUCTIONS		0xC0
#define F10H_RETIRED_UOPS			0xC1
#define F10H_RETIRED_BRANCH			0xC2
#define F10H_RETIRED_MISPREDICTED_BRANCH	0xC3
#define F10H_RETIRED_TAKEN_BRANCH		0xC4
#define F10H_RETIRED_TAKEN_BRANCH_MISPREDICTED	0xC5
#define F10H_RETIRED_FAR_CONTROL_TRANSFER	0xC6
#define F10H_RETIRED_BRANCH_RESYNC		0xC7
#define F10H_RETIRED_NEAR_RETURNS		0xC8
#define F10H_RETIRED_NEAR_RETURNS_MISPREDICTED	0xC9
#define F10H_RETIRED_INDIRECT_BRANCH_MISPREDICTED	0xCA
#define F10H_RETIRED_MMX_FP_INSTRUCTIONS	0xCB
#define F10H_RETIRED_FASTPATH_DOUBLE_OP_INSTR	0xCC
#define F10H_INTERRUPTS_MASKED_CYCLES		0xCD
#define F10H_INTERRUPTS_MASKED_CYCLES_INTERRUPT_PENDING	0xCE
#define F10H_INTERRUPTS_TAKEN			0xCF
#define F10H_DECODER_EMPTY			0xD0
#define F10H_DISPATCH_STALLS			0xD1
#define F10H_DISPATCH_STALLS_BRANCH_ABORT_RETIRE	0xD2
#define F10H_DISPATCH_STALLS_SERIALIZATION	0xD3
#define F10H_DISPATCH_STALLS_SEGMENT_LOAD	0xD4
#define F10H_DISPATCH_STALLS_REORDER_BUF_FULL	0xD5
#define F10H_DISPATCH_STALLS_RSV_STATION_FULL	0xD6
#define F10H_DISPATCH_STALLS_FPU_FULL		0xD7
#define F10H_DISPATCH_STALLS_LS_FULL		0xD8
#define F10H_DISPATCH_STALLS_WAITING_ALL_QUITE	0xD9
#define F10H_DISPATCH_STALLS_FAR_TRANSFER	0xDA
#define F10H_FPU_EXCEPTIONS			0xDB
#define F10H_DR0_BREAKPOINT_MATCHES		0xDC
#define F10H_DR1_BREAKPOINT_MATCHES		0xDD
#define F10H_DR2_BREAKPOINT_MATCHES		0xDE
#define F10H_DR3_BREAKPOINT_MATCHES		0xDF
/* F10H_RETIRED_X87_FP_OPERATIONS (0x01C0): reserved on some revisions */
/* F10H_IBS_OPS_TAGGED (0x1CF): reserved on some revisions */
/* F10H_LFENCE_INSTRUCTIONS_RETIRED (0x01D3): reserved on some revisions */
/* F10H_SFENCE_INSTRUCTIONS_RETIRED (0x01D4): reserved on some revisions */
/* F10H_MFENCE_INSTRUCTIONS_RETIRED (0x01D5): reserved on some revisions */
/*	Memory Controller Events	*/
#define F10H_DRAM_ACCESSES			0xE0
#define F10H_DRAM_CONTROLLER_PT_OVERFLOWS	0xE1
#define F10H_MEM_CONTROLLER_DRAM_COMMAND_SLOTS_MISSED	0xE2
#define F10H_MEM_CONTROLLER_TURNAROUNDS		0xE3
#define F10H_MEM_CONTROLLER_BYPASS_COUNTER_SATURATION	0xE4
#define F10H_THERMAL_STATUS			0xE8
#define F10H_CPU_IO_REQUESTS_TO_MEMORY_IO	0xE9
#define F10H_CACHE_BLOCK_COMMANDS		0xEA
#define F10H_SIZED_COMMANDS			0xEB
#define F10H_PROBE_RESPONSES_AND_UPSTREAM_REQUESTS	0xEC
#define F10H_GART_EVENTS			0xEE
#define F10H_MEMORY_CONTROLLER_REQUESTS		0x01F0
#define F10H_CPU_TO_DRAM_REQUESTS_TO_TARGET_NODE	0x01E0
#define F10H_IO_TO_DRAM_REQUESTS_TO_TARGET_NODE	0x01E1
#define F10H_CPU_READ_CMD_LATENCY_TARGET_NODE_03	0x01E2
#define F10H_CPU_READ_CMD_REQUESTS_TARGET_NODE_03	0x01E3
#define F10H_CPU_READ_CMD_LATENCY_TARGET_NODE_47	0x01E4
#define F10H_CPU_READ_CMD_REQUESTS_TARGET_NODE_47	0x01E5
#define F10H_CPU_CMD_LATENCY_TO_TARGET_NODE_0347	0x01E6
#define F10H_CPU_REQUESTS_TO_TARGET_NODE_0347	0x01E7
/*	Link Events			*/
#define F10H_HYPERTRANSPORT_LINK0_TRANSMIT_BANDWIDTH	0xF6
#define F10H_HYPERTRANSPORT_LINK1_TRANSMIT_BANDWIDTH	0xF7
#define F10H_HYPERTRANSPORT_LINK2_TRANSMIT_BANDWIDTH	0xF8
#define F10H_HYPERTRANSPORT_LINK3_TRANSMIT_BANDWIDTH	0x01F9
/*	L3 Cache Events			*/
/* F10H_READ_READ_REQUEST_TO_L3_CACHE (0x04E0): depends on the revision */
/* F10H_L3_CACHE_MISSES (0x04E1): depends on the revision */
/* F10H_L3_FILLS_FROM_L2_EVICTIONS (0x04E2): depends on the revision */
#define F10H_L3_EVICTIONS			0x04E3
/* F10H_NONCANCELLED_L3_READ_REQUESTS (0x04ED): depends on the revision */

