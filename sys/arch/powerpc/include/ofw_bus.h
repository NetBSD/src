/* $NetBSD: ofw_bus.h,v 1.4.72.1 2021/04/03 22:28:34 thorpej Exp $ */

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

#ifndef _POWERPC_OFW_BUS_H_
#define _POWERPC_OFW_BUS_H_

#define RANGE_TYPE_PCI		1
#define RANGE_TYPE_ISA		2
#define RANGE_TYPE_MACIO	3
#define RANGE_TYPE_FIRSTPCI	4
#define RANGE_IO		1
#define RANGE_MEM		2

#ifndef EXSTORAGE_MAX
#define EXSTORAGE_MAX		12
#endif

#ifdef _KERNEL
extern struct powerpc_bus_space genppc_isa_io_space_tag;
extern struct powerpc_bus_space genppc_isa_mem_space_tag;

#include <machine/powerpc.h>

void ofwoea_bus_space_init(void);
void ofwoea_initppc(u_int, u_int, char *);
int ofwoea_map_space(int, int, int, struct powerpc_bus_space *, const char *);
#endif

#endif /* _POWERPC_OFW_BUS_H_ */
