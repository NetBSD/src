/*	$NetBSD: amdgpu_trace_points.c,v 1.1.1.1 2018/08/27 01:34:44 riastradh Exp $	*/

/* Copyright Red Hat Inc 2010.
 * Author : Dave Airlie <airlied@redhat.com>
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdgpu_trace_points.c,v 1.1.1.1 2018/08/27 01:34:44 riastradh Exp $");

#include <drm/drmP.h>
#include <drm/amdgpu_drm.h>
#include "amdgpu.h"

#define CREATE_TRACE_POINTS
#include "amdgpu_trace.h"
