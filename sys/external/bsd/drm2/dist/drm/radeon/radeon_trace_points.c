/*	$NetBSD: radeon_trace_points.c,v 1.1.1.1.32.1 2019/06/10 22:08:27 christos Exp $	*/

/* Copyright Red Hat Inc 2010.
 * Author : Dave Airlie <airlied@redhat.com>
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: radeon_trace_points.c,v 1.1.1.1.32.1 2019/06/10 22:08:27 christos Exp $");

#include <drm/drmP.h>
#include <drm/radeon_drm.h>
#include "radeon.h"

#define CREATE_TRACE_POINTS
#include "radeon_trace.h"
