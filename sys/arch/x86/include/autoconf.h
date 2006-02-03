/*	$NetBSD: autoconf.h,v 1.1 2006/02/03 23:33:30 jmmv Exp $	*/

#ifndef _X86_AUTOCONF_H_
#define _X86_AUTOCONF_H_

void x86_cpu_rootconf(void);
void x86_matchbiosdisks(void);
const char *x86_findbiosdisk(int);

#endif /* _X86_AUTOCONF_H_ */
