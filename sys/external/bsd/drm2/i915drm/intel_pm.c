/*	$NetBSD: intel_pm.c,v 1.1.2.5 2013/09/08 15:54:20 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* intel_pm.c stubs */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intel_pm.c,v 1.1.2.5 2013/09/08 15:54:20 riastradh Exp $");

#include "i915_drv.h"

int
__gen6_gt_wait_for_fifo(struct drm_i915_private *dev_priv __unused)
{
	return 0;
}

void
gen6_gt_check_fifodbg(struct drm_i915_private *dev_priv __unused)
{
}

/* XXX BEGIN KLUDGEY COPYPASTA FROM intel_pm.c */

/* There's a funny hw issue where the hw returns all 0 when reading from
 * GEN6_RP_INTERRUPT_LIMITS. Hence we always need to compute the desired value
 * ourselves, instead of doing a rmw cycle (which might result in us clearing
 * all limits and the gpu stuck at whatever frequency it is at atm).
 */
static u32
gen6_rps_limits(struct drm_i915_private *dev_priv, u8 *val)
{
	u32 limits;

	limits = 0;

	if (*val >= dev_priv->rps.max_delay)
		*val = dev_priv->rps.max_delay;
	limits |= dev_priv->rps.max_delay << 24;

	/* Only set the down limit when we've reached the lowest level to avoid
	 * getting more interrupts, otherwise leave this clear. This prevents a
	 * race in the hw when coming out of rc6: There's a tiny window where
	 * the hw runs at the minimal clock before selecting the desired
	 * frequency, if the down threshold expires in that window we will not
	 * receive a down interrupt. */
	if (*val <= dev_priv->rps.min_delay) {
		*val = dev_priv->rps.min_delay;
		limits |= dev_priv->rps.min_delay << 16;
	}

	return limits;
}

void
gen6_set_rps(struct drm_device *dev, u8 val)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 limits = gen6_rps_limits(dev_priv, &val);

	WARN_ON(!mutex_is_locked(&dev_priv->rps.hw_lock));
	WARN_ON(val > dev_priv->rps.max_delay);
	WARN_ON(val < dev_priv->rps.min_delay);

	if (val == dev_priv->rps.cur_delay)
		return;

	I915_WRITE(GEN6_RPNSWREQ,
		   GEN6_FREQUENCY(val) |
		   GEN6_OFFSET(0) |
		   GEN6_AGGRESSIVE_TURBO);

	/* Make sure we continue to get interrupts
	 * until we hit the minimum or maximum frequencies.
	 */
	I915_WRITE(GEN6_RP_INTERRUPT_LIMITS, limits);

	POSTING_READ(GEN6_RPNSWREQ);

	dev_priv->rps.cur_delay = val;

	trace_intel_gpu_freq_change(val * 50);
}

/* XXX END KLUDGEY COPYPASTA FROM intel_pm.c */

void
i915_update_gfx_val(struct drm_i915_private *dev_priv)
{
	KASSERT(dev_priv->info->gen != 5); /* XXX gen<6 */
}

void
intel_disable_fbc(struct drm_device *dev __unused)
{
}

void
intel_disable_gt_powersave(struct drm_device *dev __unused)
{
}

void
intel_enable_gt_powersave(struct drm_device *dev __unused)
{
}

void
intel_gpu_ips_init(struct drm_i915_private *dev_priv)
{
}

void
intel_gpu_ips_teardown(void)
{
}

void
intel_gt_init(struct drm_device *dev __unused)
{
}

void
intel_gt_reset(struct drm_device *dev __unused)
{
}

void
intel_init_clock_gating(struct drm_device *dev __unused)
{
}

void
intel_init_pm(struct drm_device *dev __unused)
{
}

void
intel_init_power_wells(struct drm_device *dev __unused)
{
}

void
intel_update_fbc(struct drm_device *dev __unused)
{
}

void
intel_update_linetime_watermarks(struct drm_device *dev __unused,
    int pipe __unused, struct drm_display_mode *mode __unused)
{
}

void
intel_update_sprite_watermarks(struct drm_device *dev __unused,
    int pipe __unused, uint32_t sprite_width __unused, int pixel_size __unused)
{
}

void
intel_update_watermarks(struct drm_device *dev __unused)
{
}

/*
 * XXX Kludge!  Copypasta from intel_pm.c until we can use the whole
 * thing.
 */
spinlock_t mchdev_lock;

bool
ironlake_set_drps(struct drm_device *dev, u8 val)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u16 rgvswctl;

	assert_spin_locked(&mchdev_lock);

	rgvswctl = I915_READ16(MEMSWCTL);
	if (rgvswctl & MEMCTL_CMD_STS) {
		DRM_DEBUG("gpu busy, RCS change rejected\n");
		return false; /* still busy with another command */
	}

	rgvswctl = (MEMCTL_CMD_CHFREQ << MEMCTL_CMD_SHIFT) |
		(val << MEMCTL_FREQ_SHIFT) | MEMCTL_SFCAVM;
	I915_WRITE16(MEMSWCTL, rgvswctl);
	POSTING_READ16(MEMSWCTL);

	rgvswctl |= MEMCTL_CMD_STS;
	I915_WRITE16(MEMSWCTL, rgvswctl);

	return true;
}

void
ironlake_teardown_rc6(struct drm_device *dev __unused)
{
}
