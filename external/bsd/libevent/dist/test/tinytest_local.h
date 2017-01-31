/*	$NetBSD: tinytest_local.h,v 1.1.1.2 2017/01/31 21:14:53 christos Exp $	*/

#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
