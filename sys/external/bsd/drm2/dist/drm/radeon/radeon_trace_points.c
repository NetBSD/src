/*	$NetBSD: radeon_trace_points.c,v 1.2 2018/08/27 04:58:36 riastradh Exp $	*/

/* Copyright Red Hat Inc 2010.
 * Author : Dave Airlie <airlied@redhat.com>
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: radeon_trace_points.c,v 1.2 2018/08/27 04:58:36 riastradh Exp $");

#include <drm/drmP.h>
#include <drm/radeon_drm.h>
#include "radeon.h"

#define CREATE_TRACE_POINTS
#include "radeon_trace.h"
