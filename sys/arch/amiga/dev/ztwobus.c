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
 *
 *	$Id: ztwobus.c,v 1.9 1994/07/16 02:26:30 chopps Exp $
 */
#include <sys/param.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <amiga/amiga/cfdev.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/ztwobusvar.h>

struct aconfdata {
	char *name;
	int manid;
	int prodid;
};

struct preconfdata {
	int manid;
	int prodid;
};


/* 
 * explian the names.. 0123456789 => zothfisven
 */
static struct aconfdata aconftab[] = {
	/* Commodore Amiga */
	{ "atzee",	513,	1 },
	{ "atzsc",	514,	3 },
	{ "le",		514,	112 },
	{ "ql",		514,	69 },
	{ "ql",		514,	70 },
	/* Ameristart */
	{ "le",		1053,	1 },
	/* Univeristy lowell */
	{ "ulwl",	1030,	0 },
	/* Macorsystems */
	{ "grfrt",	18260,	6 },
	/* Greater valley products */
	{ "gvpbus",	2017,	2 },
	{ "gvpbus",	2017,	11 },
	{ "giv",	2017,	32 },
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
	/* IVS */
	{ "itrmp",	2112,	52 },
	{ "ivasc",	2112,	242 },
	{ "ivsc",	2112,	243 },
	/* Hydra */
	{ "ed",		2121,	1 },
	/* ASDG */
	{ "ed",		9999,	9 },		/* XXXX */
	/* Hacker Inc. */
	{ "mlhsc",	2011,	1 },
};
static int naconfent = sizeof(aconftab) / sizeof(struct aconfdata);

/*
 * Anything listed in this table is subject to pre-configuration,
 * if autoconf.c:config_console() calls amiga_config_found() on
 * the Zorro III device.
 */
static struct preconfdata preconftab[] = {
	/* Retina BLT Z3 */
	{ 18260, 6 }
};
static int npreconfent = sizeof(preconftab) / sizeof(struct preconfdata);


void ztwoattach __P((struct device *, struct device *, void *));
int ztwoprint __P((void *, char *));
int ztwomatch __P((struct device *, struct cfdata *,void *));
static char *aconflookup __P((int, int));

/*
 * given a manufacturer id and product id, find the name
 * that describes this board.
 */
static char *
aconflookup(mid, pid)
	int mid, pid;
{
	struct aconfdata *adp, *eadp;

	eadp = &aconftab[naconfent];
	for (adp = aconftab; adp < eadp; adp++)
		if (adp->manid == mid && adp->prodid == pid)
			return(adp->name);
	return("board");
}

/* 
 * mainbus driver 
 */
struct cfdriver ztwobuscd = {
	NULL, "ztwobus", ztwomatch, ztwoattach, 
	DV_DULL, sizeof(struct device), NULL, 0
};

static struct cfdata *early_cfdata;

/*ARGSUSED*/
int
ztwomatch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{
	if (matchname(auxp, "ztwobus") == 0)
		return(0);
	if (amiga_realconfig == 0)
		early_cfdata = cfp;
	return(1);
}

/*
 * called to attach bus, we probe, i.e., scan configdev structs passed
 * in, for each found name call config_found() which will do this again
 * with that driver if matched else print a diag.
 */
void
ztwoattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct ztwobus_args za;
	struct preconfdata *pcp, *epcp;
	struct cfdev *cdp, *ecdp;

	epcp = &preconftab[npreconfent];
	ecdp = &cfdev[ncfdev];
	if (amiga_realconfig) {
		if (ZTWOMEMADDR)
			printf(": mem 0x%08x-0x%08x",
			    ZTWOMEMADDR, ZTWOMEMADDR + NBPG * NZTWOMEMPG - 1);
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

		/*
		 * check if it's a Zorro II board and not linked into
		 * MemList (i.e. not a memory board)
		 */
		if ((cdp->rom.type & 0xe0) != 0xc0)
			continue;	/* er_Type != ZorroII I/O */
		za.pa = cdp->addr;
		za.va = (void *) (isztwopa(za.pa) ? ztwomap(za.pa) : 0);
		za.size = cdp->size;
		za.manid = cdp->rom.manid;
		za.prodid = cdp->rom.prodid;
		za.serno = cdp->rom.serno;
		za.slot = (((u_long)za.pa >> 16) & 0xF) - 0x9;
		amiga_config_found(early_cfdata, dp, &za, ztwoprint);
	}
}

/*
 * print configuration info.
 */
int
ztwoprint(auxp, pnp)
	void *auxp;
	char *pnp;
{
	struct ztwobus_args *zap;
	int rv;

	rv = UNCONF;
	zap = auxp;

	if (pnp) {
		printf("%s at %s:", aconflookup(zap->manid, zap->prodid),
		    pnp);
		if (zap->manid == -1)
			rv = UNSUPP;
	}
	printf(" rom 0x%x man/pro %d/%d", zap->pa, zap->manid, zap->prodid);
	return(rv);
}
