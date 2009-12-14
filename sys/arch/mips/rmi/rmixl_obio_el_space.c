/*	$NetBSD: rmixl_obio_el_space.c,v 1.1.2.1 2009/12/14 07:43:25 cliff Exp $	*/

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

/*
 * Litle Endian bus_space(9) support for RMI {XLP,XLR,XLS} obio
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_obio_el_space.c,v 1.1.2.1 2009/12/14 07:43:25 cliff Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <machine/bus.h>
#include <mips/rmi/rmixl_obiovar.h>
#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#define	CHIP			rmixl_obio_el
#define	CHIP_MEM		/* defined */
#define	CHIP_ACCESS_SIZE	4
#define CHIP_LITTLE_ENDIAN

#define CHIP_EX_MALLOC_SAFE(v)	(((struct rmixl_config *)(v))->rc_mallocsafe)
#define CHIP_EXTENT(v)		(((struct rmixl_config *)(v))->rc_obio_el_ex)


/* MEM region 1 */
#define	CHIP_W1_BUS_START(v)	0
#define	CHIP_W1_BUS_END(v)	(RMIXL_IO_DEV_SIZE - 1)
#define	CHIP_W1_SYS_START(v)	(((struct rmixl_config *)(v))->rc_io_pbase)
#define	CHIP_W1_SYS_END(v)	(CHIP_W1_SYS_START(v) + RMIXL_IO_DEV_SIZE - 1)

#include <mips/mips/bus_space_alignstride_chipdep.c>
