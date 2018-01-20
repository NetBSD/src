/*	$NetBSD: gt_mem_space.c,v 1.1 2018/01/20 13:56:09 skrll Exp $	*/

/*
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson.
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
 * Little Endian bus_space(9) support for PCI MEM access
 * on cobalt machines
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gt_mem_space.c,v 1.1 2018/01/20 13:56:09 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>

#include <cobalt/dev/gtvar.h>

#define	CHIP			gt
#define	CHIP_MEM		/* defined */

/* IO region 1 */
#define	CHIP_W1_BUS_START(v)	0x12000000UL
#define	CHIP_W1_BUS_END(v)	0x14000000UL
#define CHIP_W1_SYS_START(v)	0x12000000UL
#define CHIP_W1_SYS_END(v)	0x14000000UL

#include <mips/mips/bus_space_alignstride_chipdep.c>
