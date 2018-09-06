/*	$NetBSD: i915_trace_points.c,v 1.1.1.1.30.1 2018/09/06 06:56:17 pgoyette Exp $	*/

/*
 * Copyright © 2009 Intel Corporation
 *
 * Authors:
 *    Chris Wilson <chris@chris-wilson.co.uk>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i915_trace_points.c,v 1.1.1.1.30.1 2018/09/06 06:56:17 pgoyette Exp $");

#include "i915_drv.h"

#ifndef __CHECKER__
#define CREATE_TRACE_POINTS
#include "i915_trace.h"
#endif
