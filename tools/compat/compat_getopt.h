/*      $NetBSD: compat_getopt.h,v 1.1 2004/06/20 22:20:15 jmc Exp $ */

/* We unconditionally use the NetBSD getopt.h in libnbcompat. */

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#define option __nbcompat_option
#ifdef _GETOPT_H_
#undef _GETOPT_H_
#endif
#include "../../include/getopt.h"
