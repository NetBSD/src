/*	$NetBSD: if_cs_mainbus.c,v 1.7 2003/07/15 02:54:41 lukem Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
__KERNEL_RCSID(0, "$NetBSD: if_cs_mainbus.c,v 1.7 2003/07/15 02:54:41 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/socket.h>

#include "rnd.h"
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/pmppc.h>
#include <machine/mainbus.h>

#include <dev/ic/cs89x0reg.h>
#include <dev/ic/cs89x0var.h>

#include <sys/callout.h>

static int	cs_mainbus_match(struct device *, struct cfdata *, void *);
static void	cs_mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(cs_mainbus, sizeof(struct cs_softc),
    cs_mainbus_match, cs_mainbus_attach, NULL, NULL);

int
cs_mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	return (strcmp(maa->mb_name, "cs") == 0);
}

#if 0
static u_int64_t
in64(uint a)
{
	union {
		double d;
		u_int64_t i;
	} u;
	double save, *dp = (double *)a;
	u_int32_t msr, nmsr;

	__asm__ volatile("mfmsr %0" : "=r"(msr));
	nmsr = (msr | PSL_FP) & ~(PSL_FE0 | PSL_FE1);
	__asm__ volatile("mtmsr %0" :: "r"(nmsr));
	__asm__ volatile("mfmsr %0" : "=r"(nmsr)); /* some interlock nonsense */
	__asm__ volatile(
       "stfd 0,%0\n\
	lfd 0,%1\n\
	stfd 0,%2\n\
	lfd 0,%0"
		 : "=m"(save), "=m"(*dp)
		 : "m"(u.d)
		);
	__asm__ volatile ("eieio; sync");
	__asm__ volatile("mtmsr %0" :: "r"(msr));
	return (u.i);
}
#endif

static void
out64(uint a, u_int64_t v)
{
	union {
		double d;
		u_int64_t i;
	} u;
	double save, *dp = (double *)a;
	u_int32_t msr, nmsr;
	int s;

	s = splhigh();
	u.i = v;
	__asm__ volatile("mfmsr %0" : "=r"(msr));
	nmsr = (msr | PSL_FP) & ~(PSL_FE0 | PSL_FE1);
	__asm__ volatile("mtmsr %0" :: "r"(nmsr));
	__asm__ volatile("mfmsr %0" : "=r"(nmsr)); /* some interlock nonsense */
	__asm__ volatile(
       "stfd 0,%0\n\
	lfd 0,%2\n\
	stfd 0,%1\n\
	lfd 0,%0"
		 : "=m"(save), "=m"(*dp)
		 : "m"(u.d)
		);
	__asm__ volatile ("eieio; sync");
	__asm__ volatile("mtmsr %0" :: "r"(msr));
	splx(s);
}

static u_int8_t
cs_io_read_1(struct cs_softc *sc, bus_size_t offs)
{
	u_int32_t a, v;

	a = sc->sc_ioh + (offs << 2);
	v = in8(a);
	return v;
}

static u_int16_t
cs_io_read_2(struct cs_softc *sc, bus_size_t offs)
{
	u_int32_t a, v;

	a = sc->sc_ioh + (offs << 2);
	v = in16(a);
	return v;
}

static void
cs_io_read_multi_2(struct cs_softc *sc, bus_size_t offs, u_int16_t *buf,
		   bus_size_t cnt)
{
	u_int32_t a, v;

	a = sc->sc_ioh + (offs << 2);
	while (cnt--) {
		v = in16(a);
		*buf++ = bswap16(v);
	}
}

static void
cs_io_write_2(struct cs_softc *sc, bus_size_t offs, u_int16_t data)
{
	u_int32_t a;
	u_int64_t v;

	a = sc->sc_ioh + (offs << 2);
	v = (u_int64_t)data << 48;
	out64(a, v);

	(void)in16(a);		/* CPC700 write post bug */
}

static void
cs_io_write_multi_2(struct cs_softc *sc, bus_size_t offs,
		    const u_int16_t *buf, bus_size_t cnt)
{
	u_int16_t v;
	double save, *dp;
	union {
		double d;
		u_int64_t i;
	} u;
	u_int32_t msr, nmsr;
	int s;

	dp = (double *)(sc->sc_ioh + (offs << 2));

	s = splhigh();
	__asm__ volatile("mfmsr %0" : "=r"(msr));
	nmsr = (msr | PSL_FP) & ~(PSL_FE0 | PSL_FE1);
	__asm__ volatile("mtmsr %0" :: "r"(nmsr));
	__asm__ volatile("mfmsr %0" : "=r"(nmsr)); /* some interlock nonsense */
	__asm__ volatile("stfd 0,%0" : "=m"(save));

	while (cnt--) {
		v = *buf++;
		v = bswap16(v);
		u.i = (u_int64_t)v << 48;
		__asm__ volatile("lfd 0,%1\nstfd 0,%0" : "=m"(*dp) : "m"(u.d) );
		__asm__ volatile ("eieio; sync");
	}
	__asm__ volatile("lfd 0,%0" :: "m"(save));
	__asm__ volatile("mtmsr %0" :: "r"(msr));
	splx(s);
}

static u_int16_t
cs_mem_read_2(struct cs_softc *sc, bus_size_t offs)
{
	panic("cs_mem_read_2");
}

static void
cs_mem_write_2(struct cs_softc *sc, bus_size_t offs, u_int16_t data)
{
	panic("cs_mem_write_2");
}

static void
cs_mem_write_region_2(struct cs_softc *sc, bus_size_t offs,
		      const u_int16_t *buf, bus_size_t cnt)
{
	panic("cs_mem_write_region_2");
}

void
cs_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct cs_softc *sc = (struct cs_softc *)self;
	struct mainbus_attach_args *maa = aux;
	int media[1] = { IFM_ETHER | IFM_10_T };

	printf("\n");

	sc->sc_iot = maa->mb_bt;
	sc->sc_memt = maa->mb_bt;
	sc->sc_irq = maa->mb_irq;

	if (bus_space_map(sc->sc_iot, PMPPC_CS_IO, CS8900_IOSIZE*4,
			  0, &sc->sc_ioh)) {
		printf("%s: failed to map io\n", self->dv_xname);
		return;
	}


	sc->sc_ih = intr_establish(sc->sc_irq, IST_LEVEL, IPL_NET, cs_intr, sc);
	if (!sc->sc_ih) {
		printf("%s: unable to establish interrupt\n",
		       self->dv_xname);
		goto fail;
	}

	sc->sc_cfgflags = CFGFLG_NOT_EEPROM;

	sc->sc_io_read_1 = cs_io_read_1;
	sc->sc_io_read_2 = cs_io_read_2;
	sc->sc_io_read_multi_2 = cs_io_read_multi_2;
	sc->sc_io_write_2 = cs_io_write_2;
	sc->sc_io_write_multi_2 = cs_io_write_multi_2;
	sc->sc_mem_read_2 = cs_mem_read_2;
	sc->sc_mem_write_2 = cs_mem_write_2;
	sc->sc_mem_write_region_2 = cs_mem_write_region_2;

	/*
	 * We need interrupt on INTRQ0 from the CS8900 (that's what wired
	 * to the UIC).  The MI driver subtracts 10 from the irq, so
	 * use 10 as the irq.
	 */
	sc->sc_irq = 10;

	/* Use half duplex 10baseT. */
	if (cs_attach(sc, NULL, media, 1, IFM_ETHER | IFM_10_T)) {
		printf("%s: unable to attach\n", self->dv_xname);
		goto fail;
	}

	return;

 fail:
	/* XXX disestablish, unmap */
	return;
}
