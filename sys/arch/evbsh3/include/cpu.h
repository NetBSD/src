/*	$NetBSD: cpu.h,v 1.1.8.1 1999/12/27 18:31:54 wrstuden Exp $	*/
#ifndef _EVBSH3_CPU_H_
#define _EVBSH3_CPU_H_

#include <sh3/cpu.h>

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_NKPDE		2	/* int: number of kernel PDEs */
#define	CPU_BOOTED_KERNEL	3	/* string: booted kernel name */
#define	CPU_SETPRIVPROC		4	/* set current proc to piviledged proc
					   */
#define	CPU_DEBUGMODE		5	/* set debug mode */
#define	CPU_LOADANDRESET	6	/* load kernel image and reset */
#define	CPU_MAXID		7	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "nkpde", CTLTYPE_INT }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
	{ "set_priv_proc", CTLTYPE_INT }, \
	{ "debug_mode", CTLTYPE_INT }, \
	{ "load_and_reset", CTLTYPE_INT }, \
}

#endif /* _EVBSH3_CPU_H_ */
