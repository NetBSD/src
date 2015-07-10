/*	$NetBSD: tinytest_local.h,v 1.3 2015/07/10 14:20:35 christos Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
