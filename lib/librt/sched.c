/*	$NetBSD: sched.c,v 1.1 2008/01/15 03:37:15 rmind Exp $	*/

/*
 * Copyright (c) 2008, Mindaugas Rasiukevicius <rmind at NetBSD org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: sched.c,v 1.1 2008/01/15 03:37:15 rmind Exp $");

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <sys/param.h>
#include <sys/pset.h>
#include <sys/types.h>

/*
 * Scheduling parameters.
 */

int
sched_setparam(pid_t pid, const struct sched_param *param)
{

	return _sched_setparam(pid, 0, param);
}

int
sched_getparam(pid_t pid, struct sched_param *param)
{

	return _sched_getparam(pid, 0, param);
}

int
sched_setscheduler(pid_t pid, int policy, const struct sched_param *param)
{
	struct sched_param sp;

	memcpy(&sp, param, sizeof(struct sched_param));
	sp.sched_class = policy;
	return _sched_setparam(pid, 0, &sp);
}

int
sched_getscheduler(pid_t pid)
{
	struct sched_param sp;
	int error;

	error = _sched_getparam(pid, 0, &sp);
	if (error)
		return error;

	return sp.sched_class;
}

/*
 * Scheduling priorities.
 */

int
sched_get_priority_max(int policy)
{

	return sysconf(_SC_SCHED_PRI_MAX);
}

int
sched_get_priority_min(int policy)
{

	return sysconf(_SC_SCHED_PRI_MIN);
}

int
sched_rr_get_interval(pid_t pid, struct timespec *interval)
{

	interval->tv_sec = 0;
	interval->tv_nsec = sysconf(_SC_SCHED_RT_TS) * 1000;
	return 0;
}

/*
 * Processor-sets.
 */

int
pset_bind(psetid_t psid, idtype_t idtype, id_t id, psetid_t *opsid)
{

	return _pset_bind(idtype, id, 0, psid, opsid);
}
