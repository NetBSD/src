/*	$NetBSD: sysarch.h,v 1.12 2002/06/07 04:04:47 gmcgarry Exp $	*/

#ifndef _I386_SYSARCH_H_
#define _I386_SYSARCH_H_

/*
 * Architecture specific syscalls (i386)
 */
#define I386_GET_LDT	0
#define I386_SET_LDT	1
#define	I386_IOPL	2
#define	I386_GET_IOPERM	3
#define	I386_SET_IOPERM	4
#define	I386_VM86	5
#define	I386_PMC_INFO	8
#define	I386_PMC_STARTSTOP 9
#define	I386_PMC_READ	10
#define I386_GET_MTRR	11
#define I386_SET_MTRR	12

struct i386_get_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

struct i386_set_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

struct i386_get_mtrr_args {
	struct mtrr *mtrrp;
	int *n;
};

struct i386_set_mtrr_args {
	struct mtrr *mtrrp;
	int *n;
};

struct i386_iopl_args {
	int iopl;
};

struct i386_get_ioperm_args {
	u_long *iomap;
};

struct i386_set_ioperm_args {
	u_long *iomap;
};

struct i386_pmc_info_args {
	int	type;
	int	flags;
};

#define	PMC_TYPE_NONE		0
#define	PMC_TYPE_I586		1
#define	PMC_TYPE_I686		2
#define	PMC_TYPE_K7		3

#define	PMC_INFO_HASTSC		0x01

#define	PMC_NCOUNTERS		4

struct i386_pmc_startstop_args {
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

struct i386_pmc_read_args {
	int counter;
	u_int64_t val;
	u_int64_t time;
};

struct mtrr;

#ifndef _KERNEL
int i386_get_ldt __P((int, union descriptor *, int));
int i386_set_ldt __P((int, union descriptor *, int));
int i386_iopl __P((int));
int i386_get_ioperm __P((u_long *));
int i386_set_ioperm __P((u_long *));
int i386_pmc_info __P((struct i386_pmc_info_args *));
int i386_pmc_startstop __P((struct i386_pmc_startstop_args *));
int i386_pmc_read __P((struct i386_pmc_read_args *));
int i386_set_mtrr __P((struct mtrr *, int *));
int i386_get_mtrr __P((struct mtrr *, int *));
int sysarch __P((int, void *));
#endif

#endif /* !_I386_SYSARCH_H_ */
