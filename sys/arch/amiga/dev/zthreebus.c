/*
 * Copyright (c) 1994 Michael L. Hitch
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
 *	$Id: zthreebus.c,v 1.0
 */
#include <sys/param.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <amiga/amiga/cfdev.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/zthreebusvar.h>

struct aconfdata {
	char *name;
	int manid;
	int prodid;
};

struct preconfdata {
	int manid;
	int prodid;
	caddr_t vaddr;
};


/* 
 * explain the names.. 0123456789 => zothfisven
 */
static struct aconfdata aconftab[] = {
	/* MacroSystemsUS */
	{ "wesc",	2203,	19},	/* Warp engine */
	{ "grfrtblt",	18260,	16},	/* Retina BLT Z3 */
	/* Commodore Amiga */
	{ "afsc",	514,	84}	/* A4091 SCSI HD Controller */
};
static int naconfent = sizeof(aconftab) / sizeof(struct aconfdata);

/*
 * Anything listed in this table is subject to pre-configuration,
 * if autoconf.c:config_console() calls amiga_config_found() on
 * the Zorro III device.
 */
static struct preconfdata preconftab[] = {
	/* Retina BLT Z3 */
	{ 18260, 16, 0 }
};
static int npreconfent = sizeof(preconftab) / sizeof(struct preconfdata);

void zthreeattach __P((struct device *, struct device *, void *));
int zthreeprint __P((void *, char *));
int zthreematch __P((struct device *, struct cfdata *,void *));
caddr_t zthreemap __P((caddr_t, u_int));
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
 * zorro three bus driver 
 */
struct cfdriver zthreebuscd = {
	NULL, "zthreebus", zthreematch, zthreeattach, 
	DV_DULL, sizeof(struct device), NULL, 0
};

static struct cfdata *early_cfdata;

/*ARGSUSED*/
int
zthreematch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{
	if (matchname(auxp, "zthreebus") == 0)
		return(0);
	if (amiga_realconfig == 0)
		early_cfdata = cfp;
	return(1);
}

/*
 * called to attach bus, we probe, i.e., scan configdev structs passed
 * in, for each found name call config_found() which will do this again
 * with that driver if matched else print a diag.
 *
 * If called during config_console() (i.e., amiga_realconfig == 0), skip
 * everything but the approved devices in preconftab, and when doing
 * those devices save the allocated virtual address.
 */
void
zthreeattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct zthreebus_args za;
	struct preconfdata *pcp, *epcp;
	struct cfdev *cdp, *ecdp;

	epcp = &preconftab[npreconfent];
	ecdp = &cfdev[ncfdev];

	if (amiga_realconfig) {
		if (ZTHREEAVAIL)
			printf(": i/o size 0x%08x", ZTHREEAVAIL);
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

		za.pa = cdp->addr;
		za.size = cdp->size;

		if (amiga_realconfig && pcp < epcp && pcp->vaddr)
			za.va = pcp->vaddr;
		else {
			/*
			 * check that its from zorro III space
			 * (board type = Zorro III and not memory)
			 */
			if ((cdp->rom.type & 0xe0) != 0x80)
				continue;
			za.va = (void *)(iszthreepa(za.pa) ?
			    zthreemap(za.pa, za.size) : 0);

			/*
			 * save value if early console init 
			 */
			if (amiga_realconfig == 0)
				pcp->vaddr = za.va;
		}
		za.manid = cdp->rom.manid;
		za.prodid = cdp->rom.prodid;
		za.serno = cdp->rom.serno;
		za.slot = (((u_long)za.pa >> 16) & 0xF) - 0x9;
		amiga_config_found(early_cfdata, dp, &za, zthreeprint);
	}
}

/*
 * print configuration info.
 */
int
zthreeprint(auxp, pnp)
	void *auxp;
	char *pnp;
{
	struct zthreebus_args *zap;
	int rv;

	rv = UNCONF;
	zap = auxp;

	if (pnp) {
		printf("%s at %s", aconflookup(zap->manid, zap->prodid),
		    pnp);
		if (zap->manid == -1)
			rv = UNSUPP;
	}
	printf(" rom 0x%x man/pro %d/%d", zap->pa, zap->manid, zap->prodid);
	return(rv);
}

/*
 * this function is used to map Z3 physical addresses into kernel virtual
 * addresses. We don't keep track which address we map where, we don't NEED
 * to know this. We made sure in amiga_init.c (by scanning all available Z3
 * devices) to have enough kva-space available, so there is no extra range
 * check done here.
 */
caddr_t
zthreemap (pa, size)
	caddr_t pa;
	u_int size;
{
	static vm_offset_t nextkva = 0;
	vm_offset_t kva;

	if (nextkva == 0)
		nextkva = ZTHREEADDR;

	if (nextkva > ZTHREEADDR + ZTHREEAVAIL)
		return 0;

	/* size better be an integral multiple of the page size... */
	kva = nextkva;
	nextkva += size;
	if (nextkva > ZTHREEADDR + ZTHREEAVAIL)
		panic("allocating too much Zorro III address space");
	physaccess((caddr_t)kva, (caddr_t)pa, size, PG_RW|PG_CI);
	return((caddr_t)kva);
}
