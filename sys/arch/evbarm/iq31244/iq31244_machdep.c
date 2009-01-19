/*	$NetBSD: iq31244_machdep.c,v 1.1.4.2 2009/01/19 13:16:04 skrll Exp $	*/

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: iq31244_machdep.c,v 1.1.4.2 2009/01/19 13:16:04 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <dev/i2c/m41st84var.h>

void iq31244_reboot(int);	/* I80321_REBOOT */

/*
 * Resetting the peripherals like the IQ80321 does isn't
 * a sufficient reset for the IQ31244.  With the stock
 * Redboot, that reset will leave it in the Redboot
 * debugger on reinit.
 * The IQ31244 has an M41ST84, however, that can reset
 * the board with the watchdog.  So we just set it to
 * do so here.
 * The wdog on the 41ST84 should probably be a real
 * device, but for now, we just kick it here.
 */
void
iq31244_reboot(int howto)
{

	/* We can't halt; just reset. */
	if ((howto & RB_HALT) == 0) {
		device_t	dv;

		dv = device_find_by_xname("strtc0");
		if (dv == NULL)
		{
			printf("No strtc0!  Reset will probably fail!\n");
			return;
		}

		/*
		 * 0x80 == steer to reset
		 * 0x04 == multiplier 1, resolution 1/16s
		 */
		strtc_wdog_config(device_private(dv), 0x84);
		delay(10000);
	}
}
