/* 	$NetBSD: lock_machdep.c,v 1.2 2003/07/14 22:32:40 lukem Exp $ */

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: lock_machdep.c,v 1.2 2003/07/14 22:32:40 lukem Exp $");

/*
 * Machine-dependent spin lock operations.
 */

#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/lock.h>
#include <sys/systm.h>

#include <machine/atomic.h>
#include <machine/cpu.h>

#include <ddb/db_output.h>

void
__cpu_simple_lock_init(lockp)
	__cpu_simple_lock_t *lockp;
{

	*lockp = __SIMPLELOCK_UNLOCKED;
}

#if defined (DEBUG) && defined(DDB)
int spin_limit = 10000000;
__cpu_simple_lock_t *wantlock[X86_MAXPROCS], *gotlock[X86_MAXPROCS];
#endif

void
__cpu_simple_lock(lockp)
	__cpu_simple_lock_t *lockp;
{
#if defined (DEBUG)
#if defined(DDB)
	int spincount = 0;
	int cpu = cpu_number();
	int limit = spin_limit * (cpu + 1);
#endif
	__cpu_simple_lock_t v = *lockp;

	KDASSERT((v == __SIMPLELOCK_LOCKED) || (v == __SIMPLELOCK_UNLOCKED));
#if defined(DDB)
	wantlock[cpu] = lockp;
#endif
#endif

	while (x86_atomic_testset_i(lockp, __SIMPLELOCK_LOCKED) ==
	    __SIMPLELOCK_LOCKED) {
#if defined(DEBUG) && defined(DDB)
		spincount++;
		if (spincount == limit) {
			extern int db_active;
			spincount = 0;

			if (db_active) {
				db_printf("cpu%d: spinout while in debugger\n",
				    cpu);
				while (db_active)
					;
			}
			db_printf("cpu%d: spinout\n", cpu);
			Debugger();
		}
#endif
	}
#if defined(DEBUG) && defined(DDB)
	wantlock[cpu] = 0;
	gotlock[cpu] = lockp;
#endif
	__lockbarrier();
}

int
__cpu_simple_lock_try(lockp)
	__cpu_simple_lock_t *lockp;
{
	int r;
#ifdef DEBUG
	__cpu_simple_lock_t v = *lockp;

	KDASSERT((v == __SIMPLELOCK_LOCKED) || (v == __SIMPLELOCK_UNLOCKED));
#endif
	r = (x86_atomic_testset_i(lockp, __SIMPLELOCK_LOCKED)
	    == __SIMPLELOCK_UNLOCKED);

	__lockbarrier();

	return (r);
}

void
__cpu_simple_unlock(lockp)
	__cpu_simple_lock_t *lockp;
{
#ifdef DEBUG
	__cpu_simple_lock_t v = *lockp;

	KDASSERT((v == __SIMPLELOCK_LOCKED) || (v == __SIMPLELOCK_UNLOCKED));
#endif
	__lockbarrier();
	*lockp = __SIMPLELOCK_UNLOCKED;
}
