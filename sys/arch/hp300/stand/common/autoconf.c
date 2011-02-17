/*	$NetBSD: autoconf.c,v 1.11.74.2 2011/02/17 11:59:40 bouyer Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: autoconf.c 1.16 92/05/29
 *
 *	@(#)autoconf.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/reboot.h>

#include <hp300/stand/common/samachdep.h>
#include <hp300/stand/common/rominfo.h>
#include <hp300/stand/common/device.h>
#include <hp300/stand/common/hpibvar.h>
#include <hp300/stand/common/scsireg.h>
#include <hp300/stand/common/scsivar.h>

#include <hp300/dev/dioreg.h>
#include <hp300/stand/common/grfreg.h>
#include <hp300/dev/intioreg.h>

/*
 * Mapping of ROM MSUS types to BSD major device numbers
 * WARNING: major numbers must match bdevsw indices in hp300/conf.c.
 */
static const char rom2mdev[] = {
	0, 0, 						/* 0-1: none */
	6,	/* 2: network device; special */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		/* 3-13: none */
	4,	/* 14: SCSI disk */
	0,	/* 15: none */
	2,	/* 16: CS/80 device on HPIB */
	2,	/* 17: CS/80 device on HPIB */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 18-31: none */
};

struct hp_hw sc_table[MAXCTLRS];
int cpuspeed;

static u_long msustobdev(void);
static void find_devs(void);

#ifdef PRINTROMINFO
void
printrominfo(void)
{
	struct rominfo *rp = (struct rominfo *)ROMADDR;

	printf("boottype %x, name %s, lowram %x, sysflag %x\n",
	       rp->boottype, rp->name, rp->lowram, rp->sysflag&0xff);
	printf("rambase %x, ndrives %x, sysflag2 %x, msus %x\n",
	       rp->rambase, rp->ndrives, rp->sysflag2&0xff, rp->msus);
}
#endif

void
configure(void)
{

	switch (machineid) {
	case HP_320:
	case HP_330:
	case HP_340:
		cpuspeed = MHZ_16;
		break;
	case HP_350:
	case HP_360:
	case HP_362:
		cpuspeed = MHZ_25;
		break;
	case HP_370:
		cpuspeed = MHZ_33;
		break;
	case HP_375:
	case HP_400:
		cpuspeed = MHZ_50;
		break;
	case HP_380:
	case HP_382:
	case HP_425:
		cpuspeed = MHZ_25 * 2;	/* XXX */
		break;
	case HP_385:
	case HP_433:
		cpuspeed = MHZ_33 * 2;	/* XXX */
		break;
	default:	/* assume the fastest (largest delay value) */
		cpuspeed = MHZ_50;
		break;
	}
	find_devs();
	cninit();
#ifdef PRINTROMINFO
	printrominfo();
#endif
	hpibinit();
	scsiinit();
	if ((bootdev & B_MAGICMASK) != B_DEVMAGIC)
		bootdev = msustobdev();
}

/*
 * Convert HP MSUS to a valid bootdev layout:
 *	TYPE comes from MSUS device type as mapped by rom2mdev
 *	PARTITION is set to 0 ('a')
 *	UNIT comes from MSUS unit (almost always 0)
 *	CONTROLLER comes from MSUS primary address
 *	ADAPTER comes from SCSI/HPIB driver logical unit number
 *		(passed back via unused hw_pa field)
 */
static u_long
msustobdev(void)
{
	struct rominfo *rp = (struct rominfo *) ROMADDR;
	u_long bdev = 0;
	struct hp_hw *hw;
	int sc, type, ctlr, slave, punit;

	sc = (rp->msus >> 8) & 0xFF;
	for (hw = sc_table; hw < &sc_table[MAXCTLRS]; hw++)
		if (hw->hw_sc == sc)
			break;

	type  = rom2mdev[(rp->msus >> 24) & 0x1F];
	ctlr  = (int)hw->hw_pa;
	slave = (rp->msus & 0xFF);
	punit = ((rp->msus >> 16) & 0xFF);

	bdev  = MAKEBOOTDEV(type, ctlr, slave, punit, 0);

#ifdef PRINTROMINFO
	printf("msus %x -> bdev %x\n", rp->msus, bdev);
#endif
	return bdev;
}

int
sctoaddr(int sc)
{

	if (sc == -1)
		return INTIOBASE + FB_BASE;
	if (sc == 7 && internalhpib)
		return internalhpib ;
	if (sc < 32)
		return DIOBASE + sc * DIOCSIZE ;
	if (sc >= DIOII_SCBASE)
		return DIOIIBASE + (sc - DIOII_SCBASE) * DIOIICSIZE ;
	return sc;
}

/*
 * Probe all DIO select codes (0 - 32), the internal display address,
 * and DIO-II select codes (132 - 256).
 *
 * Note that we only care about displays, LANCEs, SCSIs and HP-IBs.
 */
static void
find_devs(void)
{
	short sc, sctop;
	u_char *id_reg;
	void *addr;
	struct hp_hw *hw;

	hw = sc_table;
	sctop = DIO_SCMAX(machineid);
	for (sc = -1; sc < sctop; sc++) {
		if (DIO_INHOLE(sc))
			continue;
		addr = (void *)sctoaddr(sc);
		if (badaddr(addr))
			continue;

		id_reg = (u_char *)addr;
		hw->hw_pa = 0;	/* XXX used to pass back LUN from driver */
		if (sc >= DIOII_SCBASE)
			hw->hw_size = DIOII_SIZE(id_reg);
		else
			hw->hw_size = DIOCSIZE;
		hw->hw_kva = addr;
		hw->hw_id = DIO_ID(id_reg);
		hw->hw_sc = sc;

		/*
		 * Not all internal HP-IBs respond rationally to id requests
		 * so we just go by the "internal HPIB" indicator in SYSFLAG.
		 */
		if (sc == 7 && internalhpib) {
			hw->hw_type = C_HPIB;
			hw++;
			continue;
		}

		switch (hw->hw_id) {
		case 5:		/* 98642A */
		case 5+128:	/* 98642A remote */
			hw->hw_type = D_COMMDCM;
			break;
		case 8:		/* 98625B */
		case 128:	/* 98624A */
			hw->hw_type = C_HPIB;
			break;
		case 21:	/* LANCE */
			hw->hw_type = D_LAN;
			break;
		case 57:	/* Displays */
			hw->hw_type = D_BITMAP;
			hw->hw_secid = id_reg[0x15];
			switch (hw->hw_secid) {
			case 4:	/* renaissance */
			case 8: /* davinci */
				sc++;		/* occupy 2 select codes */
				break;
			}
			break;
		case 9:
			hw->hw_type = D_KEYBOARD;
			break;
		case 7:
		case 7+32:
		case 7+64:
		case 7+96:
			hw->hw_type = C_SCSI;
			break;
		default:	/* who cares */
			hw->hw_type = D_MISC;
			break;
		}
		hw++;
	}
}
