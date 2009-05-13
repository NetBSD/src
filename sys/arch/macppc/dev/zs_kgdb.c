/*	$NetBSD: zs_kgdb.c,v 1.8.14.1 2009/05/13 17:18:01 jym Exp $	*/

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
 * Hooks for kgdb when attached via the z8530 driver
 *
 * To use this, build a kernel with: option KGDB, and
 * boot that kernel with "-d".  (The kernel will call
 * zs_kgdb_init, kgdb_connect.)  When the console prints
 * "kgdb waiting..." you run "gdb -k kernel" and do:
 *   (gdb) set remotebaud 19200
 *   (gdb) target remote /dev/ttyb
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zs_kgdb.c,v 1.8.14.1 2009/05/13 17:18:01 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/kgdb.h>

#include <dev/ofw/openfirm.h>
#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

static void zs_setparam(struct zs_chanstate *, int, int);
static void zskgdb(struct zs_chanstate *);

struct zsops zsops_kgdb;

static u_char zs_kgdb_regs[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: ~(ZSWR1_RIE | ZSWR1_TIE | ZSWR1_SIE) */
	0,	/* IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,	/* 6: TXSYNC/SYNCLO */
	0,	/* 7: RXSYNC/SYNCHI */
	0,	/* 8: alias for data port */
	ZSWR9_MASTER_IE,
	0,	/*10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
	((PCLK/32)/38400)-2,	/*12: BAUDLO (default=38400) */
	0,			/*13: BAUDHI (default=38400) */
	ZSWR14_BAUD_ENA,
	ZSWR15_BREAK_IE,
};

/*
 * This replaces "zs_reset()" in the sparc driver.
 */
static void
zs_setparam(struct zs_chanstate *cs, int iena, int rate)
{
	int s, tconst;

	memcpy( cs->cs_preg, zs_kgdb_regs, 16);

	if (iena) {
		cs->cs_preg[1] = ZSWR1_RIE | ZSWR1_SIE;
	}

	/* Initialize the speed, etc. */
	tconst = BPS_TO_TCONST(cs->cs_brg_clk, rate);
	cs->cs_preg[5] |= ZSWR5_DTR | ZSWR5_RTS;
	cs->cs_preg[12] = tconst;
	cs->cs_preg[13] = tconst >> 8;

	s = splhigh();
	zs_loadchannelregs(cs);
	splx(s);
}

#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME "scca"
#endif
char *zs_kgdb_devname = KGDB_DEVNAME;
/*
 * Set up for kgdb; called at boot time before configuration.
 * KGDB interrupts will be enabled later when zs0 is configured.
 * Called after cninit(), so printf() etc. works.
 */
void
zs_kgdb_init(void)
{
	extern const struct cdevsw zstty_cdevsw;
	struct zs_chanstate cs;
	volatile struct zschan *zc;
	int escc, escc_ch, obio, zs_offset;
	int channel = 0;
	u_int32_t reg[5];
	char name[16];

	if ((escc_ch = OF_finddevice(zs_kgdb_devname)) == -1)
		return;

	memset(name, 0, sizeof(name));
	if (OF_getprop(escc_ch, "device_type", name, sizeof(name)) == -1)
		return;

	if (strcmp(name, "serial") != 0)
		return;

	memset(name, 0, sizeof(name));
	if (OF_getprop(escc_ch, "name", name, sizeof(name)) == -1)
		return;

	if (strcmp(name, "ch-b") == 0)
		channel = 1;

	if (OF_getprop(escc_ch, "reg", reg, sizeof(reg)) < 4)
		return;
	zs_offset = reg[0];

	escc = OF_parent(escc_ch);
	obio = OF_parent(escc);

	if (OF_getprop(obio, "assigned-addresses", reg, sizeof(reg)) < 12)
		return;
	zc = (struct zschan *)(reg[2] + zs_offset);

	kgdb_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw), channel);

	printf("zs_kgdb_init: attaching tty%02d at %d baud\n",
		channel, kgdb_rate);

	/* Setup temporary chanstate. */
	memset((void *)&cs, 0, sizeof(cs));
	cs.cs_channel = channel;
	cs.cs_brg_clk = PCLK / 16;
	cs.cs_reg_csr  = &zc->zc_csr;
	cs.cs_reg_data = &zc->zc_data;

	/* Now set parameters. (interrupts disabled) */
	zs_setparam(&cs, 0, kgdb_rate);

	/* Store the getc/putc functions and arg. */
	kgdb_attach(zs_getc, zs_putc, __UNVOLATILE(zc));
}

/*
 * This is a "hook" called by zstty_attach to allow the tty
 * to be "taken over" for exclusive use by kgdb.
 * Return non-zero if this is the kgdb port.
 *
 * Set the speed to kgdb_rate, CS8, etc.
 */
int
zs_check_kgdb(struct zs_chanstate *cs, int dev)
{

	if (dev != kgdb_dev)
		return (0);

	/*
	 * Yes, this is port in use by kgdb.
	 */
	cs->cs_private = NULL;
	cs->cs_ops = &zsops_kgdb;

	/* Now set parameters. (interrupts enabled) */
	zs_setparam(cs, 1, kgdb_rate);

	return (1);
}

/*
 * KGDB framing character received: enter kernel debugger.  This probably
 * should time out after a few seconds to avoid hanging on spurious input.
 */
static void
zskgdb(struct zs_chanstate *cs)
{
	int unit = minor(kgdb_dev);

	printf("zstty%d: kgdb interrupt\n", unit);
	/* This will trap into the debugger. */
	kgdb_connect(1);
}


/****************************************************************
 * Interface to the lower layer (zscc)
 ****************************************************************/

static void zs_kgdb_rxint(struct zs_chanstate *);
static void zs_kgdb_stint(struct zs_chanstate *, int);
static void zs_kgdb_txint(struct zs_chanstate *);
static void zs_kgdb_softint(struct zs_chanstate *);

int kgdb_input_lost;

static void
zs_kgdb_rxint(struct zs_chanstate *cs)
{
	register u_char c, rr1;

	/*
	 * First read the status, because reading the received char
	 * destroys the status of this char.
	 */
	rr1 = zs_read_reg(cs, 1);
	c = zs_read_data(cs);

	if (rr1 & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE)) {
		/* Clear the receive error. */
		zs_write_csr(cs, ZSWR0_RESET_ERRORS);
	}

	if (c == KGDB_START) {
		zskgdb(cs);
	} else {
		kgdb_input_lost++;
	}
}

static void
zs_kgdb_txint(register struct zs_chanstate *cs)
{
	register int rr0;

	rr0 = zs_read_csr(cs);
	zs_write_csr(cs, ZSWR0_RESET_TXINT);
}

static void
zs_kgdb_stint(register struct zs_chanstate *cs, int force)
{
	register int rr0;

	rr0 = zs_read_csr(cs);
	zs_write_csr(cs, ZSWR0_RESET_STATUS);

	/*
	 * Check here for console break, so that we can abort
	 * even when interrupts are locking up the machine.
	 */
	if (rr0 & ZSRR0_BREAK) {
		zskgdb(cs);
	}
}

static void
zs_kgdb_softint(struct zs_chanstate *cs)
{
	printf("zs_kgdb_softint?\n");
}

struct zsops zsops_kgdb = {
	zs_kgdb_rxint,	/* receive char available */
	zs_kgdb_stint,	/* external/status */
	zs_kgdb_txint,	/* xmit buffer empty */
	zs_kgdb_softint,	/* process software interrupt */
};
