/*	$NetBSD: port_before.h,v 1.2.2.2 1999/12/04 17:08:40 he Exp $	*/

/*
#define WANT_IRS_NIS
#define WANT_IRS_PW
#define WANT_IRS_GR
*/
#define SIG_FN void
#if defined(HAS_PTHREADS) && defined(_REENTRANT)
#define DO_PTHREADS
#endif

#define SETGRENT_VOID
#define SETPWENT_VOID

#include "namespace.h"
