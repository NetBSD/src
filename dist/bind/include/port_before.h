/*	$NetBSD: port_before.h,v 1.3.4.1 2001/01/28 15:52:48 he Exp $	*/

#if 0
#define WANT_IRS_NIS
#define WANT_IRS_PW
#define WANT_IRS_GR
#endif
#define SIG_FN void
#if defined(HAS_PTHREADS) && defined(_REENTRANT)
#define DO_PTHREADS
#endif

#define SETGRENT_VOID
#define SETPWENT_VOID
