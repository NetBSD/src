/*	$NetBSD: sysarch.h,v 1.3 1994/10/27 04:16:29 cgd Exp $	*/

#ifndef _I386_SYSARCH_H_
#define _I386_SYSARCH_H_

/*
 * Architecture specific syscalls (i386)
 */
#define I386_GET_LDT	0
#define I386_SET_LDT	1

#ifndef KERNEL
int i386_get_ldt __P((int, union descriptor *, int));
int i386_set_ldt __P((int, union descriptor *, int));
#endif

#endif /* !_I386_SYSARCH_H_ */
