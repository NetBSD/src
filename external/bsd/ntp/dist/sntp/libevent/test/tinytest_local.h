/*	$NetBSD: tinytest_local.h,v 1.1.1.1.6.1 2014/12/24 00:05:26 riz Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
