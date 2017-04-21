/*	$NetBSD: tinytest_local.h,v 1.1.1.1.20.1 2017/04/21 16:51:33 bouyer Exp $	*/

#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
