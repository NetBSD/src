/*	$NetBSD: console.c,v 1.10 2003/07/15 03:35:54 lukem Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
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
__KERNEL_RCSID(0, "$NetBSD: console.c,v 1.10 2003/07/15 03:35:54 lukem Exp $");

#include "opt_kgdb.h"
#include "opt_machtypes.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <machine/bus.h>
#include <machine/machtype.h>

#include <dev/cons.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <sgimips/dev/macereg.h>

#include "com.h"
#include "zsc.h"

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comcnmode = CONMODE;

extern struct consdev zs_cn;

void zs_kgdb_init(void);
void kgdb_port_init(void);

void
consinit()
{
	char* consdev;
	char* dbaud;
	int   speed;

	/* Ask ARCS what it is using for console output. */
	consdev = ARCBIOS->GetEnvironmentVariable("ConsoleOut");
	if (consdev == NULL) {
		printf("WARNING: ConsoleOut environment variable not set\n");
		goto force_arcs;
	}

	/* Get comm speed from ARCS */
	dbaud = ARCBIOS->GetEnvironmentVariable("dbaud");
	speed = strtoul(dbaud, NULL, 10);
	

#if (defined(IP20) || defined(IP22)) && (NZSC > 0)
	if (mach_type == MACH_SGI_IP20 || mach_type == MACH_SGI_IP22) {
		if (strlen(consdev) == 9 &&
		    strncmp(consdev, "serial", 6) == 0 &&
		    (consdev[7] == '0' || consdev[7] == '1')) {
			cn_tab = &zs_cn;
			(*cn_tab->cn_init)(cn_tab);
			return;
		}
	}
#endif /* (IP20 || IP22) && (NZSC > 0) */

#if defined(IP32) && (NCOM > 0)
	if (mach_type == MACH_SGI_IP32) {
		if (strlen(consdev) == 9 &&
		    strncmp(consdev, "serial", 6) == 0 &&
		    (consdev[7] == '0' || consdev[7] == '1')) {
			/* XXX: hardcoded MACE iotag */
			if (comcnattach(3, ((consdev[7] == '0') ? 
					MIPS_PHYS_TO_KSEG1(MACE_ISA_SER1_BASE) :
					MIPS_PHYS_TO_KSEG1(MACE_ISA_SER2_BASE)),
					speed, COM_FREQ, COM_TYPE_NORMAL,
					comcnmode) == 0)
				return;

			printf("can't init serial hardware console!\n");
		}
	}
#endif	/* IP32 && (NCOM > 0) */

 force_arcs:
	printf("Using ARCS for console I/O.\n");
}

#if defined(KGDB)
void
kgdb_port_init()
{
# if defined(IP32) && (NCOM > 0)
#  define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)
	if (mach_type == MACH_SGI_IP32)
		com_kgdb_attach(3, 0xbf398000, 9600, COM_FREQ, COM_TYPE_NORMAL,
		    KGDB_DEVMODE);
# endif	/* IP32 && (NCOM > 0) */

# if (defined(IP20) || defined(IP22)) && (NZSC > 0)
	if (mach_type == MACH_SGI_IP20 || mach_type == MACH_SGI_IP22)
		zs_kgdb_init();			/* XXX */
# endif /* (IP20 || IP22) && (NZSC > 0) */
}
#endif	/* KGDB */

