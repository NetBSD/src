/*	$NetBSD: tinytest_local.h,v 1.1.1.7 2020/05/25 20:40:14 christos Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
