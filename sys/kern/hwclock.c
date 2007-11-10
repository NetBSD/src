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

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/hwclock.h>

static LIST_HEAD(, hwclock) hwclocks = LIST_HEAD_INITIALIZER(hwclocks);
struct simplelock hwclocks_lock = SIMPLELOCK_INITIALIZER;

static hwclock_policy_mgr_t policy_mgr;

/* Caller must hold hwclocks_lock */

static struct hwclock *clocknamelookup(const char *clockname)
{
	struct hwclock *hwc;

	LIST_FOREACH(hwc, &hwclocks, clks) {
		if (strcmp(clockname, hwc->name) == 0)
			break;
	}

	return hwc;
}


int hwclock_getfreq(const char *clockname, unsigned long *freq)
{
	struct hwclock *hwc;
	int ret = 0;

	simple_lock(&hwclocks_lock);

	if (((hwc = clocknamelookup(clockname)) == NULL) ||
		(! hwc->getfreq)) {
		ret = EINVAL;
		goto out;
	}

	ret = (*hwc->getfreq)(hwc, freq);

out:
	simple_unlock(&hwclocks_lock);
	return ret;
}


int hwclock_setfreq(const char *clockname, unsigned long freq)
{
	struct hwclock *hwc;
	int ret = 0;

	simple_lock(&hwclocks_lock);

	if (((hwc = clocknamelookup(clockname)) == NULL) ||
		(! hwc->setfreq)) {
		ret = EINVAL;
		goto out;
	}

	// todo: Check constraints?

	if ((*hwc->setfreq)(hwc, freq) == 0) {
		/* Just set "current constraints" to the new value */
		hwc->freqcons_min = freq;
		hwc->freqcons_max = freq;
	} else {
		ret = EINVAL;
		goto out;
	}


out:
	simple_unlock(&hwclocks_lock);
	return ret;
}


int hwclock_getenable(const char *clockname, unsigned long *value)
{
	struct hwclock *hwc;
	int ret = 0;

	simple_lock(&hwclocks_lock);

	if (((hwc = clocknamelookup(clockname)) == NULL) ||
		(! hwc->getenable)) {
		ret = EINVAL;
		goto out;
	}

	ret = (*hwc->getenable)(hwc, value);

out:
	simple_unlock(&hwclocks_lock);
	return ret;
}


int hwclock_setenable(const char *clockname, unsigned long value)
{
	struct hwclock *hwc;
	int ret = 0;

	simple_lock(&hwclocks_lock);

	if (((hwc = clocknamelookup(clockname)) == NULL)
		|| (! hwc->setenable)) {
		ret = EINVAL;
		goto out;
	}

	// todo: check constraints?

	ret = hwc->setenable(hwc, value);

out:
	simple_unlock(&hwclocks_lock);
	return ret;
}


static int constrain_freq_tree(struct hwclock *hwc, unsigned long fcmin,
							   unsigned long fcmax)
{
	struct hwclock_constraint *constraint;
	unsigned long newmin, newmax;
	int ret = 0;

	if (hwc->parent && hwc->parent->setfreq && hwc->parent_freq_req) {
		unsigned long pfcmin, pfcmax;

		if ((ret = hwc->parent_freq_req(hwc, fcmin, fcmax, &pfcmin, &pfcmax))
			!= 0)
			return ret;

		if ((ret = constrain_freq_tree(hwc->parent, pfcmin, pfcmax)) != 0)
			return ret;
	}

	if ((constraint = malloc(sizeof(struct hwclock_constraint), M_TEMP,
							 M_WAITOK)) == NULL)
		ret = ENOMEM;
	else {
		constraint->type = HWCLOCK_FREQ_CONSTRAINT;
		constraint->freqcons_min = fcmin;
		constraint->freqcons_max = fcmax;
		LIST_INSERT_HEAD(&hwc->constraints, constraint, clkconstraints);
	}

	newmin = hwc->freqcons_min;
	newmax = hwc->freqcons_max;

	if (fcmin > newmin)
		newmin = fcmin;

	if (fcmax < newmax)
		newmax = fcmax;

	if (newmin != hwc->freqcons_min || newmax != hwc->freqcons_max) {
		hwc->freqcons_min = newmin;
		hwc->freqcons_max = newmax;

		if (policy_mgr && (newmin != newmax))
			ret = (*policy_mgr)(hwc, FREQ_CONSTRAINT_CHANGE);
		else if (hwc->getfreq && hwc->setfreq) {
			unsigned long curfreq;

			/*
			 * In the absence of a policy maker: if the current clock
			 * rate is outside the new constraints, set it to the minimum
			 * value that satisfies the constraints.
			 */

			if ((hwc->getfreq(hwc, &curfreq) == 0) &&
				((curfreq < newmin) || (curfreq > newmax)))
				ret = (*hwc->setfreq)(hwc, newmin);
		}
	}


	return ret;
}

int hwclock_constrain_freq(const char *clockname, unsigned long fcmin,
						   unsigned long fcmax)
{
	struct hwclock *hwc;
	int ret = 0;

	simple_lock(&hwclocks_lock);

	if ((hwc = clocknamelookup(clockname)) == NULL) {
		ret = EINVAL;
		goto out;
	}

	ret = constrain_freq_tree(hwc, fcmin, fcmax);

out:
	simple_unlock(&hwclocks_lock);
	return ret;
}


static int unconstrain_freq_tree(struct hwclock *hwc, unsigned long fcmin,
								 unsigned long fcmax)
{
	struct hwclock_constraint *constraint;
	struct hwclock_constraint *delconstraint = NULL;
	unsigned long newmin, newmax;
	int ret = 0;

	newmin = 0;
	newmax = (unsigned long) -1;

	LIST_FOREACH(constraint, &hwc->constraints, clkconstraints) {
		if ((delconstraint == NULL) &&
			(constraint->type == HWCLOCK_FREQ_CONSTRAINT) &&
			((constraint->freqcons_min == fcmin) &&
			 (constraint->freqcons_max == fcmax))) {
			delconstraint = constraint;
		} else {
			if (constraint->freqcons_min > newmin)
				newmin = constraint->freqcons_min;
			if (constraint->freqcons_max < newmax)
				newmax = constraint->freqcons_max;
		}
	}

	if (delconstraint != NULL) {
		LIST_REMOVE(delconstraint, clkconstraints);
		free(delconstraint, M_TEMP);
	} else {
		printf("Error: clock %s freq constraint %lu-%lu not found.\n",
			   hwc->name, fcmin, fcmax);
		return EINVAL;
	}

	if (hwc->freqcons_min != newmin || hwc->freqcons_max != newmax) {
		hwc->freqcons_min = newmin;
		hwc->freqcons_max = newmax;

		if (policy_mgr && (newmin != newmax))
			ret = (*policy_mgr)(hwc, FREQ_CONSTRAINT_CHANGE);
		else if (hwc->setfreq && (newmin != 0))
			ret = (*hwc->setfreq)(hwc, newmin);
	}

	if (hwc->parent && hwc->parent->setfreq && hwc->parent_freq_req) {
		unsigned long pfcmin, pfcmax;

		if ((ret = hwc->parent_freq_req(hwc, fcmin, fcmax, &pfcmin,
										&pfcmax)) != 0)
			return ret;

		if ((ret = unconstrain_freq_tree(hwc->parent, pfcmin, pfcmax)) != 0)
			return ret;
	}

	return ret;
}

int hwclock_unconstrain_freq(const char *clockname, unsigned long fcmin,
							 unsigned long fcmax)
{
	struct hwclock *hwc;
	int ret = 0;

	simple_lock(&hwclocks_lock);

	if ((hwc = clocknamelookup(clockname)) == NULL) {
		ret = EINVAL;
		goto out;
	}

	ret = unconstrain_freq_tree(hwc, fcmin, fcmax);

out:
	simple_unlock(&hwclocks_lock);
	return ret;
}


static int
constrain_enable_tree(struct hwclock *hwc, unsigned long value)
{
	struct hwclock_constraint *constraint;
	unsigned long oldval;
	int ret = 0;

	if ((constraint = malloc(sizeof(struct hwclock_constraint), M_TEMP,
							 M_WAITOK)) == NULL)
		ret = ENOMEM;
	else {
		constraint->type = HWCLOCK_ENABLE_CONSTRAINT;
		constraint->enabcons_val = value;
		LIST_INSERT_HEAD(&hwc->constraints, constraint, clkconstraints);
	}

	if (hwc->parent && hwc->parent->setenable) {
		if ((ret = constrain_enable_tree(hwc->parent, value)) != 0)
			return ret;
	}

	if (hwc->getenable && ((ret = (*hwc->getenable)(hwc, &oldval)) == 0) &&
		(oldval != value) && hwc->setenable) {
		ret = (*hwc->setenable)(hwc, value);
	}

	return ret;
}

int hwclock_constrain_enable(const char *clockname, unsigned long value)
{
	struct hwclock *hwc;
	int ret = 0;

	simple_lock(&hwclocks_lock);

	if ((hwc = clocknamelookup(clockname)) == NULL) {
		ret = EINVAL;
		goto out;
	}

	ret = constrain_enable_tree(hwc, value);

out:
	simple_unlock(&hwclocks_lock);
	return ret;
}


static int unconstrain_enable_tree(struct hwclock *hwc,
								   unsigned long value)
{
	struct hwclock_constraint *constraint;
	struct hwclock_constraint *delconstraint = NULL;
	int stillconstrained = 0;
	int ret = 0;

	LIST_FOREACH(constraint, &hwc->constraints, clkconstraints) {
		if ((delconstraint == NULL) &&
			(constraint->type == HWCLOCK_ENABLE_CONSTRAINT) &&
			(constraint->enabcons_val == value)) {
			delconstraint = constraint;
		} else if (constraint->type == HWCLOCK_ENABLE_CONSTRAINT) {
			stillconstrained = 1;
		}
	}

	if (delconstraint != NULL) {
		LIST_REMOVE(delconstraint, clkconstraints);
		free(delconstraint, M_TEMP);
	} else {
		printf("Error: clock %s enable constraint %lu not found.\n",
			   hwc->name, value);
	}

	if (! stillconstrained) {
		if (policy_mgr)
			ret = (*policy_mgr)(hwc, ENABLE_CONSTRAINT_DROP);
		else if ((value == HWCLOCK_ENABLED) && (hwc->setenable))
			ret = (*hwc->setenable)(hwc, HWCLOCK_DISABLED);
	}

	if ((ret == 0) && hwc->parent && hwc->setenable)
		ret = unconstrain_enable_tree(hwc->parent, value);

	return ret;
}

int hwclock_unconstrain_enable(const char *clockname, unsigned long value)
{
	struct hwclock *hwc;
	int ret = 0;

	simple_lock(&hwclocks_lock);

	if ((hwc = clocknamelookup(clockname)) == NULL) {
		ret = EINVAL;
		goto out;
	}

	ret = unconstrain_enable_tree(hwc, value);

out:
	simple_unlock(&hwclocks_lock);
	return ret;
}


int hwclock_setattr(const char *clockname, enum hwclock_arch_attr attr,
					unsigned long value)
{
	struct hwclock *hwc;
	int ret = 0;

	simple_lock(&hwclocks_lock);

	if (((hwc = clocknamelookup(clockname)) == NULL)
		|| (! hwc->setattr)) {
		ret = EINVAL;
		goto out;
	}

	ret = (*hwc->setattr)(hwc, attr, value);

out:
	simple_unlock(&hwclocks_lock);
	return ret;
}


void hwclock_register(struct hwclock *hwc)
{
	simple_lock(&hwclocks_lock);
	LIST_INSERT_HEAD(&hwclocks, hwc, clks);
	LIST_INIT(&hwc->constraints);
	LIST_INIT(&hwc->children);

	if (hwc->parent)
		LIST_INSERT_HEAD(&hwc->parent->children, hwc, siblings);

	hwc->freqcons_min = 0;
	hwc->freqcons_max = (unsigned long) -1;

	if (hwc->init)
		hwc->init(hwc);

	simple_unlock(&hwclocks_lock);
}


void hwclock_unregister(struct hwclock *hwc)
{
	simple_lock(&hwclocks_lock);
	LIST_REMOVE(hwc, clks);
	simple_unlock(&hwclocks_lock);
}


int hwclock_register_policy_manager(hwclock_policy_mgr_t mgr)
{
	policy_mgr = mgr;
	return 0;
}

void hwclock_dump_tree(void);

void hwclock_dump_tree(void)
{
	struct hwclock *hwc;
	struct hwclock *hwc2;
	struct hwclock_constraint *constraint;

	printf("\nhwclock tree...\n");

	simple_lock(&hwclocks_lock);
	LIST_FOREACH(hwc, &hwclocks, clks) {
		printf("clock %s", hwc->name);

		if (hwc->getfreq) {
			unsigned long freq;

			hwc->getfreq(hwc, &freq);
			printf(" freq=%lu cons %lu-%lu", freq, hwc->freqcons_min,
				   hwc->freqcons_max);
		}

		if (hwc->getenable) {
			unsigned long value;

			hwc->getenable(hwc, &value);
			printf(" enabled=%lu", value);
		}

		printf("\n");

		LIST_FOREACH(constraint, &hwc->constraints, clkconstraints) {
			if (constraint->type == HWCLOCK_FREQ_CONSTRAINT)
				printf("   freq constraint: %lu-%lu\n",
					   constraint->freqcons_min, constraint->freqcons_max);
			else
				printf("   enable constraint: %s\n",
					   constraint->enabcons_val == HWCLOCK_DISABLED ?
					   "disable" : "enable");
		}

		if (hwc->parent)
			printf("   parent: %s\n", hwc->parent->name);

		if (! LIST_EMPTY(&hwc->children)) {
				printf("   children: ");

				LIST_FOREACH(hwc2, &hwc->children, siblings)
					printf("%s ", hwc2->name);

				printf("\n");
			}
	}

	simple_unlock(&hwclocks_lock);
}
