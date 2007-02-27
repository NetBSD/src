/*	$NetBSD: p_acer_pica_61.c,v 1.9.26.1 2007/02/27 16:49:02 yamt Exp $	*/
/*	$OpenBSD: picabus.c,v 1.11 1999/01/11 05:11:10 millert Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * Author: Per Fogelstrom. (Mips R4x00)
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: p_acer_pica_61.c,v 1.9.26.1 2007/02/27 16:49:02 yamt Exp $");

#include <sys/param.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/platform.h>

#include <arc/jazz/pica.h>
#include <arc/jazz/jazziovar.h>

/* ALI PICA 61 and some MAGNUM? */

void p_acer_pica_61_init(void);

struct platform platform_acer_pica_61 = {
	"PICA-61",
	"MIPS MAG",
	"",
	"PICA-61",
	"Acer",
	150, /* MHz */
	c_jazz_eisa_mainbusdevs,
	platform_generic_match,
	p_acer_pica_61_init,
	c_jazz_eisa_cons_init,
	jazzio_reset,
	c_magnum_set_intr,
};

/*
 * jazzio bus configuration
 */

struct pica_dev acer_pica_61_cpu[] = {
	{{ "timer",	-1, 0, },	(void *)R4030_SYS_IT_VALUE, },
	{{ "dallas_rtc", -1, 0, },	(void *)PICA_SYS_CLOCK, },
	{{ "LPT1",	0, 0, },	(void *)PICA_SYS_PAR1, },
	{{ "I82077",	1, 0, },	(void *)PICA_SYS_FLOPPY, },
	{{ "MAGNUM",	2, 0, },	(void *)PICA_SYS_SOUND,},
	{{ "ALI_S3",	3, 0, },	(void *)PICA_V_LOCAL_VIDEO, },
	{{ "SONIC",	4, 0, },	(void *)PICA_SYS_SONIC, },
	{{ "ESP216",	5, 0, },	(void *)PICA_SYS_SCSI, },
	{{ "I8742",	6, 0, },	(void *)PICA_SYS_KBD, },
	{{ "pms",	7, 0, },	(void *)PICA_SYS_KBD, }, /* XXX */
	{{ "COM1",	8, 0, },	(void *)PICA_SYS_COM1, },
	{{ "COM2",	9, 0, },	(void *)PICA_SYS_COM2, },
	{{ NULL,	-1, 0, },	NULL, },
};

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
p_acer_pica_61_init(void)
{

	/*
	 * PICA-61 has PC-style coherent(?) 128KB L2 cache,
	 * and mips_L2CachePresent == 0 on this machine.
	 *
	 * if page zero in the idle loop is enabled,
	 * commands dump core due to incoherent cache.
	 */
	vm_page_zero_enable = false; /* XXX - should be enabled */

	c_magnum_init();

	/* chipset-dependent jazzio bus configuration */
	jazzio_devconfig = acer_pica_61_cpu;
}
