/*	$NetBSD: cpu.h,v 1.4 2002/03/03 14:28:50 uch Exp $	*/
#ifndef _EVBSH3_CPU_H_
#define _EVBSH3_CPU_H_

#include <sh3/cpu.h>

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_LOADANDRESET	2	/* load kernel image and reset */
#define	CPU_MAXID		3	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "load_and_reset", CTLTYPE_INT }, \
}
#endif /* _EVBSH3_CPU_H_ */
