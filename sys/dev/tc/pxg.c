/* 	$NetBSD: pxg.c,v 1.2.2.2 2001/01/05 17:36:28 bouyer Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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
 * Driver for DEC PixelStamp graphics accelerators with onboard SRAM and
 * Intel i860 co-processor (PMAG-D, E and F).
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/callout.h>
#include <sys/proc.h>

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

#define	PXG_STIC_POLL_OFFSET	0x000000	/* STIC DMA poll space */
#define	PXG_STAMP_OFFSET	0x0c0000	/* pixelstamp space on STIC */
#define	PXG_STIC_OFFSET		0x180000	/* STIC registers */
#define	PXG_SRAM_OFFSET		0x200000	/* i860 SRAM */
#define	PXG_HOST_INTR_OFFSET	0x280000	/* i860 host interrupt */
#define	PXG_COPROC_INTR_OFFSET	0x2c0000	/* i860 coprocessor interrupt */
#define	PXG_VDAC_OFFSET		0x300000	/* VDAC registers (bt459) */
#define	PXG_VDAC_RESET_OFFSET	0x340000	/* VDAC reset register */
#define	PXG_ROM_OFFSET		0x380000	/* ROM code */
#define	PXG_I860_START_OFFSET	0x380000	/* i860 start register */
#define	PXG_I860_RESET_OFFSET	0x3c0000	/* i860 stop register */

static void	pxg_attach(struct device *, struct device *, void *);
static int	pxg_intr(void *);
static int	pxg_match(struct device *, struct cfdata *, void *);

static void	pxg_init(struct stic_info *);
static int	pxg_ioctl(struct stic_info *, u_long, caddr_t, int,
			  struct proc *);
static u_int32_t	*pxg_pbuf_get(struct stic_info *);
static int	pxg_pbuf_post(struct stic_info *, u_int32_t *);
static int	pxg_probe_planes(struct stic_info *);
static int	pxg_probe_sram(struct stic_info *);

void	pxg_cnattach(tc_addr_t);

struct pxg_softc {
	struct	device pxg_dv;
	struct	stic_info *pxg_si;
};

struct cfattach pxg_ca = {
	sizeof(struct pxg_softc), pxg_match, pxg_attach
};

static const char *pxg_types[] = {
	"PMAG-DA ", "LM-3DA",
	"PMAG-FA ", "HE-3DA",
	"PMAG-FB ", "HE+3DA",
	"PMAGB-FA", "HE+3DA",
	"PMAGB-FB", "HE+3DA",
};

static int
pxg_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct tc_attach_args *ta;
	int i;

	ta = aux;

	for (i = 0; i < sizeof(pxg_types) / sizeof(pxg_types[0]); i += 2)
		if (strncmp(pxg_types[i], ta->ta_modname, TC_ROM_LLEN) == 0)
			return (1);

	return (0);
}

static void
pxg_attach(struct device *parent, struct device *self, void *aux)
{
	struct stic_info *si;
	struct tc_attach_args *ta;
	struct pxg_softc *pxg;
	int console, i;

	pxg = (struct pxg_softc *)self;
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
		pxg_init(si);
		console = 0;
	}

	pxg->pxg_si = si;
	tc_intr_establish(parent, ta->ta_cookie, IPL_TTY, pxg_intr, si);

	for (i = 0; i < sizeof(pxg_types) / sizeof(pxg_types[0]); i += 2)
		if (strncmp(pxg_types[i], ta->ta_modname, TC_ROM_LLEN) == 0)
			break;

	printf(": %s, %d plane, %dx%d stamp, %dkB SRAM\n", pxg_types[i + 1],
	    si->si_depth, si->si_stampw, si->si_stamph, si->si_buf_size >> 10);

	stic_attach(self, si, console);
}

void
pxg_cnattach(tc_addr_t addr)
{
	struct stic_info *si;

	si = &stic_consinfo;
	si->si_slotbase = addr;
	pxg_init(si);
	stic_cnattach(si);
}

static void
pxg_init(struct stic_info *si)
{
	volatile u_int32_t *slot;
	caddr_t kva;
	paddr_t bpa;

	kva = (caddr_t)TC_PHYS_TO_UNCACHED(si->si_slotbase);
	bpa = STIC_KSEG_TO_PHYS((caddr_t)kva + PXG_SRAM_OFFSET);
	slot = (volatile u_int32_t *)kva;

	si->si_slotkva = (u_int32_t *)kva;
	si->si_vdac = (u_int32_t *)(kva + PXG_VDAC_OFFSET);
	si->si_vdac_reset = (u_int32_t *)(kva + PXG_VDAC_RESET_OFFSET);
	si->si_stic = (volatile struct stic_regs *)(kva + PXG_STIC_OFFSET);
	si->si_stamp = (u_int32_t *)(kva + PXG_STAMP_OFFSET);
	si->si_buf = (u_int32_t *)TC_PHYS_TO_UNCACHED(bpa);
	si->si_buf_phys = bpa;
	si->si_buf_size = pxg_probe_sram(si);
	si->si_disptype = WSDISPLAY_TYPE_PXG;

	si->si_pbuf_get = pxg_pbuf_get;
	si->si_pbuf_post = pxg_pbuf_post;
	si->si_ioctl = pxg_ioctl;

	/* Disable the co-processor. */
	slot[PXG_I860_RESET_OFFSET >> 2] = 0;
	tc_syncbus();
	slot[PXG_HOST_INTR_OFFSET >> 2] = 0;
	tc_syncbus();
	DELAY(40000);

	/* XXX Check for a second PixelStamp. */
	if (((si->si_stic->sr_modcl & 0x600) >> 9) > 1)
		si->si_depth = 24;
	else
		si->si_depth = pxg_probe_planes(si);

#ifdef notdef
	/* Restart the co-processor and enable STIC interrupts */
	slot[PXG_I860_START_OFFSET >> 2] = 1;
	tc_syncbus();
	DELAY(2000);
	sr->sr_sticsr = STIC_INT_WE | STIC_INT_CLR;
	tc_wmb();
#endif

	stic_init(si);
}

static int
pxg_probe_sram(struct stic_info *si)
{
	volatile u_int32_t *a, *b;

	a = si->si_slotkva + (PXG_SRAM_OFFSET >> 2);
	b = a + (0x20000 >> 1);
	*a = 4321;
	*b = 1234;
	tc_syncbus();
	return ((*a == *b) ? 0x20000 : 0x40000);
}

static int
pxg_probe_planes(struct stic_info *si)
{
	volatile u_int32_t *vdac;
	int id;

	/*
	 * For the visible framebuffer (# 0), we can cheat and use the VDAC
	 * ID.
	 */
	vdac = si->si_vdac;
	vdac[BT459_REG_ADDR_LOW] = (BT459_IREG_ID & 0xff) | 
	    ((BT459_IREG_ID & 0xff) << 8) | ((BT459_IREG_ID & 0xff) << 16);
	vdac[BT459_REG_ADDR_HIGH] = ((BT459_IREG_ID & 0xff00) >> 8) | 
	    (BT459_IREG_ID & 0xff00) | ((BT459_IREG_ID & 0xff00) << 8);
	tc_syncbus();
	id = vdac[BT459_REG_IREG_DATA] & 0x00ffffff;

	/* 3 VDACs */
	if (id == 0x004a4a4a)
		return (24);

	/* 1 VDAC */
	if ((id & 0xff0000) == 0x4a0000 || (id & 0x00ff00) == 0x004a00 ||
	    (id & 0x0000ff) == 0x00004a)
		return (8);

	/* XXX Assume 8 planes. */
	printf("pxg_probe_planes: invalid VDAC ID %x\n", id);
	return (8);
}

static int
pxg_intr(void *cookie)
{
	struct stic_info *si;
	volatile struct stic_regs *sr;
	volatile u_int32_t *hi;
	u_int32_t state;
	int it;

	si = cookie;
	sr = si->si_stic;
	state = sr->sr_ipdvint;
	hi = si->si_slotkva + (PXG_HOST_INTR_OFFSET / sizeof(u_int32_t));

	/* Clear the interrupt condition */
	it = hi[0] & 15;
	hi[0] = 0;
	tc_wmb();
	hi[2] = 0;
	tc_wmb();

	/*
	 * Since we disable the co-processor, we won't get to see vblank
	 * interrupts (so in effect, this code is useless).
	 *
	 * Packet-done and error interrupts will only ever be seen by the
	 * co-processor (although ULTRIX seems to think that they're posted
	 * to us - more investigation required).
	 */
	if (it == 3) {
		sr->sr_ipdvint = 
		    STIC_INT_V_WE | (sr->sr_ipdvint & STIC_INT_V_EN);
		tc_wmb();
		stic_flush(si);
	}

	return (1);
}

static u_int32_t *
pxg_pbuf_get(struct stic_info *si)
{
#ifdef notdef
	volatile u_int32_t *poll;

	/* Ask i860 which buffer to use */
	poll = si->si_slotkva;
	poll += PXG_COPROC_INTR_OFFSET >> 2;

	/* 
	 * XXX These should be defined as constants.  0x30 is "pause
	 * coprocessor and interrupt."
	 */
	*poll = 0x30;
	tc_wmb();

	for (i = 1000000; i; i--) {
		DELAY(4);
		switch(j = *poll) {
			case 2:
				si->si_pbuf_select = STIC_PACKET_SIZE;
				break;
			case 1:
				si->si_pbuf_select = 0;
				break;
			default:	
				if (j == 0x30)
					continue;
				break;
		}
		break;
	}
	
	if (j != 1 || j != 2) {
		/* STIC has lost the plot, punish it */
		stic_reset(si);
		si->si_pbuf_select = 0;
	}
#else

	/*
	 * XXX We should be synchronizing with STIC_INT_P so that an ISR
	 * doesn't blow us up.
	 */
	si->si_pbuf_select ^= STIC_PACKET_SIZE;
#endif
	return ((u_int32_t *)((caddr_t)si->si_buf + si->si_pbuf_select));
}

static int
pxg_pbuf_post(struct stic_info *si, u_int32_t *buf)
{
	volatile u_int32_t *poll;
	u_long v;
	int c;

	/* Get address of poll register for this buffer. */
	v = ((u_long)buf - (u_long)si->si_buf) >> 9;
	poll = (volatile u_int32_t *)((caddr_t)si->si_slotkva + v);

	/*
	 * Read the poll register and make sure the stamp wants to accept
	 * our packet.  This read will initiate the DMA.  Don't wait for
	 * ever, just in case something's wrong.
	 */
	tc_syncbus();

	for (c = STAMP_RETRIES; c != 0; c--) {
		if (*poll == STAMP_OK) {
#ifdef notdef
			/* Tell the co-processor that we are done. */
			poll = si->si_slotkva + (PXG_HOST_INTR_OFFSET >> 2);
			poll[0] = 0;
			tc_wmb();
			poll[2] = 0;
			tc_wmb();
#endif
				return (0);
		}

		DELAY(STAMP_DELAY);
	}

	/* STIC has lost the plot, punish it. */
	stic_reset(si);
	return (-1);
}

static int
pxg_ioctl(struct stic_info *si, u_long cmd, caddr_t data, int flag,
	  struct proc *p)
{
	int rv;

	if (cmd == STICIO_START860 || cmd == STICIO_RESET860) {
		if ((rv = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (rv);
		if (cmd == STICIO_START860)
			si->si_slotkva[PXG_I860_START_OFFSET >> 2] = 1;
		else
			si->si_slotkva[PXG_I860_RESET_OFFSET >> 2] = 0;
		tc_syncbus();
		rv = 0;
	} else
		rv = ENOTTY;

	return (rv);
}
