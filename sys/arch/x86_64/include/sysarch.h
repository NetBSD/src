/*	$NetBSD: sysarch.h,v 1.1 2001/06/19 00:20:13 fvdl Exp $	*/

#if 0
/*
 * XXXfvdl todo.
 */
#ifndef _X86_64_SYSARCH_H_
#define _X86_64_SYSARCH_H_

/*
 * Architecture specific syscalls (x86_64)
 */
#define X86_64_GET_LDT	0
#define X86_64_SET_LDT	1
#define	X86_64_IOPL	2
#define	X86_64_GET_IOPERM	3
#define	X86_64_SET_IOPERM	4
#define	X86_64_VM86	5
#define	X86_64_PMC_INFO	8
#define	X86_64_PMC_STARTSTOP 9
#define	X86_64_PMC_READ	10

struct x86_64_get_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

struct x86_64_set_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

struct x86_64_iopl_args {
	int iopl;
};

struct x86_64_get_ioperm_args {
	u_long *iomap;
};

struct x86_64_set_ioperm_args {
	u_long *iomap;
};

struct x86_64_pmc_info_args {
	int	type;
	int	flags;
};

#define	PMC_TYPE_NONE		0
#define	PMC_TYPE_I586		1
#define	PMC_TYPE_I686		2

#define	PMC_INFO_HASTSC		0x01

#define	PMC_NCOUNTERS		2

struct x86_64_pmc_startstop_args {
	int counter;
	u_int64_t val;
	u_int8_t event;
	u_int8_t unit;
	u_int8_t compare;
	u_int8_t flags;
};

#define	PMC_SETUP_KERNEL	0x01
#define	PMC_SETUP_USER		0x02
#define	PMC_SETUP_EDGE		0x04
#define	PMC_SETUP_INV		0x08

struct x86_64_pmc_read_args {
	int counter;
	u_int64_t val;
	u_int64_t time;
};

#ifndef _KERNEL
int x86_64_get_ldt __P((int, union descriptor *, int));
int x86_64_set_ldt __P((int, union descriptor *, int));
int x86_64_iopl __P((int));
int x86_64_get_ioperm __P((u_long *));
int x86_64_set_ioperm __P((u_long *));
int x86_64_pmc_info __P((struct x86_64_pmc_info_args *));
int x86_64_pmc_startstop __P((struct x86_64_pmc_startstop_args *));
int x86_64_pmc_read __P((struct x86_64_pmc_read_args *));
int sysarch __P((int, void *));
#endif

#endif /* !_X86_64_SYSARCH_H_ */
#endif /* 0 */
