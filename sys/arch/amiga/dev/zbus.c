/*	$NetBSD: zbus.c,v 1.73.14.1 2016/10/05 20:55:24 skrll Exp $ */

/*
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zbus.c,v 1.73.14.1 2016/10/05 20:55:24 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <amiga/amiga/cfdev.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/dev/z3rambdvar.h>

#include "z3rambd.h"

struct aconfdata {
	const char *name;
	int manid;
	int prodid;
};

struct preconfdata {
	int manid;
	int prodid;
	void *vaddr;
};

struct quirksdata {
	int manid;
	int prodid;
	uint8_t quirks;
};

vaddr_t		ZTWOROMADDR;
vaddr_t		ZTWOMEMADDR;
u_int		NZTWOMEMPG;
vaddr_t		ZBUSADDR;	/* kva of Zorro bus I/O pages */
u_int		ZBUSAVAIL;	/* bytes of Zorro bus I/O space left */

/*
 * explain the names.. 0123456789 => zothfisven
 */
static const struct aconfdata aconftab[] = {
	/* Commodore Amiga */
	{ "atzee",	513,	1 },
	{ "atzsc",	514,	2 },
	{ "atzsc",	514,	3 },
	{ "bah",	514,	9 },	/* A2060 */
	{ "ql",		514,	69 },
	{ "ql",		514,	70 },
	{ "atfsc",	514,	84 },
	{ "le",		514,	112 },
	/* Ameristar */
	{ "le",		1053,	1 },
	{ "bah",	1053,	9 },	/* A2060 */
	{ "es",		1053,	10 },
	/* University of Lowell */
	{ "grful",	1030,	0 },
	/* DMI */
	{ "grfrs",	2129,	1 },	/* Resolver graphics board */
	/* Macrosystems */
	{ "grfrt",	18260,	6 },
	{ "grfrh",	18260,	16},	/* Retina BLT Z3 */
	{ "grfrh",	18260,	19},	/* Altais */
	/* Greater valley products */
	{ "gvpbus",	2017,	2 },
	{ "gvpbus",	2017,	11 },
	{ "giv",	2017,	32 },
	{ "gio",	2017,	255 },
	/* progressive perhiperals */
	{ "zssc",	2026,	150 },
	{ "ppia",	2026,	187 },
	{ "ppta",	2026,	105 },
	{ "ppha",	2026,	1 },
	{ "mrsc",	2026,	0 },
	/* CSA */
	{ "mgnsc",	1058,	17 },
	{ "otgsc",	1058,	21 },
	/* Microbotics */
	{ "vhzsc",	1010,	69 },
	/* Supra */
	{ "wstsc",	1056,	12 },
	{ "wstsc",	1056,	13 },
	/* IVS */
	{ "itrmp",	2112,	48 },
	{ "itrmp",	2112,	52 },
	{ "ivasc",	2112,	242 },
	{ "ivsc",	2112,	243 },
	/* Hydra */
	{ "ed",		2121,	1 },
	/* ASDG */
	{ "ed",		1023,	254 },
	/* Village Tronic Ariadne */
	{ "le",		2167,	201},
	/* Village Tronic Ariadne II */
	{ "ne",		2167,	202},
	/* bsc/Alf Data */
	{ "Tandem", 2092,    6 },	/* Tandem AT disk controller */
	{ "mfc",	2092,	16 },
	{ "mfc",	2092,	17 },
	{ "mfc",	2092,	18 },
	/* Cirrus CL GD 5426 -> Picasso, Piccolo, EGS Spectrum */
	{ "grfcl",	2167,	11},	/* PicassoII mem */
	{ "grfcl",	2167,	12},	/* PicassoII regs */
	{ "grfcl",	2167,	21},	/* PicassoIV Z2 mem1 */
	{ "grfcl",	2167,	22},	/* PicassoIV Z2 mem2 */
	{ "grfcl",	2167,	23},	/* PicassoIV Z2 regs */
	{ "grfcl",	2167,	24},	/* PicassoIV Z3 */
	{ "grfcl",	2193,	2},	/* Spectrum mem */
	{ "grfcl",	2193,	1},	/* Spectrum regs */
	{ "grfcl",	2195,	5},	/* Piccolo mem */
	{ "grfcl",	2195,	6},	/* Piccolo regs */
	{ "grfcl",	2195,	10},	/* Piccolo SD64 mem */
	{ "grfcl",	2195,	11},	/* Piccolo SD64 regs */
	/* MacroSystemsUS */
	{ "wesc",	2203,	19},	/* Warp engine */
	/* phase 5 digital products */
	{ "flmem",	8512,	10},	/* FastlaneZ3 memory */
	{ "flsc",	8512,	11},	/* FastlaneZ3 */
	{ "cbsc",	8512,	12},	/* Cyberstorm Mk I SCSI */
	{ "bzivsc",	8512,	17},	/* Blizzard IV SCSI */
	{ "bztzsc", 	8512,	24},	/* Blizzard 2060 SCSI */
	{ "cbiisc", 	8512,	25},	/* Cyberstorm Mk II SCSI */
	{ "grfcv",	8512,	34},	/* CyberVison 64 */
	{ "grfcv3d",	8512,	67},	/* CyberVison 64/3D */
	{ "cbiiisc", 	8512,	100},	/* Cyberstorm Mk III SCSI */
	{ "p5pb", 	8512,	101},	/* CyberVisionPPC / BlizzardVisionPPC */
	{ "bppcsc", 	8512,	110},	/* Blizzard 603e+ SCSI */
	/* Hacker Inc. */
	{ "mlhsc",	2011,	1 },
	/* Resource Management Force */
	{ "qn",		2011,	2 },	/* QuickNet Ethernet */
	/* ??? */
	{ "empsc",	2171,	21 },	/* Emplant SCSI */
	{ "empsc",	2171,	32 },	/* Emplant SCSI */
	/* Tseng ET4000 boards */
	{ "grfet",	2117,	3 },	/* Merlin mem */
	{ "grfet",	2117,	4 },	/* Merlin regs */
	{ "grfet",	2167,	1 },	/* Domino mem */
	{ "grfet",	2167,	2 },	/* Domino regs */
	{ "grfet",	2167,	3 },	/* Domino regs (proto 16M) */
	{ "grfet",	2181,	0 },	/* oMniBus */
	/* Advanced Systems */
	{ "nxsc",	2102,	1 },	/* Nexus SCSI board */
	/* Masoboshi */
	{ "mcsc",	8535,	4 },	/* Masoboshi Mastercard 702 */
	/* Apollo */
	{ "apssc",	8738,	35 },	/* Apollo '060 scsi */
	/* KATO development */
	{ "aumld",	2145,	128 },	/* Melody MPEG layer 2 audio board */
	/* Individual Computers Jens Schoenfeld */
	{ "buddha",	4626,	0 },
	{ "xsurf",	4626,	23 },	/* X-Surf Ethernet */
	/* VMC Harald Frank */
	{ "blst",	5001,	1},	/* ISDN Blaster */
	{ "hyper4",	5001,	2},	/* Hypercom4-Zbus */
	{ "hyper3Z",	5001,	3},	/* Hypercom3-Zbus */
	{ "hyper4+",	5001,	6},	/* Hypercom4+ */
	{ "hyper3+",	5001,	7},	/* Hypercom3+ */
	/* Matay Grzegorz Kraszewski */
	{ "mppb",	44359,	1},	/* Prometheus PCI bridge */
	/* MNTMN */
	{ "mntva",	28014,	1}	/* MNTMN VA2000 */
};
static int naconfent = sizeof(aconftab) / sizeof(struct aconfdata);

/*
 * Anything listed in this table is subject to pre-configuration,
 * if autoconf.c:config_console() calls amiga_config_found() on
 * the Zorro III device.
 */
static struct preconfdata preconftab[] = {
	{18260, 6, 0 },	/* Retina Z2 */			/* grf1 */
	{18260, 16, 0}, /* Retina BLT Z3 */		/* grf2 */
	{18260, 19, 0}, /* Altais */
	{2167,	11, 0},	/* PicassoII mem */		/* grf3 */
	{2167,	12, 0},	/* PicassoII regs */
	{2167,	21, 0},	/* PicassoIV Z2 mem1 */
	{2167,	22, 0},	/* PicassoIV Z2 mem2 */
	{2167,	23, 0},	/* PicassoIV Z2 regs */
	{2167,	24, 0},	/* PicassoIV Z3 */
	{2193,	2, 0},	/* Spectrum mem */
	{2193,	1, 0},	/* Spectrum regs */
	{2195,	5, 0},	/* Piccolo mem */
	{2195,	6, 0},	/* Piccolo regs */
	{2195,	10, 0},	/* Piccolo SD64 mem */
	{2195,	11, 0},	/* Piccolo SD64 regs */
	{1030,	0, 0},	/* Ulwl board */		/* grf4 */
	{8512,	34, 0},	/* Cybervison 64 */		/* grf5 */
	{2117,	3, 0},	/* Merlin mem */		/* grf6 */
	{2117,	4, 0},	/* Merlin regs */
	{2167,	1, 0},	/* Domino mem */
	{2167,	2, 0},	/* Domino regs */
	{2167,	3, 0},	/* Domino regs (proto 16M) */
	{2181,	0, 0},	/* oMniBus mem or regs */
	{8512,	67, 0}	/* Cybervison 64/3D */		/* grf7 */
/*	{28014,	1, 0}	// MNTMN VA2000 */
};
static int npreconfent = sizeof(preconftab) / sizeof(struct preconfdata);

/*
 * Quirks table.
 */
#define ZORRO_QUIRK_NO_ZBUSMAP 1	/* Don't map VA=PA in zbusattach. */
static struct quirksdata quirkstab[] = {
	{8512, 101, ZORRO_QUIRK_NO_ZBUSMAP}
};
static int nquirksent = sizeof(quirkstab) / sizeof(struct quirksdata);

void zbusattach(device_t, device_t, void *);
int zbusprint(void *, const char *);
int zbusmatch(device_t, cfdata_t, void *);
static const char *aconflookup(int, int);

/*
 * given a manufacturer id and product id, find quirks
 * for this board.
 */

static uint8_t
quirkslookup(int mid, int pid)
{
	const struct quirksdata *qdp, *eqdp;

	eqdp = &quirkstab[nquirksent];
	for (qdp = quirkstab; qdp < eqdp; qdp++)
		if (qdp->manid == mid && qdp->prodid == pid) 
			return(qdp->quirks);
	return(0);
}

/*
 * given a manufacturer id and product id, find the name
 * that describes this board.
 */
static const char *
aconflookup(int mid, int pid)
{
	const struct aconfdata *adp, *eadp;

	eadp = &aconftab[naconfent];
	for (adp = aconftab; adp < eadp; adp++)
		if (adp->manid == mid && adp->prodid == pid)
			return(adp->name);
	return("board");
}

/*
 * mainbus driver
 */

CFATTACH_DECL_NEW(zbus, 0,
    zbusmatch, zbusattach, NULL, NULL);

static cfdata_t early_cfdata;

/*ARGSUSED*/
int
zbusmatch(device_t parent, cfdata_t cf, void *aux)
{

	if (matchname(aux, "zbus") == 0)
		return(0);
	if (amiga_realconfig == 0)
		early_cfdata = cf;
	return(1);
}

/*
 * called to attach bus, we probe, i.e., scan configdev structs passed
 * in, for each found name call config_found() which will do this again
 * with that driver if matched else print a diag.
 */
void
zbusattach(device_t parent, device_t self, void *aux)
{
	struct zbus_args za;
	struct preconfdata *pcp, *epcp;
	struct cfdev *cdp, *ecdp;

	epcp = &preconftab[npreconfent];
	ecdp = &cfdev[ncfdev];
	if (amiga_realconfig) {
		if (ZTWOMEMADDR)
			printf(": mem 0x%08lx-0x%08lx",
			    ZTWOMEMADDR,
			    ZTWOMEMADDR + PAGE_SIZE * NZTWOMEMPG - 1);
		if (ZBUSAVAIL)
			printf (": i/o size 0x%08x", ZBUSAVAIL);
		printf("\n");
	}
	for (cdp = cfdev; cdp < ecdp; cdp++) {
		for (pcp = preconftab; pcp < epcp; pcp++) {
			if (pcp->manid == cdp->rom.manid &&
			    pcp->prodid == cdp->rom.prodid)
				break;
		}
		if (amiga_realconfig == 0 && pcp >= epcp)
			continue;

#if NZ3RAMBD > 0
		if (z3rambd_match_id(cdp->rom.manid, cdp->rom.prodid) > 0)
		{ }
		else 
#endif /* NZ3RAMBD */
		/*
		 * check if it's a Zorro II or III board and not linked into
		 * MemList (i.e. not a memory board)
		 */
		switch (cdp->rom.type & (ERT_TYPEMASK | ERTF_MEMLIST)) {
		case ERT_ZORROII:
		case ERT_ZORROIII:
			break;
		default:
			continue;
		}

		za.pa = cdp->addr;
		za.size = cdp->size;
		za.manid = cdp->rom.manid;
		za.prodid = cdp->rom.prodid;
		za.serno = cdp->rom.serno;
		za.slot = (((u_long)za.pa >> 16) & 0xF) - 0x9;

		if (amiga_realconfig && pcp < epcp && pcp->vaddr)
			za.va = pcp->vaddr;
		else {
			if(quirkslookup(za.manid, za.prodid) != 
		 	    ZORRO_QUIRK_NO_ZBUSMAP) 
				za.va = (void *) (isztwopa(za.pa) ? 
				    __UNVOLATILE(ztwomap(za.pa)) :
				    zbusmap(za.pa, za.size));
			/*
			 * save value if early console init
			 */
			if (amiga_realconfig == 0)
				pcp->vaddr = za.va;
		}
		amiga_config_found(early_cfdata, self, &za, zbusprint);
	}
}

/*
 * print configuration info.
 */
int
zbusprint(void *aux, const char *pnp)
{
	struct zbus_args *zap;
	int rv;

	rv = UNCONF;
	zap = aux;

	if (pnp) {
		aprint_normal("%s at %s:",
		    aconflookup(zap->manid, zap->prodid), pnp);
		if (zap->manid == -1)
			rv = UNSUPP;
	}
	aprint_normal(" pa %8p man/pro %d/%d", zap->pa, zap->manid,
	    zap->prodid);
	return(rv);
}

/*
 * this function is used to map Zorro physical I/O addresses into kernel
 * virtual addresses. We don't keep track which address we map where, we don't
 * NEED to know this. We made sure in amiga_init.c (by scanning all available
 * Zorro devices) to have enough kva-space available, so there is no extra
 * range check done here.
 */
void *
zbusmap(void *pa, u_int size)
{
#if defined(__m68k__)
	static vaddr_t nextkva = 0;
	vaddr_t kva;

	if (nextkva == 0)
		nextkva = ZBUSADDR;

	if (nextkva > ZBUSADDR + ZBUSAVAIL)
		return 0;

	/* size better be an integral multiple of the page size... */
	kva = nextkva;
	nextkva += size;
	if (nextkva > ZBUSADDR + ZBUSAVAIL)
		panic("allocating too much Zorro I/O address space");
	physaccess((void *)kva, (void *)pa, size, PG_RW|PG_CI);
	return((void *)kva);
#else
/*
 * XXX we use direct constant mapping
 */
	return(pa);
#endif
}
