/* 	$NetBSD: px.c,v 1.3.2.3 2001/04/23 09:42:31 bouyer Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Driver for DEC PixelStamp graphics adapters (PMAG-C).
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/callout.h>

#include <uvm/uvm_extern.h>

#if defined(pmax)
#include <mips/cpuregs.h>
#elif defined(alpha)
#include <alpha/alpha_cpu.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/cons.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/ic/bt459reg.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/sticreg.h>
#include <dev/tc/sticvar.h>

#define	PX_STIC_POLL_OFFSET	0x000000	/* STIC DMA poll space */
#define	PX_STAMP_OFFSET		0x0c0000	/* pixelstamp space on STIC */
#define	PX_STIC_OFFSET		0x180000	/* STIC registers */
#define	PX_VDAC_OFFSET		0x200000	/* VDAC registers (bt459) */
#define	PX_VDAC_RESET_OFFSET	0x300000	/* VDAC reset register */
#define	PX_ROM_OFFSET		0x300000	/* ROM code */

#define	PX_BUF_SIZE		(STIC_PACKET_SIZE*2 + STIC_IMGBUF_SIZE*2)
#define	PX_BUF_ALIGN		32768

static void	px_attach(struct device *, struct device *, void *);
static void	px_init(struct stic_info *, int);
static int	px_match(struct device *, struct cfdata *, void *);

static int	px_intr(void *);
static u_int32_t	*px_pbuf_get(struct stic_info *);
static int	px_pbuf_post(struct stic_info *, u_int32_t *);

void	px_cnattach(tc_addr_t);

struct px_softc {
	struct	device px_dv;
	struct	stic_info *px_si;
};

struct cfattach px_ca = {
	sizeof(struct px_softc), px_match, px_attach
};

static int
px_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct tc_attach_args *ta;

	ta = aux;

	return (strncmp("PMAG-CA ", ta->ta_modname, TC_ROM_LLEN) == 0);
}

static void
px_attach(struct device *parent, struct device *self, void *aux)
{
	struct stic_info *si;
	struct tc_attach_args *ta;
	struct px_softc *px;
	int console;

	px = (struct px_softc *)self;
	ta = (struct tc_attach_args *)aux;

	if (ta->ta_addr == stic_consinfo.si_slotbase) {
		si = &stic_consinfo;
		console = 1;
	} else {
		if (stic_consinfo.si_slotbase == NULL)
			si = &stic_consinfo;
		else {
			si = malloc(sizeof(*si), M_DEVBUF, M_NOWAIT);
			memset(si, 0, sizeof(*si));
		}
		si->si_slotbase = ta->ta_addr;
		px_init(si, 0);
		console = 0;
	}

	px->px_si = si;
	tc_intr_establish(parent, ta->ta_cookie, IPL_TTY, px_intr, si);

	printf(": 2DA, 8 plane, %dx%d stamp\n", si->si_stampw, si->si_stamph);

	stic_attach(self, si, console);
}

void
px_cnattach(tc_addr_t addr)
{
	struct stic_info *si;

	si = &stic_consinfo;
	si->si_slotbase = addr;
	px_init(si, 1);
	stic_cnattach(si);
}

static void
px_init(struct stic_info *si, int bootstrap)
{
	struct pglist pglist;
	caddr_t kva;
	paddr_t bpa;

	/*
	 * Allocate memory for the packet buffers.  It must be located below
	 * 8MB, since the STIC can't access outside that region.  Also, due
	 * to the holes in STIC address space, each buffer mustn't cross a
	 * 32kB boundary.
	 */
	if (bootstrap) {
		/* 
		 * UVM won't be initalised at this point, so grab memory
		 * directly from vm_physmem[].
		 */
		kva = (caddr_t)uvm_pageboot_alloc(PX_BUF_SIZE + PX_BUF_ALIGN);
		bpa = (STIC_KSEG_TO_PHYS(kva) + PX_BUF_ALIGN - 1) &
		    ~(PX_BUF_ALIGN - 1);
		if (bpa + PX_BUF_SIZE > 8192*1024)
			panic("px_init: allocation out of bounds");
	} else {
		TAILQ_INIT(&pglist);
		if (uvm_pglistalloc(PX_BUF_SIZE, 0, 8192*1024, PX_BUF_ALIGN, 
		    PX_BUF_ALIGN, &pglist, 1, 0) != 0)
			panic("px_init: allocation failure");
		bpa = TAILQ_FIRST(&pglist)->phys_addr;
	}

	kva = (caddr_t)TC_PHYS_TO_UNCACHED(si->si_slotbase);

	si->si_vdac = (u_int32_t *)(kva + PX_VDAC_OFFSET);
	si->si_vdac_reset = (u_int32_t *)(kva + PX_VDAC_RESET_OFFSET);
	si->si_stic = (volatile struct stic_regs *)(kva + PX_STIC_OFFSET);
	si->si_stamp = (u_int32_t *)(kva + PX_STAMP_OFFSET);
	si->si_buf = (u_int32_t *)TC_PHYS_TO_UNCACHED(bpa);
	si->si_buf_phys =  bpa;
	si->si_buf_size = PX_BUF_SIZE;
	si->si_disptype = WSDISPLAY_TYPE_PX;
	si->si_depth = 8;

	si->si_pbuf_get = px_pbuf_get;
	si->si_pbuf_post = px_pbuf_post;

	memset(si->si_buf, 0, PX_BUF_SIZE);

	stic_init(si);
}

static int
px_intr(void *cookie)
{
	volatile struct stic_regs *sr;
	struct stic_info *si;
	int state;

	si = cookie;
	sr = si->si_stic;
	state = sr->sr_ipdvint;

	if ((state & STIC_INT_V) != 0) {
		sr->sr_ipdvint = 
		    STIC_INT_V_WE | (sr->sr_ipdvint & STIC_INT_V_EN);
		tc_wmb();
		stic_flush(si);
	}

#ifdef DEBUG
	if ((sr->sr_ipdvint & STIC_INT_E) != 0) {
		printf("%s: error intr, %x %x %x %x %x", si->si_dv.dv_xname,
		    sr->sr_ipdvint, sr->sr_sticsr, sr->sr_buscsr,
		    sr->sr_busadr, sr->sr_busdat);
	}
#endif

	return (1);
}

static u_int32_t *
px_pbuf_get(struct stic_info *si)
{

	si->si_pbuf_select ^= STIC_PACKET_SIZE;
	return ((u_int32_t *)((caddr_t)si->si_buf + si->si_pbuf_select));
}

static int
px_pbuf_post(struct stic_info *si, u_int32_t *buf)
{
	volatile u_int32_t *poll, junk;
	volatile struct stic_regs *sr;
	u_long v;
	int c;

	sr = si->si_stic;

	/* Get address of poll register for this buffer. */
	v = (u_long)STIC_KSEG_TO_PHYS(buf);
	v = ((v & 0xffff8000) << 3) | (v & 0x7fff);
	poll = (volatile u_int32_t *)((caddr_t)si->si_slotbase + (v >> 9));

	/*
	 * Read the poll register and make sure the stamp wants to accept
	 * our packet.  This read will initiate the DMA.  Don't wait for
	 * ever, just in case something's wrong.
	 */
	tc_mb();

	for (c = STAMP_RETRIES; c != 0; c--) {
		if ((sr->sr_ipdvint & STIC_INT_P) != 0) {
			sr->sr_ipdvint = STIC_INT_P_WE | STIC_INT_P_EN;
			tc_wmb();
			junk = *poll;
			return (0);
		}
		DELAY(STAMP_DELAY);
	}

	/* STIC has lost the plot, punish it. */
	stic_reset(si);
	return (-1);
}
