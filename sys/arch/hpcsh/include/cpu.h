/*	$NetBSD: cpu.h,v 1.3 2002/03/03 14:28:50 uch Exp $	*/
#ifndef _HPCSH_CPU_H_
#define _HPCSH_CPU_H_

#include <sh3/cpu.h>

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_MAXID		2	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}

#endif /* _HPCSH_CPU_H_ */
