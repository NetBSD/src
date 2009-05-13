/*	$NetBSD: wait.h,v 1.1.1.1.2.2 2009/05/13 18:52:20 jym Exp $	*/

#ifndef _cygwin_sys_wait_h

#include_next <sys/wait.h>

#if !defined (WCOREDUMP)
# define WCOREDUMP(x) (((x) & 0x80) == 0x80)
#endif

#endif
