/*	$NetBSD: port_before.h,v 1.5 2002/06/20 11:43:01 itojun Exp $	*/

#if 0
#define WANT_IRS_NIS
#define WANT_IRS_PW
#define HAVE_PW_CLASS
#define WANT_IRS_GR
#endif
#define SIG_FN void
#define ISC_SOCKLEN_T int
#if defined(HAS_PTHREADS) && defined(_REENTRANT)
#define DO_PTHREADS
#endif

#define SETGRENT_VOID
#define SETPWENT_VOID
#define GETGROUPLIST_ARGS const char *name, gid_t basegid, gid_t *groups, \
		      int *ngroups
#ifdef __GNUC__
#define ISC_FORMAT_PRINTF(fmt, args) \
	__attribute__((__format__(__printf__, fmt, args)))
#else
#define ISC_FORMAT_PRINTF(fmt, args)
#endif
