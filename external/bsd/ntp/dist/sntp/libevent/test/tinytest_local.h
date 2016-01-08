/*	$NetBSD: tinytest_local.h,v 1.4 2016/01/08 21:35:41 christos Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
