/*	$NetBSD: zs.c,v 1.5 2001/07/08 21:04:50 thorpej Exp $	*/

/*-
 * Copyright (c) 1996, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Wayne Knowles
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
 */

#include "opt_ddb.h"

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

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/autoconf.h>
#include <machine/z8530var.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);
int zs_major = 35;

#define PCLK		3672000	 /* PCLK pin input clock rate */

#ifndef ZS_DEFSPEED
#define ZS_DEFSPEED	9600
#endif

/*
 * Define interrupt levels.
 */
#define ZSHARD_PRI 64

/* SGI shouldn't need ZS_DELAY() as recovery time is done in hardware? */
#define ZS_DELAY()	delay(3)

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	u_char   pad1[3];
	volatile u_char	zc_csr;		/* ctrl,status, and indirect access */
	u_char   pad2[3];
	volatile u_char	zc_data;	/* data */
};

struct zsdevice {
	struct	zschan zs_chan_b;
	struct	zschan zs_chan_a;
};

/* Return the byte offset of element within a structure */
#define OFFSET(struct_def, el)		((size_t)&((struct_def *)0)->el)

#define ZS_CHAN_A	OFFSET(struct zsdevice, zs_chan_a)
#define ZS_CHAN_B	OFFSET(struct zsdevice, zs_chan_b)
#define ZS_REG_CSR	0
#define ZS_REG_DATA	1
static int zs_chan_offset[] = {ZS_CHAN_A, ZS_CHAN_B};

static void zscnprobe __P((struct consdev *));
static void zscninit __P((struct consdev *));
static int  zscngetc __P((dev_t));
static void zscnputc __P((dev_t, int));
static void zscnpollc __P((dev_t, int));

static int  cons_port;

struct consdev zs_cn = {
	zscnprobe,
	zscninit,
	zscngetc,
	zscnputc,
	zscnpollc
};

/* Flags from cninit() */
static int zs_consunit = -1;
static int zs_conschan = -1;

/* Default speed for all channels */
static int zs_defspeed = ZS_DEFSPEED;
static volatile int zssoftpending;

static u_char zs_init_reg[16] = {
	0,				/* 0: CMD (reset, etc.) */
	0,				/* 1: No interrupts yet. */
	ZSHARD_PRI,			/* 2: IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,				/* 6: TXSYNC/SYNCLO */
	0,				/* 7: RXSYNC/SYNCHI */
	0,				/* 8: alias for data port */
	ZSWR9_MASTER_IE,
	0,				/*10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD | ZSWR11_TRXC_OUT_ENA,
	BPS_TO_TCONST(PCLK/16, ZS_DEFSPEED), /*12: BAUDLO (default=9600) */
	0,				/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA,
	ZSWR15_BREAK_IE,
};


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int	zs_hpc_match __P((struct device *, struct cfdata *, void *));
static void	zs_hpc_attach __P((struct device *, struct device *, void *));
static int	zs_print __P((void *, const char *name));

struct cfattach zsc_hpc_ca = {
	sizeof(struct zsc_softc), zs_hpc_match, zs_hpc_attach
};

extern struct	cfdriver zsc_cd;

static int	zshard __P((void *));
void		zssoft __P((void *));
static int	zs_get_speed __P((struct zs_chanstate *));
struct		zschan *zs_get_chan_addr (int zs_unit, int channel);
int		zs_getc __P((void *));
void		zs_putc __P((void *, int));

/*
 * Is the zs chip present?
 */
static int
zs_hpc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

/*
 * Attach a found zs.
 *
 * Match slave number to zs unit number, so that misconfiguration will
 * not set up the keyboard as ttya, etc.
 */
static void
zs_hpc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *) self;
	struct hpc_attach_args *haa = aux;
	struct zsc_attach_args zsc_args;
	struct zs_chanstate *cs;
	struct zs_channel *ch;
	int    zs_unit, channel, err, s;
	char  *promconsdev;

	promconsdev = ARCBIOS->GetEnvironmentVariable("ConsoleOut");

	zsc->zsc_bustag = haa->ha_iot;
	if ((err = bus_space_subregion(haa->ha_iot, haa->ha_ioh,
				       HPC_PBUS_CH6_DEVREGS + 0x30, 0x10, 
				       &zsc->zsc_base)) != 0) {
		printf(": unable to map 85c30 registers, error = %d\n", err);
		return;
	}

	zs_unit = zsc->zsc_dev.dv_unit;
	printf("\n");

	/*
	 * Initialize software state for each channel.  
	 * 
	 * Done in reverse order of channels since the first serial port
	 * is actually attached to the *second* channel, and vice versa.
	 * Doing it this way should force a 'zstty*' to attach zstty0 to
	 * channel 1 and zstty1 to channel 0.  They couldn't have wired
	 * it up in a more sensible fashion, could they?
	 */
	for (channel = 1; channel >= 0; channel--) {
		zsc_args.channel = channel;
		ch = &zsc->zsc_cs_store[channel];
		cs = zsc->zsc_cs[channel] = (struct zs_chanstate *)ch;

		cs->cs_reg_csr = NULL;
		cs->cs_reg_data = NULL;
		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		if (bus_space_subregion(zsc->zsc_bustag, zsc->zsc_base,
					zs_chan_offset[channel],
					sizeof(struct zschan),
					&ch->cs_regs) != 0) {
			printf(": cannot map regs\n");
			return;
		}
		ch->cs_bustag = zsc->zsc_bustag;

		memcpy(cs->cs_creg, zs_init_reg, 16);
		memcpy(cs->cs_preg, zs_init_reg, 16);

		zsc_args.hwflags = 0;
		zsc_args.consdev = NULL;

		if (zs_consunit == -1 && zs_conschan == -1) {
		    /* 
		     * If this channel is being used by the PROM console,
		     * pass the generic zs driver a 'no reset' flag so the
		     * channel gets left in the appropriate state after
		     * attach.
		     *
		     * Note: the channel mappings are swapped.
		     */
		    if (promconsdev != NULL &&
			strlen(promconsdev) == 9 &&
			strncmp(promconsdev, "serial", 6) == 0 &&
			(promconsdev[7] == '0' || promconsdev[7] == '1')) {
			if (promconsdev[7] == '1' && channel == 0)
			    zsc_args.hwflags |= ZS_HWFLAG_NORESET;
			else if (promconsdev[7] == '0' && channel == 1)
			    zsc_args.hwflags |= ZS_HWFLAG_NORESET;
		    }
		}

		/* If console, don't stomp speed, let zstty know */
		if (zs_unit == zs_consunit && channel == zs_conschan) {
			zsc_args.consdev = &zs_cn;
			zsc_args.hwflags = ZS_HWFLAG_CONSOLE;

			cs->cs_defspeed = zs_get_speed(cs);
		} else
			cs->cs_defspeed = zs_defspeed;

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
		if (!config_found(self, (void *)&zsc_args, zs_print)) {
			/* No sub-driver.  Just reset it. */
			u_char reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;

			s = splhigh();
 			zs_write_reg(cs,  9, reset);
			splx(s);
		}
	}


	zsc->sc_si = softintr_establish(IPL_SOFTSERIAL, zssoft, zsc);
	cpu_intr_establish(29, IPL_TTY, zshard, NULL);

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
}

static int
zs_print(aux, name)
	void *aux;
	const char *name;
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		printf("%s: ", name);

	if (args->channel != -1)
		printf(" channel %d", args->channel);

	return UNCONF;
}

/*
 * Our ZS chips all share a common, autovectored interrupt,
 * so we have to look at all of them on each interrupt.
 */
static int
zshard(arg)
	void *arg;
{
	register struct zsc_softc *zsc;
	register int unit, rval, softreq;

	rval = 0;
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		rval |= zsc_intr_hard(zsc);
		softreq = zsc->zsc_cs[0]->cs_softreq;
		softreq |= zsc->zsc_cs[1]->cs_softreq;
		if (softreq && (zssoftpending == 0)) {
		    zssoftpending = 1;
		    softintr_schedule(zsc->sc_si);
		}
	}
	return rval;
}

/*
 * Similar scheme as for zshard (look at all of them)
 */
void
zssoft(arg)
	void *arg;
{
	register struct zsc_softc *zsc;
	register int s, unit;

	/* This is not the only ISR on this IPL. */
	if (zssoftpending == 0)
		return;

	/*
	 * The soft intr. bit will be set by zshard only if
	 * the variable zssoftpending is zero.  The order of
	 * these next two statements prevents our clearing
	 * the soft intr bit just after zshard has set it.
	 */
	/*isr_soft_clear(ZSSOFT_PRI);*/
	zssoftpending = 0;

	/* Make sure we call the tty layer at spltty. */
	s = spltty();
	for (unit = 0; unit < zsc_cd.cd_ndevs; unit++) {
		zsc = zsc_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		(void) zsc_intr_soft(zsc);
	}
	splx(s);
	return;
}


/*
 * Compute the current baud rate given a ZS channel.
 */
static int
zs_get_speed(cs)
	struct zs_chanstate *cs;
{
	int tconst;

	tconst = zs_read_reg(cs, 12);
	tconst |= zs_read_reg(cs, 13) << 8;
	return (TCONST_TO_BPS(cs->cs_brg_clk, tconst));
}

/*
 * MD functions for setting the baud rate and control modes.
 */
int
zs_set_speed(cs, bps)
	struct zs_chanstate *cs;
	int bps;	/* bits per second */
{
	int tconst, real_bps;

#if 0
	while (!(zs_read_csr(cs) & ZSRR0_TX_READY))
	        {/*nop*/}
#endif
	/* Wait for transmit buffer to empty */
	if (bps == 0) {
		return (0);
	}

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
#if 0
	if (real_bps != bps)
		return (EINVAL);
#endif

	cs->cs_preg[12] = tconst;
	cs->cs_preg[13] = tconst >> 8;

	/* Caller will stuff the pending registers. */
	return (0);
}

int
zs_set_modes(cs, cflag)
	struct zs_chanstate *cs;
	int cflag;	/* bits per second */
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
zs_read_reg(cs, reg)
	struct zs_chanstate *cs;
	u_char reg;
{
	u_char val;
	struct zs_channel *zsc = (struct zs_channel *)cs;

	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR, reg);
	ZS_DELAY();
	val = bus_space_read_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR);
	ZS_DELAY();
	return val;
}

void
zs_write_reg(cs, reg, val)
	struct zs_chanstate *cs;
	u_char reg, val;
{
	struct zs_channel *zsc = (struct zs_channel *)cs;

	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR, reg);
	ZS_DELAY();
	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR, val);
	ZS_DELAY();
}

u_char zs_read_csr(cs)
	struct zs_chanstate *cs;
{
	struct zs_channel *zsc = (struct zs_channel *)cs;
	register u_char val;

	val = bus_space_read_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR);
	ZS_DELAY();
	return val;
}

void  zs_write_csr(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{
	struct zs_channel *zsc = (struct zs_channel *)cs;

	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_CSR, val);
	ZS_DELAY();
}

u_char zs_read_data(cs)
	struct zs_chanstate *cs;
{
	struct zs_channel *zsc = (struct zs_channel *)cs;
	register u_char val;

	val = bus_space_read_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_DATA);
	ZS_DELAY();
	return val;
}

void  zs_write_data(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{
	struct zs_channel *zsc = (struct zs_channel *)cs;

	bus_space_write_1(zsc->cs_bustag, zsc->cs_regs, ZS_REG_DATA, val);
	ZS_DELAY();
}

void
zs_abort(cs)
	struct zs_chanstate *cs;
{
#if defined(KGDB)
	zskgdb(cs);
#elif defined(DDB)
	Debugger();
#endif
}


/*********************************************************/
/*  Polled character I/O functions for console and KGDB  */
/*********************************************************/

struct zschan *
zs_get_chan_addr(zs_unit, channel)
        int zs_unit, channel;
{
	static int dumped_addr = 0;
        struct zsdevice *addr;
        struct zschan *zc;

        addr = (struct zsdevice *) MIPS_PHYS_TO_KSEG1(0x1fbd9830);
        
        if (channel == 0) {
                zc = &addr->zs_chan_b;
        } else {
                zc = &addr->zs_chan_a;
        }

	if (dumped_addr == 0) {
		dumped_addr++;
		printf("zs channel %d had address %p\n", channel, zc);
	}

        return (zc);
}      

int
zs_getc(arg)
	void *arg;
{
	register volatile struct zschan *zc = arg;
	register int s, c, rr0;

	s = splzs();
	/* Wait for a character to arrive. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = zc->zc_data;
	ZS_DELAY();
	splx(s);

	return (c);
}

/*
 * Polled output char.
 */
void
zs_putc(arg, c)
	void *arg;
	int c;
{
	register volatile struct zschan *zc = arg;
	register int s, rr0;

	s = splzs();
	/* Wait for transmitter to become ready. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_TX_READY) == 0);

	zc->zc_data = c;
	wbflush();
	ZS_DELAY();
	splx(s);
}

/***************************************************************/
void
zscnprobe(cn)
	struct consdev *cn;
{
}

void
zscninit(cn)
	struct consdev *cn;
{
	char* consdev;

	if ((consdev = ARCBIOS->GetEnvironmentVariable("ConsoleOut")) == NULL)
	    panic("zscninit without valid ARCS ConsoleOut setting!\n");

	if (strlen(consdev) != 9 ||
	    strncmp(consdev, "serial", 6) != 0)
	    panic("zscninit with ARCS console not set to serial!\n");

	cons_port = consdev[7] - '0';

	cn->cn_dev = makedev(zs_major, cons_port);
	cn->cn_pri = CN_REMOTE;

	/* Mark this unit as the console */
	zs_consunit = 0;

	/* SGI hardware wires serial port 1 to channel B, port 2 to A */
	if (cons_port == 0)
	    zs_conschan = 1;
	else
	    zs_conschan = 0;
}

int
zscngetc(dev)
	dev_t dev;
{
	struct zschan *zs;

	zs = zs_get_chan_addr(0, cons_port);
	return zs_getc(zs);
}

void
zscnputc(dev, c)
	dev_t dev;
	int c;
{
	struct zschan *zs;

	zs = zs_get_chan_addr(0, cons_port);
	zs_putc(zs, c);
}

void
zscnpollc(dev, on)
	dev_t dev;
	int on;
{
}
