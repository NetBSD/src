/*	$NetBSD: drm_random.c,v 1.2 2021/12/18 23:45:31 riastradh Exp $	*/

// SPDX-License-Identifier: GPL-2.0
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_random.c,v 1.2 2021/12/18 23:45:31 riastradh Exp $");

#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "drm_random.h"

static inline u32 drm_prandom_u32_max_state(u32 ep_ro, struct rnd_state *state)
{
	return upper_32_bits((u64)prandom_u32_state(state) * ep_ro);
}

void drm_random_reorder(unsigned int *order, unsigned int count,
			struct rnd_state *state)
{
	unsigned int i, j;

	for (i = 0; i < count; ++i) {
		BUILD_BUG_ON(sizeof(unsigned int) > sizeof(u32));
		j = drm_prandom_u32_max_state(count, state);
		swap(order[i], order[j]);
	}
}
EXPORT_SYMBOL(drm_random_reorder);

unsigned int *drm_random_order(unsigned int count, struct rnd_state *state)
{
	unsigned int *order, i;

	order = kmalloc_array(count, sizeof(*order), GFP_KERNEL);
	if (!order)
		return order;

	for (i = 0; i < count; i++)
		order[i] = i;

	drm_random_reorder(order, count, state);
	return order;
}
EXPORT_SYMBOL(drm_random_order);
