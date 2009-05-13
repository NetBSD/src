/*	$NetBSD: wait.h,v 1.1.1.1.2.2 2009/05/13 18:52:32 jym Exp $	*/

#ifndef _sunos_sys_wait_h

#include_next <sys/wait.h>

#define WCOREDUMP(x)	(((union __wait*)&(x))->__w_coredump)

#endif
