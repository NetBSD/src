/*	$NetBSD: i915_utils.c,v 1.4 2021/12/19 11:37:41 riastradh Exp $	*/

// SPDX-License-Identifier: MIT
/*
 * Copyright © 2019 Intel Corporation
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i915_utils.c,v 1.4 2021/12/19 11:37:41 riastradh Exp $");

#include <drm/drm_drv.h>

#include "i915_drv.h"
#include "i915_utils.h"

#ifdef __NetBSD__
#define	NBSD_BUG_URL "https://gnats.NetBSD.org/"
#define	NBSD_BUG_MSG							      \
	"Please file a bug at " NBSD_BUG_URL " in category kern"	      \
	" providing the dmesg log by booting with debug/verbose"	      \
	" as in `boot -vx'."
#else
#define FDO_BUG_URL "https://gitlab.freedesktop.org/drm/intel/-/wikis/How-to-file-i915-bugs"
#define FDO_BUG_MSG "Please file a bug on drm/i915; see " FDO_BUG_URL " for details."
#endif

void
__i915_printk(struct drm_i915_private *dev_priv, const char *level,
	      const char *fmt, ...)
{
#ifdef __NetBSD__
	static volatile unsigned done = 0;
	va_list va;

	va_start(va, fmt);
	printf("%s: %s ", device_xname(dev_priv->drm.dev), level);
	vprintf(fmt, va);
	va_end(va);

	if (strncmp(level, KERN_ERR, strlen(KERN_ERR)) == 0 &&
	    atomic_swap_uint(&done, 1) == 0)
		printf("%s\n", NBSD_BUG_MSG);
#else
	static bool shown_bug_once;
	struct device *kdev = dev_priv->drm.dev;
	bool is_error = level[1] <= KERN_ERR[1];
	bool is_debug = level[1] == KERN_DEBUG[1];
	struct va_format vaf;
	va_list args;

	if (is_debug && !drm_debug_enabled(DRM_UT_DRIVER))
		return;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	if (is_error)
		dev_printk(level, kdev, "%pV", &vaf);
	else
		dev_printk(level, kdev, "[" DRM_NAME ":%ps] %pV",
			   __builtin_return_address(0), &vaf);

	va_end(args);

	if (is_error && !shown_bug_once) {
		/*
		 * Ask the user to file a bug report for the error, except
		 * if they may have caused the bug by fiddling with unsafe
		 * module parameters.
		 */
		if (!test_taint(TAINT_USER))
			dev_notice(kdev, "%s", FDO_BUG_MSG);
		shown_bug_once = true;
	}
#endif
}

#if IS_ENABLED(CONFIG_DRM_I915_DEBUG)
static unsigned int i915_probe_fail_count;

int __i915_inject_probe_error(struct drm_i915_private *i915, int err,
			      const char *func, int line)
{
	if (i915_probe_fail_count >= i915_modparams.inject_probe_failure)
		return 0;

	if (++i915_probe_fail_count < i915_modparams.inject_probe_failure)
		return 0;

	__i915_printk(i915, KERN_INFO,
		      "Injecting failure %d at checkpoint %u [%s:%d]\n",
		      err, i915_modparams.inject_probe_failure, func, line);
	i915_modparams.inject_probe_failure = 0;
	return err;
}

bool i915_error_injected(void)
{
	return i915_probe_fail_count && !i915_modparams.inject_probe_failure;
}

#endif

void cancel_timer(struct timer_list *t)
{
#ifndef __NetBSD__
	if (!READ_ONCE(t->expires))
		return;
#endif

	del_timer(t);
#ifndef __NetBSD__
	WRITE_ONCE(t->expires, 0);
#endif
}

void set_timer_ms(struct timer_list *t, unsigned long timeout)
{
	if (!timeout) {
		cancel_timer(t);
		return;
	}

	timeout = msecs_to_jiffies_timeout(timeout);

	/*
	 * Paranoia to make sure the compiler computes the timeout before
	 * loading 'jiffies' as jiffies is volatile and may be updated in
	 * the background by a timer tick. All to reduce the complexity
	 * of the addition and reduce the risk of losing a jiffie.
	 */
	barrier();

	mod_timer(t, jiffies + timeout);
}
