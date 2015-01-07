/*	$NetBSD: tinytest_local.h,v 1.2.12.2 2015/01/07 12:13:38 msaitoh Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
