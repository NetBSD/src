/*	$NetBSD: cpu.h,v 1.2 2002/03/03 14:28:49 uch Exp $	*/
#ifndef _DREAMCAST_CPU_H_
#define _DREAMCAST_CPU_H_

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

#endif /* _DREAMCAST_CPU_H_ */
