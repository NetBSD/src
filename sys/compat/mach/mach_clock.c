/*	$NetBSD: mach_clock.c,v 1.18 2007/12/20 23:02:59 dsl Exp $ */

/*-
 * Copyright (c) 2002-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_clock.c,v 1.18 2007/12/20 23:02:59 dsl Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/time.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_services.h>
#include <compat/mach/mach_syscallargs.h>

int
mach_sys_clock_sleep_trap(struct lwp *l, const struct mach_sys_clock_sleep_trap_args *uap, register_t *retval)
{
	/* {
		syscallarg(mach_clock_port_t) clock_name;
		syscallarg(mach_sleep_type_t) sleep_type;
		syscallarg(int) sleep_sec;
		syscallarg(int) sleep_nsec;
		syscallarg(mach_timespec_t *) wakeup_time;
	} */
	struct timespec mts, cts, tts;
	mach_timespec_t mcts;
	int dontcare;
	int error;
	int ticks;

	mts.tv_sec = SCARG(uap, sleep_sec);
	mts.tv_nsec = SCARG(uap, sleep_nsec);

	if (SCARG(uap, sleep_type) == MACH_TIME_ABSOLUTE) {
		nanotime(&cts);
		timespecsub(&mts, &cts, &tts);
	} else {
		tts.tv_sec = mts.tv_sec;
		tts.tv_nsec = mts.tv_nsec;
	}

	ticks = tts.tv_sec * hz;
	ticks += (tts.tv_nsec * hz) / 100000000L;

	tsleep(&dontcare, PZERO|PCATCH, "sleep", ticks);

	if (SCARG(uap, wakeup_time) != NULL) {
		nanotime(&cts);
		mcts.tv_sec = cts.tv_sec;
		mcts.tv_nsec = cts.tv_nsec;
		error = copyout(&mcts, SCARG(uap, wakeup_time), sizeof(mcts));
		if (error != 0)
			return error;
	}

	return 0;
}

int
mach_sys_timebase_info(struct lwp *l, const struct mach_sys_timebase_info_args *uap, register_t *retval)
{
	/* {
		syscallarg(mach_timebase_info_t) info;
	} */
	int error;
	struct mach_timebase_info info;

	/* XXX This is probably bus speed, fill it accurately */
	info.numer = 4000000000UL;
	info.denom = 75189611UL;

	if ((error = copyout(&info, (void *)SCARG(uap, info),
	    sizeof(info))) != 0)
		return error;

	return 0;
}



int
mach_clock_get_time(struct mach_trap_args *args)
{
	mach_clock_get_time_request_t *req = args->smsg;
	mach_clock_get_time_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct timeval tv;

	microtime(&tv);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_cur_time.tv_sec = tv.tv_sec;
	rep->rep_cur_time.tv_nsec = tv.tv_usec * 1000;

	mach_set_trailer(rep, *msglen);

	return 0;
}
