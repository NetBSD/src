/*	$NetBSD: pb1000_intr.c,v 1.2 2003/07/15 01:37:31 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 * Platform-specific interrupt support for the Alchemy Semiconductor Pb1000.
 *
 * The Alchemy Semiconductor Pb1000's interrupts are wired to two internal
 * interrupt controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pb1000_intr.c,v 1.2 2003/07/15 01:37:31 lukem Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/locore.h>
#include <mips/alchemy/include/auvar.h>

#include <evbmips/evbmips/clockvar.h>
#include <mips/alchemy/include/aubusvar.h>
#include <evbmips/alchemy/pb1000reg.h>
#include <evbmips/alchemy/pb1000var.h>

#include <dev/ic/mc146818reg.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

void
evbmips_intr_init(void)
{
	au_intr_init();
}

void *
pb1000_intr_establish(int irq, int req, int level, int type,
    int (*func)(void *), void *arg)
{

	return (au_intr_establish(irq, req, level, type, func, arg));
}

void
pb1000_intr_disestablish(void *cookie)
{

	return (au_intr_disestablish(cookie));
}

void
evbmips_iointr(u_int32_t status, u_int32_t cause, u_int32_t pc,
    u_int32_t ipending)
{

	au_iointr(status, cause, pc, ipending);
}
