/*	$NetBSD: autoconf.c,v 1.7 2024/08/15 07:06:35 skrll Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__RCSID("$NetBSD: autoconf.c,v 1.7 2024/08/15 07:06:35 skrll Exp $");

#include <sys/param.h>

#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <machine/sysreg.h>

#include <dev/fdt/fdt_boot.h>

void
cpu_configure(void)
{
	(void)splhigh();

	config_rootfound("mainbus", NULL);

	/* Time to start taking interrupts so lets open the flood gates ... */
	spl0();
}

/*
 * Set up the root device from the boot args.
 *
 * cpu_bootconf() is called before RAIDframe root detection,
 * and cpu_rootconf() is called after.
 */
void
cpu_bootconf(void)
{
#ifndef MEMORY_DISK_IS_ROOT
	fdt_cpu_rootconf();
#endif
}

void
cpu_rootconf(void)
{
	cpu_bootconf();
	aprint_normal("boot device: %s\n",
	    booted_device != NULL ? device_xname(booted_device) : "<unknown>");
	rootconf();
}

void
device_register(device_t self, void *aux)
{

	if (device_is_a(self, "mainbus")) {
		fdt_setup_initrd();
	}
}
