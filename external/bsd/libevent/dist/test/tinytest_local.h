/*	$NetBSD: tinytest_local.h,v 1.1.1.1.10.2 2014/05/22 15:48:11 yamt Exp $	*/

#ifdef WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"
#include "util-internal.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
