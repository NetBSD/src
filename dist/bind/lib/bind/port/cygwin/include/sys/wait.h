/*	$NetBSD: wait.h,v 1.1.1.1 2004/05/17 23:44:46 christos Exp $	*/

#ifndef _cygwin_sys_wait_h

#include_next <sys/wait.h>

#if !defined (WCOREDUMP)
# define WCOREDUMP(x) (((x) & 0x80) == 0x80)
#endif

#endif
