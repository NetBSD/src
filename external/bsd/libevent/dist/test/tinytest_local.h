/*	$NetBSD: tinytest_local.h,v 1.1.1.1.4.2 2013/06/23 06:28:18 tls Exp $	*/

#ifdef WIN32
#include <winsock2.h>
#endif

#include "event2/util.h"
#include "util-internal.h"

#ifdef snprintf
#undef snprintf
#endif
#define snprintf evutil_snprintf
