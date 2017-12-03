/*	$NetBSD: interrupt.c,v 1.18.14.1 2017/12/03 11:36:15 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.18.14.1 2017/12/03 11:36:15 jdolecek Exp $");

#include "opt_vr41xx.h"
#include "opt_tx39xx.h"

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/malloc.h>
#include <sys/intr.h>

#include <mips/locore.h>

#include <machine/sysconf.h>

void
intr_init(void)
{

	ipl_sr_map = CPUISMIPS3 ? __ipl_sr_map_vr : __ipl_sr_map_tx;
}

#if defined(VR41XX) && defined(TX39XX)
/*
 * cpu_intr:
 *
 *	handle MIPS CPU interrupt.
 *	if VR41XX only or TX39XX only kernel, directly jump to each handler
 *	(tx/tx39icu.c, vr/vr.c), don't use this dispather.
 * 
 */
void
cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{

	(*platform.cpu_intr)(ppl, pc, status);
}
#endif /* VR41XX && TX39XX */
