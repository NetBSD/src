/*	$NetBSD: mock_timeline.c,v 1.2 2021/12/18 23:45:31 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2017-2018 Intel Corporation
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mock_timeline.c,v 1.2 2021/12/18 23:45:31 riastradh Exp $");

#include "../intel_timeline.h"

#include "mock_timeline.h"

void mock_timeline_init(struct intel_timeline *timeline, u64 context)
{
	timeline->gt = NULL;
	timeline->fence_context = context;

	mutex_init(&timeline->mutex);

	INIT_ACTIVE_FENCE(&timeline->last_request);
	INIT_LIST_HEAD(&timeline->requests);

	i915_syncmap_init(&timeline->sync);

	INIT_LIST_HEAD(&timeline->link);
}

void mock_timeline_fini(struct intel_timeline *timeline)
{
	i915_syncmap_free(&timeline->sync);
}
