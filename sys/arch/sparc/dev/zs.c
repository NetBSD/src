/*	$NetBSD: zs.c,v 1.105.8.1 2007/07/11 20:02:22 mjf Exp $	*/

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
 * Zilog Z8530 Dual UART driver (machine-dependent part)
 *
 * Runs two serial lines per chip using slave drivers.
 * Plain tty/async lines use the zs_async slave.
 * Sun keyboard/mouse uses the zs_kbd/zs_ms slaves.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.105.8.1 2007/07/11 20:02:22 mjf Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/syslog.h>

#include <machine/bsd_openprom.h>
#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/eeprom.h>
#include <machine/psl.h>
#include <machine/z8530var.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/auxreg.h>
#include <sparc/sparc/auxiotwo.h>
#include <sparc/dev/cons.h>
#include <dev/sun/kbd_ms_ttyvar.h>

#include "kbd.h"
#include "ms.h"
#include "wskbd.h"

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);

/*
 * The Sun provides a 4.9152 MHz clock to the ZS chips.
 */
#define PCLK	(9600 * 512)	/* PCLK pin input clock rate */

#define	ZS_DELAY()		(CPU_ISSUN4C ? (0) : delay(2))

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	volatile u_char	zc_csr;		/* ctrl,status, and indirect access */
	u_char		zc_xxx0;
	volatile u_char	zc_data;	/* data */
	u_char		zc_xxx1;
};
struct zsdevice {
	/* Yes, they are backwards. */
	struct	zschan zs_chan_b;
	struct	zschan zs_chan_a;
};

/* ZS channel used as the console device (if any) */
void *zs_conschan_get, *zs_conschan_put;

static u_char zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
	0,	/* 2: IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,	/* 6: TXSYNC/SYNCLO */
	0,	/* 7: RXSYNC/SYNCHI */
	0,	/* 8: alias for data port */
	ZSWR9_MASTER_IE | ZSWR9_NO_VECTOR,
	0,	/*10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
	((PCLK/32)/9600)-2,	/*12: BAUDLO (default=9600) */
	0,			/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};

/* Console ops */
static int  zscngetc(dev_t);
static void zscnputc(dev_t, int);
static void zscnpollc(dev_t, int);

struct consdev zs_consdev = {
	NULL,
	NULL,
	zscngetc,
	zscnputc,
	zscnpollc,
	NULL,
};


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int  zs_match_mainbus(struct device *, struct cfdata *, void *);
static int  zs_match_obio(struct device *, struct cfdata *, void *);
static void zs_attach_mainbus(struct device *, struct device *, void *);
static void zs_attach_obio(struct device *, struct device *, void *);

#if defined(SUN4D)
#include <sparc/dev/bootbusvar.h>

static int  zs_match_bootbus(struct device *, struct cfdata *, void *);
static void zs_attach_bootbus(struct device *, struct device *, void *);

CFATTACH_DECL(zs_bootbus, sizeof(struct zsc_softc),
    zs_match_bootbus, zs_attach_bootbus, NULL, NULL);
#endif /* SUN4D */

static void zs_attach(struct zsc_softc *, struct zsdevice *, int);
static int  zs_print(void *, const char *name);

CFATTACH_DECL(zs_mainbus, sizeof(struct zsc_softc),
    zs_match_mainbus, zs_attach_mainbus, NULL, NULL);

CFATTACH_DECL(zs_obio, sizeof(struct zsc_softc),
    zs_match_obio, zs_attach_obio, NULL, NULL);

extern struct cfdriver zs_cd;

/* softintr(9) cookie, shared by all instances of this driver */
static void *zs_sicookie;

/* Interrupt handlers. */
static int zshard(void *);
static void zssoft(void *);

static int zs_get_speed(struct zs_chanstate *);

/* Console device support */
static int zs_console_flags(int, int, int);

/* Power management hooks */
int  zs_enable(struct zs_chanstate *);
void zs_disable(struct zs_chanstate *);


/* XXX from dev/ic/z8530tty.c */
extern struct tty *zstty_get_tty_from_dev(struct device *);

/*
 * Is the zs chip present?
 */
static int
zs_match_mainbus(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(cf->cf_name, ma->ma_name) != 0)
		return (0);

	return (1);
}

static int
zs_match_obio(struct device *parent, struct cfdata *cf, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0) {
		struct sbus_attach_args *sa = &uoba->uoba_sbus;

		if (strcmp(cf->cf_name, sa->sa_name) != 0)
			return (0);

		return (1);
	}

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
			        1, 0, 0, NULL, NULL));
}

#if defined(SUN4D)
static int
zs_match_bootbus(struct device *parent, struct cfdata *cf, void *aux)
{
	struct bootbus_attach_args *baa = aux;

	return (strcmp(cf->cf_name, baa->ba_name) == 0);
}
#endif /* SUN4D */

static void
zs_attach_mainbus(struct device *parent, struct device *self, void *aux)
{
	struct zsc_softc *zsc = (void *) self;
	struct mainbus_attach_args *ma = aux;

	zsc->zsc_bustag = ma->ma_bustag;
	zsc->zsc_dmatag = ma->ma_dmatag;
	zsc->zsc_promunit = prom_getpropint(ma->ma_node, "slave", -2);
	zsc->zsc_node = ma->ma_node;

	/*
	 * For machines with zs on mainbus (all sun4c models), we expect
	 * the device registers to be mapped by the PROM.
	 */
	zs_attach(zsc, ma->ma_promvaddr, ma->ma_pri);
}

static void
zs_attach_obio(struct device *parent, struct device *self, void *aux)
{
	struct zsc_softc *zsc = (void *) self;
	union obio_attach_args *uoba = aux;

	if (uoba->uoba_isobio4 == 0) {
		struct sbus_attach_args *sa = &uoba->uoba_sbus;
		void *va;
		struct zs_chanstate *cs;
		int channel;

		if (sa->sa_nintr == 0) {
			printf(" no interrupt lines\n");
			return;
		}

		/*
		 * Some sun4m models (Javastations) may not map the zs device.
		 */
		if (sa->sa_npromvaddrs > 0)
			va = (void *)sa->sa_promvaddr;
		else {
			bus_space_handle_t bh;

			if (sbus_bus_map(sa->sa_bustag,
					 sa->sa_slot,
					 sa->sa_offset,
					 sa->sa_size,
					 BUS_SPACE_MAP_LINEAR, &bh) != 0) {
				printf(" cannot map zs registers\n");
				return;
			}
			va = (void *)bh;
		}

		/*
		 * Check if power state can be set, e.g. Tadpole 3GX
		 */
		if (prom_getpropint(sa->sa_node, "pwr-on-auxio2", 0))
		{
			printf (" powered via auxio2");
			for (channel = 0; channel < 2; channel++) {
				cs = &zsc->zsc_cs_store[channel];
				cs->enable = zs_enable;
				cs->disable = zs_disable;
			}
		}

		zsc->zsc_bustag = sa->sa_bustag;
		zsc->zsc_dmatag = sa->sa_dmatag;
		zsc->zsc_promunit = prom_getpropint(sa->sa_node, "slave", -2);
		zsc->zsc_node = sa->sa_node;
		zs_attach(zsc, va, sa->sa_pri);
	} else {
		struct obio4_attach_args *oba = &uoba->uoba_oba4;
		bus_space_handle_t bh;
		bus_addr_t paddr = oba->oba_paddr;

		/*
		 * As for zs on mainbus, we require a PROM mapping.
		 */
		if (bus_space_map(oba->oba_bustag,
				  paddr,
				  sizeof(struct zsdevice),
				  BUS_SPACE_MAP_LINEAR | OBIO_BUS_MAP_USE_ROM,
				  &bh) != 0) {
			printf(" cannot map zs registers\n");
			return;
		}
		zsc->zsc_bustag = oba->oba_bustag;
		zsc->zsc_dmatag = oba->oba_dmatag;
		/*
		 * Find prom unit by physical address
		 * We're just comparing the address (not the iospace) here
		 */
		paddr = BUS_ADDR_PADDR(paddr);
		if (cpuinfo.cpu_type == CPUTYP_4_100)
			/*
			 * On the sun4/100, the top-most 4 bits are zero
			 * on obio addresses; force them to 1's for the
			 * sake of the comparison here.
			 */
			paddr |= 0xf0000000;
		zsc->zsc_promunit =
			(paddr == 0xf1000000) ? 0 :
			(paddr == 0xf0000000) ? 1 :
			(paddr == 0xe0000000) ? 2 : -2;

		zs_attach(zsc, (void *)bh, oba->oba_pri);
	}
}

#if defined(SUN4D)
static void
zs_attach_bootbus(struct device *parent, struct device *self, void *aux)
{
	struct zsc_softc *zsc = (void *) self;
	struct bootbus_attach_args *baa = aux;
	void *va;

	if (baa->ba_nintr == 0) {
		printf(": no interrupt lines\n");
		return;
	}

	if (baa->ba_npromvaddrs > 0)
		va = (void *) baa->ba_promvaddrs;
	else {
		bus_space_handle_t bh;

		if (bus_space_map(baa->ba_bustag,
		    BUS_ADDR(baa->ba_slot, baa->ba_offset),
		    baa->ba_size, BUS_SPACE_MAP_LINEAR, &bh) != 0) {
			printf(": cannot map zs registers\n");
			return;
		}
		va = (void *) bh;
	}

	zsc->zsc_bustag = baa->ba_bustag;
	zsc->zsc_promunit = prom_getpropint(baa->ba_node, "slave", -2);
	zsc->zsc_node = baa->ba_node;
	zs_attach(zsc, va, baa->ba_intr[0].oi_pri);
}
#endif /* SUN4D */

/*
 * Attach a found zs.
 *
 * USE ROM PROPERTIES port-a-ignore-cd AND port-b-ignore-cd FOR
 * SOFT CARRIER, AND keyboard PROPERTY FOR KEYBOARD/MOUSE?
 */
static void
zs_attach(struct zsc_softc *zsc, struct zsdevice *zsd, int pri)
{
	struct zsc_attach_args zsc_args;
	struct zs_chanstate *cs;
	int s, channel;
	static int didintr, prevpri;
	int ch0_is_cons = 0;

	if (zsd == NULL) {
		printf("configuration incomplete\n");
		return;
	}

	if (!didintr) {
		zs_sicookie = softintr_establish(IPL_SOFTSERIAL, zssoft, NULL);
		if (zs_sicookie == NULL) {
			printf("\n%s: cannot establish soft int handler\n",
				zsc->zsc_dev.dv_xname);
			return;
		}
	}
	printf(" softpri %d\n", IPL_SOFTSERIAL);

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		struct zschan *zc;
		struct device *child;
		int hwflags;

		zsc_args.channel = channel;
		cs = &zsc->zsc_cs_store[channel];
		zsc->zsc_cs[channel] = cs;

		simple_lock_init(&cs->cs_lock);
		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		zc = (channel == 0) ? &zsd->zs_chan_a : &zsd->zs_chan_b;

		hwflags = zs_console_flags(zsc->zsc_promunit,
						    zsc->zsc_node,
						    channel);

#if NWSKBD == 0	
		/* Not using wscons console, so always set console flags.*/
		zsc_args.hwflags = hwflags;
		if (zsc_args.hwflags & ZS_HWFLAG_CONSOLE) {
			zsc_args.hwflags |= ZS_HWFLAG_USE_CONSDEV;
			zsc_args.consdev = &zs_consdev;
		}
#else
		/* If we are unit 1, then this is the "real" console.
		 * Remember this in order to set up the keyboard and
		 * mouse line disciplines for SUN4 machines below.
		 * Also, don't set the console flags, otherwise we
		 * tell zstty_attach() to attach as console.
		 */
		if (zsc->zsc_promunit == 1) {
			if ((hwflags & ZS_HWFLAG_CONSOLE_INPUT) != 0 &&
			    !channel) {
				ch0_is_cons = 1;
			}
		} else {
			zsc_args.hwflags = hwflags;
		}
#endif
		if ((zsc_args.hwflags & ZS_HWFLAG_CONSOLE_INPUT) != 0) {
			zs_conschan_get = zc;
		}
		if ((zsc_args.hwflags & ZS_HWFLAG_CONSOLE_OUTPUT) != 0) {
			zs_conschan_put = zc;
		}
		/* Childs need to set cn_dev, etc */

		cs->cs_reg_csr  = &zc->zc_csr;
		cs->cs_reg_data = &zc->zc_data;

		bcopy(zs_init_reg, cs->cs_creg, 16);
		bcopy(zs_init_reg, cs->cs_preg, 16);

		/* XXX: Consult PROM properties for this?! */
		cs->cs_defspeed = zs_get_speed(cs);
		cs->cs_defcflag = zs_def_cflag;

		/* Make these correspond to cs_defcflag (-crtscts) */
		cs->cs_rr0_dcd = ZSRR0_DCD;
		cs->cs_rr0_cts = 0;
		cs->cs_wr5_dtr = ZSWR5_DTR | ZSWR5_RTS;
		cs->cs_wr5_rts = 0;

		/*
		 * Clear the master interrupt enable.
		 * The INTENA is common to both channels,
		 * so just do it on the A channel.
		 */
		if (channel == 0) {
			zs_write_reg(cs, 9, 0);
		}

		/*
		 * Look for a child driver for this channel.
		 * The child attach will setup the hardware.
		 */

		child = config_found(&zsc->zsc_dev, &zsc_args, zs_print);
		if (child == NULL) {
			/* No sub-driver.  Just reset it. */
			u_char reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splzs();
			zs_write_reg(cs,  9, reset);
			splx(s);
		}
#if (NKBD > 0) || (NMS > 0)
		/*
		 * If this was a zstty it has a keyboard
		 * property on it we need to attach the
		 * sunkbd and sunms line disciplines.
		 * There are no properties on SUN4 machines.
		 * For them, check if we have set the
		 * ch0_is_cons variable above.
		 */
		if ((child != NULL) &&
		    (device_is_a(child, "zstty")) && (
		    (CPU_ISSUN4 && ch0_is_cons) || (!CPU_ISSUN4 &&
		    (prom_getproplen(zsc->zsc_node, "keyboard") == 0))))
		{
			struct kbd_ms_tty_attach_args kma;
			struct tty *tp = zstty_get_tty_from_dev(child);
			kma.kmta_tp = tp;
			kma.kmta_dev = tp->t_dev;
			kma.kmta_consdev = zsc_args.consdev;

			/* Attach 'em if we got 'em. */
#if (NKBD > 0)
			if (channel == 0) {
				kma.kmta_name = "keyboard";
				config_found(child, &kma, NULL);
			}
#endif
#if (NMS > 0)
			if (channel == 1) {
				kma.kmta_name = "mouse";
				config_found(child, &kma, NULL);
			}
#endif
		}
#endif
	}

	/*
	 * Now safe to install interrupt handlers.  Note the arguments
	 * to the interrupt handlers aren't used.  Note, we only do this
	 * once since both SCCs interrupt at the same level and vector.
	 */
	if (!didintr) {
		didintr = 1;
		prevpri = pri;
		bus_intr_establish(zsc->zsc_bustag, pri, IPL_SERIAL,
				   zshard, NULL);
	} else if (pri != prevpri)
		panic("broken zs interrupt scheme");

	evcnt_attach_dynamic(&zsc->zsc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    zsc->zsc_dev.dv_xname, "intr");

	/*
	 * Set the master interrupt enable and interrupt vector.
	 * (common to both channels, do it on A)
	 */
	cs = zsc->zsc_cs[0];
	s = splhigh();
	/* interrupt vector */
	zs_write_reg(cs, 2, zs_init_reg[2]);
	/* master interrupt control (enable) */
	zs_write_reg(cs, 9, zs_init_reg[9]);
	splx(s);

#if 0
	/*
	 * XXX: L1A hack - We would like to be able to break into
	 * the debugger during the rest of autoconfiguration, so
	 * lower interrupts just enough to let zs interrupts in.
	 * This is done after both zs devices are attached.
	 */
	if (zsc->zsc_promunit == 1) {
		printf("zs1: enabling zs interrupts\n");
		(void)splfd(); /* XXX: splzs - 1 */
	}
#endif

}

static int
zs_print(void *aux, const char *name)
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		aprint_normal("%s: ", name);

	if (args->channel != -1)
		aprint_normal(" channel %d", args->channel);

	return (UNCONF);
}

static volatile int zssoftpending;

/*
 * Our ZS chips all share a common, autovectored interrupt,
 * so we have to look at all of them on each interrupt.
 */
static int
zshard(void *arg)
{
	struct zsc_softc *zsc;
	int unit, rr3, rval, softreq;

	rval = softreq = 0;
	for (unit = 0; unit < zs_cd.cd_ndevs; unit++) {
		struct zs_chanstate *cs;

		zsc = zs_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		rr3 = zsc_intr_hard(zsc);
		/* Count up the interrupts. */
		if (rr3) {
			rval |= rr3;
			zsc->zsc_intrcnt.ev_count++;
		}
		if ((cs = zsc->zsc_cs[0]) != NULL)
			softreq |= cs->cs_softreq;
		if ((cs = zsc->zsc_cs[1]) != NULL)
			softreq |= cs->cs_softreq;
	}

	/* We are at splzs here, so no need to lock. */
	if (softreq && (zssoftpending == 0)) {
		zssoftpending = 1;
		softintr_schedule(zs_sicookie);
	}
	return (rval);
}

/*
 * Similar scheme as for zshard (look at all of them)
 */
static void
zssoft(void *arg)
{
	struct zsc_softc *zsc;
	int s, unit;

	/* This is not the only ISR on this IPL. */
	if (zssoftpending == 0)
		return;

	/*
	 * The soft intr. bit will be set by zshard only if
	 * the variable zssoftpending is zero.  The order of
	 * these next two statements prevents our clearing
	 * the soft intr bit just after zshard has set it.
	 */
	/* ienab_bic(IE_ZSSOFT); */
	zssoftpending = 0;

	/* Make sure we call the tty layer at spltty. */
	s = spltty();
	for (unit = 0; unit < zs_cd.cd_ndevs; unit++) {
		zsc = zs_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		(void)zsc_intr_soft(zsc);
	}
	splx(s);
}


/*
 * Compute the current baud rate given a ZS channel.
 */
static int
zs_get_speed(struct zs_chanstate *cs)
{
	int tconst;

	tconst = zs_read_reg(cs, 12);
	tconst |= zs_read_reg(cs, 13) << 8;
	return (TCONST_TO_BPS(cs->cs_brg_clk, tconst));
}

/*
 * MD functions for setting the baud rate and control modes.
 * bps - in bits per second
 */
int
zs_set_speed(struct zs_chanstate *cs, int bps)
{
	int tconst, real_bps;

	if (bps == 0)
		return (0);

#ifdef	DIAGNOSTIC
	if (cs->cs_brg_clk == 0)
		panic("zs_set_speed");
#endif

	tconst = BPS_TO_TCONST(cs->cs_brg_clk, bps);
	if (tconst < 0)
		return (EINVAL);

	/* Convert back to make sure we can do it. */
	real_bps = TCONST_TO_BPS(cs->cs_brg_clk, tconst);

	/* XXX - Allow some tolerance here? */
	if (real_bps != bps)
		return (EINVAL);

	cs->cs_preg[12] = tconst;
	cs->cs_preg[13] = tconst >> 8;

	/* Caller will stuff the pending registers. */
	return (0);
}

int
zs_set_modes(struct zs_chanstate *cs, int cflag)
{
	int s;

	/*
	 * Output hardware flow control on the chip is horrendous:
	 * if carrier detect drops, the receiver is disabled, and if
	 * CTS drops, the transmitter is stoped IN MID CHARACTER!
	 * Therefore, NEVER set the HFC bit, and instead use the
	 * status interrupt to detect CTS changes.
	 */
	s = splzs();
	cs->cs_rr0_pps = 0;
	if ((cflag & (CLOCAL | MDMBUF)) != 0) {
		cs->cs_rr0_dcd = 0;
		if ((cflag & MDMBUF) == 0)
			cs->cs_rr0_pps = ZSRR0_DCD;
	} else
		cs->cs_rr0_dcd = ZSRR0_DCD;
	if ((cflag & CRTSCTS) != 0) {
		cs->cs_wr5_dtr = ZSWR5_DTR;
		cs->cs_wr5_rts = ZSWR5_RTS;
		cs->cs_rr0_cts = ZSRR0_CTS;
	} else if ((cflag & CDTRCTS) != 0) {
		cs->cs_wr5_dtr = 0;
		cs->cs_wr5_rts = ZSWR5_DTR;
		cs->cs_rr0_cts = ZSRR0_CTS;
	} else if ((cflag & MDMBUF) != 0) {
		cs->cs_wr5_dtr = 0;
		cs->cs_wr5_rts = ZSWR5_DTR;
		cs->cs_rr0_cts = ZSRR0_DCD;
	} else {
		cs->cs_wr5_dtr = ZSWR5_DTR | ZSWR5_RTS;
		cs->cs_wr5_rts = 0;
		cs->cs_rr0_cts = 0;
	}
	splx(s);

	/* Caller will stuff the pending registers. */
	return (0);
}


/*
 * Read or write the chip with suitable delays.
 */

u_char
zs_read_reg(struct zs_chanstate *cs, u_char reg)
{
	u_char val;

	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return (val);
}

void
zs_write_reg(struct zs_chanstate *cs, u_char reg, u_char val)
{

	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

u_char
zs_read_csr(struct zs_chanstate *cs)
{
	u_char val;

	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return (val);
}

void
zs_write_csr(struct zs_chanstate *cs, u_char val)
{

	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

u_char
zs_read_data(struct zs_chanstate *cs)
{
	u_char val;

	val = *cs->cs_reg_data;
	ZS_DELAY();
	return (val);
}

void
zs_write_data(struct zs_chanstate *cs, u_char val)
{

	*cs->cs_reg_data = val;
	ZS_DELAY();
}

/****************************************************************
 * Console support functions (Sun specific!)
 * Note: this code is allowed to know about the layout of
 * the chip registers, and uses that to keep things simple.
 * XXX - I think I like the mvme167 code better. -gwr
 ****************************************************************/

/*
 * Handle user request to enter kernel debugger.
 */
void
zs_abort(struct zs_chanstate *cs)
{
	struct zschan *zc = zs_conschan_get;
	int rr0;

	/* Wait for end of break to avoid PROM abort. */
	/* XXX - Limit the wait? */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while (rr0 & ZSRR0_BREAK);

#if defined(KGDB)
	zskgdb(cs);
#elif defined(DDB)
	Debugger();
#else
	printf("stopping on keyboard abort\n");
	callrom();
#endif
}

int  zs_getc(void *);
void zs_putc(void *, int);

/*
 * Polled input char.
 */
int
zs_getc(void *arg)
{
	struct zschan *zc = arg;
	int s, c, rr0;
	u_int omid;

	/* Temporarily direct interrupts at ourselves */
	s = splhigh();
	omid = setitr(cpuinfo.mid);

	/* Wait for a character to arrive. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = zc->zc_data;
	ZS_DELAY();
	setitr(omid);
	splx(s);

	/*
	 * This is used by the kd driver to read scan codes,
	 * so don't translate '\r' ==> '\n' here...
	 */
	return (c);
}

/*
 * Polled output char.
 */
void
zs_putc(void *arg, int c)
{
	struct zschan *zc = arg;
	int s, rr0;
	u_int omid;

	/* Temporarily direct interrupts at ourselves */
	s = splhigh();
	omid = setitr(cpuinfo.mid);

	/* Wait for transmitter to become ready. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_TX_READY) == 0);

	/*
	 * Send the next character.
	 * Now you'd think that this could be followed by a ZS_DELAY()
	 * just like all the other chip accesses, but it turns out that
	 * the `transmit-ready' interrupt isn't de-asserted until
	 * some period of time after the register write completes
	 * (more than a couple instructions).  So to avoid stray
	 * interrupts we put in the 2us delay regardless of CPU model.
	 */
	zc->zc_data = c;
	delay(2);

	setitr(omid);
	splx(s);
}

/*****************************************************************/
/*
 * Polled console input putchar.
 */
static int
zscngetc(dev_t dev)
{

	return (zs_getc(zs_conschan_get));
}

/*
 * Polled console output putchar.
 */
static void
zscnputc(dev_t dev, int c)
{

	zs_putc(zs_conschan_put, c);
}

static void
zscnpollc(dev_t dev, int on)
{

	/* No action needed */
}

static int
zs_console_flags(int promunit, int node, int channel)
{
	int cookie, flags = 0;

	switch (prom_version()) {
	case PROM_OLDMON:
	case PROM_OBP_V0:
		/*
		 * Use `promunit' and `channel' to derive the PROM
		 * stdio handles that correspond to this device.
		 */
		if (promunit == 0)
			cookie = PROMDEV_TTYA + channel;
		else if (promunit == 1 && channel == 0)
			cookie = PROMDEV_KBD;
		else
			cookie = -1;

		if (cookie == prom_stdin())
			flags |= ZS_HWFLAG_CONSOLE_INPUT;

		/*
		 * Prevent the keyboard from matching the output device
		 * (note that PROMDEV_KBD == PROMDEV_SCREEN == 0!).
		 */
		if (cookie != PROMDEV_KBD && cookie == prom_stdout())
			flags |= ZS_HWFLAG_CONSOLE_OUTPUT;

		break;

	case PROM_OBP_V2:
	case PROM_OBP_V3:
	case PROM_OPENFIRM:

		/*
		 * Match the nodes and device arguments prepared by
		 * consinit() against our device node and channel.
		 * (The device argument is the part of the OBP path
		 * following the colon, as in `/obio/zs@0,100000:a')
		 */

		/* Default to channel 0 if there are no explicit prom args */
		cookie = 0;

		if (node == prom_stdin_node) {
			if (prom_stdin_args[0] != '\0')
				/* Translate (a,b) -> (0,1) */
				cookie = prom_stdin_args[0] - 'a';

			if (channel == cookie)
				flags |= ZS_HWFLAG_CONSOLE_INPUT;
		}

		if (node == prom_stdout_node) {
			if (prom_stdout_args[0] != '\0')
				/* Translate (a,b) -> (0,1) */
				cookie = prom_stdout_args[0] - 'a';

			if (channel == cookie)
				flags |= ZS_HWFLAG_CONSOLE_OUTPUT;
		}

		break;

	default:
		break;
	}

	return (flags);
}

/*
 * Power management hooks for zsopen() and zsclose().
 * We use them to power on/off the ports, if necessary.
 */
int
zs_enable(struct zs_chanstate *cs)
{

	auxiotwoserialendis (ZS_ENABLE);
	cs->enabled = 1;
	return(0);
}

void
zs_disable(struct zs_chanstate *cs)
{

	auxiotwoserialendis (ZS_DISABLE);
	cs->enabled = 0;
}
