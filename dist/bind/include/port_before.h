/*	$NetBSD: port_before.h,v 1.4 2001/01/27 07:22:02 itojun Exp $	*/

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
