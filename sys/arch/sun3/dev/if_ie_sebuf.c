/*	$NetBSD: if_ie_sebuf.c,v 1.2 1997/10/17 21:49:07 gwr Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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
 * Machine-dependent glue for the Intel Ethernet (ie) driver,
 * as found on the Sun3/E SCSI/Ethernet board.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/dvma.h>
#include <machine/idprom.h>
#include <machine/vmparam.h>

#include "i82586.h"
#include "if_iereg.h"
#include "if_ievar.h"
#include "sereg.h"
#include "sevar.h"

static void ie_sebuf_reset __P((struct ie_softc *));
static void ie_sebuf_attend __P((struct ie_softc *));
static void ie_sebuf_run __P((struct ie_softc *));

/*
 * New-style autoconfig attachment
 */

static int  ie_sebuf_match __P((struct device *, struct cfdata *, void *));
static void ie_sebuf_attach __P((struct device *, struct device *, void *));

struct cfattach ie_sebuf_ca = {
	sizeof(struct ie_softc), ie_sebuf_match, ie_sebuf_attach
};


static int
ie_sebuf_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct sebuf_attach_args *aa = args;

	/* Match by name. */
	if (strcmp(aa->name, "ie"))
		return (0);

	/* Anyting else to check? */

	return (1);
}

static void
ie_sebuf_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	struct ie_softc *sc = (void *) self;
	struct sebuf_attach_args *aa = args;
	volatile struct ie_regs *regs;
	int     off;

	sc->hard_type = IE_VME3E;
	sc->reset_586 = ie_sebuf_reset;
	sc->chan_attn = ie_sebuf_attend;
	sc->run_586   = ie_sebuf_run;
	sc->sc_bcopy = bcopy;
	sc->sc_bzero = bzero;

	/*
	 * Note: the NCR registers occupy the first 32 bytes
	 * of space before the buffer pointer passed to us.
	 * The Ethernet chip actually address space actually
	 * starts _on_top_of_ those 32 bytes.  Otherwise the
	 * addressing is a simple 16-bit implementation.
	 */
	sc->sc_iobase = aa->buf - sizeof(struct se_regs);
	sc->sc_maddr = aa->buf;
	sc->sc_msize = aa->blen;
	sc->sc_reg = aa->regs;
	regs = (volatile struct ie_regs *) sc->sc_reg;

	/* Clear the memory. */
	(sc->sc_bzero)(sc->sc_maddr, sc->sc_msize);

	/*
	 * Set the System Configuration Pointer (SCP).
	 * Its location is system-dependent because the
	 * i82586 reads it from a fixed physical address.
	 * On this hardware, the i82586 address is just
	 * masked down to 16 bits, so the SCP is found
	 * at the end of the RAM on the VME board.
	 */
	off = IE_SCP_ADDR & 0xFFFF;
	sc->scp = (volatile void *) (sc->sc_iobase + off);

	/*
	 * The rest of ram is used for buffers, etc.
	 */
	sc->buf_area    = sc->sc_maddr;
	sc->buf_area_sz = sc->sc_msize;

	/* Set the ethernet address. */
	idprom_etheraddr(sc->sc_addr);

	/* Install interrupt handler. */
	regs->ie_ivec = aa->ca.ca_intvec;
	isr_add_vectored(ie_intr, (void *)sc,
		aa->ca.ca_intpri, aa->ca.ca_intvec);

	/* Do machine-independent parts of attach. */
	ie_attach(sc);

}


/*
 * MULTIBUS/VME support
 */
void 
ie_sebuf_reset(sc)
	struct ie_softc *sc;
{
	volatile struct ie_regs *regs = (struct ie_regs *) sc->sc_reg;
	regs->ie_csr = IE_CSR_RESET;
	delay(10);
	regs->ie_csr = 0;
}

void 
ie_sebuf_attend(sc)
	struct ie_softc *sc;
{
	volatile struct ie_regs *regs = (struct ie_regs *) sc->sc_reg;

	regs->ie_csr |= IE_CSR_ATTEN;	/* flag! */
	regs->ie_csr &= ~IE_CSR_ATTEN;	/* down. */
}

void 
ie_sebuf_run(sc)
	struct ie_softc *sc;
{
	volatile struct ie_regs *regs = (struct ie_regs *) sc->sc_reg;

	regs->ie_csr |= IE_CSR_IENAB;
}
