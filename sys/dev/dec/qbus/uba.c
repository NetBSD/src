/*	$NetBSD: uba.c,v 1.45 1999/05/27 03:45:21 ragge Exp $	   */
/*
 * Copyright (c) 1996 Jonathan Stone.
 * Copyright (c) 1994, 1996 Ludd, University of Lule}, Sweden.
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)uba.c	7.10 (Berkeley) 12/16/90
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/dkstat.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/bus.h>
#include <machine/scb.h>
#include <machine/cpu.h>

#ifdef __vax__
#include <machine/pte.h>
#endif

#include <dev/dec/qbus/ubareg.h>
#include <dev/dec/qbus/ubavar.h>

static	int ubasearch __P((struct device *, struct cfdata *, void *));
static	int ubaprint __P((void *, const char *));
static	void ubainitmaps __P((struct uba_softc *));

extern struct cfdriver uba_cd;

#define spluba	spl7

/*
 * Do transfer on device argument.  The controller
 * and uba involved are implied by the device.
 * We queue for resource wait in the uba code if necessary.
 * We return 1 if the transfer was started, 0 if it was not.
 *
 * The onq argument must be zero iff the device is not on the
 * queue for this UBA.	If onq is set, the device must be at the
 * head of the queue.  In any case, if the transfer is started,
 * the device will be off the queue, and if not, it will be on.
 *
 * Drivers that allocate one BDP and hold it for some time should
 * set ud_keepbdp.  In this case um_bdp tells which BDP is allocated
 * to the controller, unless it is zero, indicating that the controller
 * does not now have a BDP.
 */
int
ubaqueue(uu, bp)
	register struct uba_unit *uu;
	struct buf *bp;
{
	register struct uba_softc *uh;
	register int s;

	uh = (void *)((struct device *)(uu->uu_softc))->dv_parent;
	s = spluba();
	/*
	 * Honor exclusive BDP use requests.
	 */
	if ((uu->uu_xclu && uh->uh_users > 0) || uh->uh_xclu)
		goto rwait;
	if (uu->uu_keepbdp) {
		/*
		 * First get just a BDP (though in fact it comes with
		 * one map register too).
		 */
		if (uu->uu_bdp == 0) {
			uu->uu_bdp = uballoc(uh, (caddr_t)0, 0,
			    UBA_NEEDBDP|UBA_CANTWAIT);
			if (uu->uu_bdp == 0)
				goto rwait;
		}
		/* now share it with this transfer */
		uu->uu_ubinfo = ubasetup(uh, bp,
		    uu->uu_bdp|UBA_HAVEBDP|UBA_CANTWAIT);
	} else
		uu->uu_ubinfo = ubasetup(uh, bp, UBA_NEEDBDP|UBA_CANTWAIT);
	if (uu->uu_ubinfo == 0)
		goto rwait;
	uh->uh_users++;
	if (uu->uu_xclu)
		uh->uh_xclu = 1;

	splx(s);
	return (1);

rwait:
	SIMPLEQ_INSERT_TAIL(&uh->uh_resq, uu, uu_resq);
	splx(s);
	return (0);
}

void
ubadone(uu)
	struct uba_unit *uu;
{
	struct uba_softc *uh = (void *)((struct device *)
	    (uu->uu_softc))->dv_parent;

	if (uu->uu_xclu)
		uh->uh_xclu = 0;
	uh->uh_users--;
	if (uu->uu_keepbdp)
		uu->uu_ubinfo &= ~BDPMASK;	/* keep BDP for misers */
	ubarelse(uh, &uu->uu_ubinfo);
}

/*
 * Allocate and setup UBA map registers, and bdp's
 * Flags says whether bdp is needed, whether the caller can't
 * wait (e.g. if the caller is at interrupt level).
 * Return value encodes map register plus page offset,
 * bdp number and number of map registers.
 */
int
ubasetup(uh, bp, flags)
	struct	uba_softc *uh;
	struct	buf *bp;
	int	flags;
{
	int npf;
	int temp;
	int reg, bdp;
	int a, o, ubinfo;

	if (uh->uh_nbdp == 0)
		flags &= ~UBA_NEEDBDP;

	o = (int)bp->b_un.b_addr & VAX_PGOFSET;
	npf = vax_btoc(bp->b_bcount + o) + 1;
	if (npf > UBA_MAXNMR)
		panic("uba xfer too big");
	a = spluba();
	while ((reg = rmalloc(uh->uh_map, (long)npf)) == 0) {
		if (flags & UBA_CANTWAIT) {
			splx(a);
			return (0);
		}
		uh->uh_mrwant++;
		sleep((caddr_t)&uh->uh_mrwant, PSWP);
	}
	if ((flags & UBA_NEED16) && reg + npf > 128) {
		/*
		 * Could hang around and try again (if we can ever succeed).
		 * Won't help any current device...
		 */
		rmfree(uh->uh_map, (long)npf, (long)reg);
		splx(a);
		return (0);
	}
	bdp = 0;
	if (flags & UBA_NEEDBDP) {
		while ((bdp = ffs((long)uh->uh_bdpfree)) == 0) {
			if (flags & UBA_CANTWAIT) {
				rmfree(uh->uh_map, (long)npf, (long)reg);
				splx(a);
				return (0);
			}
			uh->uh_bdpwant++;
			sleep((caddr_t)&uh->uh_bdpwant, PSWP);
		}
		uh->uh_bdpfree &= ~(1 << (bdp-1));
	} else if (flags & UBA_HAVEBDP)
		bdp = (flags >> 28) & 0xf;
	splx(a);
	reg--;
	ubinfo = UBAI_INFO(o, reg, npf, bdp);
	temp = (bdp << 21) | UBAMR_MRV;
	if (bdp && (o & 01))
		temp |= UBAMR_BO;

	disk_reallymapin(bp, uh->uh_mr, reg, temp | PG_V);

	return (ubinfo);
}

/*
 * Non buffer setup interface... set up a buffer and call ubasetup.
 */
int
uballoc(uh, addr, bcnt, flags)
	struct	uba_softc *uh;
	caddr_t addr;
	int	bcnt, flags;
{
	struct buf ubabuf;

	ubabuf.b_un.b_addr = addr;
	ubabuf.b_flags = B_BUSY;
	ubabuf.b_bcount = bcnt;
	/* that's all the fields ubasetup() needs */
	return (ubasetup(uh, &ubabuf, flags));
}
 
/*
 * Release resources on uba uban, and then unblock resource waiters.
 * The map register parameter is by value since we need to block
 * against uba resets on 11/780's.
 */
void
ubarelse(uh, amr)
	struct	uba_softc *uh;
	int	*amr;
{
	struct uba_unit *uu;
	register int bdp, reg, npf, s;
	int mr;
 
	/*
	 * Carefully see if we should release the space, since
	 * it may be released asynchronously at uba reset time.
	 */
	s = spluba();
	mr = *amr;
	if (mr == 0) {
		/*
		 * A ubareset() occurred before we got around
		 * to releasing the space... no need to bother.
		 */
		splx(s);
		return;
	}
	*amr = 0;
	bdp = UBAI_BDP(mr);
	if (bdp) {
		if (uh->uh_ubapurge)
			(*uh->uh_ubapurge)(uh, bdp);

		uh->uh_bdpfree |= 1 << (bdp-1);		/* atomic */
		if (uh->uh_bdpwant) {
			uh->uh_bdpwant = 0;
			wakeup((caddr_t)&uh->uh_bdpwant);
		}
	}
	/*
	 * Put back the registers in the resource map.
	 * The map code must not be reentered,
	 * nor can the registers be freed twice.
	 * Unblock interrupts once this is done.
	 */
	npf = UBAI_NMR(mr);
	reg = UBAI_MR(mr) + 1;
	rmfree(uh->uh_map, (long)npf, (long)reg);
	splx(s);

	/*
	 * Wakeup sleepers for map registers,
	 * and also, if there are processes blocked in dgo(),
	 * give them a chance at the UNIBUS.
	 */
	if (uh->uh_mrwant) {
		uh->uh_mrwant = 0;
		wakeup((caddr_t)&uh->uh_mrwant);
	}
	while ((uu = uh->uh_resq.sqh_first)) {
		SIMPLEQ_REMOVE_HEAD(&uh->uh_resq, uu, uu_resq);
		if ((*uu->uu_ready)(uu) == 0)
			break;
	}
}

void
ubainitmaps(uhp)
	register struct uba_softc *uhp;
{

	if (uhp->uh_memsize > UBA_MAXMR)
		uhp->uh_memsize = UBA_MAXMR;
	rminit(uhp->uh_map, (long)uhp->uh_memsize, (long)1, "uba", UAMSIZ);
	uhp->uh_bdpfree = (1 << uhp->uh_nbdp) - 1;
}

/*
 * Generate a reset on uba number uban.	 Then
 * call each device that asked to be called during attach,
 * giving it a chance to clean up so as to be able to continue.
 */
void
ubareset(uban)
	int uban;
{
	register struct uba_softc *uh = uba_cd.cd_devs[uban];
	int s, i;

	s = spluba();
	uh->uh_users = 0;
	uh->uh_zvcnt = 0;
	uh->uh_xclu = 0;
	SIMPLEQ_INIT(&uh->uh_resq);
	uh->uh_bdpwant = 0;
	uh->uh_mrwant = 0;
	ubainitmaps(uh);
	wakeup((caddr_t)&uh->uh_bdpwant);
	wakeup((caddr_t)&uh->uh_mrwant);
	printf("%s: reset", uh->uh_dev.dv_xname);
	(*uh->uh_ubainit)(uh);

	for (i = 0; i < uh->uh_resno; i++)
		(*uh->uh_reset[i])(uh->uh_resarg[i]);
	printf("\n");
	splx(s);
}

/*
 * The common attach routines:
 *   Allocates interrupt vectors.
 *   Puts correct values in uba_softc.
 *   Calls the scan routine to search for uba devices.
 */
void
uba_attach(sc, iopagephys)
	struct uba_softc *sc;
	paddr_t iopagephys;
{

	/*
	 * Set last free interrupt vector for devices with
	 * programmable interrupt vectors.  Use is to decrement
	 * this number and use result as interrupt vector.
	 */
	sc->uh_lastiv = 0x200;
	SIMPLEQ_INIT(&sc->uh_resq);

	/*
	 * Allocate place for unibus I/O space in virtual space.
	 */
	if (bus_space_map(sc->uh_iot, iopagephys, UBAIOSIZE, 0, &sc->uh_ioh))
		return;
	/*
	 * Initialize the UNIBUS, by freeing the map
	 * registers and the buffered data path registers
	 */
	sc->uh_map = (struct map *)malloc((u_long)
	    (UAMSIZ * sizeof(struct map)), M_DEVBUF, M_NOWAIT);
	bzero((caddr_t)sc->uh_map, (unsigned)(UAMSIZ * sizeof (struct map)));
	ubainitmaps(sc);

	if (sc->uh_beforescan)
		(*sc->uh_beforescan)(sc);
	/*
	 * Now start searching for devices.
	 */
	config_search(ubasearch,(struct device *)sc, NULL);

	if (sc->uh_afterscan)
		(*sc->uh_afterscan)(sc);
}

int
ubasearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	uba_softc *sc = (struct uba_softc *)parent;
	struct	uba_attach_args ua;
	int	i, vec, br;

	ua.ua_ioh = ubdevreg(cf->cf_loc[0]) + sc->uh_ioh;
	ua.ua_iot = sc->uh_iot;
	ua.ua_reset = NULL;

	if (badaddr((caddr_t)ua.ua_ioh, 2) ||
	    (sc->uh_errchk ? (*sc->uh_errchk)(sc):0))
		goto forgetit;

	scb_vecref(0, 0); /* Clear vector ref */
	i = (*cf->cf_attach->ca_match) (parent, cf, &ua);

	if (sc->uh_errchk)
		if ((*sc->uh_errchk)(sc))
			goto forgetit;
	if (i == 0)
		goto forgetit;

	i = scb_vecref(&vec, &br);
	if (i == 0)
		goto fail;
	if (vec == 0)
		goto fail;
		
	scb_vecalloc(vec, ua.ua_ivec, cf->cf_unit, SCB_ISTACK);
	if (ua.ua_reset) { /* device wants ubareset */
		if (sc->uh_resno == 0) {
			sc->uh_reset = malloc(sizeof(ua.ua_reset),
			    M_DEVBUF, M_NOWAIT);
			sc->uh_resarg = (int *)sc->uh_reset + 128;
		}
		if (sc->uh_resno > 127) {
			printf("%s: Expand reset table, skipping reset %s%d\n",
			    sc->uh_dev.dv_xname, cf->cf_driver->cd_name,
			    cf->cf_unit);
		} else {
			sc->uh_resarg[sc->uh_resno] = cf->cf_unit;
			sc->uh_reset[sc->uh_resno++] = ua.ua_reset;
		}
	}
	ua.ua_br = br;
	ua.ua_cvec = vec;
	ua.ua_iaddr = cf->cf_loc[0];

	config_attach(parent, cf, &ua, ubaprint);
	return 0;

fail:
	printf("%s%d at %s csr %o %s\n",
	    cf->cf_driver->cd_name, cf->cf_unit, parent->dv_xname,
	    cf->cf_loc[0], (i ? "zero vector" : "didn't interrupt"));

forgetit:
	return 0;
}

/*
 * Print out some interesting info common to all unibus devices.
 */
int
ubaprint(aux, uba)
	void *aux;
	const char *uba;
{
	struct uba_attach_args *ua = aux;

	printf(" csr %o vec %o ipl %x", ua->ua_iaddr,
	    ua->ua_cvec & 511, ua->ua_br);
	return UNCONF;
}
