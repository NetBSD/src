/* $NetBSD: ttwoga_bus_io.c,v 1.1 2000/12/21 20:51:54 thorpej Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(1, "$NetBSD: ttwoga_bus_io.c,v 1.1 2000/12/21 20:51:54 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <alpha/pci/ttwogareg.h>
#include <alpha/pci/ttwogavar.h>

#define	CHIP		ttwoga

#define	CHIP_V(v)		((struct ttwoga_config *)(v))

#define	CHIP_EX_MALLOC_SAFE(v)	(CHIP_V(v)->tc_mallocsafe)

#define	CHIP_IO_EXTENT(v)	(CHIP_V(v)->tc_io_ex)
#define	CHIP_IO_EX_STORE(v)	(CHIP_V(v)->tc_io_exstorage)
#define	CHIP_IO_EX_STORE_SIZE(v) sizeof(CHIP_IO_EX_STORE(v))

/* IO region 1 */
#define	CHIP_IO_W1_BUS_START(v)						\
    (CHIP_V(v)->tc_io_bus_start)
#define	CHIP_IO_W1_BUS_END(v)						\
    (CHIP_IO_W1_BUS_START(v) + CHIP_V(v)->tc_sysmap->tsmap_sio_bussize - 1)
#define	CHIP_IO_W1_SYS_START(v)						\
    (ttwoga_gamma_cbus_bias + CHIP_V(v)->tc_sysmap->tsmap_sio_base)
#define	CHIP_IO_W1_SYS_END(v)						\
    (CHIP_IO_W1_SYS_START(v) + CHIP_V(v)->tc_sysmap->tsmap_sio_syssize - 1)

#include <alpha/pci/pci_swiz_bus_io_chipdep.c>
