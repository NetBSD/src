/*	$NetBSD: p_dti_tyne.c,v 1.13 2007/06/26 12:03:32 tsutsui Exp $	*/
/*	$OpenBSD: machdep.c,v 1.36 1999/05/22 21:22:19 weingart Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	from: @(#)machdep.c	8.3 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	from: @(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: p_dti_tyne.c,v 1.13 2007/06/26 12:03:32 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/platform.h>
#include <machine/wired_map.h>
#include <mips/pte.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/i8042reg.h>

#include <arc/dti/desktech.h>

void arc_sysreset(bus_addr_t, bus_size_t);

#include "pc.h"
#if NPC_ISA > 0 || NOPMS_ISA > 0
#include <arc/dev/pcconsvar.h>
#include <arc/isa/pccons_isavar.h>
#endif

#include "btl.h"
#if NBTL > 0
#include <arc/dti/btlvar.h>
#endif

const char *p_dti_tyne_mainbusdevs[] = {
	"tyneisabr",
	NULL
};

void p_dti_tyne_init(void);
void p_dti_tyne_cons_init(void);
void p_dti_tyne_reset(void);

struct platform platform_desktech_tyne = {
	"DESKTECH-TYNE",
	"DESKTECH",
	"",
	"DeskStation Tyne",
	"DESKTECH",
	133, /* MHz */
	p_dti_tyne_mainbusdevs,
	platform_generic_match,
	p_dti_tyne_init,
	c_isa_cons_init,
	p_dti_tyne_reset,
	arc_set_intr,
};

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
/* XXX see comments in p_dti_tyne_init() */
static const uint32_t dti_tyne_ipl_sr_bits[_IPL_N] = {
	[IPL_NONE] =0,
	[IPL_SOFT] =
	    MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTCLOCK] =
	    MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_SOFTSERIAL] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_BIO] =	/* XXX */
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0|
	    MIPS_INT_MASK_1|
	    MIPS_INT_MASK_2|
	    MIPS_INT_MASK_3|
	    MIPS_INT_MASK_4|
	    MIPS_INT_MASK_5,
	[IPL_NET] =	/* XXX */
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0|
	    MIPS_INT_MASK_1|
	    MIPS_INT_MASK_2|
	    MIPS_INT_MASK_3|
	    MIPS_INT_MASK_4|
	    MIPS_INT_MASK_5,
	[IPL_TTY] =	/* XXX */
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0|
	    MIPS_INT_MASK_1|
	    MIPS_INT_MASK_2|
	    MIPS_INT_MASK_3|
	    MIPS_INT_MASK_4|
	    MIPS_INT_MASK_5,
	[IPL_CLOCK] =	/* XXX */
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0|
	    MIPS_INT_MASK_1|
	    MIPS_INT_MASK_2|
	    MIPS_INT_MASK_3|
	    MIPS_INT_MASK_4|
	    MIPS_INT_MASK_5,
};

#if NPC_ISA > 0 || NOPMS_ISA > 0
/*
 * platform-dependent pccons configuration
 */

void pccons_dti_tyne_init(void);

struct pccons_config pccons_dti_tyne_conf = {
	0x3b4, 0xb0000,	/* mono: iobase, memaddr */
	0x3d4, 0xb8000,	/* cga:  iobase, memaddr */
	0x64, 0x60,	/* kbdc: cmdport, dataport */
	pccons_dti_tyne_init,
};

void
pccons_dti_tyne_init(void)
{

	outb(arc_bus_io.bs_vbase + 0x3ce, 6);	/* Correct video mode */
	outb(arc_bus_io.bs_vbase + 0x3cf,
	 inb(arc_bus_io.bs_vbase + 0x3cf) | 0xc);
	kbc_put8042cmd(CMDBYTE);		/* Want XT codes.. */
}

#endif /* NPC_ISA > 0 || NOPMS_ISA > 0 */

#if NBTL > 0
/*
 * platform-dependent btl configuration
 */

void btl_dti_tyne_bouncemem(u_int *, u_int *);
uint32_t btl_dti_tyne_kvtophys(uint32_t);
uint32_t btl_dti_tyne_phystokv(uint32_t);

struct btl_config btl_dti_tyne_conf = {
	btl_dti_tyne_bouncemem,
	btl_dti_tyne_kvtophys,
	btl_dti_tyne_phystokv,
};

void
btl_dti_tyne_bouncemem(u_int *basep, u_int*sizep)
{

	*basep = TYNE_V_BOUNCE;
	*sizep = TYNE_S_BOUNCE;
}

uint32_t
btl_dti_tyne_kvtophys(uint32_t v)
{
	return (v & 0x7fffff) | 0x800000;
}

uint32_t
btl_dti_tyne_phystokv(uint32_t p)
{

	return (p & 0x7fffff) | TYNE_V_BOUNCE;
}
#endif /* NBTL > 0 */

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
p_dti_tyne_init(void)
{
	/*
	 * XXX - should be enabled, if tested.
	 *
	 * We use safe default for now, because this platform is untested.
	 * In other words, the following may not be needed at all.
	 */
	vm_page_zero_enable = false;

	/*
	 * Initialize I/O address offset
	 */
	arc_bus_space_init(&arc_bus_io, "tyneisaio",
	    TYNE_P_ISA_IO, TYNE_V_ISA_IO, 0, TYNE_S_ISA_IO);
	arc_bus_space_init(&arc_bus_mem, "tyneisamem",
	    TYNE_P_ISA_MEM, TYNE_V_ISA_MEM, 0, TYNE_S_ISA_MEM);

	/*
	 * Initialize wired TLB for I/O space which is used on early stage
	 */
	arc_wired_enter_page(TYNE_V_BOUNCE, TYNE_P_BOUNCE,
	    MIPS3_PG_SIZE_MASK_TO_SIZE(MIPS3_PG_SIZE_256K));

	arc_wired_enter_page(TYNE_V_ISA_IO, TYNE_P_ISA_IO, TYNE_S_ISA_IO);
	arc_wired_enter_page(TYNE_V_ISA_MEM, TYNE_P_ISA_MEM, TYNE_S_ISA_MEM);

	arc_wired_enter_page(0xe3000000, 0xfff00000,
	    MIPS3_PG_SIZE_MASK_TO_SIZE(MIPS3_PG_SIZE_4K));

	/*
	 * Initialize interrupt priority
	 */
	/*
	 * XXX
	 *	- rewrite spl handling to allow ISA clock > bio|tty|net
	 * or
	 *	- use MIP3_INTERNAL_TIMER_INTERRUPT for clock
	 */
	ipl_sr_bits = dti_tyne_ipl_sr_bits;

	/*
	 * common configuration for DTI platforms
	 */
	c_isa_init();

#if NPC_ISA > 0 || NOPMS_ISA > 0
	/* platform-dependent pccons configuration */
	pccons_isa_conf = &pccons_dti_tyne_conf;
#endif

#if NBTL > 0
	/* platform-dependent btl configuration */
	btl_conf = &btl_dti_tyne_conf;
#endif
}

void
p_dti_tyne_reset(void)
{

	arc_sysreset(TYNE_V_ISA_IO + IO_KBD, KBCMDP);
}
