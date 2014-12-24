/*	$NetBSD: ppsapi_timepps.h,v 1.1.1.1.26.1 2014/12/24 00:05:21 riz Exp $	*/

/* ppsapi_timepps.h */

/*
 * This logic first tries to get the timepps.h file from a standard
 * location, and then from our include/ subdirectory.
 */

#ifdef HAVE_TIMEPPS_H
# include <timepps.h>
#else
# ifdef HAVE_SYS_TIMEPPS_H
#  include <sys/timepps.h>
# else
#  ifdef HAVE_CIOGETEV
#   include "timepps-SunOS.h"
#  else
#   ifdef HAVE_TIOCGPPSEV
#    include "timepps-Solaris.h"
#   else
#    ifdef TIOCDCDTIMESTAMP
#     include "timepps-SCO.h"
#    endif
#   endif
#  endif
# endif
#endif
