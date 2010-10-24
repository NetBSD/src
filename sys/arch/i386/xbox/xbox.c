/* $NetBSD: xbox.c,v 1.4.14.1 2010/10/24 22:48:06 jym Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

/*
 * Microsoft XBOX helper functions
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xbox.c,v 1.4.14.1 2010/10/24 22:48:06 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <arch/i386/include/xbox.h>

#include "pic16lc.h"

#if NPIC16LC > 0
extern void pic16lc_setled(uint8_t);
extern void pic16lc_reboot(void);
extern void pic16lc_poweroff(void);
#endif

#define XBOX_NFORCE_NIC	0xfef00000

void
xbox_startup(void)
{
	bus_space_handle_t h;
	int rv;

	if (!arch_i386_is_xbox)
		return;

	/*
	 * From Rink Springer @ FreeBSD:
	 *
	 * Some XBOX loaders, such as Cromwell, have a flaw which cause the
	 * nfe(4) driver to fail attaching to the NIC.
	 *
	 * This is because they leave the NIC running; this will cause the
	 * Nvidia driver to fail as the NIC does not return any sensible
	 * values and thus fails attaching (using an error 0x5, this means
	 * it cannot find a valid PHY)
	 *
	 * We bluntly tell the NIC to stop whatever it's doing; this makes
	 * nfe(4) attach correctly. As the NIC always resides at
	 * 0xfef00000-0xfef003ff on an XBOX, we simply hardcode this address.
	 */
	rv = bus_space_map(x86_bus_space_mem, XBOX_NFORCE_NIC,
	    0x400, 0, &h);
	if (!rv) {
		bus_space_write_4(x86_bus_space_mem, h, 0x188, 0);
		bus_space_unmap(x86_bus_space_mem, h, 0x400);
	}

	
}

void
xbox_setled(uint8_t val)
{
#if NPIC16LC > 0
	pic16lc_setled(val);
#else
	printf("xbox_setled: pic16lc driver missing from kernel!\n");
#endif
}

void
xbox_reboot(void)
{
#if NPIC16LC > 0
	pic16lc_reboot();
#else
	printf("xbox_reboot: pic16lc driver missing from kernel!\n");
	printf("xbox_reboot: halting...\n");
	for (;;)
		;
#endif
}

void
xbox_poweroff(void)
{
#if NPIC16LC > 0
	pic16lc_poweroff();
#else
	printf("xbox_poweroff: pic16lc driver missing from kernel!\n");
	printf("xbox_poweroff: halting...\n");
	for (;;)
		;
#endif
}
