/*	$NetBSD: p_ms_jazz.c,v 1.1 2001/06/13 15:28:23 soda Exp $	*/
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

#include <sys/param.h>

#include <machine/autoconf.h>
#include <machine/platform.h>

#include <arc/jazz/pica.h>
#include <arc/jazz/jazziovar.h>

#include "com.h"

#include "asc.h"
#if NASC > 0
#include <arc/jazz/ascreg.h>
#include <arc/jazz/ascvar.h>
#endif

/* MAGNUM. NEC goes here too. */

#ifndef COM_FREQ_MAGNUM
#if 0
#define COM_FREQ_MAGNUM	4233600 /* 4.2336MHz - ARC? */
#else
#define COM_FREQ_MAGNUM	8192000	/* 8.192 MHz - NEC RISCstation M402 */
#endif
#endif /* COM_FREQ_MAGNUM */

#if NASC > 0
struct asc_config asc_ms_jazz_conf = {
	&asc_timing_40mhz,

	/* only if EPL is FE (Feature Enable bit for 53CF94) */
	ASC_CNFG3_FCLK, /* clock 40MHz */
};
#endif

void p_ms_jazz_init __P((void));

struct platform platform_microsoft_jazz = {
	"Microsoft-Jazz",
	"MIPS MAG",
	"",
	"Magnum",
	"MIPS",
	150, /* MHz */
	c_jazz_eisa_mainbusdevs,
	platform_generic_match,
	p_ms_jazz_init,
	c_jazz_eisa_cons_init,
	jazzio_reset,
	c_magnum_set_intr,
};

/*
 * jazzio bus configuration
 */

struct pica_dev mips_magnum_r4000_cpu[] = {
	{{ "timer",	-1, 0, },	(void *)R4030_SYS_IT_VALUE, },
	{{ "dallas_rtc", -1, 0, },	(void *)PICA_SYS_CLOCK, },
	{{ "lpt",	0, 0, },	(void *)PICA_SYS_PAR1, },
	{{ "fdc",	1, 0, },	(void *)PICA_SYS_FLOPPY, },
	{{ "MAGNUM",	2, 0, },	(void *)PICA_SYS_SOUND,},
	{{ "VXL",	3, 0, },	(void *)PICA_V_LOCAL_VIDEO, },
	{{ "sonic",	4, 0, },	(void *)PICA_SYS_SONIC, },
	{{ "asc",	5, 0, },	(void *)PICA_SYS_SCSI, },
	{{ "pckbd",	6, 0, },	(void *)PICA_SYS_KBD, },
	{{ "pms",	7, 0, },	(void *)PICA_SYS_KBD, },
	{{ "com",	8, 0, },	(void *)PICA_SYS_COM1, },
	{{ "com",	9, 0, },	(void *)PICA_SYS_COM2, },
	{{ NULL,	-1, 0, },	(void *)NULL, },
};

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
p_ms_jazz_init()
{
	c_magnum_init();

	/* jazzio bus configuration */
	jazzio_devconfig = mips_magnum_r4000_cpu;

#if NASC > 0
	asc_conf = &asc_ms_jazz_conf;
#endif

#if NCOM > 0
	com_freq = COM_FREQ_MAGNUM;
#endif
}
