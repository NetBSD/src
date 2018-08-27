/*	$NetBSD: drm_trace_points.c,v 1.3 2018/08/27 04:58:19 riastradh Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_trace_points.c,v 1.3 2018/08/27 04:58:19 riastradh Exp $");

#include <drm/drmP.h>

#define CREATE_TRACE_POINTS
#include "drm_trace.h"
