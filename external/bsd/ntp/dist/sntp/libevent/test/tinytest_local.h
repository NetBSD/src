/*	$NetBSD: tinytest_local.h,v 1.1.1.1 2013/12/27 23:31:27 christos Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
