/*	$NetBSD: tinytest_local.h,v 1.1.1.1.4.2 2014/05/22 15:50:14 yamt Exp $	*/


#include "util-internal.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
