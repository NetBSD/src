/*	$NetBSD: uda.c,v 1.19 1996/07/20 19:00:22 ragge Exp $	*/
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
 *	@(#)uda.c	7.32 (Berkeley) 2/13/91
 */

/*
 * UDA50 disk device driver
 */

#include <sys/param.h>
#include <sys/kernel.h>

#include <machine/sid.h>
#include <machine/pte.h>

#include <vax/uba/ubavar.h>
#include <vax/uba/ubareg.h>
#include <vax/uba/udareg.h>

#include <vax/mscp/mscp.h>
#include <vax/mscp/mscpvar.h>
#include <vax/mscp/mscpreg.h>

/*
 * Software status, per controller.
 */
struct	uda_softc {
	struct	device sc_dev;	/* Autoconfig info */
	struct	uba_unit sc_unit; /* Struct common for UBA to communicate */
	struct	mscp_pack *sc_uuda;	/* Unibus address of uda struct */
	struct	mscp_pack sc_uda;	/* Struct for uda communication */
	struct	udadevice *sc_udadev;	/* pointer to ip/sa regs */
	struct	mscp *sc_mscp;		/* Keep pointer to active mscp */
	short	sc_ipl;		/* interrupt priority, Q-bus */
	struct	mscp_softc *sc_softc;	/* MSCP info (per mscpvar.h) */
	int	sc_wticks;	/* watchdog timer ticks */
};

int	udamatch __P((struct device *, void *, void *));
void	udaattach __P((struct device *, struct device *, void *));
void	udareset __P((int));
void	udaintr __P((int));
void	udadgo __P((struct uba_unit *));
void	udactlrdone __P((struct device *, int));
int	udaprint __P((void *, char *));
void	udasaerror __P((struct device *, int));
void	udastart  __P((struct uba_unit *));
int	udago __P((struct device *));

struct	cfdriver uda_cd = {
	NULL, "uda", DV_DULL
};

struct	cfattach uda_ca = {
	sizeof(struct uda_softc), udamatch, udaattach
};

/*
 * More driver definitions, for generic MSCP code.
 */
struct	mscp_ctlr uda_mscp_ctlr = {
	udactlrdone,
	udago,
	udasaerror,
};

/*
 * Miscellaneous private variables.
 */
static	char	udasr_bits[] = UDASR_BITS;
static	int	ivec_no;

int
udaprint(aux, name)
	void	*aux;
	char	*name;
{
	if (name)
		printf("%s: mscpbus", name);
	return UNCONF;
}

/*
 * Poke at a supposed UDA50 to see if it is there.
 */
int
udamatch(parent, match, aux)
	struct	device *parent;
	void	*match, *aux;
{
	struct	uba_attach_args *ua = aux;
	struct	mscp_softc mi;	/* Nice hack */
	struct	uba_softc *ubasc;
	int	tries, count;
#ifdef QBA
	extern volatile int rbr;
	int s;
#endif

	/* Get an interrupt vector. */
	ubasc = (void *)parent;
	ivec_no = ubasc->uh_lastiv - 4;

	mi.mi_sa = &((struct udadevice *)ua->ua_addr)->udasa;
	mi.mi_ip = &((struct udadevice *)ua->ua_addr)->udaip;

	/*
	 * Initialise the controller (partially).  The UDA50 programmer's
	 * manual states that if initialisation fails, it should be retried
	 * at least once, but after a second failure the port should be
	 * considered `down'; it also mentions that the controller should
	 * initialise within ten seconds.  Or so I hear; I have not seen
	 * this manual myself.
	 */
#if 0
	s = spl6();
#endif
	tries = 0;
again:

	*mi.mi_ip = 0;
	if (mscp_waitstep(&mi, MP_STEP1, MP_STEP1) == 0)
		return 0; /* Nothing here... */

	*mi.mi_sa = MP_ERR | (NCMDL2 << 11) | (NRSPL2 << 8) | MP_IE |
		(ivec_no >> 2);

	if (mscp_waitstep(&mi, MP_STEP2, MP_STEP2) == 0) {
		printf("udaprobe: init step2 no change. sa=%x\n", *mi.mi_sa);
		goto bad;
	}

	/* should have interrupted by now */
#if 0
	rbr = qbgetpri();
#endif
	ua->ua_ivec = udaintr;
	ua->ua_reset = udareset;
	return 1;
bad:
	if (++tries < 2)
		goto again;
#if 0
	splx(s);
#endif
	return 0;
}

void
udaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	volatile struct udadevice *udaddr;
	volatile struct mscp *mp;
	struct	uda_softc *sc = (void *)self;
	struct	uba_attach_args *ua = aux;
	struct	uba_softc *uh = (void *)parent;
	struct	mscp_info *mi;
	struct	mscp_attach_args ma;
	int	tries, ctlr, timo, next = 0, ubinfo;
	volatile int i;

	printf("\n");

	uh->uh_lastiv -= 4;	/* remove dynamic interrupt vector */
#ifdef QBA
	sc->sc_ipl = ua->ua_br;
#endif

	ctlr = sc->sc_dev.dv_unit;
	sc->sc_udadev = (struct udadevice *)ua->ua_addr;

	/*
	 * Fill in the uba_unit struct, so we can communicate with the uba.
	 */
	sc->sc_unit.uu_softc = sc;	/* Backpointer to softc */
	sc->sc_unit.uu_dgo = udadgo;	/* go routine called from adapter */
	sc->sc_unit.uu_keepbdp = vax_cputype == VAX_750 ? 1 : 0;

	/*
	 * Map the communication area and command and
	 * response packets into Unibus space.
	 */
	ubinfo = uballoc(sc->sc_dev.dv_parent->dv_unit,
		(caddr_t) &sc->sc_uda,
		sizeof (struct mscp_pack), UBA_CANTWAIT);
	if (ubinfo == 0) {
		printf("%s: uballoc map failed\n", sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_uuda = (struct mscp_pack *) UBAI_ADDR(ubinfo);

	bzero(&sc->sc_uda, sizeof (struct mscp_pack));

	ma.ma_mc = &uda_mscp_ctlr;
	ma.ma_type = MSCPBUS_DISK|MSCPBUS_UDA;
	ma.ma_cbuf = &sc->sc_unit.uu_tab;
	ma.ma_uuda = sc->sc_uuda;
	ma.ma_uda = &sc->sc_uda;
	ma.ma_ip = &sc->sc_udadev->udaip;
	ma.ma_sa = ma.ma_sw = &sc->sc_udadev->udasa;
	ma.ma_softc = &sc->sc_softc;
	ma.ma_ivec = ivec_no;
	ma.ma_ctlrnr = (ua->ua_iaddr == 0772150 ? 0 : 1);	/* XXX */
	ma.ma_adapnr = uh->uh_nr;
	config_found(&sc->sc_dev, &ma, udaprint);
}


int
udago(usc)
	struct device *usc;
{
	struct uda_softc *sc = (void *)usc;

	return ubago(&sc->sc_unit);
}

/*
 * Start a transfer.
 *
 * If we are not called from within udastart(), we must have been
 * blocked, so call udastart to do more requests (if any).  If
 * this calls us again immediately we will not recurse, because
 * that time we will be in udastart().	Clever....
 */
void
udadgo(uu)
	register struct uba_unit *uu;
{
	struct uda_softc *sc = uu->uu_softc;
	struct mscp *mp = sc->sc_mscp;

	mscp_dgo(sc->sc_softc, (UBAI_ADDR(uu->uu_ubinfo) |
	    (UBAI_BDP(uu->uu_ubinfo) << 24)), uu->uu_ubinfo);
}

static struct saerr {
	int	code;		/* error code (including UDA_ERR) */
	char	*desc;		/* what it means: Efoo => foo error */
} saerr[] = {
	{ 0100001, "Eunibus packet read" },
	{ 0100002, "Eunibus packet write" },
	{ 0100003, "EUDA ROM and RAM parity" },
	{ 0100004, "EUDA RAM parity" },
	{ 0100005, "EUDA ROM parity" },
	{ 0100006, "Eunibus ring read" },
	{ 0100007, "Eunibus ring write" },
	{ 0100010, " unibus interrupt master failure" },
	{ 0100011, "Ehost access timeout" },
	{ 0100012, " host exceeded command limit" },
	{ 0100013, " unibus bus master failure" },
	{ 0100014, " DM XFC fatal error" },
	{ 0100015, " hardware timeout of instruction loop" },
	{ 0100016, " invalid virtual circuit id" },
	{ 0100017, "Eunibus interrupt write" },
	{ 0104000, "Efatal sequence" },
	{ 0104040, " D proc ALU" },
	{ 0104041, "ED proc control ROM parity" },
	{ 0105102, "ED proc w/no BD#2 or RAM parity" },
	{ 0105105, "ED proc RAM buffer" },
	{ 0105152, "ED proc SDI" },
	{ 0105153, "ED proc write mode wrap serdes" },
	{ 0105154, "ED proc read mode serdes, RSGEN & ECC" },
	{ 0106040, "EU proc ALU" },
	{ 0106041, "EU proc control reg" },
	{ 0106042, " U proc DFAIL/cntl ROM parity/BD #1 test CNT" },
	{ 0106047, " U proc const PROM err w/D proc running SDI test" },
	{ 0106055, " unexpected trap" },
	{ 0106071, "EU proc const PROM" },
	{ 0106072, "EU proc control ROM parity" },
	{ 0106200, "Estep 1 data" },
	{ 0107103, "EU proc RAM parity" },
	{ 0107107, "EU proc RAM buffer" },
	{ 0107115, " test count wrong (BD 12)" },
	{ 0112300, "Estep 2" },
	{ 0122240, "ENPR" },
	{ 0122300, "Estep 3" },
	{ 0142300, "Estep 4" },
	{ 0, " unknown error code" }
};

/*
 * If the error bit was set in the controller status register, gripe,
 * then (optionally) reset the controller and requeue pending transfers.
 */
void
udasaerror(usc, doreset)
	struct device *usc;
	int doreset;
{
	struct	uda_softc *sc = (void *)usc;
	register int code = sc->sc_udadev->udasa;
	register struct saerr *e;

	if ((code & MP_ERR) == 0)
		return;
	for (e = saerr; e->code; e++)
		if (e->code == code)
			break;
	printf("%s: controller error, sa=0%o (%s%s)\n",
		sc->sc_dev.dv_xname, code, e->desc + 1,
		*e->desc == 'E' ? " error" : "");
	if (doreset) {
		mscp_requeue(sc->sc_softc);
/*		(void) udainit(sc);	XXX */
	}
}

/*
 * Interrupt routine.  Depending on the state of the controller,
 * continue initialisation, or acknowledge command and response
 * interrupts, and process responses.
 */
void
udaintr(ctlr)
	int	ctlr;
{
	struct uda_softc *sc = uda_cd.cd_devs[ctlr];
	volatile struct udadevice *udaddr = sc->sc_udadev;
	struct	uba_softc *uh;
	struct mscp_pack *ud;

#ifdef QBA
	if(vax_cputype == VAX_TYP_UV2)
		splx(sc->sc_ipl);	/* Qbus interrupt protocol is odd */
#endif
	sc->sc_wticks = 0;	/* reset interrupt watchdog */

	if (udaddr->udasa & MP_ERR) {	/* ctlr fatal error */
		udasaerror(&sc->sc_dev, 1);
		return;
	}
	ud = &sc->sc_uda;
	/*
	 * Handle buffer purge requests.
	 */
	uh = (void *)sc->sc_dev.dv_parent;
	if (ud->mp_ca.ca_bdp) {
		UBAPURGE(uh->uh_uba, ud->mp_ca.ca_bdp);
		ud->mp_ca.ca_bdp = 0;
		udaddr->udasa = 0;	/* signal purge complete */
	}

	mscp_intr(sc->sc_softc);
}

/*
 * A Unibus reset has occurred on UBA uban.  Reinitialise the controller(s)
 * on that Unibus, and requeue outstanding I/O.
 */
void
udareset(ctlr)
	int ctlr;
{
	register struct uda_softc *sc;

	sc = uda_cd.cd_devs[ctlr];
	printf(" uda%d", ctlr);

	/*
	 * Our BDP (if any) is gone; our command (if any) is
	 * flushed; the device is no longer mapped; and the
	 * UDA50 is not yet initialised.
	 */
	if (sc->sc_unit.uu_bdp) {
		printf("<%d>", UBAI_BDP(sc->sc_unit.uu_bdp));
		sc->sc_unit.uu_bdp = 0;
	}
	sc->sc_unit.uu_ubinfo = 0;
/*	sc->sc_unit.uu_cmd = 0; XXX */

	/* reset queues and requeue pending transfers */
	mscp_requeue(sc->sc_softc);

	/*
	 * If it fails to initialise we will notice later and
	 * try again (and again...).  Do not call udastart()
	 * here; it will be done after the controller finishes
	 * initialisation.
	 */
/* XXX	if (udainit(sc)) */
		printf(" (hung)");
}

void
udactlrdone(usc, info)
	struct device *usc;
	int info;
{
	struct uda_softc *sc = (void *)usc;

	/* XXX check if we shall release the BDP */
	sc->sc_unit.uu_ubinfo = info;
	ubadone(&sc->sc_unit);
}
