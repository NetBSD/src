/*	$NetBSD: port_before.h,v 1.2.2.3 2000/03/01 14:00:31 he Exp $	*/

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
