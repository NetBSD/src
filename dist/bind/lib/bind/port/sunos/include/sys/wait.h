/*	$NetBSD: wait.h,v 1.1.1.1 2004/05/17 23:44:47 christos Exp $	*/

#ifndef _sunos_sys_wait_h

#include_next <sys/wait.h>

#define WCOREDUMP(x)	(((union __wait*)&(x))->__w_coredump)

#endif
