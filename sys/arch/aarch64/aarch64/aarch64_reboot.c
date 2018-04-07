/*	$NetBSD: aarch64_reboot.c,v 1.1.2.2 2018/04/07 04:12:10 pgoyette Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2001 Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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
 *
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997,1998 Causality Limited.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 2007 Microsoft
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
 *	This product includes software developed by Microsoft
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aarch64_reboot.c,v 1.1.2.2 2018/04/07 04:12:10 pgoyette Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/mount.h>

#include <dev/cons.h>
#include <uvm/uvm_extern.h>

#include <aarch64/machdep.h>
#include <aarch64/armreg.h>
#include <aarch64/locore.h>
#include <aarch64/cpufunc.h>

void (*cpu_reset_address0)(void);
void (*cpu_reset_address)(void);
void (*cpu_powerdown_address)(void);

static int
docpureset(int howto)
{
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		if (cpu_powerdown_address) {
			(*cpu_powerdown_address)();
			printf("WARNING: powerdown failed\r\n");
		} else {
			printf("WARNING: powerdown not supported\r\n");
		}
	}

	if (howto & RB_HALT) {
		printf("The operating system has halted.\r\n");
		printf("Please press any key to reboot.\r\n");
		cnpollc(true);	/* for proper keyboard command handling */
		/* If there is no keyboard, cngetc() returns 0, so loop */
		while (cngetc() == 0)
			delay(100000);
		cnpollc(false);
	}

	printf("rebooting...\r\n");

	/* make sure to write to memory (msgbuf) */
	aarch64_dcache_wb_all();

	/* console output delay */
	delay(1000);

	if (cpu_reset_address != NULL)
		(*cpu_reset_address)();
	if (cpu_reset_address0 != NULL)
		(*cpu_reset_address0)();

	for (;;)
		asm("wfi");

	/*NOTREACHED*/
}

/* Sync the discs, unmount the filesystems, and adjust the todr */
static void
bootsync(void)
{
	static bool bootsyncdone = false;

	if (bootsyncdone)
		return;
	bootsyncdone = true;

	/* Make sure we can still manage to do things */
	if ((reg_daif_read() & DAIF_I) != 0) {
		/*
		 * If we get here then boot has been called without RB_NOSYNC
		 * and interrupts were disabled. This means the boot() call
		 * did not come from a user process e.g. shutdown, but must
		 * have come from somewhere in the kernel.
		 */
		ENABLE_INTERRUPT();
		printf("Warning IRQ's disabled during boot()\n");
	}

	vfs_shutdown();

	resettodr();
}

void
cpu_reboot(int howto, char *bootstr)
{

	/*
	 * If we are still cold then hit the air brakes
	 * and crash to earth fast
	 */
	if (cold) {
		doshutdownhooks();
		docpureset(RB_HALT | howto);
	}

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during the
	 * unmount.  It looks like syslogd is getting woken up only to find
	 * that it cannot page part of the binary in as the filesystem has
	 * been unmounted.
	 */
	if ((howto & RB_NOSYNC) == 0)
		bootsync();

	/* Say NO to interrupts for the duration of the dump */
	int s = splhigh();

#if notyet	/* XXX */
	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();
#endif

	splx(s);

	pmf_system_shutdown(boothowto);

	/* Say NO to interrupts for good */
	splhigh();

	/* Run any shutdown hooks */
	doshutdownhooks();

	/* Make sure IRQ's are disabled */
	DISABLE_INTERRUPT();

	docpureset(howto);
	__unreachable();
}
