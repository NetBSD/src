/*	$NetBSD: rmixl_iobus_space.c,v 1.2.2.2 2011/04/21 01:41:13 rmind Exp $	*/

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
 * bus_space(9) support for Peripherals IO Bus on RMI {XLP,XLR,XLS} chips
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_iobus_space.c,v 1.2.2.2 2011/04/21 01:41:13 rmind Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <machine/bus.h>
#include <mips/rmi/rmixl_iobusvar.h>
#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#define	CHIP			rmixl_iobus
#define	CHIP_MEM		/* defined */
#define	CHIP_ACCESS_SIZE	1
#define CHIP_BIG_ENDIAN

#define CHIP_EX_MALLOC_SAFE(v)	(((struct rmixl_config *)(v))->rc_mallocsafe)

/*
 * RMIXL_FLASH_BAR_MASK_MAX is __BITS(34,0)
 * so avoid extent if that value overflows 'long'
 */
#ifdef _LP64
#define CHIP_EXTENT(v)		(((struct rmixl_config *)(v))->rc_iobus_ex)
#endif


/* MEM region 1 */
#define	CHIP_W1_BUS_START(v)	0
#define	CHIP_W1_BUS_END(v)	RMIXL_FLASH_BAR_MASK_MAX
#define	CHIP_W1_SYS_START(v)	(((struct rmixl_config *)(v))->rc_flash_pbase)
#define	CHIP_W1_SYS_END(v)	(CHIP_W1_SYS_START(v) + RMIXL_FLASH_BAR_MASK_MAX)

#include <mips/mips/bus_space_alignstride_chipdep.c>
