/*	$NetBSD: sebuf.c,v 1.12 2003/07/15 03:36:15 lukem Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Sun3/E SCSI/Ethernet board.  This is a VME board with some memory,
 * an Intel Ether, and an NCR5380 SCSI with a cheap DMA engine.
 * Note that the SCSI DMA engine can ONLY access the memory on
 * the SE board, NOT the main memory, because it can not master
 * VME transfers.
 *
 * This driver ("sebuf") is the parent of two child drivers:
 *   se: yet another variant of NCR 5380 SCSI H/W
 *   ie: yet anotehr variant of Intel 82586 Ethernet
 *
 * The job of this parent is to map the memory and partition it for
 * the two children.  This driver has no device nodes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sebuf.c,v 1.12 2003/07/15 03:36:15 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/tty.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include "sereg.h"
#include "sevar.h"

struct sebuf_softc {
	struct	device sc_dev;		/* base device (required) */
	struct sebuf_regs *sc_regs;
};

/*
 * Autoconfig attachment
 */

static int  sebuf_match __P((struct device *, struct cfdata *, void *));
static void sebuf_attach __P((struct device *, struct device *, void *));
static int  sebuf_print __P((void *, const char *));

CFATTACH_DECL(sebuf, sizeof(struct sebuf_softc),
    sebuf_match, sebuf_attach, NULL, NULL);

static int
sebuf_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct confargs *ca = args;
	struct se_regs *sreg;
	struct ie_regs *ereg;
	int pa, x;

	if (ca->ca_paddr == -1)
		return (0);

	/* Is it there at all? */
	pa = ca->ca_paddr;
	x = bus_peek(ca->ca_bustype, pa, 2);
	if (x == -1)
		return (0);

	/* Look at the CSR for the SCSI part. */
	pa = ca->ca_paddr + offsetof(struct sebuf_regs, se_scsi_regs);
	sreg = bus_tmapin(ca->ca_bustype, pa);
	/* Write some bits that are wired to zero. */
	sreg->se_csr = 0xFFF3;
	x = peek_word((caddr_t)(&sreg->se_csr));
	bus_tmapout(sreg);
	if ((x == -1) || (x & 0xFCF0)) {
#ifdef	DEBUG
		printf("sebuf_match: SCSI csr=0x%x\n", x);
#endif
		return (0);
	}

	/* Look at the CSR for the Ethernet part. */
	pa = ca->ca_paddr + offsetof(struct sebuf_regs, se_eth_regs);
	ereg = bus_tmapin(ca->ca_bustype, pa);
	/* Write some bits that are wired to zero. */
	ereg->ie_csr = 0x0FFF;
	x = peek_word((caddr_t)(&ereg->ie_csr));
	bus_tmapout(ereg);
	if ((x == -1) || (x & 0xFFF)) {
#ifdef	DEBUG
		printf("sebuf_match: Ether csr=0x%x\n", x);
#endif
		return (0);
	}

	/* Default interrupt priority always splbio==2 */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = 2;

	return (1);
}

static void
sebuf_attach(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	struct sebuf_softc *sc = (struct sebuf_softc *)self;
	struct confargs *ca = args;
	struct sebuf_attach_args aa;
	struct sebuf_regs *regs;

	printf("\n");

	if (ca->ca_intpri != 2)
		panic("sebuf: bad level");

	/* Map in the whole board. */
	regs = (struct sebuf_regs *)
		bus_mapin(ca->ca_bustype, ca->ca_paddr,
			  sizeof(struct sebuf_regs));
	if (regs == NULL)
		panic("sebuf_attach");
	sc->sc_regs = regs;

	/* Attach the SCSI child. */
	aa.ca = *ca;	/* structure copy */
	aa.ca.ca_paddr = 0; /* prevent misuse */
	aa.name = "se";
	aa.buf  = &regs->se_scsi_buf[0];
	aa.blen = SE_NCRBUFSIZE;
	aa.regs = &regs->se_scsi_regs;
	(void) config_found(self, (void *) &aa, sebuf_print);

	/* Attach the Ethernet child. */
	aa.ca.ca_intpri++;
	aa.ca.ca_intvec++;
	aa.name = "ie";
	aa.buf  = &regs->se_eth_buf[0];
	aa.blen = SE_IEBUFSIZE;
	aa.regs = &regs->se_eth_regs;
	(void) config_found(self, (void *) &aa, sebuf_print);
}

static int
sebuf_print(aux, name)
	void *aux;
	const char *name;
{

	if (name != NULL)
		aprint_normal("%s: ", name);

	return UNCONF;
}
