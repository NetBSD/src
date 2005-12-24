/*	$NetBSD: if_cs_mainbus.c,v 1.10 2005/12/24 23:24:01 perry Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_cs_mainbus.c,v 1.10 2005/12/24 23:24:01 perry Exp $");

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

#define ATSN_EEPROM_MAC_OFFSET           0x20


static void	cs_check_eeprom(struct cs_softc *sc);

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

	__asm volatile("mfmsr %0" : "=r"(msr));
	nmsr = (msr | PSL_FP) & ~(PSL_FE0 | PSL_FE1);
	__asm volatile("mtmsr %0" :: "r"(nmsr));
	__asm volatile("mfmsr %0" : "=r"(nmsr)); /* some interlock nonsense */
	__asm volatile(
       "stfd 0,%0\n\
	lfd 0,%1\n\
	stfd 0,%2\n\
	lfd 0,%0"
		 : "=m"(save), "=m"(*dp)
		 : "m"(u.d)
		);
	__asm volatile ("eieio; sync");
	__asm volatile("mtmsr %0" :: "r"(msr));
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
	__asm volatile("mfmsr %0" : "=r"(msr));
	nmsr = (msr | PSL_FP) & ~(PSL_FE0 | PSL_FE1);
	__asm volatile("mtmsr %0" :: "r"(nmsr));
	__asm volatile("mfmsr %0" : "=r"(nmsr)); /* some interlock nonsense */
	__asm volatile(
       "stfd 0,%0\n\
	lfd 0,%2\n\
	stfd 0,%1\n\
	lfd 0,%0"
		 : "=m"(save), "=m"(*dp)
		 : "m"(u.d)
		);
	__asm volatile ("eieio; sync");
	__asm volatile("mtmsr %0" :: "r"(msr));
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
	__asm volatile("mfmsr %0" : "=r"(msr));
	nmsr = (msr | PSL_FP) & ~(PSL_FE0 | PSL_FE1);
	__asm volatile("mtmsr %0" :: "r"(nmsr));
	__asm volatile("mfmsr %0" : "=r"(nmsr)); /* some interlock nonsense */
	__asm volatile("stfd 0,%0" : "=m"(save));

	while (cnt--) {
		v = *buf++;
		v = bswap16(v);
		u.i = (u_int64_t)v << 48;
		__asm volatile("lfd 0,%1\nstfd 0,%0" : "=m"(*dp) : "m"(u.d) );
		__asm volatile ("eieio; sync");
	}
	__asm volatile("lfd 0,%0" :: "m"(save));
	__asm volatile("mtmsr %0" :: "r"(msr));
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

	cs_check_eeprom(sc);

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


/*
 * EEPROM initialization code.
 */

static uint16_t default_eeprom_cfg[] = 
{ 0xA100, 0x2020, 0x0300, 0x0000, 0x0000,
  0x102C, 0x1000, 0x0008, 0x2158, 0x0000,
  0x0000, 0x0000 };

static uint16_t
cs_readreg(struct cs_softc *sc, uint pp_offset)
{
	cs_io_write_2(sc, PORT_PKTPG_PTR, pp_offset);
	(void)cs_io_read_2(sc, PORT_PKTPG_PTR);
	return (cs_io_read_2(sc, PORT_PKTPG_DATA));
}

static void
cs_writereg(struct cs_softc *sc, uint pp_offset, uint16_t value)
{
	cs_io_write_2(sc, PORT_PKTPG_PTR, pp_offset);
	(void)cs_io_read_2(sc, PORT_PKTPG_PTR);
	cs_io_write_2(sc, PORT_PKTPG_DATA, value);
	(void)cs_io_read_2(sc, PORT_PKTPG_DATA);
}

static int
cs_wait_eeprom_ready(struct cs_softc *sc)
{
	int ms;
	
	/*
	 * Check to see if the EEPROM is ready, a timeout is used -
	 * just in case EEPROM is ready when SI_BUSY in the
	 * PP_SelfST is clear.
	 */
	ms = 0;
	while(cs_readreg(sc, PKTPG_SELF_ST) & SELF_ST_SI_BUSY) {
		delay(1000);
		if (ms++ > 20)
			return 0;
	}
	return 1;
}

static void
cs_wr_eeprom(struct cs_softc *sc, uint16_t offset, uint16_t data)
{

	/* Check to make sure EEPROM is ready. */
	if (!cs_wait_eeprom_ready(sc)) {
		printf("%s: write EEPROM not ready\n", sc->sc_dev.dv_xname);
		return;
	}

	/* Enable writing. */
	cs_writereg(sc, PKTPG_EEPROM_CMD, EEPROM_WRITE_ENABLE);

	/* Wait for WRITE_ENABLE command to complete. */
	if (!cs_wait_eeprom_ready(sc)) {
		printf("%s: EEPROM WRITE_ENABLE timeout", sc->sc_dev.dv_xname);
	} else {
		/* Write data into EEPROM_DATA register. */
		cs_writereg(sc, PKTPG_EEPROM_DATA, data);
		delay(1000);
		cs_writereg(sc, PKTPG_EEPROM_CMD, EEPROM_CMD_WRITE | offset);
    
		/* Wait for WRITE_REGISTER command to complete. */
		if (!cs_wait_eeprom_ready(sc)) {
			printf("%s: EEPROM WRITE_REGISTER timeout\n",
			       sc->sc_dev.dv_xname);
		} 
	}

	/* Disable writing. */
	cs_writereg(sc, PKTPG_EEPROM_CMD, EEPROM_WRITE_DISABLE);

	/* Wait for WRITE_DISABLE command to complete. */
	if (!cs_wait_eeprom_ready(sc)) {
		printf("%s: WRITE_DISABLE timeout\n", sc->sc_dev.dv_xname);
	}
}

static uint16_t
cs_rd_eeprom(struct cs_softc *sc, uint16_t offset)
{

	if (!cs_wait_eeprom_ready(sc)) {
		printf("%s: read EEPROM not ready\n", sc->sc_dev.dv_xname);
		return 0;
	}
	cs_writereg(sc, PKTPG_EEPROM_CMD, EEPROM_CMD_READ | offset);

	if (!cs_wait_eeprom_ready(sc)) {
		printf("%s: EEPROM_READ timeout\n", sc->sc_dev.dv_xname);
		return 0;
	}
	return cs_readreg(sc, PKTPG_EEPROM_DATA);
}

static void
cs_check_eeprom(struct cs_softc *sc)
{
	uint8_t checksum;
	int i;
        uint16_t tmp;

	/*
	 * If the SELFST[EEPROMOK] is set, then assume EEPROM configuration
	 * is valid.
	 */
	if (cs_readreg(sc, PKTPG_SELF_ST) & SELF_ST_EEP_OK) {
		printf("%s: EEPROM OK, skipping initialization\n", 
		       sc->sc_dev.dv_xname);
		return;
	}
	printf("%s: updating EEPROM\n", sc->sc_dev.dv_xname);

	/*
	 * Calculate the size (in bytes) of the default config array and write
	 * it to the lower byte of the array itself.
	 */
	default_eeprom_cfg[0] |= sizeof(default_eeprom_cfg);

	/*
	 * Read the MAC address from its Artesyn-specified offset in the EEPROM.
	 */
	for (i = 0; i < 3; i++) {
		tmp = cs_rd_eeprom(sc, ATSN_EEPROM_MAC_OFFSET + i);
		default_eeprom_cfg[EEPROM_MAC + i] = bswap16(tmp);
	}

	/* 
	 * Program the EEPROM with our default configuration,
	 * calculating checksum as we proceed.
	 */
	checksum = 0;
	for (i = 0; i < sizeof(default_eeprom_cfg)/2 ; i++) {
		tmp = default_eeprom_cfg[i];
		cs_wr_eeprom(sc, i, tmp);
		checksum += tmp >> 8;
		checksum += tmp & 0xff;
	}

	/*
	 * The CS8900a datasheet calls for the two's complement of the checksum
	 * to be prgrammed in the most significant byte of the last word of the
	 * header.
	 */
	checksum = ~checksum + 1;
	cs_wr_eeprom(sc, i++, checksum << 8);
	/* write "end of data" flag */
	cs_wr_eeprom(sc, i, 0xffff);
}
