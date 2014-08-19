/*	$NetBSD: tinytest_local.h,v 1.1.1.1.8.2 2014/08/19 23:51:47 tls Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
