/*	$NetBSD: wait.h,v 1.1.1.1.6.2 2011/01/09 20:43:05 riz Exp $	*/

#ifndef _cygwin_sys_wait_h

#include_next <sys/wait.h>

#if !defined (WCOREDUMP)
# define WCOREDUMP(x) (((x) & 0x80) == 0x80)
#endif

#endif
