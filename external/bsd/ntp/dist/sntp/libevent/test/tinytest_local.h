/*	$NetBSD: tinytest_local.h,v 1.2.8.2 2015/01/07 04:45:39 msaitoh Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
