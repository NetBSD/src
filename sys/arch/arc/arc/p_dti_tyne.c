/*	$NetBSD: p_dti_tyne.c,v 1.1 2001/06/13 15:27:18 soda Exp $	*/
/*	$OpenBSD: machdep.c,v 1.36 1999/05/22 21:22:19 weingart Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/platform.h>
#include <mips/pte.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/i8042reg.h>

#include <arc/arc/wired_map.h>
#include <arc/dti/desktech.h>

void arc_sysreset __P((bus_addr_t, bus_size_t));

#include "pc.h"
#if NPC_ISA > 0 || NOPMS_ISA > 0
#include <arc/dev/pcconsvar.h>
#include <arc/isa/pccons_isavar.h>
#endif

#include "btl.h"
#if NBTL > 0
#include <arc/dti/btlvar.h>
#endif

char *p_dti_tyne_mainbusdevs[] = {
	"tyneisabr",
	NULL
};

void p_dti_tyne_init __P((void));
void p_dti_tyne_cons_init __P((void));
void p_dti_tyne_reset __P((void));

struct platform platform_desktech_tyne = {
	"DESKTECH-TYNE",
	"DESKTECH",
	"",
	"DESKstation Tyne",
	"DESKTECH",
	150, /* MHz ?? */
	p_dti_tyne_mainbusdevs,
	platform_generic_match,
	p_dti_tyne_init,
	c_isa_cons_init,
	p_dti_tyne_reset,
	arc_set_intr,
};

#if NPC_ISA > 0 || NOPMS_ISA > 0
/*
 * platform-dependent pccons configuration
 */

void pccons_dti_tyne_init __P((void));

struct pccons_config pccons_dti_tyne_conf = {
	0x3b4, 0xb0000,	/* mono: iobase, memaddr */
	0x3d4, 0xb8000,	/* cga:  iobase, memaddr */
	0x64, 0x60,	/* kbdc: cmdport, dataport */
	pccons_dti_tyne_init,
};

void
pccons_dti_tyne_init()
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

void btl_dti_tyne_bouncemem __P((u_int *, u_int *));
u_int32_t btl_dti_tyne_kvtophys __P((u_int32_t));
u_int32_t btl_dti_tyne_phystokv __P((u_int32_t));

struct btl_config btl_dti_tyne_conf = {
	btl_dti_tyne_bouncemem,
	btl_dti_tyne_kvtophys,
	btl_dti_tyne_phystokv,
};

void
btl_dti_tyne_bouncemem(basep, sizep)
	u_int *basep, *sizep;
{
	*basep = TYNE_V_BOUNCE;
	*sizep = TYNE_S_BOUNCE;
}

u_int32_t
btl_dti_tyne_kvtophys(v)
	u_int32_t v;
{
	return ((v & 0x7fffff) | 0x800000);
}

u_int32_t
btl_dti_tyne_phystokv(p)
	u_int32_t p;
{
	return ((v & 0x7fffff) | TYNE_V_BOUNCE);
}
#endif /* NBTL > 0 */

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
p_dti_tyne_init()
{
	/*
	 * XXX - should be enabled, if tested.
	 *
	 * We use safe default for now, because this platform is untested.
	 * In other words, the following may not be needed at all.
	 */
	vm_page_zero_enable = FALSE;

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
	arc_enter_wired(TYNE_V_BOUNCE, TYNE_P_BOUNCE, 0, MIPS3_PG_SIZE_256K);
	arc_enter_wired(TYNE_V_ISA_IO, TYNE_P_ISA_IO, 0, MIPS3_PG_SIZE_1M);
	arc_enter_wired(TYNE_V_ISA_MEM, TYNE_P_ISA_MEM, 0, MIPS3_PG_SIZE_1M);
	arc_enter_wired(0xe3000000, 0xfff00000, 0, MIPS3_PG_SIZE_4K);

	/*
	 * Initialize interrupt priority
	 */
	/*
	 * XXX
	 *	- rewrite spl handling to allow ISA clock > bio|tty|net
	 * or
	 *	- use MIP3_INTERNAL_TIMER_INTERRUPT for clock
	 */

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
p_dti_tyne_reset()
{
	arc_sysreset(TYNE_V_ISA_IO + IO_KBD, KBCMDP);
}
