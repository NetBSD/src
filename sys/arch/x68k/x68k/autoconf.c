/*	$NetBSD: autoconf.c,v 1.10 1997/03/26 22:39:25 gwr Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman
 * Copyright (c) 1994 Christian E. Hopps
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/malloc.h>
#include <machine/cpu.h>

void configure __P((void));
static void findroot __P((struct device **, int *));
void mbattach __P((struct device *, struct device *, void *));
int mbprint __P((void *, const char *));
int mbmatch __P((struct device *, void *, void *));

extern int cold;	/* 1 if still booting (locore.s) */
int x68k_realconfig;
#include <sys/kernel.h>

struct devnametobdevmaj x68k_nam2blk[] = {
	{ "fd",		2 },
	{ "sd",		4 },
	{ "md",		8 },
	{ NULL,		0 },
};

/*
 * called at boot time, configure all devices on system
 */
void
configure()
{
	extern int x68k_realconfig;
	
	x68k_realconfig = 1;

	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	cold = 0;
}

void
cpu_rootconf()
{
	struct device *booted_device;
	int booted_partition;

	findroot(&booted_device, &booted_partition);

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition, x68k_nam2blk);
}

/*ARGSUSED*/
int
simple_devprint(auxp, pnp)
	void *auxp;
	const char *pnp;
{
	return(QUIET);
}

/*
 * use config_search to find appropriate device, then call that device
 * directly with NULL device variable storage.  A device can then 
 * always tell the difference between the real and console init 
 * by checking for NULL.
 */
int
x68k_config_found(pcfp, pdp, auxp, pfn)
	struct cfdata *pcfp;
	struct device *pdp;
	void *auxp;
	cfprint_t pfn;
{
	struct device temp;
	struct cfdata *cf;
	extern int x68k_realconfig;

	if (x68k_realconfig)
		return(config_found(pdp, auxp, pfn) != NULL);

	if (pdp == NULL)
		pdp = &temp;

	pdp->dv_cfdata = pcfp;
	if ((cf = config_search((cfmatch_t)NULL, pdp, auxp)) != NULL) {
		cf->cf_attach->ca_attach(pdp, NULL, auxp);
		pdp->dv_cfdata = NULL;
		return(1);
	}
	pdp->dv_cfdata = NULL;
	return(0);
}

/*
 * this function needs to get enough configured to do a console
 * basically this means start attaching the grfxx's that support 
 * the console. Kinda hacky but it works.
 */
void
config_console()
{	
	struct cfdata *cf;

	/*
	 * we need mainbus' cfdata.
	 */
	cf = config_rootsearch(NULL, "mainbus", "mainbus");
	if (cf == NULL)
		panic("no mainbus");
	x68k_config_found(cf, NULL, "grfbus", NULL);
}

dev_t	bootdev = 0;

static void
findroot(devpp, partp)
	struct device **devpp;
	int *partp;
{
	int i, majdev, unit, part;
	struct device *dv;
	char buf[32];

	/*
	 * Default to "not found".
	 */
	*devpp = NULL;
	*partp = 0;

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;

	majdev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
	for (i = 0; x68k_nam2blk[i].d_name != NULL; i++)
		if (majdev == x68k_nam2blk[i].d_maj)
			break;
	if (x68k_nam2blk[i].d_name == NULL)
		return;

	part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;

	sprintf(buf, "%s%d", x68k_nam2blk[i].d_name, unit);
	for (dv = alldevs.tqh_first; dv != NULL;
	    dv = dv->dv_list.tqe_next) {
		if (strcmp(buf, dv->dv_xname) == 0) {
			*devpp = dv;
			*partp = part;
			return;
		}
	}
}

/* 
 * mainbus driver 
 */
struct cfattach mainbus_ca = {
	sizeof(struct device), mbmatch, mbattach
};

struct cfdriver mainbus_cd = {
	NULL, "mainbus", DV_DULL, NULL, 0
};

int
mbmatch(pdp, match, auxp)
	struct device *pdp;
	void *match, *auxp;
{
	struct cfdata *cfp = match;

	if (cfp->cf_unit > 0)
		return(0);
	/*
	 * We are always here
	 */
	return(1);
}

/*
 * "find" all the things that should be there.
 */
void
mbattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	printf ("\n");
	config_found(dp, "clock"  , simple_devprint);
	config_found(dp, "grfbus" , simple_devprint);
	config_found(dp, "par"    , simple_devprint);
	config_found(dp, "fdc"    , simple_devprint);
	config_found(dp, "ed"     , simple_devprint);
	config_found(dp, "zs"     , simple_devprint);
	config_found(dp, "com"    , simple_devprint);
	config_found(dp, "com"    , simple_devprint);
	config_found(dp, "spc"    , simple_devprint);
	config_found(dp, "adpcm"  , simple_devprint);
	config_found(dp, "*"      , simple_devprint);
}

int
mbprint(auxp, pnp)
	void *auxp;
	const char *pnp;
{
	if (pnp)
		printf("%s at %s", (char *)auxp, pnp);
	return(UNCONF);
}
