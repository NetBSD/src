/*	$NetBSD: drm_trace_points.c,v 1.1.1.3 2021/12/18 20:11:03 riastradh Exp $	*/


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_trace_points.c,v 1.1.1.3 2021/12/18 20:11:03 riastradh Exp $");

#include <drm/drm_file.h>

#define CREATE_TRACE_POINTS
#include "drm_trace.h"
