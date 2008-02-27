/*	$NetBSD: sysctl.h,v 1.2 2008/02/27 18:26:15 xtraeme Exp $	*/

/*
 * CTL_MACHDEP definitions.  (Common to all m68k ports.)
 * This should be included by each m68k port's cpu.h so
 * /usr/sbin/sysctl can be shared on all of them.
 */
#ifndef CTL_MACHDEP_NAMES

#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_ROOT_DEVICE		2	/* string: root device name */
#define	CPU_BOOTED_KERNEL	3	/* string: booted kernel name */
#define	CPU_MAXID		4	/* number of valid machdep ids */

#endif	/* CTL_MACHDEP_NAMES */
