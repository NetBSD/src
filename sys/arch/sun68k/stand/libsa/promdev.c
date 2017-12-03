/*	$NetBSD: promdev.c,v 1.5.24.2 2017/12/03 11:36:46 jdolecek Exp $ */

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

#ifdef DEBUG_PROM
# define DPRINTF(fmt, ...) \
	do { \
		if (debug) \
			printf("%s: " fmt "\n", __func__, __VA_ARGS__); \
	} while (/*CONSTCOND*/0)
#else
# define DPRINTF(fmt, ...)
#endif

/*
 * Note: caller sets the fields:
 *	si->si_boottab
 *	si->si_ctlr
 *	si->si_unit
 *	si->si_boff
 */

int 
prom_iopen(struct saioreq *si)
{
	struct boottab *ops;
	struct devinfo *dip;
	int	ctlr, error;

	if (promdev_inuse)
		return EMFILE;

	ops = si->si_boottab;
	dip = ops->b_devinfo;
	ctlr = si->si_ctlr;


	DPRINTF("Boot device type: %s", ops->b_desc);

	if (!_is2) {
#ifdef DEBUG_PROM
		if (debug) {
			printf("d_devbytes=%d\n", dip->d_devbytes);
			printf("d_dmabytes=%d\n", dip->d_dmabytes);
			printf("d_localbytes=%d\n", dip->d_localbytes);
			printf("d_devtype=%d\n", dip->d_devtype);
			printf("d_maxiobytes=%d\n", dip->d_maxiobytes);
			printf("d_stdcount=%d\n", dip->d_stdcount);
			for (int i = 0; i < dip->d_stdcount; i++)
				printf("d_stdaddrs[%d]=%#x\n",
				    i, dip->d_stdaddrs[0]);
		}
#endif

		if (dip->d_devbytes && dip->d_stdcount) {
			if (ctlr >= dip->d_stdcount) {
				putstr("Invalid controller number\n");
				return ENXIO;
			}
			si->si_devaddr = dev_mapin(dip->d_devtype,
			    dip->d_stdaddrs[ctlr], dip->d_devbytes);
			DPRINTF("devaddr=%#x", si->si_devaddr);
		}

		if (dip->d_dmabytes) {
			si->si_dmaaddr = dvma_alloc(dip->d_dmabytes);
			DPRINTF("dmaaddr=%#x", si->si_dmaaddr);
		}

		if (dip->d_localbytes) {
			si->si_devdata = alloc(dip->d_localbytes);
			DPRINTF("devdata=%#x", si->si_devdata);
		}
	}

	/* OK, call the PROM device open routine. */
	DPRINTF("calling prom open... %p", si);
	error = (*ops->b_open)(si);
	DPRINTF("prom open returned %d", error);
	if (error != 0) {
#if 0		/* XXX: printf is too big for bootxx */
		printf("%s: \"%s\" error=%d\n", __func__,
		    ops->b_desc, error);
#else
		putstr("prom_iopen: prom open failed");
#endif
		return ENXIO;
	}

	promdev_inuse++;
	return 0;
}

void 
prom_iclose(struct saioreq *si)
{
	struct boottab *ops;

	if (promdev_inuse == 0)
		return;

	ops = si->si_boottab;

	DPRINTF("calling prom close... %p", si);
	(*ops->b_close)(si);

	promdev_inuse = 0;
}
