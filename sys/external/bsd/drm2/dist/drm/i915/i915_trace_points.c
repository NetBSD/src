/*	$NetBSD: i915_trace_points.c,v 1.1.1.1.32.1 2019/06/10 22:08:05 christos Exp $	*/

/*
 * Copyright © 2009 Intel Corporation
 *
 * Authors:
 *    Chris Wilson <chris@chris-wilson.co.uk>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i915_trace_points.c,v 1.1.1.1.32.1 2019/06/10 22:08:05 christos Exp $");

#include "i915_drv.h"

#ifndef __CHECKER__
#define CREATE_TRACE_POINTS
#include "i915_trace.h"
#endif
