/*	$NetBSD: cons_machdep.c,v 1.8 2003/07/15 01:37:40 lukem Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-dependent Console Initialisation
 *
 * XXX: Needs a rototil.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cons_machdep.c,v 1.8 2003/07/15 01:37:40 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/termios.h>
#include <sys/ttydefaults.h>
#include <sys/ksyms.h>

#include <machine/bus.h>

#include <evbsh5/evbsh5/machdep.h>

#include <evbsh5/dev/superiovar.h>

#include <dev/cons.h>

#ifdef DDB
#include <sys/boot_flag.h>
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include "com.h"
#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
static bus_space_tag_t comtag;
static bus_addr_t comaddr;
cons_decl(com);
#define	COM_SPEED	38400
#endif


#include "scif.h"
#if NSCIF > 0
#include <sh5/dev/pbridgereg.h>
#include <sh5/dev/scifreg.h>
#include <sh5/dev/scifvar.h>
/* XXX: Gross hack until scif is re-written */
bus_space_tag_t _sh5_scif_bt;
bus_space_handle_t _sh5_scif_bh;
#endif


#include "dtfcons.h"
#if NDTFCONS > 0
#include <sh5/dev/dtfconsvar.h>
cons_decl(dtfcons);
#endif

#include "ksyms.h"

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{

#if NSCIF > 0
	_sh5_scif_bt = &_sh5_bus_space_tag;

	bus_space_subregion(_sh5_scif_bt, _evbsh5_bh_pbridge,
	    PBRIDGE_OFFSET_SCIF, SCIF_REG_SZ, &_sh5_scif_bh);
#endif

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();
#if NKSYMS || defined(DDB) || defined(LKM)
	{
		extern void *symbol_table;
		extern long symbol_table_size;

		ksyms_init((int)symbol_table_size, symbol_table,
		    (void *)((long)symbol_table + symbol_table_size));
	}
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

#if NCOM > 0
void
comcnprobe(struct consdev *cn)
{
	extern const struct cdevsw com_cdevsw;
	bus_space_handle_t bh;
	int i, pri = CN_DEAD;

	i = superio_console_tag(&_sh5_bus_space_tag, 0, &comtag, &comaddr);
	if (i < 0)
		goto done;

	i = bus_space_map(comtag, comaddr, COM_NPORTS, 0, &bh);
	if (i)
		goto done;

	i = comprobe1(comtag, bh);
	bus_space_unmap(comtag, bh, COM_NPORTS);
	if (i == 0)
		goto done;

	cn->cn_dev = makedev(cdevsw_lookup_major(&com_cdevsw), 0);
	pri = CN_NORMAL;

done:
	cn->cn_pri = pri;
}

void
comcninit(struct consdev *cn)
{

	comcnattach(comtag, comaddr, COM_SPEED, COM_FREQ, COM_TYPE_NORMAL,
	    TTYDEF_CFLAG);
}
#endif /* NCOM > 0 */

#if NDTFCONS > 0
void
dtfconscnprobe(struct consdev *cn)
{
	extern const struct cdevsw dtf_cdevsw;

	cn->cn_dev = makedev(cdevsw_lookup_major(&dtf_cdevsw), 0);
	cn->cn_pri = CN_NORMAL;
}

void
dtfconscninit(struct consdev *cn)
{

	dtfcons_cnattach();
}
#endif /* NDTFCONS > 0 */
