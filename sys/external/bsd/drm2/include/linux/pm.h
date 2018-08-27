/*	$NetBSD: pm.h,v 1.5 2018/08/27 06:07:20 riastradh Exp $	*/

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

#ifndef _LINUX_PM_H_
#define _LINUX_PM_H_

/* Silly stub for i915_suspend.  */

typedef struct {
	int event;
} pm_message_t;

#define	PM_EVENT_PRETHAW		0
#define	PM_EVENT_SUSPEND		1
#define	PM_EVENT_FREEZE			2

struct dev_pm_domain {
	char dummy;		/* XXX */
};

struct dev_pm_ops {
	int	(*resume)(struct device *);
	int	(*resume_early)(struct device *);
	int	(*suspend)(struct device *);
	int	(*suspend_late)(struct device *);
	int	(*freeze)(struct device *);
	int	(*freeze_late)(struct device *);
	int	(*thaw_early)(struct device *);
	int	(*thaw)(struct device *);
	int	(*poweroff)(struct device *);
	int	(*poweroff_late)(struct device *);
	int	(*restore_early)(struct device *);
	int	(*restore)(struct device *);
	int	(*runtime_suspend)(struct device *);
	int	(*runtime_resume)(struct device *);
};

#endif  /* _LINUX_PM_H_ */
