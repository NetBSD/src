/*	$NetBSD: promdev.c,v 1.1 2001/06/14 12:57:15 fredette Exp $ */

/*
 * Copyright (c) 1995 Gordon W. Ross
 * Copyright (c) 1993 Paul Kranenburg
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <machine/mon.h>

#include <stand.h>

#include "libsa.h"
#include "dvma.h"
#include "saio.h"

int promdev_inuse;

/*
 * Note: caller sets the fields:
 *	si->si_boottab
 *	si->si_ctlr
 *	si->si_unit
 *	si->si_boff
 */

int
prom_iopen(si)
	struct saioreq *si;
{
	struct boottab *ops;
	struct devinfo *dip;
	int	i, ctlr, error;

	if (promdev_inuse)
		return(EMFILE);

	ops = si->si_boottab;
	dip = ops->b_devinfo;
	ctlr = si->si_ctlr;

#ifdef DEBUG_PROM
	if (debug) {
		printf("Boot device type: %s\n", ops->b_desc);
	}
#endif

	if (!_is2) {
#ifdef DEBUG_PROM
		if (debug) {
		printf("d_devbytes=%d\n", dip->d_devbytes);
		printf("d_dmabytes=%d\n", dip->d_dmabytes);
		printf("d_localbytes=%d\n", dip->d_localbytes);
		printf("d_devtype=%d\n", dip->d_devtype);
		printf("d_maxiobytes=%d\n", dip->d_maxiobytes);
		printf("d_stdcount=%d\n", dip->d_stdcount);
		for (i = 0; i < dip->d_stdcount; i++)
			printf("d_stdaddrs[i]=0x%x\n",
				   i, dip->d_stdaddrs[0]);
	}
#endif

	if (dip->d_devbytes && dip->d_stdcount) {
		if (ctlr >= dip->d_stdcount) {
			printf("Invalid controller number\n");
			return(ENXIO);
		}
		si->si_devaddr = dev_mapin(dip->d_devtype,
			dip->d_stdaddrs[ctlr], dip->d_devbytes);
#ifdef	DEBUG_PROM
		if (debug)
			printf("prom_iopen: devaddr=0x%x\n", si->si_devaddr);
#endif
	}

	if (dip->d_dmabytes) {
		si->si_dmaaddr = dvma_alloc(dip->d_dmabytes);
#ifdef	DEBUG_PROM
		if (debug)
			printf("prom_iopen: dmaaddr=0x%x\n", si->si_dmaaddr);
#endif
	}

	if (dip->d_localbytes) {
		si->si_devdata = alloc(dip->d_localbytes);
#ifdef	DEBUG_PROM
		if (debug)
			printf("prom_iopen: devdata=0x%x\n", si->si_devdata);
#endif
		}
	}

	/* OK, call the PROM device open routine. */
#ifdef	DEBUG_PROM
	if (debug)
		printf("prom_iopen: calling prom open...\n");
#endif
	error = (*ops->b_open)(si);
	if (error != 0) {
		printf("prom_iopen: \"%s\" error=%d\n",
			   ops->b_desc, error);
		return (ENXIO);
	}
#ifdef	DEBUG_PROM
	if (debug)
		printf("prom_iopen: prom open returned %d\n", error);
#endif

	promdev_inuse++;
	return (0);
}

void
prom_iclose(si)
	struct saioreq *si;
{
	struct boottab *ops;
	struct devinfo *dip;

	if (promdev_inuse == 0)
		return;

	ops = si->si_boottab;
	dip = ops->b_devinfo;

#ifdef	DEBUG_PROM
	if (debug)
		printf("prom_iclose: calling prom close...\n");
#endif
	(*ops->b_close)(si);

	promdev_inuse = 0;
}

