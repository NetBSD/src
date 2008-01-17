/* $NetBSD: isa_machdep.c,v 1.4 2008/01/17 23:42:58 garbled Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isa_machdep.c,v 1.4 2008/01/17 23:42:58 garbled Exp $");

#include <sys/param.h>

#include <machine/bus.h>
#include <machine/pio.h>

#include <sys/extent.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#define IO_ELCR1        0x4d0
#define IO_ELCR2        0x4d1

struct powerpc_isa_chipset genppc_ict;
bus_space_handle_t io_icu1h, io_icu2h, io_elcrh;

/*
 * These functions can *ONLY* be used to talk to the i8259.  Leave them
 * alone.  I know they are greusome.
 */

int
map_isa_ioregs(void)
{
	int err, noerr;

	err = bus_space_map(&genppc_isa_io_space_tag, IO_ICU1, 2, 0,
	    &io_icu1h);
	if (err != 0)
		panic("Can't map IO_ICU1 error %d\n", err);

	err = bus_space_map(&genppc_isa_io_space_tag, IO_ICU2, 2, 0,
	    &io_icu2h);
	if (err != 0)
		panic("Can't map IO_ICU2 error %d\n", err);

	noerr = bus_space_map(&genppc_isa_io_space_tag, IO_ELCR1, 2, 0,
	    &io_elcrh);
	if (noerr != 0)
		aprint_error("Can't map IO_ELCR error %d\n", noerr);
	
	return err;
}

uint8_t
isa_inb(uint32_t addr)
{
	if (addr == IO_ICU1 || addr == IO_ICU1+1)
		return bus_space_read_1(&genppc_isa_io_space_tag, io_icu1h,
		    addr-IO_ICU1);
	if (addr == IO_ICU2 || addr == IO_ICU2+1)
		return bus_space_read_1(&genppc_isa_io_space_tag, io_icu2h,
		    addr-IO_ICU2);
	if (addr == IO_ELCR1 || addr == IO_ELCR2)
		return bus_space_read_1(&genppc_isa_io_space_tag, io_elcrh,
		    addr-IO_ELCR1);
	return 0;
}

void
isa_outb(uint32_t addr, uint8_t val)
{
	if (addr == IO_ICU1 || addr == IO_ICU1+1)
		bus_space_write_1(&genppc_isa_io_space_tag, io_icu1h,
		    addr-IO_ICU1, val);
	if (addr == IO_ICU2 || addr == IO_ICU2+1)
		bus_space_write_1(&genppc_isa_io_space_tag, io_icu2h,
		    addr-IO_ICU2, val);
	if (addr == IO_ELCR1 || addr == IO_ELCR2)
		bus_space_write_1(&genppc_isa_io_space_tag, io_elcrh,
		    addr-IO_ELCR1, val);
}
