/*	$Id: rmixl_fmnvar.h,v 1.1.4.1 2011/06/06 09:06:09 jruoho Exp $	*/
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
	u_int rxsid;
	u_int code;
	u_int size;
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
#define RMIXL_CPU_CORE(cpuid)	((uint32_t)((cpuid) & __BITS(9,0)) >> 2)
#define RMIXL_CPU_THREAD(cpuid)	((uint32_t)((cpuid) & __BITS(1,0)))
#define RMIXL_FMN_CORE_DESTID(core, bucket)	\
		 (((core) << 3) | (bucket))


#define RMIXL_DMFC2(regnum, sel, rv)					\
do {									\
	uint64_t __val;							\
									\
	__asm volatile(							\
		".set push" 			"\n\t"			\
		".set mips64"			"\n\t"			\
		".set noat"			"\n\t"			\
		"dmfc2 %0,$%1,%2"		"\n\t"			\
		".set pop"			"\n\t"			\
	    : "=r"(__val) : "n"(regnum), "n"(sel));			\
	rv = __val;							\
} while (0)

#define RMIXL_DMTC2(regnum, sel, val)					\
do {									\
	uint64_t __val = val;						\
									\
	__asm volatile(							\
		".set push" 			"\n\t"			\
		".set mips64"			"\n\t"			\
		".set noat"			"\n\t"			\
		"dmtc2 %0,$%1,%2"		"\n\t"			\
		".set pop"			"\n\t"			\
	    :: "r"(__val), "n"(regnum), "n"(sel));			\
} while (0)

#define RMIXL_MFC2(regnum, sel, rv)					\
do {									\
	uint32_t __val;							\
									\
	__asm volatile(							\
		".set push"			"\n\t"			\
		".set mips64"			"\n\t"			\
		"mfc2 %0,$%1,%2"		"\n\t"			\
		".set pop"			"\n\t"			\
	    : "=r"(__val) : "n"(regnum), "n"(sel));			\
	rv = __val;							\
} while (0)

#define RMIXL_MTC2(regnum, sel, val)					\
do {									\
	uint32_t __val = val;						\
									\
	__asm volatile(							\
		".set push"			"\n\t"			\
		".set mips64"			"\n\t"			\
		"mtc2 %0,$%1,%2"		"\n\t"			\
		".set pop"			"\n\t"			\
	    :: "r"(__val), "n"(regnum), "n"(sel));			\
} while (0)

#define CPU2_PRINT_8(regno, sel)					\
do {									\
	uint64_t r;							\
	RMIXL_DMFC2(regno, sel, r);					\
	printf("%s: CP2(%d,%d) = %#"PRIx64"\n",				\
		__func__, regno, sel, r);				\
} while (0)

#define CPU2_PRINT_4(regno, sel)					\
do {									\
	uint32_t r;							\
	RMIXL_MFC2(regno, sel, r);					\
	printf("%s: CP2(%d,%d) = %#x\n",				\
		__func__, regno, sel, r);				\
} while (0)


/*
 * encode 'dest' for msgsnd op 'rt'
 */
#define RMIXL_MSGSND_DESC(size, code, dest_id)	\
		((((size) - 1) << 16) | ((code) << 8) | (dest_id))

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

/*
 * the seemingly-spurious add is recommended by RMI
 * see XLS PRM (rev. 3.21) 5.3.9
 */
static inline void
rmixl_fmn_msgwait(u_int mask)
{
	__asm__ volatile (
		".set push"		"\n\t"
		".set noreorder"	"\n\t"
		".set arch=xlr"		"\n\t"
		"addu %0,%0,0"		"\n\t"
		"msgwait %0"		"\n\t"
		".set pop"		"\n\t"
			:: "r"(mask));
}

static inline uint32_t
rmixl_cp2_enable(void)
{
	uint32_t rv;
	uint32_t cu2;

	KASSERT(curcpu()->ci_cpl == IPL_HIGH);
	__asm volatile(
		".set push"		"\n\t"
		".set noreorder"	"\n\t"
		"li	%1,%3"		"\n\t"
		"mfc0	%0,$%2"		"\n\t"
		"or	%1,%1,%0"	"\n\t"
		"mtc0	%1,$%2"		"\n\t"
		".set pop"		"\n\t"
			: "=r"(rv), "=r"(cu2)
			: "n"(MIPS_COP_0_STATUS), "n"(1 << 30));

	return (rv & (1 << 30));
}

static inline void
rmixl_cp2_restore(uint32_t ocu)
{
	uint32_t cu2;
	uint32_t mask = ~(1 << 30);

	KASSERT(curcpu()->ci_cpl == IPL_HIGH);
	__asm volatile(
		".set push"		"\n\t"
		".set noreorder"	"\n\t"
		"mfc0	%0,$%1"		"\n\t"
		"and	%0,%2,%0"	"\n\t"
		"or	%0,%3,%0"	"\n\t"
		"mtc0	%0,$%1"		"\n\t"
		".set pop"		"\n\t"
			: "=r"(cu2)
			: "n"(MIPS_COP_0_STATUS), "r"(mask), "r"(ocu));
}

/*
 * logical station IDs for RMI XLR
 * see Table 13.2 "Addressable Buckets" in the XLR PRM
 */
#define RMIXLR_FMN_STID_CORE0			0
#define RMIXLR_FMN_STID_CORE1			1
#define RMIXLR_FMN_STID_CORE2			2
#define RMIXLR_FMN_STID_CORE3			3
#define RMIXLR_FMN_STID_CORE4			4
#define RMIXLR_FMN_STID_CORE5			5
#define RMIXLR_FMN_STID_CORE6			6
#define RMIXLR_FMN_STID_CORE7			7
#define RMIXLR_FMN_STID_TXRX_0			8
#define RMIXLR_FMN_STID_TXRX_1			9
#define RMIXLR_FMN_STID_RGMII			10
#define RMIXLR_FMN_STID_DMA			11
#define RMIXLR_FMN_STID_FREE_0			12
#define RMIXLR_FMN_STID_FREE_1			13
#define RMIXLR_FMN_STID_SAE			14
#define RMIXLR_FMN_NSTID			(RMIXLR_FMN_STID_SAE+1)
#define RMIXLR_FMN_STID_RESERVED		-1

/*
 * logical station IDs for RMI XLS
 * see Table 12.1 "Stations and Addressable Buckets ..." in the XLS PRM
 */
#define RMIXLS_FMN_STID_CORE0			0
#define RMIXLS_FMN_STID_CORE1			1
#define RMIXLS_FMN_STID_CORE2			2
#define RMIXLS_FMN_STID_CORE3			3
#define RMIXLS_FMN_STID_GMAC_Q0			4
#define RMIXLS_FMN_STID_GMAC_Q1			5
#define RMIXLS_FMN_STID_DMA			6
#define RMIXLS_FMN_STID_CDE			7
#define RMIXLS_FMN_STID_PCIE			8
#define RMIXLS_FMN_STID_SAE			9
#define RMIXLS_FMN_NSTID			(RMIXLS_FMN_STID_SAE+1)
#define RMIXLS_FMN_STID_RESERVED		-1

/*
 * logical station IDs for RMI XLP
 * TBD!
 */
#define RMIXLP_FMN_NSTID			0	/* XXX */


#define RMIXL_FMN_NSTID		\
		MAX(MAX(RMIXLR_FMN_NSTID, RMIXLS_FMN_NSTID), RMIXLP_FMN_NSTID)


#define RMIXL_FMN_INTR_IPL	IPL_HIGH

void	rmixl_fmn_init(void);
void	rmixl_fmn_init_core(void);
void	rmixl_fmn_init_cpu_intr(void);
void   *rmixl_fmn_intr_establish(int, int (*)(void *, rmixl_fmn_rxmsg_t *), void *);
void	rmixl_fmn_intr_disestablish(void *);
void	rmixl_fmn_intr_poll(u_int, rmixl_fmn_rxmsg_t *);
int	rmixl_fmn_msg_send(u_int, u_int, u_int, rmixl_fmn_msg_t *);
int	rmixl_fmn_msg_recv(u_int, rmixl_fmn_rxmsg_t *);



#endif	/* _ARCH_MIPS_RMIXL_RMIXL_FMNVAR_H_ */
