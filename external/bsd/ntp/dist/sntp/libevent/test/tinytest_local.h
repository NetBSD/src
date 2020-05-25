/*	$NetBSD: tinytest_local.h,v 1.5 2020/05/25 20:47:34 christos Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
