/*	$NetBSD: bus.h,v 1.1.4.2 2008/01/02 21:49:26 bouyer Exp $	*/
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by 
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _RS6000_BUS_H_
#define _RS6000_BUS_H_

/*
 * Values for the Be bus space tag, not to be used directly by MI code.
 */
#define	RS6000_BUS_SPACE_IO	0xC0000000	/* i/o space */
#define RS6000_BUS_SPACE_MEM	0xC0000000	/* mem space */

/*
 * Address conversion as seen from a PCI master.
 */
#define MPC105_DIRECT_MAPPED_SPACE	0xC0000000
#define PHYS_TO_BUS_MEM(t, x)	((x) | MPC105_DIRECT_MAPPED_SPACE)
#define BUS_MEM_TO_PHYS(t, x)	((x) & ~MPC105_DIRECT_MAPPED_SPACE)

#ifdef _KERNEL
extern struct powerpc_bus_space rs6000_iocc0_io_space_tag;
#endif

#include <powerpc/bus.h>

#endif /* _RS6000_BUS_H_ */
