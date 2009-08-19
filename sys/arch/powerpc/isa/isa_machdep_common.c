/*	$NetBSD: isa_machdep_common.c,v 1.2.26.2 2009/08/19 18:46:41 yamt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
__KERNEL_RCSID(0, "$NetBSD: isa_machdep_common.c,v 1.2.26.2 2009/08/19 18:46:41 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/pio.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#define	IO_ELCR1	0x4d0
#define	IO_ELCR2	0x4d1

const struct evcnt *
genppc_isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

/*
 * Set up an interrupt handler to start being called.
 */
void *
genppc_isa_intr_establish(isa_chipset_tag_t ic, int irq, int type, int level,
    int (*ih_fun)(void *), void *ih_arg)
{

	return (void *)intr_establish(irq, type, level, ih_fun, ih_arg);
}

/*
 * Deregister an interrupt handler.
 */
void
genppc_isa_intr_disestablish(isa_chipset_tag_t ic, void *arg)
{

	intr_disestablish(arg);
}

void
genppc_isa_attach_hook(struct device *parent, struct device *self,
    struct isabus_attach_args *iba)
{

	/* Nothing to do. */
}

void
genppc_isa_detach_hook(isa_chipset_tag_t ic, device_t self)
{

	/* Nothing to do. */
}
