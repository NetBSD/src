/*	$NetBSD: vrpiu.c,v 1.1 1999/12/28 03:15:18 takemura Exp $	*/

/*
 * Copyright (c) 1999 Shin Takemura All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <machine/bus.h>

#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/cmureg.h>
#include <hpcmips/vr/vrpiuvar.h>
#include <hpcmips/vr/vrpiureg.h>

/*
 * contant and macro definitions
 */
#define VRPIUDEBUG
#ifdef VRPIUDEBUG
int	vrpiu_debug = 0;
#define	DPRINTF(arg) if (vrpiu_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

#define SCALE	(1024*1024)

/*
 * data types
 */
/* struct vrpiu_softc is defined in vrpiuvar.h */

/*
 * function prototypes
 */
static int	vrpiumatch __P((struct device *, struct cfdata *, void *));
static void	vrpiuattach __P((struct device *, struct device *, void *));

static void	vrpiu_write __P((struct vrpiu_softc *, int, unsigned short));
static u_short	vrpiu_read __P((struct vrpiu_softc *, int));

static int	vrpiu_intr __P((void *));
static void	vrpiu_reset_param __P((struct vrpiu_softc *sc));
static void	vrpiu_timeout __P((void *));
#ifdef DEBUG
static void	vrpiu_dump_cntreg __P((unsigned int cmd));
#endif

static int	vrpiu_enable __P((void *));
static int	vrpiu_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static void	vrpiu_disable __P((void *));

/* mra is defined in mra.c */
int mra_Y_AX1_BX2_C __P((int *y, int ys, int *x1, int x1s, int *x2, int x2s,
			 int n, int scale, int *a, int *b, int *c));

/*
 * static or global variables
 */
struct cfattach vrpiu_ca = {
	sizeof(struct vrpiu_softc), vrpiumatch, vrpiuattach
};

const struct wsmouse_accessops vrpiu_accessops = {
	vrpiu_enable,
	vrpiu_ioctl,
	vrpiu_disable,
};

/*
 * function definitions
 */
static inline void
vrpiu_write(sc, port, val)
	struct vrpiu_softc *sc;
	int port;
	unsigned short val;
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, port, val);
}

static inline u_short
vrpiu_read(sc, port)
	struct vrpiu_softc *sc;
	int port;
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, port);
}

static int
vrpiumatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

static void
vrpiuattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vrpiu_softc *sc = (struct vrpiu_softc *)self;
	struct vrip_attach_args *va = aux;
	struct wsmousedev_attach_args wsmaa;

	bus_space_tag_t iot = va->va_iot;
	bus_space_handle_t ioh;

	if (bus_space_map(iot, va->va_addr, 1, 0, &ioh)) {
		printf(": can't map bus space\n");
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_vrip = va->va_vc;

	/*
	 * disable device until vrpiu_enable called
	 */
	sc->sc_stat = VRPIU_STAT_DISABLE;

	vrpiu_reset_param(sc);
#if 1
	/*
	 * XXX, calibrate parameters
	 */
	{
		static struct sample {
			int xraw, yraw, x, y;
		} D[] = {
			/* samples got on my MC-R500 */
			{ 502, 486, 320, 120 },
			{  55, 109,   0,   0 },
			{  54, 913,   0, 239 },
			{ 973, 924, 639, 239 },
			{ 975, 123, 639,   0 },
		};
		int s = sizeof(*D);
		int n = sizeof(D)/s;

		sc->sc_prmxs = 640;
		sc->sc_prmys = 240;

		if (mra_Y_AX1_BX2_C(&D->x, s, &D->xraw, s, &D->yraw, s, n,
				    SCALE, &sc->sc_prmax, &sc->sc_prmbx,
				    &sc->sc_prmcx) ||
		    mra_Y_AX1_BX2_C(&D->y, s, &D->xraw, s, &D->yraw, s, n,
				    SCALE, &sc->sc_prmay,
				    &sc->sc_prmby, &sc->sc_prmcy)) {
			printf(": MRA error");
			vrpiu_reset_param(sc);
		} else {
			DPRINTF(("Ax=%d Bx=%d Cx=%d\n",
				 sc->sc_prmax, sc->sc_prmbx, sc->sc_prmcx));
			DPRINTF(("Ay=%d By=%d Cy=%d\n",
				 sc->sc_prmay, sc->sc_prmby, sc->sc_prmcy));
		}
	}
#endif

	/* install interrupt handler and enable interrupt */
	if (!(sc->sc_handler = 
	      vrip_intr_establish(va->va_vc, va->va_intr, IPL_TTY,
				  vrpiu_intr, sc))) {
		printf (": can't map interrupt line.\n");
		return;
	}

	/* mask level2 interrupt, stop scan sequencer and mask clock to piu */
	vrpiu_disable(sc);

	printf("\n");

	wsmaa.accessops = &vrpiu_accessops;
	wsmaa.accesscookie = sc;

	/*
	 * attach the wsmouse
	 */
	sc->sc_wsmousedev = config_found(self, &wsmaa, wsmousedevprint);
}

int
vrpiu_enable(v)
	void *v;
{
	struct vrpiu_softc *sc = v;
	int s;
	unsigned int cnt;

	DPRINTF(("%s(%d): vrpiu_enable()\n", __FILE__, __LINE__));
	if (sc->sc_stat != VRPIU_STAT_DISABLE)
		return EBUSY;

	/* supply clock to PIU */
	__vrcmu_supply(CMUMSKPIU, 1);

	/* Scan interval 0x7FF is maximum value */
	vrpiu_write(sc, PIUSIVL_REG_W, 0x7FF);

	s = spltty();

	/* clear interrupt status */
	vrpiu_write(sc, PIUINT_REG_W, PIUINT_ALLINTR);

	/* Disable -> Standby */
	cnt = PIUCNT_PIUPWR |
		PIUCNT_PIUMODE_COORDINATE |
		PIUCNT_PADATSTART | PIUCNT_PADATSTOP;
	vrpiu_write(sc, PIUCNT_REG_W, cnt);

	/* save pen status, touch or release */
	cnt = vrpiu_read(sc, PIUCNT_REG_W);

	/* Level2 interrupt register setting */
	vrip_intr_setmask2(sc->sc_vrip, sc->sc_handler, PIUINT_ALLINTR, 1);

	/*
	 * Enable scan sequencer operation
	 * Standby -> WaitPenTouch
	 */
	cnt |= PIUCNT_PIUSEQEN;
	vrpiu_write(sc, PIUCNT_REG_W, cnt);

	/* transit status DISABLE -> TOUCH or RELEASE */
	sc->sc_stat = (cnt & PIUCNT_PENSTC) ?
		VRPIU_STAT_TOUCH : VRPIU_STAT_RELEASE;

	sc->sc_timeout = 1;

	splx(s);

	return 0;
}

void
vrpiu_disable(v)
	void *v;
{
	struct vrpiu_softc *sc = v;
	int s;

	DPRINTF(("%s(%d): vrpiu_disable()\n", __FILE__, __LINE__));

	/* Set level2 interrupt register to mask interrupts */
	vrip_intr_setmask2(sc->sc_vrip, sc->sc_handler, PIUINT_ALLINTR, 0);

	sc->sc_stat = VRPIU_STAT_DISABLE;

	/* Disable scan sequencer operation and power off */
	vrpiu_write(sc, PIUCNT_REG_W, 0);

	/* mask clock to PIU */
	__vrcmu_supply(CMUMSKPIU, 1);

	s = spltty();
	if (!sc->sc_timeout) {
		untimeout(vrpiu_timeout, sc);
	}
	splx(s);
}

int
vrpiu_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct vrpiu_softc *sc = v;

	DPRINTF(("%s(%d): vrpiu_ioctl(%08lx)\n", __FILE__, __LINE__, cmd));

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2;
		break;
		
	case WSMOUSEIO_SRES:
		printf("%s(%d): WSMOUSRIO_SRES is not supported",
		       __FILE__, __LINE__);
		break;
		
	default:
		return (-1);
	}
	return (0);
}

/*
 * PIU interrupt handler.
 */
int
vrpiu_intr(arg)
	void *arg;
{
        struct vrpiu_softc *sc = arg;
	unsigned int cnt, i;
	unsigned int intrstat, page;
	int tpx0, tpx1, tpy0, tpy1;
	int x, y, xraw, yraw;

	intrstat = vrpiu_read(sc, PIUINT_REG_W);

	if (sc->sc_stat == VRPIU_STAT_DISABLE) {
		/*
		 * the device isn't enabled. just clear interrupt.
		 */
		vrpiu_write(sc, PIUINT_REG_W, intrstat);
		return (0);
	}

	page = (intrstat & PIUINT_OVP) ? 1 : 0;
	if (intrstat & (PIUINT_PADPAGE0INTR | PIUINT_PADPAGE1INTR)) {
		tpx0 = vrpiu_read(sc, PIUPB(page, 0));
		tpx1 = vrpiu_read(sc, PIUPB(page, 1));
		tpy0 = vrpiu_read(sc, PIUPB(page, 2));
		tpy1 = vrpiu_read(sc, PIUPB(page, 3));
	}

	if (intrstat & PIUINT_PADDLOSTINTR) {
		page = page ? 0 : 1;
		for (i = 0; i < 4; i++)
			vrpiu_read(sc, PIUPB(page, i));
	}

	cnt = vrpiu_read(sc, PIUCNT_REG_W);
#ifdef DEBUG
	if (vrpiu_debug)
		vrpiu_dump_cntreg(cnt);
#endif

	/* clear interrupt status */
	vrpiu_write(sc, PIUINT_REG_W, intrstat);

#if 0
	DPRINTF(("vrpiu_intr: OVP=%d", page));
	if (intrstat & PIUINT_PADCMDINTR)
		DPRINTF((" CMD"));
	if (intrstat & PIUINT_PADADPINTR)
		DPRINTF((" A/D"));
	if (intrstat & PIUINT_PADPAGE1INTR)
		DPRINTF((" PAGE1"));
	if (intrstat & PIUINT_PADPAGE0INTR)
		DPRINTF((" PAGE0"));
	if (intrstat & PIUINT_PADDLOSTINTR)
		DPRINTF((" DLOST"));
	if (intrstat & PIUINT_PENCHGINTR)
		DPRINTF((" PENCHG"));
	DPRINTF(("\n"));
#endif
	if (cnt & PIUCNT_PENSTC) {
		if (sc->sc_stat == VRPIU_STAT_RELEASE) {
			/*
			 * pen touch
			 */
			DPRINTF(("PEN TOUCH\n"));
			sc->sc_stat = VRPIU_STAT_TOUCH;
			if (sc->sc_timeout) {
				sc->sc_timeout = 0;
				sc->sc_releasecount = 0;
				timeout(vrpiu_timeout, sc, hz/3);
			}
		}
	} else {
		if (sc->sc_stat == VRPIU_STAT_TOUCH ||
		    sc->sc_stat == VRPIU_STAT_DRAG) {
			/*
			 * pen release
			 */
			DPRINTF(("RELEASE\n"));
			sc->sc_stat = VRPIU_STAT_RELEASE;
			if (!sc->sc_timeout) {
				if (++sc->sc_releasecount == 2) {
					untimeout(vrpiu_timeout, sc);
					sc->sc_timeout = 1;
					DPRINTF(("TAP!\n"));
					/* button 0 DOWN */
					wsmouse_input(sc->sc_wsmousedev,
						      (1 << 0),
						      0, 0, 0);
					/* button 0 UP */
					wsmouse_input(sc->sc_wsmousedev,
						      0, 0, 0, 0);
				}
			}
		}
	}

	if (intrstat & (PIUINT_PADPAGE0INTR | PIUINT_PADPAGE1INTR)) {
		if (!((tpx0 & PIUPB_VALID) && (tpx1 & PIUPB_VALID) &&
		      (tpy0 & PIUPB_VALID) && (tpy1 & PIUPB_VALID))) {
			printf("vrpiu: internal error, data is not valid!\n");
		} else {
			tpx0 &= PIUPB_PADDATA_MASK;
			tpx1 &= PIUPB_PADDATA_MASK;
			tpy0 &= PIUPB_PADDATA_MASK;
			tpy1 &= PIUPB_PADDATA_MASK;
#define ISVALID(n, c, m)	((c) - (m) < (n) && (n) < (c) + (m))
			if (ISVALID(tpx0 + tpx1, 1024, 200) &&
			    ISVALID(tpx0 + tpx1, 1024, 200)) {
#if 0
				DPRINTF(("%04x %04x %04x %04x\n",
					 tpx0, tpx1, tpy0, tpy1));
				DPRINTF(("%3d %3d (%4d %4d)->", tpx0, tpy0,
					 tpx0 + tpx1, tpy0 + tpy1);
#endif
				xraw = tpy1 * 1024 / (tpy0 + tpy1);
				yraw = tpx1 * 1024 / (tpx0 + tpx1);
				DPRINTF(("%3d %3d", xraw, yraw));

				x = (sc->sc_prmax*xraw + sc->sc_prmbx*yraw) /
					SCALE + sc->sc_prmcx;
				y = (sc->sc_prmay*xraw + sc->sc_prmby*yraw) /
					SCALE + sc->sc_prmcy;
				if (x < 0) x = 0;
				if (y < 0) y = 0;
				if (sc->sc_prmxs <= x)
					x = sc->sc_prmxs - 1;
				if (sc->sc_prmys <= y)
					y = sc->sc_prmys - 1;
				DPRINTF(("->%4d %4d", x, y));
				if (sc->sc_stat == VRPIU_STAT_TOUCH) {
					sc->sc_stat = VRPIU_STAT_DRAG;
					sc->sc_x = x;
					sc->sc_y = y;
				} else
				if (sc->sc_stat == VRPIU_STAT_DRAG) {
					DPRINTF((" delta %d %d",
						 x - sc->sc_x, y - sc->sc_y));
					wsmouse_input(sc->sc_wsmousedev,
						      0, /* all buttons up */
						      x - sc->sc_x, /* dx */
						      y - sc->sc_y, /* dy */
						      0); /* dz */
					sc->sc_x = x;
					sc->sc_y = y;
				}
				DPRINTF(("\n"));
			}
		}
	}

	if (intrstat & PIUINT_PADDLOSTINTR) {
		cnt |= PIUCNT_PIUSEQEN;
		vrpiu_write(sc, PIUCNT_REG_W, cnt);
	}

	return 0;
}

void
vrpiu_reset_param(sc)
        struct vrpiu_softc *sc;
{
	sc->sc_prmax = SCALE;
	sc->sc_prmbx = 0;
	sc->sc_prmcx = 0;
	sc->sc_prmxs = PIUPB_PADDATA_MAX;
	sc->sc_prmay = 0;
	sc->sc_prmby = SCALE;
	sc->sc_prmcy = 0;
	sc->sc_prmys = PIUPB_PADDATA_MAX;
}

void
vrpiu_timeout(arg)
	void *arg;
{
        struct vrpiu_softc *sc = arg;
	sc->sc_timeout = 1;
}

#ifdef DEBUG
void
vrpiu_dump_cntreg(cnt)
	unsigned int cnt;
{
	printf("%s", (cnt & PIUCNT_PENSTC) ? "Touch" : "Release");
	printf(" state=");
	if ((cnt & PIUCNT_PADSTATE_MASK) == PIUCNT_PADSTATE_CmdScan)
		printf("CmdScan");
	if ((cnt & PIUCNT_PADSTATE_MASK) == PIUCNT_PADSTATE_IntervalNextScan)
		printf("IntervalNextScan");
	if ((cnt & PIUCNT_PADSTATE_MASK) == PIUCNT_PADSTATE_PenDataScan)
		printf("PenDataScan");
	if ((cnt & PIUCNT_PADSTATE_MASK) == PIUCNT_PADSTATE_WaitPenTouch)
		printf("WaitPenTouch");
	if ((cnt & PIUCNT_PADSTATE_MASK) == PIUCNT_PADSTATE_RFU)
		printf("???");
	if ((cnt & PIUCNT_PADSTATE_MASK) == PIUCNT_PADSTATE_ADPortScan)
		printf("ADPortScan");
	if ((cnt & PIUCNT_PADSTATE_MASK) == PIUCNT_PADSTATE_Standby)
		printf("Standby");
	if ((cnt & PIUCNT_PADSTATE_MASK) == PIUCNT_PADSTATE_Disable)
		printf("Disable");
	if (cnt & PIUCNT_PADATSTOP)
		printf(" AutoStop");
	if (cnt & PIUCNT_PADATSTART)
		printf(" AutoStart");
	if (cnt & PIUCNT_PADSCANSTOP)
		printf(" Stop");
	if (cnt & PIUCNT_PADSCANSTART)
		printf(" Start");
	if (cnt & PIUCNT_PADSCANTYPE)
		printf(" ScanPressure");
	if ((cnt & PIUCNT_PIUMODE_MASK) == PIUCNT_PIUMODE_ADCONVERTER)
		printf(" A/D");
	if ((cnt & PIUCNT_PIUMODE_MASK) == PIUCNT_PIUMODE_COORDINATE)
		printf(" Coordinate");
	if (cnt & PIUCNT_PIUSEQEN)
		printf(" SeqEn");
	if ((cnt & PIUCNT_PIUPWR) == 0)
		printf(" PowerOff");
	if ((cnt & PIUCNT_PADRST) == 0)
		printf(" Reset");
	printf("\n");
}
#endif
