/*	$NetBSD: ppsapi_timepps.h,v 1.5 2020/05/25 20:47:25 christos Exp $	*/

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
