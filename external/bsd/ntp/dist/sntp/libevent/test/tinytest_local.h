/*	$NetBSD: tinytest_local.h,v 1.2.4.2 2014/12/25 02:28:17 snj Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
