/*	$NetBSD: zs_kgdb.c,v 1.7.4.1 2001/10/01 12:42:01 fvdl Exp $	*/

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
 * Hooks for kgdb when attached via the z8530 driver
 *
 * To use this, build a kernel with: option KGDB, and
 * boot that kernel with "-d".  (The kernel will call
 * zs_kgdb_init, kgdb_connect.)  When the console prints
 * "kgdb waiting..." you run "gdb -k kernel" and do:
 *   (gdb) set remotebaud 19200
 *   (gdb) target remote /dev/ttyb
 */

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/kgdb.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>
#include <machine/autoconf.h>
#include <machine/promlib.h>
#include <sparc/dev/cons.h>

/* Suns provide a 4.9152 MHz clock to the ZS chips. */
#define PCLK	(9600 * 512)	/* PCLK pin input clock rate */

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

static void zs_setparam __P((struct zs_chanstate *, int, int));
static void *findzs __P((int));
struct zsops zsops_kgdb;

extern int  zs_getc __P((void *arg));
extern void zs_putc __P((void *arg, int c));

static u_char zs_kgdb_regs[16] = {
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
	14,	/*12: BAUDLO (default=9600) */
	0,	/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};

/*
 * This replaces "zs_reset()" in the sparc driver.
 */
static void
zs_setparam(cs, iena, rate)
	struct zs_chanstate *cs;
	int iena;
	int rate;
{
	int s, tconst;

	bcopy(zs_kgdb_regs, cs->cs_preg, 16);

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

/*
 * Set up for kgdb; called at boot time before configuration.
 * KGDB interrupts will be enabled later when zs0 is configured.
 * Called after cninit(), so printf() etc. works.
 */
void
zs_kgdb_init()
{
	struct zs_chanstate cs;
	struct zsdevice *zsd;
	volatile struct zschan *zc;
	int channel, promzs_unit;

	/* printf("zs_kgdb_init: kgdb_dev=0x%x\n", kgdb_dev); */
	if (major(kgdb_dev) != zs_major)
		return;

	/* Note: (ttya,ttyb) on zs0, and (ttyc,ttyd) on zs2 */
	promzs_unit = (kgdb_dev & 2) ? 2 : 0;
	channel  =  kgdb_dev & 1;
	printf("zs_kgdb_init: attaching tty%c at %d baud\n",
		   'a' + (kgdb_dev & 3), kgdb_rate);

	/* Setup temporary chanstate. */
	bzero((caddr_t)&cs, sizeof(cs));
	zsd = findzs(promzs_unit);
	if (zsd == NULL) {
		printf("zs_kgdb_init: zs not mapped.\n");
		return;
	}
	zc = (channel == 0) ? &zsd->zs_chan_a : &zsd->zs_chan_b;

	cs.cs_channel = channel;
	cs.cs_brg_clk = PCLK / 16;
	cs.cs_reg_csr  = &zc->zc_csr;
	cs.cs_reg_data = &zc->zc_data;

	/* Now set parameters. (interrupts disabled) */
	zs_setparam(&cs, 0, kgdb_rate);

	/* Store the getc/putc functions and arg. */
	kgdb_attach(zs_getc, zs_putc, (void *)zc);
}

/*
 * This is a "hook" called by zstty_attach to allow the tty
 * to be "taken over" for exclusive use by kgdb.
 * Return non-zero if this is the kgdb port.
 *
 * Set the speed to kgdb_rate, CS8, etc.
 */
int
zs_check_kgdb(cs, dev)
	struct zs_chanstate *cs;
	int dev;
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
void
zskgdb(cs)
	struct zs_chanstate *cs;
{
	int unit = minor(kgdb_dev);

	printf("zstty%d: kgdb interrupt\n", unit);
	/* This will trap into the debugger. */
	kgdb_connect(1);
}


/****************************************************************
 * Interface to the lower layer (zscc)
 ****************************************************************/

static void zs_kgdb_rxint __P((struct zs_chanstate *));
static void zs_kgdb_stint __P((struct zs_chanstate *, int));
static void zs_kgdb_txint __P((struct zs_chanstate *));
static void zs_kgdb_softint __P((struct zs_chanstate *));

int kgdb_input_lost;

static void
zs_kgdb_rxint(cs)
	struct zs_chanstate *cs;
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
zs_kgdb_txint(cs)
	register struct zs_chanstate *cs;
{
	register int rr0;

	rr0 = zs_read_csr(cs);
	zs_write_csr(cs, ZSWR0_RESET_TXINT);
}

static void
zs_kgdb_stint(cs, force)
	register struct zs_chanstate *cs;
	int force;
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
zs_kgdb_softint(cs)
	struct zs_chanstate *cs;
{
	printf("zs_kgdb_softint?\n");
}

struct zsops zsops_kgdb = {
	zs_kgdb_rxint,	/* receive char available */
	zs_kgdb_stint,	/* external/status */
	zs_kgdb_txint,	/* xmit buffer empty */
	zs_kgdb_softint,	/* process software interrupt */
};

/*
 * findzs() should return the address of the given zs channel.
 * Here we count on the PROM to map in the required zs chips.
 */
void *
findzs(zs)
	int zs;
{

#if defined(SUN4)
	if (CPU_ISSUN4) {
		/*
		 * On sun4, we use hard-coded physical addresses
		 */
#define ZS0_PHYS	0xf1000000
#define ZS1_PHYS	0xf0000000
#define ZS2_PHYS	0xe0000000
		bus_space_handle_t bh;
		bus_addr_t paddr;

		switch (zs) {
		case 0:
			paddr = ZS0_PHYS;
			break;
		case 1:
			paddr = ZS1_PHYS;
			break;
		case 2:
			paddr = ZS2_PHYS;
			break;
		default:
			return (NULL);
		}

		if (cpuinfo.cpu_type == CPUTYP_4_100)
			/* Clear top bits of physical address on 4/100 */
			paddr &= ~0xf0000000;

		/*
		 * Have the obio module figure out which virtual
		 * address the device is mapped to.
		 */
		if (obio_find_rom_map(paddr, PMAP_OBIO, NBPG, &bh) != 0)
			return (NULL);

		return ((void *)bh);
	}
#endif

#if defined(SUN4C) || defined(SUN4M)
	if (CPU_ISSUN4COR4M) {
		int node;

		node = firstchild(findroot());
		if (CPU_ISSUN4M) {
			/*
			 * On sun4m machines zs is in "obio" tree.
			 */
			node = findnode(node, "obio");
			if (node == 0)
				panic("findzs: no obio node");
			node = firstchild(node);
		}
		while ((node = findnode(node, "zs")) != 0) {
			int nvaddrs, *vaddrs, vstore[10];

			if (PROM_getpropint(node, "slave", -1) != zs) {
				node = nextsibling(node);
				continue;
			}

			/*
			 * On some machines (e.g. the Voyager), the zs
			 * device has multi-valued register properties.
			 */
			vaddrs = vstore;
			nvaddrs = sizeof(vstore)/sizeof(vstore[0]);
			if (PROM_getprop(node, "address", sizeof(int),
				    &nvaddrs, (void **)&vaddrs) != 0)
				return (NULL);

			return ((void *)vaddrs[0]);
		}
	}
#endif
	return (NULL);
}
