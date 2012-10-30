/*	$NetBSD: zs.c,v 1.20.2.1 2012/10/30 17:20:25 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: zs.c,v 1.20.2.1 2012/10/30 17:20:25 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

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
#include <sys/intr.h>

#include <machine/autoconf.h>
#include <machine/promlib.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/psl.h>
#include <machine/z8530var.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>
#include <dev/sun/kbd_ms_ttyvar.h>
#include <ddb/db_output.h>

#include <sun2/dev/cons.h>

#include "ioconf.h"
#include "kbd.h"	/* NKBD */
#include "ms.h"		/* NMS */

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);

/* ZS channel used as the console device (if any) */
void *zs_conschan_get, *zs_conschan_put;

static uint8_t zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
#ifdef  ZS_INIT_IVECT
	ZS_INIT_IVECT,	/* 2: IVECT */
#else
	0,	/* 2: IVECT */
#endif
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,	/* 6: TXSYNC/SYNCLO */
	0,	/* 7: RXSYNC/SYNCHI */
	0,	/* 8: alias for data port */
#ifdef  ZS_INIT_IVECT
	ZSWR9_MASTER_IE,
#else
	ZSWR9_MASTER_IE | ZSWR9_NO_VECTOR,
#endif
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

static int  zs_print(void *, const char *name);

/* Interrupt handlers. */
int zscheckintr(void *);
static int zshard(void *);
static void zssoft(void *);

static int zs_get_speed(struct zs_chanstate *);

/*
 * Attach a found zs.
 *
 * USE ROM PROPERTIES port-a-ignore-cd AND port-b-ignore-cd FOR
 * SOFT CARRIER, AND keyboard PROPERTY FOR KEYBOARD/MOUSE?
 */
void 
zs_attach(struct zsc_softc *zsc, struct zsdevice *zsd, int pri)
{
	struct zsc_attach_args zsc_args;
	struct zs_chanstate *cs;
	int channel;

	memset(&zsc_args, 0, sizeof zsc_args);
	if (zsd == NULL) {
		aprint_error(": configuration incomplete\n");
		return;
	}

#if 0
	/* we should use ipl2si(softpri) but it isn't exported */
	aprint_normal(" softpri %d\n", _IPL_SOFT_LEVEL3);
#else
	aprint_normal("\n");
#endif

	/*
	 * Initialize software state for each channel.
	 */
	for (channel = 0; channel < 2; channel++) {
		struct zschan *zc;
		device_t child;

		zsc_args.channel = channel;
		zsc_args.hwflags = 0;
		cs = &zsc->zsc_cs_store[channel];
		zsc->zsc_cs[channel] = cs;

		zs_lock_init(cs);
		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		zc = (channel == 0) ? &zsd->zs_chan_a : &zsd->zs_chan_b;

		zsc_args.consdev = NULL;
		zsc_args.hwflags = zs_console_flags(zsc->zsc_promunit,
						    zsc->zsc_node,
						    channel);

		if (zsc_args.hwflags & ZS_HWFLAG_CONSOLE) {
			zsc_args.hwflags |= ZS_HWFLAG_USE_CONSDEV;
			zsc_args.consdev = &zs_consdev;
		}

		if ((zsc_args.hwflags & ZS_HWFLAG_CONSOLE_INPUT) != 0) {
			zs_conschan_get = zc;
		}
		if ((zsc_args.hwflags & ZS_HWFLAG_CONSOLE_OUTPUT) != 0) {
			zs_conschan_put = zc;
		}

		/* Children need to set cn_dev, etc */
		cs->cs_reg_csr  = &zc->zc_csr;
		cs->cs_reg_data = &zc->zc_data;

		memcpy(cs->cs_creg, zs_init_reg, 16);
		memcpy(cs->cs_preg, zs_init_reg, 16);

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
		if ((child = config_found(zsc->zsc_dev, (void *)&zsc_args,
		    zs_print)) == NULL) {
			/* No sub-driver.  Just reset it. */
			uint8_t reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			zs_lock_chan(cs);
			zs_write_reg(cs,  9, reset);
			zs_unlock_chan(cs);
		} 
#if (NKBD > 0) || (NMS > 0)
		/* 
		 * If this was a zstty it has a keyboard
		 * property on it we need to attach the
		 * sunkbd and sunms line disciplines.
		 */
		if (child 
		    && device_is_a(child, "zstty")) {
			struct kbd_ms_tty_attach_args kma;
			struct zstty_softc {	
				/*
				 * The following are the only fields
				 * we need here
				 */
				device_t zst_dev;
				struct  tty *zst_tty;
				struct	zs_chanstate *zst_cs;
			} *zst = device_private(child);
			struct tty *tp;

			kma.kmta_tp = tp = zst->zst_tty;
			if (tp != NULL) {
				kma.kmta_dev = tp->t_dev;
				kma.kmta_consdev = zsc_args.consdev;
			
				/* Attach 'em if we got 'em. */
				switch(zs_peripheral_type(zsc->zsc_promunit,
						 	  zsc->zsc_node,
						  	  channel)) {
				case ZS_PERIPHERAL_SUNKBD:
#if (NKBD > 0)
					kma.kmta_name = "keyboard";
					config_found(child, (void *)&kma, NULL);
#endif
					break;
				case ZS_PERIPHERAL_SUNMS:
#if (NMS > 0)
					kma.kmta_name = "mouse";
					config_found(child, (void *)&kma, NULL);
#endif
					break;
				default:
					break;
				}
			}
		}
#endif
	}

	/*
	 * Now safe to install interrupt handlers.
	 */
	bus_intr_establish(zsc->zsc_bustag, pri, IPL_SERIAL, 0, zshard, zsc);
	if ((zsc->zsc_softintr = softint_establish(SOFTINT_SERIAL,
	    zssoft, zsc)) == NULL)
		panic("%s: could not establish soft interrupt", __func__);

	evcnt_attach_dynamic(&zsc->zsc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(zsc->zsc_dev), "intr");


	/*
	 * Set the master interrupt enable and interrupt vector.
	 * (common to both channels, do it on A)
	 */
	cs = zsc->zsc_cs[0];
	zs_lock_chan(cs);
	/* interrupt vector */
	zs_write_reg(cs, 2, zs_init_reg[2]);
	/* master interrupt control (enable) */
	zs_write_reg(cs, 9, zs_init_reg[9]);
	zs_unlock_chan(cs);

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

static int 
zshard(void *arg)
{
	struct zsc_softc *zsc = arg;
	int rval;
	uint8_t rr3;

	rval = 0;
	while ((rr3 = zsc_intr_hard(zsc))) {
		/* Count up the interrupts. */
		rval |= rr3;
		zsc->zsc_intrcnt.ev_count++;
	}
	if (((zsc->zsc_cs[0] && zsc->zsc_cs[0]->cs_softreq) ||
	     (zsc->zsc_cs[1] && zsc->zsc_cs[1]->cs_softreq)) &&
	    zsc->zsc_softintr) {
		softint_schedule(zsc->zsc_softintr);
	}
	return (rval);
}

int 
zscheckintr(void *arg)
{
	struct zsc_softc *zsc;
	int unit, rval;

	rval = 0;
	for (unit = 0; unit < zs_cd.cd_ndevs; unit++) {

		zsc = device_lookup_private(&zs_cd, unit);
		if (zsc == NULL)
			continue;
		rval = (zshard((void *)zsc) || rval);
	}
	return (rval);
}


/*
 * We need this only for TTY_DEBUG purposes.
 */
static void 
zssoft(void *arg)
{
	struct zsc_softc *zsc = arg;
	int s;

	/* Make sure we call the tty layer at spltty. */
	s = spltty();
	(void)zsc_intr_soft(zsc);
#ifdef TTY_DEBUG
	{
		struct zstty_softc *zst0 = zsc->zsc_cs[0]->cs_private;
		struct zstty_softc *zst1 = zsc->zsc_cs[1]->cs_private;
		if (zst0->zst_overflows || zst1->zst_overflows ) {
			struct trapframe *frame = arg;	/* XXX */
			
			printf("zs silo overflow from %p\n",
			    (long)frame->tf_pc);
		}
	}
#endif
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
zs_set_modes(struct zs_chanstate *cs, int cflag	/* bits per second */)
{

	/*
	 * Output hardware flow control on the chip is horrendous:
	 * if carrier detect drops, the receiver is disabled, and if
	 * CTS drops, the transmitter is stoped IN MID CHARACTER!
	 * Therefore, NEVER set the HFC bit, and instead use the
	 * status interrupt to detect CTS changes.
	 */
	zs_lock_chan(cs);
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
	zs_unlock_chan(cs);

	/* Caller will stuff the pending registers. */
	return (0);
}


/*
 * Read or write the chip with suitable delays.
 */

uint8_t
zs_read_reg(struct zs_chanstate *cs, uint8_t reg)
{
	uint8_t val;

	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return (val);
}

void
zs_write_reg(struct zs_chanstate *cs, uint8_t reg, uint8_t val)
{
	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

uint8_t
zs_read_csr(struct zs_chanstate *cs)
{
	uint8_t val;

	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return (val);
}

void
zs_write_csr(struct zs_chanstate *cs, uint8_t val)
{
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

uint8_t
zs_read_data(struct zs_chanstate *cs)
{
	uint8_t val;

	val = *cs->cs_reg_data;
	ZS_DELAY();
	return (val);
}

void
zs_write_data(struct zs_chanstate *cs, uint8_t val)
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

extern void Debugger(void);

/*
 * Handle user request to enter kernel debugger.
 */
void 
zs_abort(struct zs_chanstate *cs)
{
	volatile struct zschan *zc = zs_conschan_get;
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
	{
		extern int db_active;
		
		if (!db_active)
			Debugger();
		else
			/* Debugger is probably hozed */
			callrom();
	}
#else
	printf("stopping on keyboard abort\n");
	callrom();
#endif
}


/*
 * Polled input char.
 */
int 
zs_getc(void *arg)
{
	volatile struct zschan *zc = arg;
	int s, c, rr0;

	s = splhigh();
	/* Wait for a character to arrive. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = zc->zc_data;
	ZS_DELAY();
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
	volatile struct zschan *zc = arg;
	int s, rr0;

	s = splhigh();

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

int swallow_zsintrs;

static void 
zscnpollc(dev_t dev, int on)
{
	/* 
	 * Need to tell zs driver to acknowledge all interrupts or we get
	 * annoying spurious interrupt messages.  This is because mucking
	 * with spl() levels during polling does not prevent interrupts from
	 * being generated.
	 */

	if (on) swallow_zsintrs++;
	else swallow_zsintrs--;
}

