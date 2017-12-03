/*	$NetBSD: octeon_ciu.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * Copyright (c) 2008 Internet Initiative Japan, Inc.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_ciu.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include "opt_octeon.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/malloc.h>

#include <sys/bus.h>
#include <machine/locore.h>
#include <machine/vmparam.h>

#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>

#ifdef CIUDEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

#ifdef OCTEON_ETH_DEBUG
void		octeon_ciu_dump(void);
void		octeon_ciu_dump_regs(void);

#define	_ENTRY(x)	{ #x, x##_BITS, x }

struct octeon_ciu_dump_reg_entry {
	const char *name;
	const char *format;
	paddr_t address;
};

static const struct octeon_ciu_dump_reg_entry octeon_ciu_dump_regs_entries[] = {
	_ENTRY(CIU_INT0_SUM0),
	_ENTRY(CIU_INT1_SUM0),
	_ENTRY(CIU_INT2_SUM0),
	_ENTRY(CIU_INT3_SUM0),
	_ENTRY(CIU_INT32_SUM0),
	_ENTRY(CIU_INT_SUM1),
	_ENTRY(CIU_INT0_EN0),
	_ENTRY(CIU_INT1_EN0),
	_ENTRY(CIU_INT2_EN0),
	_ENTRY(CIU_INT3_EN0),
	_ENTRY(CIU_INT32_EN0),
	_ENTRY(CIU_INT0_EN1),
	_ENTRY(CIU_INT1_EN1),
	_ENTRY(CIU_INT2_EN1),
	_ENTRY(CIU_INT3_EN1),
	_ENTRY(CIU_INT32_EN1),
	_ENTRY(CIU_TIM0),
	_ENTRY(CIU_TIM1),
	_ENTRY(CIU_TIM2),
	_ENTRY(CIU_TIM3),
	_ENTRY(CIU_WDOG0),
	_ENTRY(CIU_WDOG1),
	/* _ENTRY(CIU_PP_POKE0), */
	/* _ENTRY(CIU_PP_POKE1), */
	_ENTRY(CIU_MBOX_SET0),
	_ENTRY(CIU_MBOX_SET1),
	_ENTRY(CIU_MBOX_CLR0),
	_ENTRY(CIU_MBOX_CLR1),
	_ENTRY(CIU_PP_RST),
	_ENTRY(CIU_PP_DBG),
	_ENTRY(CIU_GSTOP),
	_ENTRY(CIU_NMI),
	_ENTRY(CIU_DINT),
	_ENTRY(CIU_FUSE),
	_ENTRY(CIU_BIST),
	_ENTRY(CIU_SOFT_BIST),
	_ENTRY(CIU_SOFT_RST),
	_ENTRY(CIU_SOFT_PRST),
	_ENTRY(CIU_PCI_INTA)
};

void
octeon_ciu_dump(void)
{
	octeon_ciu_dump_regs();
}

void
octeon_ciu_dump_regs(void)
{
	const struct octeon_ciu_dump_reg_entry *reg;
	uint64_t tmp;
	char buf[512];
	int i;

	for (i = 0; i < (int)__arraycount(octeon_ciu_dump_regs_entries); i++) {
		reg = &octeon_ciu_dump_regs_entries[i];
		tmp = octeon_xkphys_read_8(reg->address);
		if (reg->format == NULL) {
			snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		} else {
			snprintb(buf, sizeof(buf), reg->format, tmp);
		}
		printf("\t%-24s: %s\n", reg->name, buf);
	}
}
#endif
