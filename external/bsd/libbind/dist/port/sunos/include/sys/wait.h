/*	$NetBSD: wait.h,v 1.1.1.1.4.2 2011/01/06 21:42:42 riz Exp $	*/

#ifndef _sunos_sys_wait_h

#include_next <sys/wait.h>

#define WCOREDUMP(x)	(((union __wait*)&(x))->__w_coredump)

#endif
