/*	$NetBSD: mcontext.h,v 1.7.6.1 2014/08/20 00:03:05 tls Exp $	*/

#ifndef _HPPA_MCONTEXT_H_
#define	_HPPA_MCONTEXT_H_

/*
 * General register state
 */
#define	_NGREG		44

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
#define	_UC_MACHINE_PC(uc) 	((uc)->uc_mcontext.__gregs[_REG_PCOQH])
#define	_UC_MACHINE_SET_PC(uc, pc)					\
do {									\
	(uc)->uc_mcontext.__gregs[_REG_PCOQH] = (pc);			\
	(uc)->uc_mcontext.__gregs[_REG_PCOQT] = (pc) + 4;		\
} while (/*CONSTCOND*/0)

static __inline void *
__lwp_getprivate_fast(void)
{
	register void *__tmp;

	__asm volatile("mfctl\t27 /* CR_TLS */, %0" : "=r" (__tmp));

	return __tmp;
}

#endif /* !__ASSEMBLER__ */

#define	_UC_SETSTACK	0x00010000
#define	_UC_CLRSTACK	0x00020000
#define	_UC_TLSBASE	0x00040000

#endif /* _HPPA_MCONTEXT_H_ */
