/*	$NetBSD: wait.h,v 1.1.1.1.6.2 2011/01/09 20:43:21 riz Exp $	*/

#ifndef _sunos_sys_wait_h

#include_next <sys/wait.h>

#define WCOREDUMP(x)	(((union __wait*)&(x))->__w_coredump)

#endif
