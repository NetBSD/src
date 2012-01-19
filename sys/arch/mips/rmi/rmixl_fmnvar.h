/*	rmixl_fmnvar.h,v 1.1.2.7 2012/01/19 09:59:08 matt Exp	*/
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors.
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

#ifndef _ARCH_MIPS_RMIXL_RMIXL_FMNVAR_H_
#define _ARCH_MIPS_RMIXL_RMIXL_FMNVAR_H_

#include <sys/cpu.h>
#include <mips/cpuregs.h>

#define RMIXL_FMN_CODE_PSB_WAKEUP	200	/* firmware MSGRNG_CODE_BOOT_WAKEUP */
#define RMIXL_FMN_CODE_HELLO_REQ	201
#define RMIXL_FMN_CODE_HELLO_ACK	202

#define RMIXL_FMN_HELLO_REQ_SZ		4
#define RMIXL_FMN_HELLO_ACK_SZ		4

typedef struct rmixl_fmn_msg {
	uint64_t data[4];
} rmixl_fmn_msg_t;

typedef struct rmixl_fmn_rxmsg {
	uint16_t rxsid;
	u_int code;
	uint8_t size;
	rmixl_fmn_msg_t msg;
} rmixl_fmn_rxmsg_t;


/*
 * compute FMN dest_id from MIPS cpuid
 * - each Core FMN sation has 8 buckets
 * - each Core has 4 threads
 * - here we use 1 bucket per thread
 *   (the first four buckets)
 * - if we need { hi, lo } priority buckets per thread
 *   need to adjust the RMIXL_FMN_DESTID macro
 *   and use the 'pri' parameter
 * - i.e. for now there is only one priority
 */
#define RMIXL_CPU_CORE(cpuid)	((uint32_t)__SHIFTOUT((cpuid), __BITS(7,3)))
#define RMIXL_CPU_THREAD(cpuid)	((uint32_t)__SHIFTOUT((cpuid), __BITS(1,0)))

static inline uint64_t
mips_dmfc2(const u_int regnum, const u_int sel)
{
	uint64_t __val;

	__asm volatile(
		".set push" 			"\n\t"
		".set mips64"			"\n\t"
		".set noat"			"\n\t"
		"dmfc2 %0,$%1,%2"		"\n\t"
		".set pop"			"\n\t"
	    : "=r"(__val) : "n"(regnum), "n"(sel));

	return __val;
}

static inline void
mips_dmtc2(u_int regnum, u_int sel, uint64_t val)
{
	__asm volatile(
		".set push" 			"\n\t"
		".set mips64"			"\n\t"
		".set noat"			"\n\t"
		"dmtc2 %0,$%1,%2"		"\n\t"
		".set pop"			"\n\t"
	    :: "r"(val), "n"(regnum), "n"(sel));
}

static inline uint64_t
mips_mfc2(const u_int regnum, const u_int sel)
{
	uint32_t __val;

	__asm volatile(
		".set push"			"\n\t"
		".set mips32"			"\n\t"
		"mfc2 %0,$%1,%2"		"\n\t"
		".set pop"			"\n\t"
	    : "=r"(__val) : "n"(regnum), "n"(sel));
	return __val;
}

static inline void
mips_mtc2(u_int regnum, u_int sel, uint32_t val)
{
	__asm volatile(
		".set push" 			"\n\t"
		".set mips32"			"\n\t"
		".set noat"			"\n\t"
		"mtc2 %0,$%1,%2"		"\n\t"
		".set pop"			"\n\t"
	    :: "r"(val), "n"(regnum), "n"(sel));
}

#define COP2_PRINT_8(regno, sel)					\
do {									\
	printf("%s: COP2(%d,%d) = %#"PRIx64"\n",			\
	    __func__, regno, sel, mips_dmfc2(regno, sel));		\
} while (0)

#define COP2_PRINT_4(regno, sel)					\
do {									\
	printf("%s: COP2(%d,%d) = %#"PRIx32"\n",			\
	    __func__, regno, sel, mips_mfc2(regno, sel));		\
} while (0)


/*
 * encode 'dest' for msgsnd op 'rt'
 */
#define RMIXL_MSGSND_DESC(size, code, dest_id)			\
		(__SHIFTOUT((dest_id), __BITS(7,0))		\
		|__SHIFTOUT((code), __BITS(15,8))		\
		|__SHIFTOUT((size)-1, __BITS(17,16)))
#define RMIXLP_MSGSND_DESC(size, code, dest_id, dest_vc)	\
		(__SHIFTOUT((dest_id), __BITS(11,0))		\
		|__SHIFTOUT((size)-1, __BITS(17,16))		\
		|__SHIFTOUT((dest_vc), __BITS(20,19))		\
		|__SHIFTOUT((code), __BITS(31,24)))

static inline void
rmixl_msgsnd(uint32_t desc)
{
	__asm__ volatile (
		".set push"		"\n\t"
		".set noreorder"	"\n\t"
		".set arch=xlr"		"\n\t"
		"sync"			"\n\t"
		"msgsnd %0"		"\n\t"
		".set pop"		"\n\t"
	    :: "r"(desc));
}

static inline uint32_t
rmixlp_msgsnd(uint32_t desc)
{
	uint32_t rv;

	__asm__ volatile (
		".set push"		"\n\t"
		".set noreorder"	"\n\t"
		".set arch=xlp"		"\n\t"
		"sync"			"\n\t"
		"msgsnds %[desc],%[rv]"	"\n\t"
		".set pop"		"\n\t"
	    :	[rv] "=r" (rv)
	    :	[desc] "r" (desc));

	return rv;
}

static inline void
rmixl_msgld(uint32_t bucket)
{
	__asm__ volatile (
		".set push"		"\n\t"
		".set noreorder"	"\n\t"
		".set arch=xlr"		"\n\t"
		"msgld %0"		"\n\t"
		".set pop"		"\n\t"
	    :: "r"(bucket));
}

static inline uint32_t
rmixlp_msgld(uint32_t rxq)
{
	uint32_t rv;

	__asm__ volatile (
		".set push"		"\n\t"
		".set noreorder"	"\n\t"
		".set arch=xlp"		"\n\t"
		"msglds %[rxq],%[rv]"	"\n\t"
		".set pop"		"\n\t"
	    :	[rv] "=r"(rv)
	    :	[rxq] "r"(rxq));

	return rv;
}

/*
 * the seemingly-spurious add is recommended by RMI
 * see XLS PRM (rev. 3.21) 5.3.9
 */
static inline void
rmixl_msgwait(u_int mask)
{
	__asm__ volatile (
		".set push"		"\n\t"
		".set noreorder"	"\n\t"
		".set arch=xlr"		"\n\t"
		"daddu %0,%0,0"		"\n\t"
		"msgwait %0"		"\n\t"
		".set pop"		"\n\t"
	    :: "r"(mask));
}

static inline uint32_t
rmixl_cp2_enable(void)
{
	uint32_t rv;
	uint32_t sr;

	__asm volatile(
		".set push"			"\n\t"
		".set noreorder"		"\n\t"
		".set noat"			"\n\t"
		"mfc0	%[sr],$%[c0_status]"	"\n\t"
		"and	%[rv],%[sr],%[mask]"	"\n\t"
		"or	%[sr],%[mask]"		"\n\t"
		"mtc0	%[sr],$%[c0_status]"	"\n\t"
		".set pop"			"\n\t"
	    :	[rv] "=r" (rv),
		[sr] "=r" (sr)
	    :	[c0_status] "n" (MIPS_COP_0_STATUS),
		[mask] "r" (MIPS_SR_COP_2_BIT));

	return rv;
}

static inline void
rmixl_cp2_restore(uint32_t ocu)
{
	uint32_t cu2;

	__asm volatile(
		".set push"			"\n\t"
		".set noreorder"		"\n\t"
		".set noat"			"\n\t"
		"mfc0	%[sr],$%[c0_status]"	"\n\t"
		"and	%[sr],%[mask]"		"\n\t"
		"or	%[sr],%[ocu]"		"\n\t"
		"mtc0	%[sr],$%[c0_status]"	"\n\t"
		".set pop"			"\n\t"
	    :	[sr] "=r"(cu2)
	    :	[c0_status] "n" (MIPS_COP_0_STATUS),
		[mask] "r" (~MIPS_SR_COP_2_BIT),
		[ocu] "r" (ocu));
}

#ifdef MIPS64_XLP
/*
 * logical station IDs for RMI XLP
 */
#define	RMIXLP_FMN_STID_RESERVED	0
#define	RMIXLP_FMN_STID_CPU		1	
#define	RMIXLP_FMN_STID_POPQ		2
#define	RMIXLP_FMN_STID_PCIE0		3
#define	RMIXLP_FMN_STID_PCIE1		4
#define	RMIXLP_FMN_STID_PCIE2		5
#define	RMIXLP_FMN_STID_PCIE3		6
#define	RMIXLP_FMN_STID_DMA		7
#define	RMIXLP_FMN_STID_PKE		8
#define	RMIXLP_FMN_STID_SAE		9
#define	RMIXLP_FMN_STID_CDE		10
#define	RMIXLP_FMN_STID_POE		11
#define	RMIXLP_FMN_STID_NAE		12	// NAE Egress
#define	RMIXLP_FMN_STID_RXE		13
#define	RMIXLP_FMN_STID_SRIO		14
#define	RMIXLP_FMN_STID_FMN		15
#define	RMIXLP_FMN_STID_NAE_FREEIN	16
#define	RMIXLP_FMN_NSTID		17
#else
#define	RMIXLP_FMN_NSTID		0
#endif

#ifdef MIPS64_XLS
/*
 * logical station IDs for RMI XLR
 * see Table 13.2 "Addressable Buckets" in the XLR PRM
 */
#define RMIXLR_FMN_STID_RESERVED		0
#define RMIXLR_FMN_STID_CORE0			1
#define RMIXLR_FMN_STID_CORE1			2
#define RMIXLR_FMN_STID_CORE2			3
#define RMIXLR_FMN_STID_CORE3			4
#define RMIXLR_FMN_STID_CORE4			5
#define RMIXLR_FMN_STID_CORE5			6
#define RMIXLR_FMN_STID_CORE6			7
#define RMIXLR_FMN_STID_CORE7			8
#define RMIXLR_FMN_STID_TXRX_0			9
#define RMIXLR_FMN_STID_TXRX_1			10
#define RMIXLR_FMN_STID_RGMII			11
#define RMIXLR_FMN_STID_DMA			12
#define RMIXLR_FMN_STID_FREE_0			13
#define RMIXLR_FMN_STID_FREE_1			14
#define RMIXLR_FMN_STID_SAE			15
#define RMIXLR_FMN_NSTID			(RMIXLR_FMN_STID_SAE+1)
#else
#define	RMIXLR_FMN_NSTID		0
#endif

#ifdef MIPS64_XLR
/*
 * logical station IDs for RMI XLS
 * see Table 12.1 "Stations and Addressable Buckets ..." in the XLS PRM
 */
#define RMIXLS_FMN_STID_RESERVED		0
#define RMIXLS_FMN_STID_CORE0			1
#define RMIXLS_FMN_STID_CORE1			2
#define RMIXLS_FMN_STID_CORE2			3
#define RMIXLS_FMN_STID_CORE3			4
#define RMIXLS_FMN_STID_GMAC_Q0			5
#define RMIXLS_FMN_STID_GMAC_Q1			6
#define RMIXLS_FMN_STID_DMA			7
#define RMIXLS_FMN_STID_CDE			8
#define RMIXLS_FMN_STID_PCIE			9
#define RMIXLS_FMN_STID_SAE			10
#define RMIXLS_FMN_NSTID			(RMIXLS_FMN_STID_SAE+1)
#else
#define	RMIXLS_FMN_NSTID		0
#endif

#define RMIXL_FMN_NSTID		\
	MAX(MAX(RMIXLR_FMN_NSTID, RMIXLS_FMN_NSTID), RMIXLP_FMN_NSTID)

typedef int (*rmixl_fmn_intr_handler_t)(void *, rmixl_fmn_rxmsg_t *);

void	rmixl_fmn_cpu_attach(struct cpu_info *ci);
void	rmixl_fmn_init(void);
void	rmixl_fmn_init_thread(void);
void *	rmixl_fmn_intr_establish(size_t, rmixl_fmn_intr_handler_t, void *);
void	rmixl_fmn_intr_disestablish(void *);
void	rmixl_fmn_intr_poll(u_int, rmixl_fmn_rxmsg_t *);

size_t	rmixl_fmn_qid_to_stid(size_t);
const char *
	rmixl_fmn_stid_name(size_t);

/*
 * true == succes, false = failure
 */
bool	rmixl_fmn_msg_send(u_int, u_int, u_int, u_int, const rmixl_fmn_msg_t *);
bool	rmixl_fmn_msg_recv(u_int, rmixl_fmn_rxmsg_t *);

#endif	/* _ARCH_MIPS_RMIXL_RMIXL_FMNVAR_H_ */
