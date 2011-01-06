/*	$NetBSD: wait.h,v 1.1.1.1.4.2 2011/01/06 21:42:27 riz Exp $	*/

#ifndef _cygwin_sys_wait_h

#include_next <sys/wait.h>

#if !defined (WCOREDUMP)
# define WCOREDUMP(x) (((x) & 0x80) == 0x80)
#endif

#endif
