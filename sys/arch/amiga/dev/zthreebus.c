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
#include <amiga/dev/zthreebusvar.h>

struct aconfdata {
	char *name;
	int manid;
	int prodid;
};

/* 
 * explain the names.. 0123456789 => zothfisven
 */
struct aconfdata aconf3tab[] = {
	/* MacroSystemsUS */
	{ "wesc",	2203,	19},
};
int naconf3ent = sizeof(aconf3tab) / sizeof(struct aconfdata);

extern caddr_t ZTHREEADDR;
extern u_int ZTHREEAVAIL;

void zthreeattach __P((struct device *, struct device *, void *));
int zthreeprint __P((void *, char *));
int zthreematch __P((struct device *, struct cfdata *,void *));
caddr_t zthreemap __P((caddr_t, u_int));
char *aconf3lookup __P((int, int));

/*
 * given a manufacturer id and product id, find the name
 * that describes this board.
 */
char *
aconf3lookup(mid, pid)
	int mid, pid;
{
	int an;

	for (an = 0; an < naconf3ent; an++) {
		if (aconf3tab[an].manid == mid && aconf3tab[an].prodid == pid)
			return(aconf3tab[an].name);
	}
	return("board");
}

/* 
 * mainbus driver 
 */
struct cfdriver zthreebuscd = {
	NULL, "zthreebus", zthreematch, zthreeattach, 
	DV_DULL, sizeof(struct device), NULL, 0
};

/*ARGSUSED*/
int
zthreematch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{
	if (matchname(auxp, "zthreebus"))
		return(1);
	return(0);
}

/*
 * called to attach bus, we probe, i.e., scan configdev structs passed
 * in, for each found name call config_found() which will do this again
 * with that driver if matched else print a diag.
 */
void
zthreeattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct zthreebus_args za;
	u_long lpa;
	int i, zcnt;

	if (ZTHREEAVAIL)
	  	printf (" I/O size 0x%08x", ZTHREEAVAIL);
#if 0
	if (ZTHREEMEMADDR)
		printf (" mem %08x-%08x\n",
		  ZTHREEMEMADDR, ZTHREEMEMADDR + ZTHREEMEMSIZE - 1);
#endif
	printf("\n");

	for (i = 0; i < ncfdev; i++) {
		za.pa = cfdev[i].addr;
		za.size = cfdev[i].size;
		/*
		 * check that its from zorro III space
		 */
		if ((u_long)za.pa >= ZTHREETOP || (u_long)za.pa < ZTHREEBASE)
			continue;
		za.va = (void *) (iszthreepa(za.pa) ? zthreemap(za.pa, za.size) : 0);
		za.manid = cfdev[i].rom.manid;
		za.prodid = cfdev[i].rom.prodid;
		za.serno = cfdev[i].rom.serno;
		za.slot = (((u_long)za.pa >> 16) & 0xF) - 0x9;
		config_found(dp, &za, zthreeprint);
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
		printf("%s at %s", aconf3lookup(zap->manid, zap->prodid),
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
	static caddr_t nextkva = 0;
	caddr_t kva;

	if (! nextkva)
		nextkva = ZTHREEADDR;

	if (nextkva > ZTHREEADDR + ZTHREEAVAIL)
		return 0;

	/* size better be an integral multiple of the page size... */
	kva = nextkva;
	nextkva += size;
	if (nextkva > ZTHREEADDR + ZTHREEAVAIL)
		panic("allocating too much Zorro III address space");
	physaccess  (kva, pa, size, PG_RW|PG_CI);
	return kva;
}
