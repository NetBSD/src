/*	$NetBSD: i2c_subr.c,v 1.3 2025/09/20 21:24:29 thorpej Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i2c_subr.c,v 1.3 2025/09/20 21:24:29 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/module.h>

#include <dev/i2c/i2cvar.h>

MODULE(MODULE_CLASS_EXEC, i2c_subr, NULL);

static int
i2c_subr_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}

int
iicbus_print(void *aux, const char *pnp)
{

	if (pnp != NULL)
		aprint_normal("iic at %s", pnp);

	return UNCONF;
}

/*
 * iic_acquire_bus_lock --
 *
 *	Acquire an i2c bus lock.  Used by iic_acquire_bus() and other
 *	places that need to acquire an i2c-related lock with the same
 *	logic.
 */
int
iic_acquire_bus_lock(kmutex_t *lock, int flags)
{
	if (flags & I2C_F_POLL) {
		/*
		 * Polling should only be used in rare and/or
		 * extreme circumstances; most i2c clients should
		 * be allowed to sleep.
		 *
		 * Really, the ONLY user of I2C_F_POLL should be
		 * "when cold", i.e. during early autoconfiguration
		 * when there is only proc0, and we might have to
		 * read SEEPROMs, etc.  There should be no other
		 * users interfering with our access of the i2c bus
		 * in that case.
		 */
		if (mutex_tryenter(lock) == 0) {
			return EBUSY;
		}
	} else {
		/*
		 * N.B. We implement this as a mutex that we hold across
		 * across a series of requests beause we'd like to get the
		 * priority boost if a higher-priority process wants the
		 * i2c bus while we're asleep waiting for the controller
		 * to perform the I/O.
		 *
		 * XXXJRT Disable preemption here?  We'd like to keep the
		 * CPU while holding this resource, unless we release it
		 * voluntarily (which should only happen while waiting for
		 * a controller to complete I/O).
		 */
		mutex_enter(lock);
	}

	return 0;
}

/*
 * iic_release_bus_lock --
 *
 *	Release a previously-acquired i2c-related bus lock.
 */
void
iic_release_bus_lock(kmutex_t *lock)
{
	mutex_exit(lock);
}
