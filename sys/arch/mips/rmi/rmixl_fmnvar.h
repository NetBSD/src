/*	$Id: rmixl_fmnvar.h,v 1.1.2.1 2010/03/21 20:41:43 cliff Exp $	*/

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
		".set mips64"		"\n\t"
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
		".set mips64"		"\n\t"
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
		".set mips64"		"\n\t"
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
 * logical station IDs
 * see Table 12.1 in the XLS PRM
 */
#define RMIXL_FMN_STID_CORE0		0
#define RMIXL_FMN_STID_CORE1		1
#define RMIXL_FMN_STID_CORE2		2
#define RMIXL_FMN_STID_CORE3		3
#define RMIXL_FMN_STID_GMAC_Q0		4
#define RMIXL_FMN_STID_GMAC_Q1		5
#define RMIXL_FMN_STID_DMA		6
#define RMIXL_FMN_STID_CDE		7
#define RMIXL_FMN_STID_PCIE		8
#define RMIXL_FMN_STID_SAE		9
#define RMIXL_FMN_NSTID		(RMIXL_FMN_STID_SAE+1)
#define RMIXL_FMN_STID_RESERVED	-1

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
