/*	$NetBSD: p_sni_rm200pci.c,v 1.13 2010/11/12 16:09:57 uebayasi Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: p_sni_rm200pci.c,v 1.13 2010/11/12 16:09:57 uebayasi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/platform.h>
#include <machine/wired_map.h>

#include <mips/pte.h>

void p_sni_rm200pci_init(void);
void p_sni_rm200pci_cons_init(void);

#include "com.h"
#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

struct platform platform_sni_rm200pci = {
	"RM200PCI",
	NULL, /* unknown */
	"",
	"RM200",
	"Siemens Nixdorf",
	150, /* MHz ?? */
	NULL, /* XXX */
	platform_generic_match,
	p_sni_rm200pci_init,
	p_sni_rm200pci_cons_init,
	platform_nop, /* reset */
	arc_set_intr, /* ??? */
};

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
/* XXX lack of hardware info for sni_rm200pci */
static const uint32_t sni_rm200pci_ipl_sr_bits[_IPL_N] = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] =
	    MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_VM] =	/* XXX */
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
	[IPL_SCHED] =	/* XXX */
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
};

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
p_sni_rm200pci_init(void)
{

	/*
	 * Initialize wired TLB for I/O space which is used on early stage
	 */

	/*
	 * Initialize interrupt priority
	 */
	ipl_sr_bits = sni_rm200pci_ipl_sr_bits;

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
	arc_init_wired_map();
#if 0
	arc_bus_space_init(&arc_bus_io, "rm200isaio",
	    RM200_P_ISA_IO, RM200_V_ISA_IO, 0, RM200_S_ISA_IO);
	arc_bus_space_init(&arc_bus_mem, "rm200isamem",
	    RM200_P_ISA_MEM, RM200_V_ISA_MEM, 0, RM200_S_ISA_MEM);
#endif
}

void
p_sni_rm200pci_cons_init(void)
{

	if (!com_console) {
		/* XXX For now... */
	}
	if (com_console_address == 0) {
#if 0		/* XXX */
		com_console_address = xxx;
#else
		panic("console address unknown");
#endif
	}
	comcnattach(&arc_bus_io /* XXX? */, com_console_address,
	    com_console_speed, com_freq, COM_TYPE_NORMAL, com_console_mode);
}
