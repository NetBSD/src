/*	$NetBSD: alpha_pci_conf.c,v 1.2 2001/07/17 17:46:42 thorpej Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Support for accessing PCI configuration space.  This is currently
 * used to provide such support for XFree86.  In a perfect world, this
 * would go away in favor of a real bus space mapping framework.
 */

#include <sys/param.h>

#include <machine/sysarch.h>

uint32_t
alpha_pci_conf_read(u_int bus, u_int device, u_int func, u_int reg)
{
	struct alpha_pci_conf_readwrite_args args;

	args.write = 0;
	args.bus = bus;
	args.device = device;
	args.function = func;
	args.reg = reg;

	if (sysarch(ALPHA_PCI_CONF_READWRITE, &args) == -1)
		return (0xffffffffU);

	return (args.val);
}

void
alpha_pci_conf_write(u_int bus, u_int device, u_int func, u_int reg,
    uint32_t val)
{
	struct alpha_pci_conf_readwrite_args args;

	args.write = 1;
	args.bus = bus;
	args.device = device;
	args.function = func;
	args.reg = reg;
	args.val = val;

	(void) sysarch(ALPHA_PCI_CONF_READWRITE, &args);
}
