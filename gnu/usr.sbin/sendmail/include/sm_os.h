/* $NetBSD: sm_os.h,v 1.2 2003/03/25 03:55:38 atatat Exp $ */

# if defined(__NetBSD__) && defined(__NetBSD_Version__) && __NetBSD_Version__ >= 104170000
#  define HASSETUSERCONTEXT	1	/* BSDI-style login classes */
# endif
