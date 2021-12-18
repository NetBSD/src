/*	$NetBSD: radeon_trace_points.c,v 1.3 2021/12/18 23:45:43 riastradh Exp $	*/

// SPDX-License-Identifier: MIT
/* Copyright Red Hat Inc 2010.
 * Author : Dave Airlie <airlied@redhat.com>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: radeon_trace_points.c,v 1.3 2021/12/18 23:45:43 riastradh Exp $");

#include <drm/radeon_drm.h>
#include "radeon.h"

#define CREATE_TRACE_POINTS
#include "radeon_trace.h"
