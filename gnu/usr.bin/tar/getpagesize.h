/*	$Id: getpagesize.h,v 1.2 1993/08/02 17:48:49 mycroft Exp $ */

#ifdef BSD
#ifndef BSD4_1
#define HAVE_GETPAGESIZE
#endif
#endif

#ifndef HAVE_GETPAGESIZE

#ifdef VMS
#define getpagesize() 512
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _SC_PAGESIZE
#define getpagesize() sysconf(_SC_PAGESIZE)
#else

#include <sys/param.h>

#ifdef EXEC_PAGESIZE
#define getpagesize() EXEC_PAGESIZE
#else
#ifdef NBPG
#define getpagesize() NBPG * CLSIZE
#ifndef CLSIZE
#define CLSIZE 1
#endif /* no CLSIZE */
#else /* no NBPG */
#define getpagesize() NBPC
#endif /* no NBPG */
#endif /* no EXEC_PAGESIZE */
#endif /* no _SC_PAGESIZE */

#endif /* not HAVE_GETPAGESIZE */

