/* $NetBSD: vr4181aiu.c,v 1.2 2003/07/15 02:29:35 lukem Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vr4181aiu.c,v 1.2 2003/07/15 02:29:35 lukem Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <mips/cpuregs.h>

#include <machine/bus.h>

#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vr4181aiureg.h>
#include <hpcmips/vr/vr4181dcureg.h>

#define INBUFLEN	1024	/* length in u_int16_t */
#define INPUTLEN	1000
#define SAMPLEFREQ	1000
#define PICKUPFREQ	100
#define PICKUPCOUNT	(SAMPLEFREQ / PICKUPFREQ)

#define ST_BUSY		0x01
#define ST_OVERRUN	0x02

#define	INBUF_MASK	0x3ff	/* 2Kbyte */
#define	INBUF_RAW_SIZE	(INBUFLEN * 4 + (INBUF_MASK + 1))

#ifdef VR4181AIU_DEBUG
int	vr4181aiu_debug = 0;
#define DPRINTF(x)	if (vr4181aiu_debug) printf x
#else
#define DPRINTF(x)
#endif


struct vr4181aiu_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_dcu1_ioh;
	bus_space_handle_t	sc_dcu2_ioh;
	bus_space_handle_t	sc_aiu_ioh;
	u_int16_t		*sc_inbuf_head;
	u_int16_t		*sc_inbuf_tail;
	u_int16_t		*sc_inbuf_which;
	u_int16_t		*sc_inbuf1;
	u_int16_t		*sc_inbuf2;
	u_int16_t		*sc_inbuf_raw;
	int			sc_status;
};

static int vr4181aiu_match(struct device *, struct cfdata *, void *);
static void vr4181aiu_attach(struct device *, struct device *, void *);
static int vr4181aiu_intr(void *);

extern struct cfdriver vr4181aiu_cd;

CFATTACH_DECL(vr4181aiu, sizeof(struct vr4181aiu_softc),
	      vr4181aiu_match, vr4181aiu_attach, NULL, NULL);

dev_type_open(vr4181aiuopen);
dev_type_close(vr4181aiuclose);
dev_type_read(vr4181aiuread);
dev_type_write(vr4181aiuwrite);

const struct cdevsw vr4181aiu_cdevsw = {
	vr4181aiuopen, vr4181aiuclose, vr4181aiuread, vr4181aiuwrite, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

static int
vr4181aiu_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

static void
vr4181aiu_init_inbuf(struct vr4181aiu_softc *sc)
{
	/*
	 * XXXXXXXXXXXXXXXXX
	 *
	 * this is just a quick and dirty hack to locate the buffer
	 * in KSEG0 space.  the only reason is that i want the physical
	 * address of the buffer.
	 *
	 * bus_dma framework should be used.
	 */
	static char inbufbase[INBUF_RAW_SIZE];

	sc->sc_inbuf_raw = (u_int16_t *) inbufbase;

	sc->sc_inbuf1 = (u_int16_t *) ((((u_int32_t) sc->sc_inbuf_raw)
					+ INBUF_MASK)
				       & ~INBUF_MASK);
	sc->sc_inbuf2 = sc->sc_inbuf1 + INBUFLEN;
}

static void
vr4181aiu_disable(struct vr4181aiu_softc *sc)
{
	/* irq clear */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu2_ioh,
			  DCU_DMAITRQ_REG_W, DCU_MICEOP);
	bus_space_write_2(sc->sc_iot, sc->sc_aiu_ioh,
			  VR4181AIU_INT_REG_W,
			  VR4181AIU_MIDLEINTR
			  | VR4181AIU_MSTINTR
			  | VR4181AIU_SIDLEINTR);

	/* disable microphone */
	bus_space_write_2(sc->sc_iot, sc->sc_aiu_ioh,
			  VR4181AIU_SEQ_REG_W, 0);

	/* disable ADC */
	bus_space_write_2(sc->sc_iot, sc->sc_aiu_ioh,
			  VR4181AIU_MCNT_REG_W, 0);

	/* disable DMA */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu1_ioh,
			  DCU_AIUDMAMSK_REG_W, 0);
	bus_space_write_2(sc->sc_iot, sc->sc_dcu2_ioh,
			  DCU_DMAITMK_REG_W, 0);

	sc->sc_status = 0;
}

static void
vr4181aiu_attach(struct device *parent, struct device *self, void *aux)
{
	struct vrip_attach_args	*va = aux;
	struct vr4181aiu_softc	*sc = (void *) self;

	vr4181aiu_init_inbuf(sc);
	memset(sc->sc_inbuf1, 0x55, INBUFLEN * 2);
	memset(sc->sc_inbuf2, 0xaa, INBUFLEN * 2);
	
	sc->sc_status = 0;
	sc->sc_iot = va->va_iot;

	if (bus_space_map(sc->sc_iot,
			  VR4181AIU_DCU1_BASE, VR4181AIU_DCU1_SIZE,
			  0, &sc->sc_dcu1_ioh))
		goto out_dcu1;
	if (bus_space_map(sc->sc_iot,
			  VR4181AIU_DCU2_BASE, VR4181AIU_DCU2_SIZE,
			  0, &sc->sc_dcu2_ioh))
		goto out_dcu2;
	if (bus_space_map(sc->sc_iot,
			  VR4181AIU_AIU_BASE, VR4181AIU_AIU_SIZE,
			  0, &sc->sc_aiu_ioh))
		goto out_aiu;

	/*
	 * reset AIU
	 */
	bus_space_write_2(sc->sc_iot, sc->sc_aiu_ioh,
			  VR4181AIU_SEQ_REG_W, VR4181AIU_AIURST);
	bus_space_write_2(sc->sc_iot, sc->sc_aiu_ioh,
			  VR4181AIU_SEQ_REG_W, 0);

	/*
	 * set sample rate (1kHz fixed)
	 * XXXX
	 * assume to PCLK is 32.768MHz
	 */
	bus_space_write_2(sc->sc_iot, sc->sc_aiu_ioh,
			  VR4181AIU_MCNVC_END,
			  32768000 / SAMPLEFREQ);

	/*
	 * XXXX
	 * assume to PCLK is 32.768MHz
	 * DAVREF_SETUP = 5usec * PCLK = 163.84
	 */
	bus_space_write_2(sc->sc_iot, sc->sc_aiu_ioh,
			  VR4181AIU_DAVREF_SETUP_REG_W, 164);

	vr4181aiu_disable(sc);

	if (vrip_intr_establish(va->va_vc, va->va_unit, 0,
				IPL_BIO, vr4181aiu_intr, sc) == NULL) {
		printf("%s: can't establish interrupt\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	printf("\n");
	return;

out_aiu:
	bus_space_unmap(sc->sc_iot, sc->sc_dcu2_ioh, VR4181AIU_DCU2_SIZE);
out_dcu2:
	bus_space_unmap(sc->sc_iot, sc->sc_dcu1_ioh, VR4181AIU_DCU1_SIZE);
out_dcu1:
	printf(": can't map i/o space\n");
}

int
vr4181aiuopen(dev_t dev, int flag, int mode, struct proc *p)
{
	struct vr4181aiu_softc	*sc;

	if ((sc = device_lookup(&vr4181aiu_cd, minor(dev))) == NULL)
		return ENXIO;

	if (sc->sc_status & ST_BUSY)
		return EBUSY;
	
	sc->sc_inbuf_head = sc->sc_inbuf_tail
		= sc->sc_inbuf_which = sc->sc_inbuf1;
	sc->sc_status &= ~ST_OVERRUN;

	/* setup DMA */
	/* reset */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu1_ioh,
			  DCU_DMARST_REG_W, 0);
	bus_space_write_2(sc->sc_iot, sc->sc_dcu1_ioh,
			  DCU_DMARST_REG_W, DCU_DMARST);
	/* dest1 <- sc_inbuf1 */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu1_ioh,
			  DCU_MICDEST1REG1_W,
			  MIPS_KSEG0_TO_PHYS(sc->sc_inbuf1) & 0xffff);
	bus_space_write_2(sc->sc_iot, sc->sc_dcu1_ioh,
			  DCU_MICDEST1REG2_W,
			  MIPS_KSEG0_TO_PHYS(sc->sc_inbuf1) >> 16);
	/* dest2 <- sc_inbuf2 */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu1_ioh,
			  DCU_MICDEST2REG1_W,
			  MIPS_KSEG0_TO_PHYS(sc->sc_inbuf2) & 0xffff);
	bus_space_write_2(sc->sc_iot, sc->sc_dcu1_ioh,
			  DCU_MICDEST2REG2_W,
			  MIPS_KSEG0_TO_PHYS(sc->sc_inbuf2) >> 16);
	/* record length <- INPUTLEN */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu2_ioh,
			  DCU_MICRCLEN_REG_W, INPUTLEN);
	/* config <- auto load */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu2_ioh,
			  DCU_MICDMACFG_REG_W, DCU_MICLOAD);
	/* irq <- irq clear */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu2_ioh,
			  DCU_DMAITRQ_REG_W, DCU_MICEOP);
	/* control <- INC */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu2_ioh,
			  DCU_DMACTL_REG_W, DCU_MICCNT_INC);
	/* irq mask <- microphone end of process */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu2_ioh,
			  DCU_DMAITMK_REG_W, DCU_MICEOP_ENABLE);

	/* enable DMA */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu1_ioh,
			  DCU_AIUDMAMSK_REG_W, DCU_ENABLE_MIC);

	/* enable ADC */
	bus_space_write_2(sc->sc_iot, sc->sc_aiu_ioh,
			  VR4181AIU_MCNT_REG_W, VR4181AIU_ADENAIU);

	/* enable microphone */
	bus_space_write_2(sc->sc_iot, sc->sc_aiu_ioh,
			  VR4181AIU_SEQ_REG_W, VR4181AIU_AIUMEN);

	sc->sc_status |= ST_BUSY;

	return 0;
}

int
vr4181aiuclose(dev_t dev, int flag, int mode, struct proc *p)
{
	vr4181aiu_disable(device_lookup(&vr4181aiu_cd, minor(dev)));
	return 0;
}

int
vr4181aiuread(dev_t dev, struct uio *uio, int flag)
{
	struct vr4181aiu_softc	*sc;
	int			s;
	u_int16_t		*fence;
	int			avail;
	int			count;
	u_int8_t		tmp[INPUTLEN / PICKUPCOUNT];
	u_int16_t		*src;
	u_int8_t		*dst;

	sc = device_lookup(&vr4181aiu_cd, minor(dev));

	src = sc->sc_inbuf_tail;
	s = splbio();
	if (src == sc->sc_inbuf_head) {
		/* wait for DMA to complete writing */
		tsleep(sc, PRIBIO, "aiu read", 0);
		/* now sc_inbuf_head points alternate buffer */
	}
	splx(s);
	
	fence = sc->sc_inbuf_which == sc->sc_inbuf1
		? &sc->sc_inbuf1[INPUTLEN]
		: &sc->sc_inbuf2[INPUTLEN];
	avail = (fence - src) / PICKUPCOUNT;
	count = min(avail, uio->uio_resid);
	dst = tmp;
	while (count > 0) {
		*dst++ = (u_int8_t) (*src >> 2);
		src += PICKUPCOUNT;
		count--;
	}

	if (src < fence) {
		sc->sc_inbuf_tail = src;
	} else {
		/* alter the buffer */
		sc->sc_inbuf_tail
			= sc->sc_inbuf_which
			= sc->sc_inbuf_which == sc->sc_inbuf1
			? sc->sc_inbuf2 : sc->sc_inbuf1;
	}

	return uiomove(tmp, dst - tmp, uio);
}

int
vr4181aiuwrite(dev_t dev, struct uio *uio, int flag)
{
	return 0;
}

int
vr4181aiupoll(dev_t dev, int events, struct proc *p)
{
	return 0;
}

int
vr4181aiuioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	return 0;
}

/*
 * interrupt handler
 */
static int
vr4181aiu_intr(void *arg)
{
	struct vr4181aiu_softc	*sc = arg;

	if (!(sc->sc_status & ST_BUSY)) {
		printf("vr4181aiu_intr: stray interrupt\n");
		vr4181aiu_disable(sc);
		return 0;
	}

	/* irq clear */
	bus_space_write_2(sc->sc_iot, sc->sc_dcu2_ioh,
			  DCU_DMAITRQ_REG_W, DCU_MICEOP);

	if (sc->sc_inbuf_head == sc->sc_inbuf1) {
		if (sc->sc_inbuf_tail != sc->sc_inbuf1)
			sc->sc_status |= ST_OVERRUN;
		sc->sc_inbuf_head = sc->sc_inbuf2;
	} else {
		if (sc->sc_inbuf_tail != sc->sc_inbuf2)
			sc->sc_status |= ST_OVERRUN;
		sc->sc_inbuf_head = sc->sc_inbuf1;
	}

	if (sc->sc_status & ST_OVERRUN) {
		printf("vr4181aiu_intr: overrun\n");
	}

	DPRINTF(("vr4181aiu_intr: sc_inbuf1 = %04x, sc_inbuf2 = %04x\n",
		 sc->sc_inbuf1[0], sc->sc_inbuf2[0]));

	wakeup(sc);
	
	return 0;
}
