/*
 * Copyright (c) 2007 Danger Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Danger Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _SYS_HWCLOCK_H_
#define _SYS_HWCLOCK_H_

#include <sys/queue.h>
#include "opt_hwclock_machine.h"

#if defined(HWCLOCK_MACHINE)
#include HWCLOCK_MACHINE
#else
#error "HWCLOCK_MACHINE not defined"
#endif

#define HWCLOCK_DISABLED 0
#define HWCLOCK_ENABLED 1

enum hwclock_constraint_type {
	HWCLOCK_FREQ_CONSTRAINT,
	HWCLOCK_ENABLE_CONSTRAINT,
};

struct hwclock_constraint {
	enum hwclock_constraint_type type;
	unsigned long freqcons_min;
	unsigned long freqcons_max;
	unsigned long enabcons_val;
	LIST_ENTRY(hwclock_constraint) clkconstraints;
};


struct hwclock {
	const char *name;
	struct hwclock_machine machine;
	unsigned long freqcons_min;
	unsigned long freqcons_max;
	int (*setfreq)(struct hwclock *hwc, unsigned long freq);
	int (*getfreq)(struct hwclock *hwc, unsigned long *freq);
	int (*setenable)(struct hwclock *hwc, unsigned long val);
	int (*getenable)(struct hwclock *hwc, unsigned long *val);
	int (*setattr)(struct hwclock *hwc, enum hwclock_arch_attr attr,
				   unsigned long val);
	int (*parent_freq_req)(struct hwclock *hwc, unsigned long fcmin,
						   unsigned long fcmax, unsigned long *pfcmin,
						   unsigned long *pfcmax);
	void (*init)(struct hwclock *hwc);
	LIST_HEAD(, hwclock_constraint) constraints;
	LIST_ENTRY(hwclock) clks;
	struct hwclock *parent;
	LIST_HEAD(, hwclock) children;
	LIST_ENTRY(hwclock) siblings;
};

enum hwclock_policy_action {
	FREQ_CONSTRAINT_CHANGE,
	ENABLE_CONSTRAINT_DROP,
};

typedef int (*hwclock_policy_mgr_t)(struct hwclock *hwc,
									enum hwclock_policy_action action);

extern void hwclock_register(struct hwclock *hwc);
extern void hwclock_unregister(struct hwclock *hwc);
extern int hwclock_getfreq(const char *clockname, unsigned long *freq);
extern int hwclock_setfreq(const char *clockname, unsigned long freq);
extern int hwclock_constrain_freq(const char *clockname, unsigned long cmin,
								  unsigned long cmax);
extern int hwclock_unconstrain_freq(const char *clockname, unsigned long cmin,
									unsigned long cmax);
extern int hwclock_getenable(const char *clockname, unsigned long *value);
extern int hwclock_setenable(const char *clockname, unsigned long value);
extern int hwclock_constrain_enable(const char *clockname,
									unsigned long value);
extern int hwclock_unconstrain_enable(const char *clockname,
									  unsigned long value);
extern int hwclock_setattr(const char *clockname, enum hwclock_arch_attr attr,
						   unsigned long value);
extern int hwclock_register_policy_manager(hwclock_policy_mgr_t mgr);

#endif /* _SYS_HWCLOCK_H_ */
