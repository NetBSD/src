/*	$NetBSD: intiovar.h,v 1.10 2005/12/24 20:07:03 perry Exp $	*/

/*-
 * Copyright (c) 1996, 1998, 2001 The NetBSD Foundation, Inc.
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

/*
 * Autoconfiguration definitions and prototypes for the hp300
 * internal i/o space.
 */

#include <machine/bus.h>
#include <machine/cpu.h>

#include <arch/hp300/dev/intioreg.h>

#define INTIO_MOD_LEN	8

/*
 * Arguments used to attach a device to the internal i/o space.
 */
struct intio_attach_args {
	char ia_modname[INTIO_MOD_LEN+1];	/* module name */
	bus_space_tag_t ia_bst;			/* bus space tag */
	bus_addr_t ia_addr;			/* physical address */
	bus_size_t ia_iobase;			/* intio iobase */
	int ia_ipl;				/* interrupt priority level */
};

struct intio_builtins {
	const char *ib_modname;			/* module name */
	bus_size_t ib_offset;			/* intio offset */
	int ib_ipl;				/* interrupt priority level */
};

/*
 * Devices such as the HIL and RTC chips are wired in a consistent
 * fashion.  These routines provide a uniform mechanism for accessing
 * the devices that are wired to the machine.  Doesn't include
 * memory-mapped devices such as framebuffers.
 */

#define WAIT(bst,bsh)		\
	while (bus_space_read_1(bst,bsh,INTIO_DEV_3xx_STAT) \
		& INTIO_DEV_BUSY)
#define DATAWAIT(bst,bsh)	\
	while (!(bus_space_read_1(bst, bsh, INTIO_DEV_3xx_STAT) \
		& INTIO_DEV_DATA_READY))

static inline int
intio_device_readcmd(bus_space_tag_t bst, bus_space_handle_t bsh, int cmd,
	uint8_t *datap)
{
        uint8_t status;

	if (cmd != 0) {
		WAIT(bst, bsh);
		bus_space_write_1(bst, bsh, INTIO_DEV_3xx_CMD, cmd);
	}
        do {
		DATAWAIT(bst, bsh);
		status = bus_space_read_1(bst, bsh, INTIO_DEV_3xx_STAT);
		*datap = bus_space_read_1(bst, bsh, INTIO_DEV_3xx_DATA);
	} while (((status >> INTIO_DEV_SRSHIFT) & INTIO_DEV_SRMASK)
		!= INTIO_DEV_SR_DATAAVAIL);
	return (0);
}

static inline int
intio_device_writecmd(bus_space_tag_t bst, bus_space_handle_t bsh,
	int cmd, uint8_t *datap, int len)
{
        WAIT(bst,bsh);
	bus_space_write_1(bst, bsh, INTIO_DEV_3xx_CMD, cmd);
        while (len--) {
                WAIT(bst,bsh);
		bus_space_write_1(bst, bsh, INTIO_DEV_3xx_DATA, *datap++);
        }
	return (0);
}

static inline int
intio_device_readstate(bus_space_tag_t bst, bus_space_handle_t bsh,
	uint8_t *statusp, uint8_t *datap)
{
	*statusp = bus_space_read_1(bst, bsh, INTIO_DEV_3xx_STAT);
	*datap = bus_space_read_1(bst, bsh, INTIO_DEV_3xx_DATA);
	return (0);
}
#undef WAIT
#undef DATAWAIT

#define INTIO_DEVSIZE	4096	/* large enough for all machines */

#define intio_intr_establish(func, arg, ipl, priority)			\
	intr_establish((func),(arg),(ipl),(priority))
