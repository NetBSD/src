/*	$NetBSD: executor.c,v 1.1.2.2 2018/04/16 02:00:08 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: executor.c,v 1.1.2.2 2018/04/16 02:00:08 pgoyette Exp $");

#include <sys/param.h>
#include <sys/module.h>
#include <sys/callout.h>
#include <sys/once.h>

/*
 * Check if dmesg prints
 * "executor <no-of-seconds-after-loading>" every second
 * and "executor once" exactly once in the beginning
 * to test this module
 */

MODULE(MODULE_CLASS_MISC, executor, NULL);

static void callout_example(void *);
static int runonce_example(void);

static int executor_count = 0; //To hold the count of seconds

static callout_t sc;

/* Creating a variable that marks whether the function has executed or not */
static once_t ctl;
static ONCE_DECL(ctl);

/*
 * runonce_example : This function should execute only once
 * It should return 0 as RUN_ONCE calls the function until it returns 0.
 */

static int
runonce_example(void) {
	printf("executor once\n");
	return 0;
}

/*
 * callout_example : This function should get executed every second.
 * It calls runonce_example each time.
 * It prints the seconds elasped after the module was loaded.
 * It reschedules the callout to the next second using callout_schedule
 */

static void
callout_example(void *arg) {
	RUN_ONCE(&ctl, runonce_example);
	executor_count++;
	printf("Callout %d\n", executor_count);
	callout_schedule(&sc, mstohz(1000));
}

/*
 * executor_modcmd : has two tasks
 * On loading the module it needs to initialize a callout handle and schedule
 * the first callout.
 * On unloading the module it needs to stop and destroy the callout handle
 */

static int
executor_modcmd(modcmd_t cmd, void *arg)
{
	switch(cmd) {
	case MODULE_CMD_INIT:
		printf("executor module inserted\n");
		callout_init(&sc, 0);
		callout_reset(&sc, mstohz(1000), callout_example, NULL);
		break;
	case MODULE_CMD_FINI:
		printf("executor module unloaded\n");
		callout_stop(&sc);
		callout_destroy(&sc);
		break;
	default:
		return ENOTTY;
	}
	return 0;
}
