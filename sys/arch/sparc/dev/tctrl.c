/*	$NetBSD: tctrl.c,v 1.10.2.1 2000/07/19 02:53:13 mrg Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/envsys.h>
#include <sys/poll.h>

#include <machine/apmvar.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/tctrl.h>

#include <sparc/dev/ts102reg.h>
#include <sparc/dev/tctrlvar.h>
#include <sparc/sparc/auxiotwo.h>

cdev_decl(tctrl);

extern struct cfdriver tctrl_cd;

static const char *tctrl_ext_statuses[16] = {
	"main power available",
	"internal battery attached",
	"external battery attached",
	"external VGA attached",
	"external keyboard attached",
	"external mouse attached",
	"lid down",
	"internal battery charging",
	"external battery charging",
	"internal battery discharging",
	"external battery discharging",
};

struct tctrl_softc {
	struct	device sc_dev;
	bus_space_tag_t	sc_memt;
	bus_space_handle_t	sc_memh;
	unsigned int	sc_junk;
	unsigned int	sc_ext_status;
	unsigned int	sc_flags;
#define TCTRL_SEND_REQUEST		0x0001
#define TCTRL_APM_CTLOPEN		0x0002
	unsigned int	sc_wantdata;
	volatile unsigned short	sc_lcdstate;
	enum { TCTRL_IDLE, TCTRL_ARGS,
		TCTRL_ACK, TCTRL_DATA } sc_state;
	u_int8_t	sc_cmdbuf[16];
	u_int8_t	sc_rspbuf[16];
	u_int8_t	sc_bitport;
	u_int8_t	sc_tft_on;
	u_int8_t	sc_op;
	u_int8_t	sc_cmdoff;
	u_int8_t	sc_cmdlen;
	u_int8_t	sc_rspoff;
	u_int8_t	sc_rsplen;
	/* APM stuff */
#define APM_NEVENTS 16
	struct	apm_event_info sc_event_list[APM_NEVENTS];
	int	sc_event_count;
	int	sc_event_ptr;
	struct	selinfo sc_rsel;
	/* ENVSYS stuff */
#define ENVSYS_NUMSENSORS 3
	struct	envsys_sensor sc_esensors[ENVSYS_NUMSENSORS];

	struct	evcnt sc_intrcnt;	/* interrupt counting */
};

#define TCTRL_STD_DEV		0
#define TCTRL_APMCTL_DEV	8

static struct callout tctrl_event_ch = CALLOUT_INITIALIZER;

static int tctrl_match __P((struct device *parent, struct cfdata *cf,
	void *aux));
static void tctrl_attach __P((struct device *parent, struct device *self,
	void *aux));
static void tctrl_write __P((struct tctrl_softc *sc, bus_size_t off,
	u_int8_t v));
static u_int8_t tctrl_read __P((struct tctrl_softc *sc, bus_size_t off));
static void tctrl_write_data __P((struct tctrl_softc *sc, u_int8_t v));
static u_int8_t tctrl_read_data __P((struct tctrl_softc *sc));
static int tctrl_intr __P((void *arg));
static void tctrl_setup_bitport __P((void));
static void tctrl_setup_bitport_nop __P((void));
static void tctrl_read_ext_status __P((void));
static void tctrl_read_event_status __P((void *arg));
static int tctrl_apm_record_event __P((struct tctrl_softc *sc,
	u_int event_type));
static void tctrl_init_lcd __P((void));

struct cfattach tctrl_ca = {
	sizeof(struct tctrl_softc), tctrl_match, tctrl_attach
};

extern struct cfdriver tctrl_cd;
/* XXX wtf is this? see i386/apm.c */
int tctrl_apm_evindex;

static int
tctrl_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;

	if (uoba->uoba_isobio4 != 0) {
		return (0);
	}

	/* Tadpole 3GX/3GS uses "uctrl" for the Tadpole Microcontroller
	 * (who's interface is off the TS102 PCMCIA controller but there
	 * exists a OpenProm for microcontroller interface).
	 */
	return strcmp("uctrl", sa->sa_name) == 0;
}

static void
tctrl_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct tctrl_softc *sc = (void *)self;
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	unsigned int i, v;
#if 0
	unsigned int ack, msb, lsb;
#endif

	/* We're living on a sbus slot that looks like an obio that
	 * looks like an sbus slot.
	 */
	sc->sc_memt = sa->sa_bustag;
	if (sbus_bus_map(sc->sc_memt, sa->sa_slot,
			 sa->sa_offset - TS102_REG_UCTRL_INT, sa->sa_size,
			 BUS_SPACE_MAP_LINEAR, 0,
			 &sc->sc_memh) != 0) {
		printf(": can't map registers\n");
		return;
	}

	printf("\n");

	sc->sc_tft_on = 1;

	/* clear any pending data.
	 */
	for (i = 0; i < 10000; i++) {
		if ((TS102_UCTRL_STS_RXNE_STA &
		    tctrl_read(sc, TS102_REG_UCTRL_STS)) == 0) {
			break;
		}
		v = tctrl_read(sc, TS102_REG_UCTRL_DATA);
		tctrl_write(sc, TS102_REG_UCTRL_STS, TS102_UCTRL_STS_RXNE_STA);
	}

	if (sa->sa_nintr != 0) {
		(void)bus_intr_establish(sc->sc_memt, sa->sa_pri, IPL_NONE,
		    0, tctrl_intr, sc);
		evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
		    sc->sc_dev.dv_xname, "intr");
	}

	/* See what the external status is
	 */

	tctrl_read_ext_status();
	if (sc->sc_ext_status != 0) {
		const char *sep;

		printf("%s: ", sc->sc_dev.dv_xname);
		v = sc->sc_ext_status;
		for (i = 0, sep = ""; v != 0; i++, v >>= 1) {
			if (v & 1) {
				printf("%s%s", sep, tctrl_ext_statuses[i]);
				sep = ", ";
			}
		}
		printf("\n");
	}

	/* Get a current of the control bitport;
	 */
	tctrl_setup_bitport_nop();
	tctrl_write(sc, TS102_REG_UCTRL_INT,
		    TS102_UCTRL_INT_RXNE_REQ|TS102_UCTRL_INT_RXNE_MSK);

	sc->sc_wantdata = 0;
	sc->sc_event_count = 0;

	/* prime the sensor data */
	sprintf(sc->sc_esensors[0].desc, "%s", "Internal Unit Temperature");
	sc->sc_esensors[0].units = ENVSYS_STEMP;
	sprintf(sc->sc_esensors[1].desc, "%s", "Internal Battery Voltage");
	sc->sc_esensors[1].units = ENVSYS_SVOLTS_DC;
	sprintf(sc->sc_esensors[2].desc, "%s", "DC-In Voltage");
	sc->sc_esensors[2].units = ENVSYS_SVOLTS_DC;

	/* initialize the LCD */
	tctrl_init_lcd();

	/* initialize sc_lcdstate */
	sc->sc_lcdstate = 0;
	tctrl_set_lcd(2, 0);
}

static int
tctrl_intr(arg)
	void *arg;
{
	struct tctrl_softc *sc = arg;
	unsigned int v, d;
	int progress = 0;

    again:
	/* find out the cause(s) of the interrupt */
	v = tctrl_read(sc, TS102_REG_UCTRL_STS);

	/* clear the cause(s) of the interrupt */
	tctrl_write(sc, TS102_REG_UCTRL_STS, v);

	v &= ~(TS102_UCTRL_STS_RXO_STA|TS102_UCTRL_STS_TXE_STA);
	if (sc->sc_cmdoff >= sc->sc_cmdlen) {
		v &= ~TS102_UCTRL_STS_TXNF_STA;
		if (tctrl_read(sc, TS102_REG_UCTRL_INT) & TS102_UCTRL_INT_TXNF_REQ) {
			tctrl_write(sc, TS102_REG_UCTRL_INT, 0);
			progress = 1;
		}
	}
	if ((v == 0) && ((sc->sc_flags & TCTRL_SEND_REQUEST) == 0 ||
	    sc->sc_state != TCTRL_IDLE)) {
		wakeup(sc);
		return progress;
	}

	progress = 1;
	if (v & TS102_UCTRL_STS_RXNE_STA) {
		d = tctrl_read_data(sc);
		switch (sc->sc_state) {
		case TCTRL_IDLE:
			if (d == 0xfa) {
				/* external event */
				callout_reset(&tctrl_event_ch, 1,
				    tctrl_read_event_status, NULL);
			} else {
				printf("%s: (op=0x%02x): unexpected data (0x%02x)\n",
					sc->sc_dev.dv_xname, sc->sc_op, d);
			}
			goto again;
		case TCTRL_ACK:
			if (d != 0xfe) {
				printf("%s: (op=0x%02x): unexpected ack value (0x%02x)\n",
					sc->sc_dev.dv_xname, sc->sc_op, d);
			}
#ifdef TCTRLDEBUG
			printf(" ack=0x%02x", d);
#endif
			sc->sc_rsplen--;
			sc->sc_rspoff = 0;
			sc->sc_state = sc->sc_rsplen ? TCTRL_DATA : TCTRL_IDLE;
			sc->sc_wantdata = sc->sc_rsplen ? 1 : 0;
#ifdef TCTRLDEBUG
			if (sc->sc_rsplen > 0) {
				printf(" [data(%u)]", sc->sc_rsplen);
			} else {
				printf(" [idle]\n");
			}
#endif
			goto again;
		case TCTRL_DATA:
			sc->sc_rspbuf[sc->sc_rspoff++] = d;
#ifdef TCTRLDEBUG
			printf(" [%d]=0x%02x", sc->sc_rspoff-1, d);
#endif
			if (sc->sc_rspoff == sc->sc_rsplen) {
#ifdef TCTRLDEBUG
				printf(" [idle]\n");
#endif
				sc->sc_state = TCTRL_IDLE;
				sc->sc_wantdata = 0;
			}
			goto again;
		default:
			printf("%s: (op=0x%02x): unexpected data (0x%02x) in state %d\n",
			       sc->sc_dev.dv_xname, sc->sc_op, d, sc->sc_state);
			goto again;
		}
	}
	if ((sc->sc_state == TCTRL_IDLE && sc->sc_wantdata == 0) ||
	    sc->sc_flags & TCTRL_SEND_REQUEST) {
		if (sc->sc_flags & TCTRL_SEND_REQUEST) {
			sc->sc_flags &= ~TCTRL_SEND_REQUEST;
			sc->sc_wantdata = 1;
		}
		if (sc->sc_cmdlen > 0) {
			tctrl_write(sc, TS102_REG_UCTRL_INT,
				tctrl_read(sc, TS102_REG_UCTRL_INT)
				|TS102_UCTRL_INT_TXNF_MSK
				|TS102_UCTRL_INT_TXNF_REQ);
			v = tctrl_read(sc, TS102_REG_UCTRL_STS);
		}
	}
	if ((sc->sc_cmdoff < sc->sc_cmdlen) && (v & TS102_UCTRL_STS_TXNF_STA)) {
		tctrl_write_data(sc, sc->sc_cmdbuf[sc->sc_cmdoff++]);
#ifdef TCTRLDEBUG
		if (sc->sc_cmdoff == 1) {
			printf("%s: op=0x%02x(l=%u)", sc->sc_dev.dv_xname,
				sc->sc_cmdbuf[0], sc->sc_rsplen);
		} else {
			printf(" [%d]=0x%02x", sc->sc_cmdoff-1,
				sc->sc_cmdbuf[sc->sc_cmdoff-1]);
		}
#endif
		if (sc->sc_cmdoff == sc->sc_cmdlen) {
			sc->sc_state = sc->sc_rsplen ? TCTRL_ACK : TCTRL_IDLE;
#ifdef TCTRLDEBUG
			printf(" %s", sc->sc_rsplen ? "[ack]" : "[idle]\n");
#endif
			if (sc->sc_cmdoff == 1) {
				sc->sc_op = sc->sc_cmdbuf[0];
			}
			tctrl_write(sc, TS102_REG_UCTRL_INT,
				tctrl_read(sc, TS102_REG_UCTRL_INT)
				& (~TS102_UCTRL_INT_TXNF_MSK
				   |TS102_UCTRL_INT_TXNF_REQ));
		} else if (sc->sc_state == TCTRL_IDLE) {
			sc->sc_op = sc->sc_cmdbuf[0];
			sc->sc_state = TCTRL_ARGS;
#ifdef TCTRLDEBUG
			printf(" [args]");
#endif
		}
	}
	goto again;
}

static void
tctrl_setup_bitport_nop(void)
{
	struct tctrl_softc *sc;
	struct tctrl_req req;
	int s;
	
	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[TCTRL_STD_DEV];
	req.cmdbuf[0] = TS102_OP_CTL_BITPORT;
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0;
	req.cmdlen = 3;
	req.rsplen = 2;
	req.p = NULL;
	tadpole_request(&req, 1);
	s = splts102();
	sc->sc_bitport = (req.rspbuf[0] & req.cmdbuf[1]) ^ req.cmdbuf[2];
	splx(s);
}

static void
tctrl_setup_bitport(void)
{
	struct tctrl_softc *sc;
	struct tctrl_req req;
	int s;
	
	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[TCTRL_STD_DEV];
	s = splts102();
	if ((sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN)
	    || (!sc->sc_tft_on)) {
		req.cmdbuf[2] = TS102_BITPORT_TFTPWR;
	} else {
		req.cmdbuf[2] = 0;
	}
	req.cmdbuf[0] = TS102_OP_CTL_BITPORT;
	req.cmdbuf[1] = ~TS102_BITPORT_TFTPWR;
	req.cmdlen = 3;
	req.rsplen = 2;
	req.p = NULL;
	tadpole_request(&req, 1);
	s = splts102();
	sc->sc_bitport = (req.rspbuf[0] & req.cmdbuf[1]) ^ req.cmdbuf[2];
	splx(s);
}

/*
 * The tadpole microcontroller is not preprogrammed with icon
 * representations.  The machine boots with the DC-IN light as
 * a blank (all 0x00) and the other lights, as 4 rows of horizontal
 * bars.  The below code initializes the icons in the system to
 * sane values.  Some of these icons could be used for any purpose
 * desired, namely the pcmcia, LAN and WAN lights.  For the disk spinner,
 * only the backslash is unprogrammed.  (sigh)
 *
 * programming the icons is simple.  It is a 5x8 matrix, which each row a
 * bitfield in the order 0x10 0x08 0x04 0x02 0x01.
 */

static void
tctrl_init_lcd(void)
{
	struct tctrl_req req;

	req.cmdbuf[0] = TS102_OP_BLK_DEF_SPCL_CHAR;
	req.cmdlen = 11;
	req.rsplen = 1;
	req.cmdbuf[1] = 0x08;	/*len*/
	req.cmdbuf[2] = TS102_BLK_OFF_DEF_DC_GOOD;
	req.cmdbuf[3] =  0x00;	/* ..... */
	req.cmdbuf[4] =  0x00;	/* ..... */
	req.cmdbuf[5] =  0x1f;	/* XXXXX */
	req.cmdbuf[6] =  0x00;	/* ..... */
	req.cmdbuf[7] =  0x15;	/* X.X.X */
	req.cmdbuf[8] =  0x00;	/* ..... */
	req.cmdbuf[9] =  0x00;	/* ..... */
	req.cmdbuf[10] = 0x00;	/* ..... */
	req.p = NULL;
	tadpole_request(&req, 1);

	req.cmdbuf[0] = TS102_OP_BLK_DEF_SPCL_CHAR;
	req.cmdlen = 11;
	req.rsplen = 1;
	req.cmdbuf[1] = 0x08;	/*len*/
	req.cmdbuf[2] = TS102_BLK_OFF_DEF_BACKSLASH;
	req.cmdbuf[3] =  0x00;	/* ..... */
	req.cmdbuf[4] =  0x10;	/* X.... */
	req.cmdbuf[5] =  0x08;	/* .X... */
	req.cmdbuf[6] =  0x04;	/* ..X.. */
	req.cmdbuf[7] =  0x02;	/* ...X. */
	req.cmdbuf[8] =  0x01;	/* ....X */
	req.cmdbuf[9] =  0x00;	/* ..... */
	req.cmdbuf[10] = 0x00;	/* ..... */
	req.p = NULL;
	tadpole_request(&req, 1);

	req.cmdbuf[0] = TS102_OP_BLK_DEF_SPCL_CHAR;
	req.cmdlen = 11;
	req.rsplen = 1;
	req.cmdbuf[1] = 0x08;	/*len*/
	req.cmdbuf[2] = TS102_BLK_OFF_DEF_WAN1;
	req.cmdbuf[3] =  0x0c;	/* .XXX. */
	req.cmdbuf[4] =  0x16;	/* X.XX. */
	req.cmdbuf[5] =  0x10;	/* X.... */
	req.cmdbuf[6] =  0x15;	/* X.X.X */
	req.cmdbuf[7] =  0x10;	/* X.... */
	req.cmdbuf[8] =  0x16;	/* X.XX. */
	req.cmdbuf[9] =  0x0c;	/* .XXX. */
	req.cmdbuf[10] = 0x00;	/* ..... */
	req.p = NULL;
	tadpole_request(&req, 1);

	req.cmdbuf[0] = TS102_OP_BLK_DEF_SPCL_CHAR;
	req.cmdlen = 11;
	req.rsplen = 1;
	req.cmdbuf[1] = 0x08;	/*len*/
	req.cmdbuf[2] = TS102_BLK_OFF_DEF_WAN2;
	req.cmdbuf[3] =  0x0c;	/* .XXX. */
	req.cmdbuf[4] =  0x0d;	/* .XX.X */
	req.cmdbuf[5] =  0x01;	/* ....X */
	req.cmdbuf[6] =  0x15;	/* X.X.X */
	req.cmdbuf[7] =  0x01;	/* ....X */
	req.cmdbuf[8] =  0x0d;	/* .XX.X */
	req.cmdbuf[9] =  0x0c;	/* .XXX. */
	req.cmdbuf[10] = 0x00;	/* ..... */
	req.p = NULL;
	tadpole_request(&req, 1);

	req.cmdbuf[0] = TS102_OP_BLK_DEF_SPCL_CHAR;
	req.cmdlen = 11;
	req.rsplen = 1;
	req.cmdbuf[1] = 0x08;	/*len*/
	req.cmdbuf[2] = TS102_BLK_OFF_DEF_LAN1;
	req.cmdbuf[3] =  0x00;	/* ..... */
	req.cmdbuf[4] =  0x04;	/* ..X.. */
	req.cmdbuf[5] =  0x08;	/* .X... */
	req.cmdbuf[6] =  0x13;	/* X..XX */
	req.cmdbuf[7] =  0x08;	/* .X... */
	req.cmdbuf[8] =  0x04;	/* ..X.. */
	req.cmdbuf[9] =  0x00;	/* ..... */
	req.cmdbuf[10] = 0x00;	/* ..... */
	req.p = NULL;
	tadpole_request(&req, 1);

	req.cmdbuf[0] = TS102_OP_BLK_DEF_SPCL_CHAR;
	req.cmdlen = 11;
	req.rsplen = 1;
	req.cmdbuf[1] = 0x08;	/*len*/
	req.cmdbuf[2] = TS102_BLK_OFF_DEF_LAN2;
	req.cmdbuf[3] =  0x00;	/* ..... */
	req.cmdbuf[4] =  0x04;	/* ..X.. */
	req.cmdbuf[5] =  0x02;	/* ...X. */
	req.cmdbuf[6] =  0x19;	/* XX..X */
	req.cmdbuf[7] =  0x02;	/* ...X. */
	req.cmdbuf[8] =  0x04;	/* ..X.. */
	req.cmdbuf[9] =  0x00;	/* ..... */
	req.cmdbuf[10] = 0x00;	/* ..... */
	req.p = NULL;
	tadpole_request(&req, 1);

	req.cmdbuf[0] = TS102_OP_BLK_DEF_SPCL_CHAR;
	req.cmdlen = 11;
	req.rsplen = 1;
	req.cmdbuf[1] = 0x08;	/*len*/
	req.cmdbuf[2] = TS102_BLK_OFF_DEF_PCMCIA;
	req.cmdbuf[3] =  0x00;	/* ..... */
	req.cmdbuf[4] =  0x0c;	/* .XXX. */
	req.cmdbuf[5] =  0x1f;	/* XXXXX */
	req.cmdbuf[6] =  0x1f;	/* XXXXX */
	req.cmdbuf[7] =  0x1f;	/* XXXXX */
	req.cmdbuf[8] =  0x1f;	/* XXXXX */
	req.cmdbuf[9] =  0x00;	/* ..... */
	req.cmdbuf[10] = 0x00;	/* ..... */
	req.p = NULL;
	tadpole_request(&req, 1);
}



/*
 * set the blinken-lights on the lcd.  what:
 * what = 0 off,  what = 1 on,  what = 2 toggle
 */

void
tctrl_set_lcd(what, which)
	int what;
	unsigned short which;
{
	struct tctrl_softc *sc;
	struct tctrl_req req;
	int s;
	
	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[TCTRL_STD_DEV];
	s = splts102();

	/* provide a quick exit to save cpu time */
	if ((what == 1 && sc->sc_lcdstate & which) ||
	    (what == 0 && !(sc->sc_lcdstate & which))) {
		splx(s);
		return;
	}
	/*
	 * the mask setup on this particular command is *very* bizzare
	 * and totally undocumented.
	 */
	if ((what == 1) || (what == 2 && !(sc->sc_lcdstate & which))) {
		req.cmdbuf[2] = (u_int8_t)(which&0xff);
		req.cmdbuf[3] = (u_int8_t)(which>>8);
	} else {
		req.cmdbuf[2] = 0;
		req.cmdbuf[3] = 0;
	}
	req.cmdbuf[0] = TS102_OP_CTL_LCD;
	req.cmdbuf[4] = (u_int8_t)(~which>>8);
	req.cmdbuf[1] = (u_int8_t)(~which&0xff);

	/* XXX this thing is wierd.... */
	req.cmdlen = 3;
	req.rsplen = 2;
#if 0
	req.cmdlen = 5;
	req.rsplen = 4;
#endif
	req.p = NULL;
	tadpole_request(&req, 1);
	s = splts102();
	sc->sc_lcdstate = (unsigned short)req.rspbuf[0];
	splx(s);
}

static void
tctrl_read_ext_status(void)
{
	struct tctrl_softc *sc;
	struct tctrl_req req;
	int s;
	
	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[TCTRL_STD_DEV];
	req.cmdbuf[0] = TS102_OP_RD_EXT_STATUS;
	req.cmdlen = 1;
	req.rsplen = 3;
	req.p = NULL;
#ifdef TCTRLDEBUG
	printf("pre read: sc->sc_ext_status = 0x%x\n", sc->sc_ext_status);
#endif
	tadpole_request(&req, 1);
	s = splts102();
	sc->sc_ext_status = req.rspbuf[0] * 256 + req.rspbuf[1];
	splx(s);
#ifdef TCTRLDEBUG
	printf("post read: sc->sc_ext_status = 0x%x\n", sc->sc_ext_status);
#endif
}

/*
 * return 0 if the user will notice and handle the event,
 * return 1 if the kernel driver should do so.
 */
static int
tctrl_apm_record_event(sc, event_type)
	struct tctrl_softc *sc;
	u_int event_type;
{
	struct apm_event_info *evp;

	if ((sc->sc_flags & TCTRL_APM_CTLOPEN) &&
	    (sc->sc_event_count < APM_NEVENTS)) {
		evp = &sc->sc_event_list[sc->sc_event_ptr];
		sc->sc_event_count++;
		sc->sc_event_ptr++;
		sc->sc_event_ptr %= APM_NEVENTS;
		evp->type = event_type;
		evp->index = ++tctrl_apm_evindex;
		selwakeup(&sc->sc_rsel);
		return(sc->sc_flags & TCTRL_APM_CTLOPEN) ? 0 : 1;
	}
	return(1);
}

static void
tctrl_read_event_status(arg)
	void *arg;
{
	struct tctrl_softc *sc;
	struct tctrl_req req;
	int s;
	unsigned int v;
	
	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[TCTRL_STD_DEV];
	req.cmdbuf[0] = TS102_OP_RD_EVENT_STATUS;
	req.cmdlen = 1;
	req.rsplen = 3;
	req.p = NULL;
	tadpole_request(&req, 1);
	s = splts102();
	v = req.rspbuf[0] * 256 + req.rspbuf[1];
	if (v & TS102_EVENT_STATUS_SHUTDOWN_REQUEST) {
		printf("%s: SHUTDOWN REQUEST!\n", sc->sc_dev.dv_xname);
	}
	if (v & TS102_EVENT_STATUS_VERY_LOW_POWER_WARNING) {
/*printf("%s: VERY LOW POWER WARNING!\n", sc->sc_dev.dv_xname);*/
/* according to a tadpole header, and observation */
#ifdef TCTRLDEBUG
		printf("%s: Battery charge level change\n", sc->sc_dev.dv_xname);
#endif
	}
	if (v & TS102_EVENT_STATUS_LOW_POWER_WARNING) {
		if (tctrl_apm_record_event(sc, APM_BATTERY_LOW))
			printf("%s: LOW POWER WARNING!\n", sc->sc_dev.dv_xname);
	}
	if (v & TS102_EVENT_STATUS_DC_STATUS_CHANGE) {
		splx(s);
		tctrl_read_ext_status();
		s = splts102();
		if (tctrl_apm_record_event(sc, APM_POWER_CHANGE))
			printf("%s: main power %s\n", sc->sc_dev.dv_xname,
			    (sc->sc_ext_status &
			    TS102_EXT_STATUS_MAIN_POWER_AVAILABLE) ?
			    "restored" : "removed");
	}
	if (v & TS102_EVENT_STATUS_LID_STATUS_CHANGE) {
		splx(s);
		tctrl_read_ext_status();
		tctrl_setup_bitport();
#ifdef TCTRLDEBUG
		printf("%s: lid %s\n", sc->sc_dev.dv_xname,
		    (sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN)
		    ? "closed" : "opened");
#endif
	}
	splx(s);
}

void
tadpole_request(req, spin)
	struct tctrl_req *req;
	int spin;
{
	struct tctrl_softc *sc;
	int i, s;

	if (tctrl_cd.cd_devs == NULL
	    || tctrl_cd.cd_ndevs == 0
	    || tctrl_cd.cd_devs[TCTRL_STD_DEV] == NULL) {
		return;
	}

	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[TCTRL_STD_DEV];
	while (sc->sc_wantdata != 0) {
		if (req->p != NULL)
			tsleep(&sc->sc_wantdata, PLOCK, "tctrl_lock", 10);
		else
			DELAY(1);
	}
	if (spin)
		s = splhigh();
	else
		s = splts102();
	sc->sc_flags |= TCTRL_SEND_REQUEST;
	memcpy(sc->sc_cmdbuf, req->cmdbuf, req->cmdlen);
	sc->sc_wantdata = 1;
	sc->sc_rsplen = req->rsplen;
	sc->sc_cmdlen = req->cmdlen;
	sc->sc_cmdoff = sc->sc_rspoff = 0;

	/* we spin for certain commands, like poweroffs */
	if (spin) {
/*		for (i = 0; i < 30000; i++) {*/
		while (sc->sc_wantdata == 1) {
			tctrl_intr(sc);
			DELAY(1);
		}
	} else {
		tctrl_intr(sc);
		i = 0;
		while (((sc->sc_rspoff != sc->sc_rsplen) ||
		    (sc->sc_cmdoff != sc->sc_cmdlen)) &&
		    (i < (5 * sc->sc_rsplen + sc->sc_cmdlen)))
			if (req->p != NULL) {
				tsleep(sc, PWAIT, "tctrl_data", 15);
				i++;
			}
			else
				DELAY(1);
	}
	/*
	 * we give the user a reasonable amount of time for a command
	 * to complete.  If it doesn't complete in time, we hand them
	 * garbage.  This is here to stop things like setting the
	 * rsplen too long, and sleeping forever in a CMD_REQ ioctl.
	 */
	sc->sc_wantdata = 0;
	memcpy(req->rspbuf, sc->sc_rspbuf, req->rsplen);
	splx(s);
}

void
tadpole_powerdown(void)
{
	struct tctrl_req req;
	
	req.cmdbuf[0] = TS102_OP_ADMIN_POWER_OFF;
	req.cmdlen = 1;
	req.rsplen = 1;
	req.p = NULL;
	tadpole_request(&req, 1);
}

void
tadpole_set_video(enabled)
	int enabled;
{
	struct tctrl_softc *sc;
	struct tctrl_req req;
	int s;

	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[TCTRL_STD_DEV];
	while (sc->sc_wantdata != 0)
		DELAY(1);
	s = splts102();
	req.p = NULL;
	if ((sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN && !enabled)
	    || (sc->sc_tft_on)) {
		req.cmdbuf[2] = TS102_BITPORT_TFTPWR;
	} else {
		req.cmdbuf[2] = 0;
	}
	req.cmdbuf[0] = TS102_OP_CTL_BITPORT;
	req.cmdbuf[1] = ~TS102_BITPORT_TFTPWR;
	req.cmdlen = 3;
	req.rsplen = 2;

	if ((sc->sc_tft_on && !enabled) || (!sc->sc_tft_on && enabled)) {
		sc->sc_tft_on = enabled;
		if (sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN) {
			splx(s);
			return;
		}
		tadpole_request(&req, 1);
		sc->sc_bitport =
		    (req.rspbuf[0] & req.cmdbuf[1]) ^ req.cmdbuf[2];
	}
	splx(s);
}

static void
tctrl_write_data(sc, v)
	struct tctrl_softc *sc;
	u_int8_t v;
{
	unsigned int i;

	for (i = 0; i < 100; i++)  {
		if (TS102_UCTRL_STS_TXNF_STA & tctrl_read(sc, TS102_REG_UCTRL_STS))
			break;
	}
	tctrl_write(sc, TS102_REG_UCTRL_DATA, v);
}

static u_int8_t
tctrl_read_data(sc)
	struct tctrl_softc *sc;
{
	unsigned int i, v;

	for (i = 0; i < 100000; i++) {
		if (TS102_UCTRL_STS_RXNE_STA & tctrl_read(sc, TS102_REG_UCTRL_STS))
			break;
		DELAY(1);
	}

	v = tctrl_read(sc, TS102_REG_UCTRL_DATA);
	tctrl_write(sc, TS102_REG_UCTRL_STS, TS102_UCTRL_STS_RXNE_STA);
	return v;
}

static u_int8_t
tctrl_read(sc, off)
	struct tctrl_softc *sc;
	bus_size_t off;
{

	sc->sc_junk = bus_space_read_1(sc->sc_memt, sc->sc_memh, off);
	return sc->sc_junk;
}

static void
tctrl_write(sc, off, v)
	struct tctrl_softc *sc;
	bus_size_t off;
	u_int8_t v;
{

	sc->sc_junk = v;
	bus_space_write_1(sc->sc_memt, sc->sc_memh, off, v);
}

int
tctrlopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = (minor(dev)&0xf0);
	int ctl = (minor(dev)&0x0f);
	struct tctrl_softc *sc;

	if (unit >= tctrl_cd.cd_ndevs)
		return(ENXIO);
	sc = tctrl_cd.cd_devs[TCTRL_STD_DEV];
	if (!sc)
		return(ENXIO);
	
	switch (ctl) {
	case TCTRL_STD_DEV:
		break;
	case TCTRL_APMCTL_DEV:
		if (!(flags & FWRITE))
			return(EINVAL);
		if (sc->sc_flags & TCTRL_APM_CTLOPEN)
			return(EBUSY);
		sc->sc_flags |= TCTRL_APM_CTLOPEN;
		break;
	default:
		return(ENXIO);
		break;
	}

	return(0);
}

int
tctrlclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int ctl = (minor(dev)&0x0f);
	struct tctrl_softc *sc;

	sc = tctrl_cd.cd_devs[TCTRL_STD_DEV];
	if (!sc)
		return(ENXIO);

	switch (ctl) {
	case TCTRL_STD_DEV:
		break;
	case TCTRL_APMCTL_DEV:
		sc->sc_flags &= ~TCTRL_APM_CTLOPEN;
		break;
	}
	return(0);
}

int
tctrlioctl(dev, cmd, data, flags, p)
        dev_t dev;
        u_long cmd;
        caddr_t data;
        int flags;
        struct proc *p;
{
	struct tctrl_req req, *reqn;
	struct tctrl_pwr *pwrreq;  
	envsys_range_t *envrange;
	envsys_temp_data_t *envdata;
	envsys_temp_info_t *envinfo;
	struct apm_power_info *powerp;
	struct apm_event_info *evp;
	struct tctrl_softc *sc;
	int i;
	u_int j;
	u_int16_t a;
	u_int8_t c;

	if (tctrl_cd.cd_devs == NULL
	    || tctrl_cd.cd_ndevs == 0
	    || tctrl_cd.cd_devs[TCTRL_STD_DEV] == NULL) {
		return ENXIO;
	}
	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[TCTRL_STD_DEV];
        switch (cmd) {

	case APM_IOC_STANDBY:
		return(EOPNOTSUPP); /* for now */

	case APM_IOC_SUSPEND:
		return(EOPNOTSUPP); /* for now */

	case APM_IOC_GETPOWER:
		powerp = (struct apm_power_info *)data;
		req.cmdbuf[0] = TS102_OP_RD_INT_CHARGE_RATE;
		req.cmdlen = 1;
		req.rsplen = 2;
		req.p = p;
		tadpole_request(&req, 0);
		if (req.rspbuf[0] > 0x00)
			powerp->battery_state = APM_BATT_CHARGING;
		req.cmdbuf[0] = TS102_OP_RD_INT_CHARGE_LEVEL;
		req.cmdlen = 1;
		req.rsplen = 3;
		req.p = p;
		tadpole_request(&req, 0);
		c = req.rspbuf[0];
		powerp->battery_life = c;
		if (c > 0x70)	/* the tadpole sometimes dips below zero, and */
			c = 0;	/* into the 255 range. */
		powerp->minutes_left = (45 * c) / 100; /* XXX based on 45 min */
		if (powerp->battery_state != APM_BATT_CHARGING) {
			if (c < 0x20)
				powerp->battery_state = APM_BATT_CRITICAL;
			else if (c < 0x40)
				powerp->battery_state = APM_BATT_LOW;
			else if (c < 0x66)
				powerp->battery_state = APM_BATT_HIGH;
			else
				powerp->battery_state = APM_BATT_UNKNOWN;
		}
		req.cmdbuf[0] = TS102_OP_RD_EXT_STATUS;
		req.cmdlen = 1;
		req.rsplen = 3;
		req.p = p;
		tadpole_request(&req, 0);
		a = req.rspbuf[0] * 256 + req.rspbuf[1];
		if (a & TS102_EXT_STATUS_MAIN_POWER_AVAILABLE)
			powerp->ac_state = APM_AC_ON;
		else
			powerp->ac_state = APM_AC_OFF;
		break;

	case APM_IOC_NEXTEVENT:
		if (!sc->sc_event_count)
			return EAGAIN;

		evp = (struct apm_event_info *)data;
		i = sc->sc_event_ptr + APM_NEVENTS - sc->sc_event_count;
		i %= APM_NEVENTS;
		*evp = sc->sc_event_list[i];
		sc->sc_event_count--;
		return(0);

	/* this ioctl assumes the caller knows exactly what he is doing */
	case TCTRL_CMD_REQ:
		reqn = (struct tctrl_req *)data;
		if ((i = suser(p->p_ucred, &p->p_acflag)) != 0 &&
		    (reqn->cmdbuf[0] == TS102_OP_CTL_BITPORT ||
		    (reqn->cmdbuf[0] >= TS102_OP_CTL_WATCHDOG &&
		    reqn->cmdbuf[0] <= TS102_OP_CTL_SECURITY_KEY) ||
		    reqn->cmdbuf[0] == TS102_OP_CTL_TIMEZONE ||
		    reqn->cmdbuf[0] == TS102_OP_CTL_DIAGNOSTIC_MODE ||
		    reqn->cmdbuf[0] == TS102_OP_CMD_SOFTWARE_RESET ||
		    (reqn->cmdbuf[0] >= TS102_OP_CMD_SET_RTC &&
		    reqn->cmdbuf[0] < TS102_OP_RD_INT_CHARGE_LEVEL) ||
		    reqn->cmdbuf[0] > TS102_OP_RD_EXT_CHARGE_LEVEL))
			return(i);
		reqn->p = p;
		tadpole_request(reqn, 0);
		break;

	case ENVSYS_VERSION:
		*(int32_t *)data = 1000;
		break;

	case ENVSYS_GRANGE:
		envrange = (envsys_range_t *)data;
		i = 0;
		envrange->high = envrange->low = 0;
		for (j=0; j < ENVSYS_NUMSENSORS; j++) {
			if (!i && envrange->units == sc->sc_esensors[j].units) {
				envrange->low = j;
				i++;
			}
			if (i && envrange->units == sc->sc_esensors[j].units)
				envrange->high = j;
		}
		if (!i) {
			envrange->high = 0;
			envrange->low = 1;
		}
		break;

	case ENVSYS_GTREDATA:
		envdata = (envsys_temp_data_t *)data;
		if (envdata->sensor >= ENVSYS_NUMSENSORS) {
			envdata->validflags = 0;
			break;
		}
		envdata->warnflags = ENVSYS_WARN_OK;
		if (envdata->sensor == 0) {
			envdata->validflags |= ENVSYS_FVALID;
			req.cmdbuf[0] = TS102_OP_RD_CURRENT_TEMP;
			req.cmdlen = 1;
			req.rsplen = 2;
			req.p = p;
			tadpole_request(&req, 0);
			envdata->cur.data_us =             /* 273160? */
			    (u_int32_t)((int)((int)req.rspbuf[0]-32)*5000000/9+273150000);
			envdata->validflags |= ENVSYS_FCURVALID;
			req.cmdbuf[0] = TS102_OP_RD_MAX_TEMP;
			req.cmdlen = 1;
			req.rsplen = 2;
			req.p = p;
			tadpole_request(&req, 0);
			envdata->max.data_us =
			    (u_int32_t)((int)((int)req.rspbuf[0]-32)*5000000/9+273150000);
			envdata->validflags |= ENVSYS_FMAXVALID;
			req.cmdbuf[0] = TS102_OP_RD_MIN_TEMP;
			req.cmdlen = 1;
			req.rsplen = 2;
			req.p = p;
			tadpole_request(&req, 0);
			envdata->min.data_us =
			    (u_int32_t)((int)((int)req.rspbuf[0]-32)*5000000/9+273150000);
			envdata->validflags |= ENVSYS_FMINVALID;
			envdata->units = sc->sc_esensors[envdata->sensor].units;
			break;
		} else if (envdata->sensor == 1 || envdata->sensor == 2) {
			envdata->validflags = ENVSYS_FVALID|ENVSYS_FCURVALID;
			envdata->units = sc->sc_esensors[envdata->sensor].units;
			if (envdata->sensor == 1)
				req.cmdbuf[0] = TS102_OP_RD_INT_BATT_VLT;
			else
				req.cmdbuf[0] = TS102_OP_RD_DC_IN_VLT;
			req.cmdlen = 1;
			req.rsplen = 2;
			req.p = p;
			tadpole_request(&req, 0);
			envdata->cur.data_s = (int32_t)req.rspbuf[0]*1000000/11;
			break;			
		}
		break;

        case ENVSYS_GTREINFO:
		envinfo = (envsys_temp_info_t *)data;
		if (envinfo->sensor >= ENVSYS_NUMSENSORS) {
			envinfo->validflags = 0;
			break;
		}
		envinfo->units = sc->sc_esensors[envinfo->sensor].units;
		memcpy(envinfo->desc, sc->sc_esensors[envinfo->sensor].desc,
		    sizeof(sc->sc_esensors[envinfo->sensor].desc) >
		    sizeof(envinfo->desc) ? sizeof(envinfo->desc) :
		    sizeof(sc->sc_esensors[envinfo->sensor].desc));
		if (envinfo->units == ENVSYS_STEMP) {
			envinfo->validflags = ENVSYS_FVALID|ENVSYS_FCURVALID|
			    ENVSYS_FMINVALID|ENVSYS_FMAXVALID;
		} else if (envinfo->units == ENVSYS_SVOLTS_DC) {
			envinfo->validflags = ENVSYS_FVALID|ENVSYS_FCURVALID;
		} else
			envinfo->validflags = 0;
                break;

        case ENVSYS_STREINFO:
		envinfo = (envsys_temp_info_t *)data;
		if (envinfo->sensor >= ENVSYS_NUMSENSORS) {
			envinfo->validflags = 0;
			break;
		}
		if (envinfo->units == sc->sc_esensors[envinfo->sensor].units)
			memcpy(sc->sc_esensors[envinfo->sensor].desc,
			    envinfo->desc,
			    sizeof(envinfo->desc) > sizeof(char)*32 ?
			    sizeof(char)*32 : sizeof(envinfo->desc) );
		if (envinfo->units == ENVSYS_STEMP) {
			envinfo->validflags = ENVSYS_FVALID|ENVSYS_FCURVALID|
			    ENVSYS_FMINVALID|ENVSYS_FMAXVALID;
		} else if (envinfo->units == ENVSYS_SVOLTS_DC) {
			envinfo->validflags = ENVSYS_FVALID|ENVSYS_FCURVALID;
		} else
			envinfo->validflags = 0;
                break;

	/* serial power mode (via auxiotwo) */
	case TCTRL_SERIAL_PWR:
		pwrreq = (struct tctrl_pwr *)data;  
		if (pwrreq->rw)
			pwrreq->state = auxiotwoserialgetapm();
		else
			auxiotwoserialsetapm(pwrreq->state);
		break;

	/* modem power mode (via auxio) */
	case TCTRL_MODEM_PWR:
		return(EOPNOTSUPP); /* for now */
		break;


        default:
                return (ENOTTY);
        }
        return (0);
}

int
tctrlpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct tctrl_softc *sc = tctrl_cd.cd_devs[TCTRL_STD_DEV];
	int revents = 0;

	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_event_count)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &sc->sc_rsel);
	}

	return (revents);
}
/* DO NOT SET THIS OPTION */
#ifdef TADPOLE_BLINK
void
cpu_disk_unbusy(busy)
        int busy;
{
	static struct timeval tctrl_ds_timestamp;
        struct timeval dv_time, diff_time;
	struct tctrl_softc *sc;

	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[TCTRL_STD_DEV];

	/* quickly bail */
	if (!(sc->sc_lcdstate & TS102_LCD_DISK_ACTIVE) || busy > 0)
		return;

        /* we aren't terribly concerned with precision here */
        dv_time = mono_time;
        timersub(&dv_time, &tctrl_ds_timestamp, &diff_time);

	if (diff_time.tv_sec > 0) {
                tctrl_set_lcd(0, TS102_LCD_DISK_ACTIVE);
		tctrl_ds_timestamp = mono_time;
	}
}
#endif
