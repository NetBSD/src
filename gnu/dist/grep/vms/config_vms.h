/*	$NetBSD: config_vms.h,v 1.1.1.1 2003/01/26 23:27:31 wiz Exp $	*/

/* Configuration file for OpenVMS */

#define HAVE_STRING_H 1

#define HAVE_MEMCHAR

#define HAVE_STRERROR

#define HAVE_STDLIB_H 1

#define HAVE_UNISTD_H 1

#define STDC_HEADERS

#define HAVE_DIRENT_H 1

#define VERSION "2.4.1"
/* Avoid namespace collision with operating system supplied C library */

/* Make sure we have the C-RTL definitions */
#include <unistd.h>
#include <stdio.h>

/* Now override everything with the GNU version */
#ifdef VMS
# define getopt gnu_getopt
# define optarg gnu_optarg
# define optopt gnu_optopt
# define optind gnu_optind
# define opterr gnu_opterr
#endif

#if defined(VMS) && defined(__DECC) /* need function prototype */
#if (__DECC_VER<50790004)           /* have an own one         */
char *alloca(unsigned int);
#else
#define alloca __ALLOCA
#endif
#endif
