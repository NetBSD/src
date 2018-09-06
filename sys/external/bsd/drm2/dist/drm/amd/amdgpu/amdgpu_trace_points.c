/*	$NetBSD: amdgpu_trace_points.c,v 1.2.2.2 2018/09/06 06:56:10 pgoyette Exp $	*/

/* Copyright Red Hat Inc 2010.
 * Author : Dave Airlie <airlied@redhat.com>
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdgpu_trace_points.c,v 1.2.2.2 2018/09/06 06:56:10 pgoyette Exp $");

#include <drm/drmP.h>
#include <drm/amdgpu_drm.h>
#include "amdgpu.h"

#define CREATE_TRACE_POINTS
#include "amdgpu_trace.h"
