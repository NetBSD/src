/*	$NetBSD: tinytest_local.h,v 1.1.1.1.16.1 2017/03/20 06:52:23 pgoyette Exp $	*/

#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
