/*	$NetBSD: if_ie_sebuf.c,v 1.13.26.1 2007/03/12 05:51:06 rmind Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ie_sebuf.c,v 1.13.26.1 2007/03/12 05:51:06 rmind Exp $");

#include "opt_inet.h"

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

static void ie_sebuf_reset(struct ie_softc *);
static void ie_sebuf_attend(struct ie_softc *);
static void ie_sebuf_run(struct ie_softc *);

/*
 * zero/copy functions: OBIO can use the normal functions, but VME
 *    must do only byte or half-word (16 bit) accesses...
 */
static void *wmemcpy(void *, const void *, size_t);
static void *wmemset(void *, int, size_t);


/*
 * New-style autoconfig attachment
 */

static int  ie_sebuf_match(struct device *, struct cfdata *, void *);
static void ie_sebuf_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ie_sebuf, sizeof(struct ie_softc),
    ie_sebuf_match, ie_sebuf_attach, NULL, NULL);

static int 
ie_sebuf_match(struct device *parent, struct cfdata *cf, void *args)
{
	struct sebuf_attach_args *aa = args;

	/* Match by name. */
	if (strcmp(aa->name, "ie"))
		return (0);

	/* Anyting else to check? */

	return (1);
}

static void 
ie_sebuf_attach(struct device *parent, struct device *self, void *args)
{
	struct ie_softc *sc = (void *) self;
	struct sebuf_attach_args *aa = args;
	volatile struct ie_regs *regs;
	int     off;

	sc->hard_type = IE_VME3E;
	sc->reset_586 = ie_sebuf_reset;
	sc->chan_attn = ie_sebuf_attend;
	sc->run_586   = ie_sebuf_run;
	sc->sc_memcpy = wmemcpy;
	sc->sc_memset = wmemset;

	/* Control regs mapped by parent. */
	sc->sc_reg = aa->regs;
	regs = (struct ie_regs *) sc->sc_reg;

	/*
	 * On this hardware, the i82586 address zero
	 * maps to the start of the board, which we
	 * happen to know is 128K below (XXX).
	 */
	sc->sc_iobase = aa->buf - 0x20000;

	/*
	 * There is 128K of memory for the i82586 on
	 * the Sun3/E SCSI/Ethernet board, and the
	 * "sebuf" driver has mapped it in for us.
	 * (Fixed in hardware; NOT configurable!)
	 */
	if (aa->blen < SE_IEBUFSIZE)
		panic("ie_sebuf: bad size");
	sc->sc_msize = SE_IEBUFSIZE;
	sc->sc_maddr = aa->buf;

	/* Clear the memory. */
	(sc->sc_memset)(sc->sc_maddr, 0, sc->sc_msize);

	/*
	 * XXX:  Unfortunately, the common driver code
	 * is not ready to deal with more than 64K of
	 * memory, so skip the first 64K.  Too bad.
	 * Note: device needs the SCP at the end.
	 */
	sc->sc_msize -= 0x10000;
	sc->sc_maddr = (char *)sc->sc_maddr + 0x10000;

	/*
	 * Set the System Configuration Pointer (SCP).
	 * Its location is system-dependent because the
	 * i82586 reads it from a fixed physical address.
	 * On this hardware, the i82586 address is just
	 * masked down to 17 bits, so the SCP is found
	 * at the end of the RAM on the VME board.
	 */
	off = IE_SCP_ADDR & 0xFFFF;
	sc->scp = (volatile void *)((char *)sc->sc_maddr + off);

	/*
	 * The rest of ram is used for buffers, etc.
	 */
	sc->buf_area = sc->sc_maddr;
	sc->buf_area_sz = off;

	/* Install interrupt handler. */
	regs->ie_ivec = aa->ca.ca_intvec;
	isr_add_vectored(ie_intr, (void *)sc,
		aa->ca.ca_intpri, aa->ca.ca_intvec);

	/* Set the ethernet address. */
	idprom_etheraddr(sc->sc_addr);

	/* Do machine-independent parts of attach. */
	ie_attach(sc);

}

/* Whack the "channel attetion" line. */
void 
ie_sebuf_attend(struct ie_softc *sc)
{
	volatile struct ie_regs *regs = (struct ie_regs *) sc->sc_reg;

	regs->ie_csr |= IE_CSR_ATTEN;	/* flag! */
	regs->ie_csr &= ~IE_CSR_ATTEN;	/* down. */
}

/*
 * This is called during driver attach.
 * Reset and initialize.
 */
void 
ie_sebuf_reset(struct ie_softc *sc)
{
	volatile struct ie_regs *regs = (struct ie_regs *) sc->sc_reg;
	regs->ie_csr = IE_CSR_RESET;
	delay(20);
	regs->ie_csr = (IE_CSR_NOLOOP | IE_CSR_IENAB);
}

/*
 * This is called at the end of ieinit().
 * optional.
 */
void 
ie_sebuf_run(struct ie_softc *sc)
{
	/* do it all in reset */
}

/*
 * wmemcpy/wmemset - like memcpy/memset but largest access is 16-bits,
 * and also does byte swaps...
 * XXX - Would be nice to have asm versions in some library...
 */

static void *
wmemset(void *vb, int val, size_t l)
{
	u_char *b = vb;
	u_char *be = b + l;
	u_short *sp;

	if (l == 0)
		return (vb);

	/* front, */
	if ((u_long)b & 1)
		*b++ = val;

	/* back, */
	if (b != be && ((u_long)be & 1) != 0) {
		be--;
		*be = val;
	}

	/* and middle. */
	sp = (u_short *)b;
	while (sp != (u_short *)be)
		*sp++ = val;

	return (vb);
}

static void *
wmemcpy(void *dst, const void *src, size_t l)
{
	const u_char *b1e, *b1 = src;
	u_char *b2 = dst;
	const u_short *sp;
	int bstore = 0;

	if (l == 0)
		return (dst);

	/* front, */
	if ((u_long)b1 & 1) {
		*b2++ = *b1++;
		l--;
	}

	/* middle, */
	sp = (const u_short *)b1;
	b1e = b1 + l;
	if (l & 1)
		b1e--;
	bstore = (u_long)b2 & 1;

	while (sp < (const u_short *)b1e) {
		if (bstore) {
			b2[1] = *sp & 0xff;
			b2[0] = *sp >> 8;
		} else
			*((short *)b2) = *sp;
		sp++;
		b2 += 2;
	}

	/* and back. */
	if (l & 1)
		*b2 = *b1e;

	return (dst);
}
