/*	$NetBSD: mcontext.h,v 1.12 2022/06/26 14:37:13 skrll Exp $	*/

#ifndef _HPPA_MCONTEXT_H_
#define	_HPPA_MCONTEXT_H_

/*
 * General register state
 */
#define	_NGREG		44

#define _REG_R1		1
#define _REG_R2		2
#define _REG_R3		3
#define _REG_R4		4
#define _REG_R5		5
#define _REG_R6		6
#define _REG_R7		7
#define _REG_R8		8
#define _REG_R9		9
#define _REG_R10	10
#define _REG_R11	11
#define _REG_R12	12
#define _REG_R13	13
#define _REG_R14	14
#define _REG_R15	15
#define _REG_R16	16
#define _REG_R17	17
#define _REG_R18	18
#define _REG_R19	19
#define _REG_R20	20
#define _REG_R21	21
#define _REG_R22	22
#define _REG_R23	23
#define _REG_R24	24
#define _REG_R25	25
#define _REG_R26	26
#define _REG_R27	27
#define _REG_R28	28
#define _REG_R29	29
#define _REG_R30	30
#define _REG_R31	31

#define	_REG_PSW	0
#define	_REG_RP		2
#define	_REG_R19	19
#define	_REG_ARG0	26
#define	_REG_DP		27
#define	_REG_RET0	28
#define	_REG_SP		30
#define	_REG_SAR	32
#define	_REG_PCSQH	33
#define	_REG_PCSQT	34
#define	_REG_PCOQH	35
#define	_REG_PCOQT	36
#define	_REG_SR0	37
#define	_REG_SR1	38
#define	_REG_SR2	39
#define	_REG_SR3	40
#define	_REG_SR4	41
#define	_REG_CR26	42
#define	_REG_CR27	43

#ifndef __ASSEMBLER__

typedef	unsigned long	__greg_t;
typedef	__greg_t	__gregset_t[_NGREG];

/*
 * Floating point register state
 */

typedef struct {
	union {
		unsigned long long	__fp_regs[32];
		double			__fp_dregs[32];
	}	__fp_fr;
} __fpregset_t;

typedef struct {
	__gregset_t	__gregs;
	__fpregset_t	__fpregs;
} mcontext_t;

#define	_UC_MACHINE_SP(uc)	((uc)->uc_mcontext.__gregs[_REG_SP])
#define	_UC_MACHINE_FP(uc)	((uc)->uc_mcontext.__gregs[3])
#define	_UC_MACHINE_PC(uc) 	((uc)->uc_mcontext.__gregs[_REG_PCOQH])
#define	_UC_MACHINE_SET_PC(uc, pc)					\
do {									\
	(uc)->uc_mcontext.__gregs[_REG_PCOQH] = (pc);			\
	(uc)->uc_mcontext.__gregs[_REG_PCOQT] = (pc) + 4;		\
} while (/*CONSTCOND*/0)
#define	_UC_MACHINE_INTRV(uc) 	((uc)->uc_mcontext.__gregs[_REG_RET0])

#if defined(_RTLD_SOURCE) || defined(_LIBC_SOURCE) || \
    defined(__LIBPTHREAD_SOURCE__)
#include <sys/tls.h>

__BEGIN_DECLS
static __inline void *
__lwp_getprivate_fast(void)
{
	register void *__tmp;

	__asm volatile("mfctl\t27 /* CR_TLS */, %0" : "=r" (__tmp));

	return __tmp;
}
__END_DECLS

#endif

#endif /* !__ASSEMBLER__ */

#define	_UC_SETSTACK	0x00010000
#define	_UC_CLRSTACK	0x00020000
#define	_UC_TLSBASE	0x00040000

#endif /* _HPPA_MCONTEXT_H_ */
