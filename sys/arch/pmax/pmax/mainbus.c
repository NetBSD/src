/*	$NetBSD: mainbus.c,v 1.5 1996/01/29 22:52:34 jonathan Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * DECstation port: Jonathan Stone
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/autoconf.h>
/*#include <machine/rpb.h>*/
#include "pmaxtype.h"
#include "machine/machConst.h"
#include "nameglue.h"
#include "kn01.h"

struct mainbus_softc {
	struct	device sc_dv;
};

/* Definition of the mainbus driver. */
static int	mbmatch __P((struct device *, void *, void *));
static void	mbattach __P((struct device *, struct device *, void *));
static int	mbprint __P((void *, char *));
struct cfdriver mainbuscd =
    { NULL, "mainbus", mbmatch, mbattach, DV_DULL,
	sizeof (struct mainbus_softc) };
void	mb_intr_establish __P((struct confargs *ca,
			       int (*handler)(intr_arg_t),
			       intr_arg_t val ));
void	mb_intr_disestablish __P((struct confargs *));

/* KN01 has devices directly on the system bus */
void	kn01_intr_establish __P((struct confargs *ca,
			       int (*handler)(intr_arg_t),
			       intr_arg_t val ));
void	kn01_intr_disestablish __P((struct confargs *));
static void	kn01_attach __P((struct device *, struct device *, void *));


static int
mbmatch(parent, cfdata, aux)
	struct device *parent;
	void *cfdata;
	void *aux;
{
	struct cfdata *cf = cfdata;

	/*
	 * Only one mainbus, but some people are stupid...
	 */	
	if (cf->cf_unit > 0)
		return(0);

	/*
	 * That one mainbus is always here.
	 */
	return(1);
}

int ncpus = 0;	/* only support uniprocessors, for now */

static void
mbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_softc *sc = (struct mainbus_softc *)self;
	struct confargs nca;
	struct pcs *pcsp;
	int i, cpuattachcnt;
	extern int cputype, ncpus;

	printf("\n");

	/*
	 * Try to find and attach all of the CPUs in the machine.
	 */
	cpuattachcnt = 0;

#ifdef notyet	/* alpha code */
	for (i = 0; i < hwrpb->rpb_pcs_cnt; i++) {
		struct pcs *pcsp;

		pcsp = (struct pcs *)((char *)hwrpb + hwrpb->rpb_pcs_off +
		    (i * hwrpb->rpb_pcs_size));
		if ((pcsp->pcs_flags & PCS_PP) == 0)
			continue;

		nca.ca_name = "cpu";
		nca.ca_slot = 0;
		nca.ca_offset = 0;
		nca.ca_addr = 0;
		if (config_found(self, &nca, mbprint))
			cpuattachcnt++;
	}
#endif

	if (ncpus != cpuattachcnt)
		printf("WARNING: %d cpus in machine, %d attached\n",
			ncpus, cpuattachcnt);

#if	defined(DS_5000) || defined(DS5000_240) || defined(DS_5000_100) || \
	defined(DS_5000_25)

	if (cputype == DS_3MAXPLUS ||
	    cputype == DS_3MAX ||
	    cputype == DS_3MIN ||
	    cputype == DS_MAXINE) {

	    if (cputype == DS_3MIN || cputype == DS_MAXINE)
		printf("UNTESTED autoconfiguration!!\n");	/*XXX*/

#if 0
		/* we have a TurboChannel bus! */
		strcpy(nca.ca_name, "tc");
		nca.ca_slot = 0;
		nca.ca_offset = 0;
		
		config_found(self, &nca, mbprint);
#else
		config_tcbus(self, &nca, mbprint);
#endif

	}
#endif /*Turbochannel*/

	/*
	 * We haven't yet decided how to handle the PMAX (KN01)
	 * which really only has a mainbus, baseboard devices, and an
	 * optional framebuffer.
	 */
#if defined(DS3100)
	/* XXX mipsfair: just a guess */
	if (cputype == DS_PMAX || cputype == DS_MIPSFAIR) {
		kn01_attach(parent, self, aux);
	}
#endif /*DS3100*/

}


#define KN01_MAXDEVS 5
struct confargs kn01_devs[KN01_MAXDEVS] = {
	/*   name       slot  offset 		   addr intpri  */
	{ "pm",		0,   0,  (u_int)KV(KN01_PHYS_FBUF_START), 3,  },
	{ "dc",  	1,   0,  (u_int)KV(KN01_SYS_DZ),	  2,  },
	{ "lance", 	2,   0,  (u_int)KV(KN01_SYS_LANCE),       1,  },
	{ "sii",	3,   0,  (u_int)KV(KN01_SYS_SII),	  0,  },
	{ "mc146818",	4,   0,  (u_int)KV(KN01_SYS_CLOCK),       16, }
};

/*
 * Configure baseboard devices on KN01 attached directly to mainbus 
 */
void
kn01_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_softc *sc = (struct mainbus_softc *)self;
	struct confargs *nca;
	register int i;

	/* Try to configure each KN01 mainbus device */
	for (i = 0; i < KN01_MAXDEVS; i++) {

		nca = &kn01_devs[i];
		if (nca == NULL) {
			printf("mbattach: bad config for slot %d\n", i);
			break;
		}

#if defined(DIAGNOSTIC) || defined(DEBUG)
		if (nca->ca_slot > KN01_MAXDEVS)
			panic("kn01 mbattach: \
dev slot > number of slots for %s",
			    nca->ca_name);
#endif

		if (nca->ca_name == NULL) {
			panic("No name for mainbus device\n");
		}

		/* Tell the autoconfig machinery we've found the hardware. */
		config_found(self, nca, mbprint);
	}

	/*
	 * The Decstation 5100 has an sii, clock, ethernet, and dc,
	 * like  the 3100 and presumably at the same address. The
	 * 5100 also has a slot for PrestoServe NVRAM and for
	 * an additional `mbc' dc-like serial option.
	 * If we supported those devices, we would attempt to configure
	 * them now.
	 */

}

static int
mbprint(aux, pnp)
	void *aux;
	char *pnp;
{

	if (pnp)
		return (QUIET);
	return (UNCONF);
}

void
mb_intr_establish(ca, handler, val)
	struct confargs *ca;
	int (*handler) __P((intr_arg_t));
	intr_arg_t val;
{

	panic("can never mb_intr_establish");
}

void
mb_intr_disestablish(ca)
	struct confargs *ca;
{

	panic("can never mb_intr_disestablish");
}

void
kn01_intr_establish(ca, handler, val)
	struct confargs *ca;
	int (*handler) __P((intr_arg_t));
	intr_arg_t val;
{
	/* Interrupts on the KN01 are currently hardcoded. */
	printf(" (kn01: intr_establish hardcoded) ");
}

void
kn01_intr_disestablish(ca)
	struct confargs *ca;
{
	printf("(kn01: ignoring intr_disestablish) ");
}

/*
 * An  interrupt-establish method.  This should somehow be folded
 * back into the autoconfiguration machinery. Until the TC machine
 * portmasters agree on how to do that, it's a separate function
 */
void
generic_intr_establish(ca, handler, arg)
	struct confargs *ca;
	intr_handler_t handler;
	intr_arg_t arg;
{
	struct device *dev = arg;

	extern struct cfdriver ioasiccd, tccd;

	if (dev->dv_parent->dv_cfdata->cf_driver == &ioasiccd) {
		/*XXX*/ printf("ioasic interrupt for %d\n", ca->ca_slotpri);
		asic_intr_establish(ca, handler, arg);
	} else
	if (dev->dv_parent->dv_cfdata->cf_driver == &tccd) {
		tc_intr_establish(dev->dv_parent, ca->ca_slotpri, 0, handler, arg);
	} else
	if (dev->dv_parent->dv_cfdata->cf_driver == &mainbuscd) {
		kn01_intr_establish(ca, handler, arg);
	}
	else {
		printf("intr_establish: unknown parent bustype for %s\n",
			dev->dv_xname);
	}
}
