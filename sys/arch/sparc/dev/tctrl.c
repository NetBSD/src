/*	$NetBSD: tctrl.c,v 1.58.2.1 2014/08/10 06:54:08 tls Exp $	*/

/*-
 * Copyright (c) 1998, 2005, 2006 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tctrl.c,v 1.58.2.1 2014/08/10 06:54:08 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/envsys.h>
#include <sys/poll.h>
#include <sys/kauth.h>

#include <machine/apmvar.h>
#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/tctrl.h>

#include <sparc/dev/ts102reg.h>
#include <sparc/dev/tctrlvar.h>
#include <sparc/sparc/auxiotwo.h>
#include <sparc/sparc/auxreg.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include "sysmon_envsys.h"

/*#define TCTRLDEBUG*/

/* disk spinner */
#include <sys/disk.h>
#include <dev/scsipi/sdvar.h>

/* ethernet carrier */
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <dev/ic/lancevar.h>

extern struct cfdriver tctrl_cd;

dev_type_open(tctrlopen);
dev_type_close(tctrlclose);
dev_type_ioctl(tctrlioctl);
dev_type_poll(tctrlpoll);
dev_type_kqfilter(tctrlkqfilter);

const struct cdevsw tctrl_cdevsw = {
	.d_open = tctrlopen,
	.d_close = tctrlclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = tctrlioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = tctrlpoll,
	.d_mmap = nommap,
	.d_kqfilter = tctrlkqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

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
	device_t	sc_dev;
	bus_space_tag_t	sc_memt;
	bus_space_handle_t	sc_memh;
	unsigned int	sc_junk;
	unsigned int	sc_ext_status;
	unsigned int	sc_flags;
#define TCTRL_SEND_REQUEST		0x0001
#define TCTRL_APM_CTLOPEN		0x0002
	uint32_t	sc_wantdata;
	uint32_t	sc_ext_pending;
	volatile uint16_t	sc_lcdstate;
	uint16_t	sc_lcdwanted;
	
	enum { TCTRL_IDLE, TCTRL_ARGS,
		TCTRL_ACK, TCTRL_DATA } sc_state;
	uint8_t		sc_cmdbuf[16];
	uint8_t		sc_rspbuf[16];
	uint8_t		sc_bitport;
	uint8_t		sc_tft_on;
	uint8_t		sc_op;
	uint8_t		sc_cmdoff;
	uint8_t		sc_cmdlen;
	uint8_t		sc_rspoff;
	uint8_t		sc_rsplen;
	/* APM stuff */
#define APM_NEVENTS 16
	struct	apm_event_info sc_event_list[APM_NEVENTS];
	int	sc_event_count;
	int	sc_event_ptr;
	struct	selinfo sc_rsel;

	/* ENVSYS stuff */
#define ENVSYS_NUMSENSORS 3
	struct	evcnt sc_intrcnt;	/* interrupt counting */
	struct	sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[ENVSYS_NUMSENSORS];

	struct	sysmon_pswitch sc_sm_pbutton;	/* power button */
	struct	sysmon_pswitch sc_sm_lid;	/* lid state */
	struct	sysmon_pswitch sc_sm_ac;	/* AC adaptor presence */
	int	sc_powerpressed;
	
	/* hardware status stuff */
	int sc_lid;	/* 1 - open, 0 - closed */
	int sc_power_state;
	int sc_spl;
	
	/*
	 * we call this when we detect connection or removal of an external
	 * monitor. 0 for no monitor, !=0 for monitor present
	 */
	void (*sc_video_callback)(void *, int);
	void *sc_video_callback_cookie;
	int sc_extvga;
	
	uint32_t sc_events;
	lwp_t *sc_thread;			/* event thread */
	kmutex_t sc_requestlock;
};

#define TCTRL_STD_DEV		0
#define TCTRL_APMCTL_DEV	8

static int tctrl_match(device_t, cfdata_t, void *);
static void tctrl_attach(device_t, device_t, void *);
static void tctrl_write(struct tctrl_softc *, bus_size_t, uint8_t);
static uint8_t tctrl_read(struct tctrl_softc *, bus_size_t);
static void tctrl_write_data(struct tctrl_softc *, uint8_t);
static uint8_t tctrl_read_data(struct tctrl_softc *);
static int tctrl_intr(void *);
static void tctrl_setup_bitport(void);
static void tctrl_setup_bitport_nop(void);
static void tctrl_read_ext_status(void);
static void tctrl_read_event_status(struct tctrl_softc *);
static int tctrl_apm_record_event(struct tctrl_softc *, u_int);
static void tctrl_init_lcd(void);

static void tctrl_sensor_setup(struct tctrl_softc *);
static void tctrl_refresh(struct sysmon_envsys *, envsys_data_t *);

static void tctrl_power_button_pressed(void *);
static void tctrl_lid_state(struct tctrl_softc *);
static void tctrl_ac_state(struct tctrl_softc *);

static int tctrl_powerfail(void *);

static void tctrl_event_thread(void *);
void tctrl_update_lcd(struct tctrl_softc *);

static void tctrl_lock(struct tctrl_softc *);
static void tctrl_unlock(struct tctrl_softc *);

CFATTACH_DECL_NEW(tctrl, sizeof(struct tctrl_softc),
    tctrl_match, tctrl_attach, NULL, NULL);

static int tadpole_request(struct tctrl_req *, int, int);

/* XXX wtf is this? see i386/apm.c */
int tctrl_apm_evindex;

static int
tctrl_match(device_t parent, cfdata_t cf, void *aux)
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
tctrl_attach(device_t parent, device_t self, void *aux)
{
	struct tctrl_softc *sc = device_private(self);
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	unsigned int i, v;

	/* We're living on a sbus slot that looks like an obio that
	 * looks like an sbus slot.
	 */
	sc->sc_dev = self;
	sc->sc_memt = sa->sa_bustag;
	if (sbus_bus_map(sc->sc_memt,
			 sa->sa_slot,
			 sa->sa_offset - TS102_REG_UCTRL_INT,
			 sa->sa_size,
			 BUS_SPACE_MAP_LINEAR, &sc->sc_memh) != 0) {
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
					 tctrl_intr, sc);
		evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
				     device_xname(sc->sc_dev), "intr");
	}

	/* See what the external status is */
	sc->sc_ext_status = 0;
	tctrl_read_ext_status();
	if (sc->sc_ext_status != 0) {
		const char *sep;

		printf("%s: ", device_xname(sc->sc_dev));
		v = sc->sc_ext_status;
		for (i = 0, sep = ""; v != 0; i++, v >>= 1) {
			if (v & 1) {
				printf("%s%s", sep, tctrl_ext_statuses[i]);
				sep = ", ";
			}
		}
		printf("\n");
	}

	/* Get a current of the control bitport */
	tctrl_setup_bitport_nop();
	tctrl_write(sc, TS102_REG_UCTRL_INT,
		    TS102_UCTRL_INT_RXNE_REQ|TS102_UCTRL_INT_RXNE_MSK);
	sc->sc_lid = (sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN) == 0;
	sc->sc_power_state = PWR_RESUME;
	
	sc->sc_extvga = (sc->sc_ext_status &
	    TS102_EXT_STATUS_EXTERNAL_VGA_ATTACHED) != 0;
	sc->sc_video_callback = NULL;
	
	
	sc->sc_wantdata = 0;
	sc->sc_event_count = 0;
	sc->sc_ext_pending = 0;
		sc->sc_ext_pending = 0;

	mutex_init(&sc->sc_requestlock, MUTEX_DEFAULT, IPL_NONE);
	selinit(&sc->sc_rsel);

	/* setup sensors and register the power button */
	tctrl_sensor_setup(sc);
	tctrl_lid_state(sc);
	tctrl_ac_state(sc);

	/* initialize the LCD */
	tctrl_init_lcd();

	/* initialize sc_lcdstate */
	sc->sc_lcdstate = 0;
	sc->sc_lcdwanted = 0;
	tadpole_set_lcd(2, 0);
	
	/* fire up the LCD event thread */
	sc->sc_events = 0;

	if (kthread_create(PRI_NONE, 0, NULL, tctrl_event_thread, sc,
	    &sc->sc_thread, "%s", device_xname(sc->sc_dev)) != 0) {
		printf("%s: unable to create event kthread",
		    device_xname(sc->sc_dev));
	}
}

static int
tctrl_intr(void *arg)
{
	struct tctrl_softc *sc = arg;
	unsigned int v, d;
	int progress = 0;

    again:
	/* find out the cause(s) of the interrupt */
	v = tctrl_read(sc, TS102_REG_UCTRL_STS) & TS102_UCTRL_STS_MASK;

	/* clear the cause(s) of the interrupt */
	tctrl_write(sc, TS102_REG_UCTRL_STS, v);

	v &= ~(TS102_UCTRL_STS_RXO_STA|TS102_UCTRL_STS_TXE_STA);
	if (sc->sc_cmdoff >= sc->sc_cmdlen) {
		v &= ~TS102_UCTRL_STS_TXNF_STA;
		if (tctrl_read(sc, TS102_REG_UCTRL_INT) &
		    TS102_UCTRL_INT_TXNF_REQ) {
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
				/* 
				 * external event,
				 * set a flag and wakeup the event thread 
				 */
				sc->sc_ext_pending = 1;
			} else {
				printf("%s: (op=0x%02x): unexpected data (0x%02x)\n",
					device_xname(sc->sc_dev), sc->sc_op, d);
			}
			goto again;
		case TCTRL_ACK:
			if (d != 0xfe) {
				printf("%s: (op=0x%02x): unexpected ack value (0x%02x)\n",
					device_xname(sc->sc_dev), sc->sc_op, d);
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
			       device_xname(sc->sc_dev), sc->sc_op, d, sc->sc_state);
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
			printf("%s: op=0x%02x(l=%u)", device_xname(sc->sc_dev),
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

	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);
	req.cmdbuf[0] = TS102_OP_CTL_BITPORT;
	req.cmdbuf[1] = 0xff;
	req.cmdbuf[2] = 0x00;
	req.cmdlen = 3;
	req.rsplen = 2;
	tadpole_request(&req, 1, 0);
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

	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);
	s = splts102();
	req.cmdbuf[2] = 0;
	if ((sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN)
	    || (!sc->sc_tft_on)) {
		req.cmdbuf[2] = TS102_BITPORT_TFTPWR;
	}
	req.cmdbuf[0] = TS102_OP_CTL_BITPORT;
	req.cmdbuf[1] = ~TS102_BITPORT_TFTPWR;
	req.cmdlen = 3;
	req.rsplen = 2;
	tadpole_request(&req, 1, 0);
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
	tadpole_request(&req, 1, 0);

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
	tadpole_request(&req, 1, 0);

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
	tadpole_request(&req, 1, 0);

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
	tadpole_request(&req, 1, 0);

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
	tadpole_request(&req, 1, 0);

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
	tadpole_request(&req, 1, 0);

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
	tadpole_request(&req, 1, 0);
}

/* sc_lcdwanted -> lcd_state */
void
tctrl_update_lcd(struct tctrl_softc *sc)
{
	struct tctrl_req req;
	int s;

	s = splhigh();
	if (sc->sc_lcdwanted == sc->sc_lcdstate) {
		splx(s);
		return;
	}
	sc->sc_lcdstate = sc->sc_lcdwanted;
	splx(s);
	
	/*
	 * the mask setup on this particular command is *very* bizzare
	 * and totally undocumented.
	 */
	req.cmdbuf[0] = TS102_OP_CTL_LCD;

	/* leave caps-lock alone */
	req.cmdbuf[2] = (u_int8_t)(sc->sc_lcdstate & 0xfe);
	req.cmdbuf[3] = (u_int8_t)((sc->sc_lcdstate & 0x100)>>8);

	req.cmdbuf[1] = 1;
	req.cmdbuf[4] = 0;
	

	/* XXX this thing is weird.... */
	req.cmdlen = 3;
	req.rsplen = 2;
	
	/* below are the values one would expect but which won't work */
#if 0
	req.cmdlen = 5;
	req.rsplen = 4;
#endif
	tadpole_request(&req, 1, 0);
}


/*
 * set the blinken-lights on the lcd.  what:
 * what = 0 off,  what = 1 on,  what = 2 toggle
 */

void
tadpole_set_lcd(int what, unsigned short which)
{
	struct tctrl_softc *sc;
	int s;

	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);

	s = splhigh();
	switch (what) {
		case 0:
			sc->sc_lcdwanted &= ~which;
			break;
		case 1:
			sc->sc_lcdwanted |= which;
			break;
		case 2:
			sc->sc_lcdwanted ^= which;
			break;
	}
	splx(s);
}

static void
tctrl_read_ext_status(void)
{
	struct tctrl_softc *sc;
	struct tctrl_req req;
	int s;

	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);
	req.cmdbuf[0] = TS102_OP_RD_EXT_STATUS;
	req.cmdlen = 1;
	req.rsplen = 3;
#ifdef TCTRLDEBUG
	printf("pre read: sc->sc_ext_status = 0x%x\n", sc->sc_ext_status);
#endif
	tadpole_request(&req, 1, 0);
	s = splts102();
	sc->sc_ext_status = (req.rspbuf[0] << 8) + req.rspbuf[1];
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
tctrl_apm_record_event(struct tctrl_softc *sc, u_int event_type)
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
		selnotify(&sc->sc_rsel, 0, 0);
		return(sc->sc_flags & TCTRL_APM_CTLOPEN) ? 0 : 1;
	}
	return(1);
}

static void
tctrl_read_event_status(struct tctrl_softc *sc)
{
	struct tctrl_req req;
	int s;
	uint32_t v;

	req.cmdbuf[0] = TS102_OP_RD_EVENT_STATUS;
	req.cmdlen = 1;
	req.rsplen = 3;
	tadpole_request(&req, 1, 0);
	s = splts102();
	v = req.rspbuf[0] * 256 + req.rspbuf[1];
#ifdef TCTRLDEBUG
	printf("event: %x\n",v);
#endif
	if (v & TS102_EVENT_STATUS_POWERON_BTN_PRESSED) {
		printf("%s: Power button pressed\n",device_xname(sc->sc_dev));
		tctrl_powerfail(sc);
	}
	if (v & TS102_EVENT_STATUS_SHUTDOWN_REQUEST) {
		printf("%s: SHUTDOWN REQUEST!\n", device_xname(sc->sc_dev));
		tctrl_powerfail(sc);
	}
	if (v & TS102_EVENT_STATUS_VERY_LOW_POWER_WARNING) {
/*printf("%s: VERY LOW POWER WARNING!\n", device_xname(sc->sc_dev));*/
/* according to a tadpole header, and observation */
#ifdef TCTRLDEBUG
		printf("%s: Battery charge level change\n",
		    device_xname(sc->sc_dev));
#endif
	}
	if (v & TS102_EVENT_STATUS_LOW_POWER_WARNING) {
		if (tctrl_apm_record_event(sc, APM_BATTERY_LOW))
			printf("%s: LOW POWER WARNING!\n", device_xname(sc->sc_dev));
	}
	if (v & TS102_EVENT_STATUS_DC_STATUS_CHANGE) {
		splx(s);
		tctrl_read_ext_status();
		tctrl_ac_state(sc);
		s = splts102();
		if (tctrl_apm_record_event(sc, APM_POWER_CHANGE))
			printf("%s: main power %s\n", device_xname(sc->sc_dev),
			    (sc->sc_ext_status &
			    TS102_EXT_STATUS_MAIN_POWER_AVAILABLE) ?
			    "restored" : "removed");
	}
	if (v & TS102_EVENT_STATUS_LID_STATUS_CHANGE) {
		splx(s);
		tctrl_read_ext_status();
		tctrl_lid_state(sc);
		tctrl_setup_bitport();
#ifdef TCTRLDEBUG
		printf("%s: lid %s\n", device_xname(sc->sc_dev),
		    (sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN)
		    ? "closed" : "opened");
#endif
	}
	if (v & TS102_EVENT_STATUS_EXTERNAL_VGA_STATUS_CHANGE) {
		int vga;
		splx(s);
		tctrl_read_ext_status();
		vga = (sc->sc_ext_status &
		    TS102_EXT_STATUS_EXTERNAL_VGA_ATTACHED) != 0;
		if (vga != sc->sc_extvga) {
			sc->sc_extvga = vga;
			if (sc->sc_video_callback != NULL) {
				sc->sc_video_callback(
				    sc->sc_video_callback_cookie,
				    sc->sc_extvga);
			}
		}
	}
#ifdef DIAGNOSTIC
	if (v & TS102_EVENT_STATUS_EXT_MOUSE_STATUS_CHANGE) {
		splx(s);
		tctrl_read_ext_status();
		if (sc->sc_ext_status &
		    TS102_EXT_STATUS_EXTERNAL_MOUSE_ATTACHED) {
			printf("tctrl: external mouse detected\n");
		}
	}
#endif
	sc->sc_ext_pending = 0;
	splx(s);
}

static void
tctrl_lock(struct tctrl_softc *sc)
{

	mutex_enter(&sc->sc_requestlock);
}

static void
tctrl_unlock(struct tctrl_softc *sc)
{

	mutex_exit(&sc->sc_requestlock);
}

int
tadpole_request(struct tctrl_req *req, int spin, int sleep)
{
	struct tctrl_softc *sc;
	int i, s;

	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);
	if (!sc)
		return ENODEV;

	tctrl_lock(sc);

	if (spin)
		s = splhigh();
	else
		s = splts102();
	sc->sc_flags |= TCTRL_SEND_REQUEST;
	memcpy(sc->sc_cmdbuf, req->cmdbuf, req->cmdlen);
#ifdef DIAGNOSTIC
	if (sc->sc_wantdata != 0) {
		splx(s);
		printf("tctrl: we lost the race\n");
		tctrl_unlock(sc);
		return EAGAIN;
	}
#endif
	sc->sc_wantdata = 1;
	sc->sc_rsplen = req->rsplen;
	sc->sc_cmdlen = req->cmdlen;
	sc->sc_cmdoff = sc->sc_rspoff = 0;

	/* we spin for certain commands, like poweroffs */
	if (spin) {
/*		for (i = 0; i < 30000; i++) {*/
		i = 0;
		while ((sc->sc_wantdata == 1) && (i < 30000)) {
			tctrl_intr(sc);
			DELAY(1);
			i++;
		}
#ifdef DIAGNOSTIC
		if (i >= 30000) {
			printf("tctrl: timeout busy waiting for micro controller request!\n");
			sc->sc_wantdata = 0;
			splx(s);
			tctrl_unlock(sc);
			return EAGAIN;
		}
#endif
	} else {
		int timeout = 5 * (sc->sc_rsplen + sc->sc_cmdlen);
		tctrl_intr(sc);
		i = 0;
		while (((sc->sc_rspoff != sc->sc_rsplen) ||
		    (sc->sc_cmdoff != sc->sc_cmdlen)) &&
		    (i < timeout))
			if (sleep) {
				tsleep(sc, PWAIT, "tctrl_data", 15);
				i++;
			} else
				DELAY(1);
#ifdef DIAGNOSTIC
		if (i >= timeout) {
			printf("tctrl: timeout waiting for microcontroller request\n");
			sc->sc_wantdata = 0;
			splx(s);
			tctrl_unlock(sc);
			return EAGAIN;
		}
#endif
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
	
	tctrl_unlock(sc);
	return 0;
}

void
tadpole_powerdown(void)
{
	struct tctrl_req req;

	req.cmdbuf[0] = TS102_OP_ADMIN_POWER_OFF;
	req.cmdlen = 1;
	req.rsplen = 1;
	tadpole_request(&req, 1, 0);
}

void
tadpole_set_video(int enabled)
{
	struct tctrl_softc *sc;
	struct tctrl_req req;
	int s;

	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);
	while (sc->sc_wantdata != 0)
		DELAY(1);
	s = splts102();
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
		tadpole_request(&req, 1, 0);
		sc->sc_bitport =
		    (req.rspbuf[0] & req.cmdbuf[1]) ^ req.cmdbuf[2];
	}
	splx(s);
}

static void
tctrl_write_data(struct tctrl_softc *sc, uint8_t v)
{
	unsigned int i;

	for (i = 0; i < 100; i++)  {
		if (TS102_UCTRL_STS_TXNF_STA &
		    tctrl_read(sc, TS102_REG_UCTRL_STS))
			break;
	}
	tctrl_write(sc, TS102_REG_UCTRL_DATA, v);
}

static uint8_t
tctrl_read_data(struct tctrl_softc *sc)
{
	unsigned int i, v;

	for (i = 0; i < 100000; i++) {
		if (TS102_UCTRL_STS_RXNE_STA &
		    tctrl_read(sc, TS102_REG_UCTRL_STS))
			break;
		DELAY(1);
	}

	v = tctrl_read(sc, TS102_REG_UCTRL_DATA);
	tctrl_write(sc, TS102_REG_UCTRL_STS, TS102_UCTRL_STS_RXNE_STA);
	return v;
}

static uint8_t
tctrl_read(struct tctrl_softc *sc, bus_size_t off)
{

	sc->sc_junk = bus_space_read_1(sc->sc_memt, sc->sc_memh, off);
	return sc->sc_junk;
}

static void
tctrl_write(struct tctrl_softc *sc, bus_size_t off, uint8_t v)
{

	sc->sc_junk = v;
	bus_space_write_1(sc->sc_memt, sc->sc_memh, off, v);
}

int
tctrlopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = (minor(dev)&0xf0);
	int ctl = (minor(dev)&0x0f);
	struct tctrl_softc *sc;

	if (unit >= tctrl_cd.cd_ndevs)
		return(ENXIO);
	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);
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
tctrlclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	int ctl = (minor(dev)&0x0f);
	struct tctrl_softc *sc;

	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);
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
tctrlioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct tctrl_req req, *reqn;
	struct tctrl_pwr *pwrreq;
	struct apm_power_info *powerp;
	struct apm_event_info *evp;
	struct tctrl_softc *sc;
	int i;
	uint8_t c;

	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);
	if (!sc)
		return ENXIO;

        switch (cmd) {

	case APM_IOC_STANDBY:
		/* turn off backlight and so on ? */
		
		return 0; /* for now */

	case APM_IOC_SUSPEND:
		/* not sure what to do here - we can't really suspend */
		
		return 0; /* for now */

	case OAPM_IOC_GETPOWER:
	case APM_IOC_GETPOWER:
		powerp = (struct apm_power_info *)data;
		req.cmdbuf[0] = TS102_OP_RD_INT_CHARGE_RATE;
		req.cmdlen = 1;
		req.rsplen = 2;
		tadpole_request(&req, 0, l->l_proc ? 1 : 0);
		if (req.rspbuf[0] > 0x00)
			powerp->battery_state = APM_BATT_CHARGING;
		req.cmdbuf[0] = TS102_OP_RD_INT_CHARGE_LEVEL;
		req.cmdlen = 1;
		req.rsplen = 3;
		tadpole_request(&req, 0, l->l_proc ? 1 : 0);
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
		
		if (sc->sc_ext_status & TS102_EXT_STATUS_MAIN_POWER_AVAILABLE)
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
		if ((i = kauth_authorize_device_passthru(l->l_cred,
		    dev, KAUTH_REQ_DEVICE_RAWIO_PASSTHRU_ALL, data)) != 0 &&
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
		tadpole_request(reqn, 0, l->l_proc ? 1 : 0);
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
tctrlpoll(dev_t dev, int events, struct lwp *l)
{
	struct tctrl_softc *sc = device_lookup_private(&tctrl_cd,
						       TCTRL_STD_DEV);
	int revents = 0;

	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_event_count)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(l, &sc->sc_rsel);
	}

	return (revents);
}

static void
filt_tctrlrdetach(struct knote *kn)
{
	struct tctrl_softc *sc = kn->kn_hook;
	int s;

	s = splts102();
	SLIST_REMOVE(&sc->sc_rsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_tctrlread(struct knote *kn, long hint)
{
	struct tctrl_softc *sc = kn->kn_hook;

	kn->kn_data = sc->sc_event_count;
	return (kn->kn_data > 0);
}

static const struct filterops tctrlread_filtops =
	{ 1, NULL, filt_tctrlrdetach, filt_tctrlread };

int
tctrlkqfilter(dev_t dev, struct knote *kn)
{
	struct tctrl_softc *sc = device_lookup_private(&tctrl_cd,
						       TCTRL_STD_DEV);
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.sel_klist;
		kn->kn_fop = &tctrlread_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = sc;

	s = splts102();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

static void
tctrl_sensor_setup(struct tctrl_softc *sc)
{
	int i, error;

	sc->sc_sme = sysmon_envsys_create();

	/* case temperature */
	(void)strlcpy(sc->sc_sensor[0].desc, "Case temperature",
	    sizeof(sc->sc_sensor[0].desc));
	sc->sc_sensor[0].units = ENVSYS_STEMP;
	sc->sc_sensor[0].state = ENVSYS_SINVALID;

	/* battery voltage */
	(void)strlcpy(sc->sc_sensor[1].desc, "Internal battery voltage",
	    sizeof(sc->sc_sensor[1].desc));
	sc->sc_sensor[1].units = ENVSYS_SVOLTS_DC;
	sc->sc_sensor[1].state = ENVSYS_SINVALID;

	/* DC voltage */
	(void)strlcpy(sc->sc_sensor[2].desc, "DC-In voltage",
	    sizeof(sc->sc_sensor[2].desc));
	sc->sc_sensor[2].units = ENVSYS_SVOLTS_DC;
	sc->sc_sensor[2].state = ENVSYS_SINVALID;

	for (i = 0; i < ENVSYS_NUMSENSORS; i++) {
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
						&sc->sc_sensor[i])) {
			sysmon_envsys_destroy(sc->sc_sme);
			return;
		}
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = tctrl_refresh;

	if ((error = sysmon_envsys_register(sc->sc_sme)) != 0) {
		printf("%s: couldn't register sensors (%d)\n",
		    device_xname(sc->sc_dev), error);
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	/* now register the power button */

	sysmon_task_queue_init();

	sc->sc_powerpressed = 0;
	memset(&sc->sc_sm_pbutton, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_pbutton.smpsw_name = device_xname(sc->sc_dev);
	sc->sc_sm_pbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_sm_pbutton) != 0)
		printf("%s: unable to register power button with sysmon\n",
		    device_xname(sc->sc_dev));

	memset(&sc->sc_sm_lid, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_lid.smpsw_name = device_xname(sc->sc_dev);
	sc->sc_sm_lid.smpsw_type = PSWITCH_TYPE_LID;
	if (sysmon_pswitch_register(&sc->sc_sm_lid) != 0)
		printf("%s: unable to register lid switch with sysmon\n",
		    device_xname(sc->sc_dev));

	memset(&sc->sc_sm_ac, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_ac.smpsw_name = device_xname(sc->sc_dev);
	sc->sc_sm_ac.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&sc->sc_sm_ac) != 0)
		printf("%s: unable to register AC adaptor with sysmon\n",
		    device_xname(sc->sc_dev));
}

static void
tctrl_power_button_pressed(void *arg)
{
	struct tctrl_softc *sc = arg;

	sysmon_pswitch_event(&sc->sc_sm_pbutton, PSWITCH_EVENT_PRESSED);
	sc->sc_powerpressed = 0;
}

static void
tctrl_lid_state(struct tctrl_softc *sc)
{
	int state;
	
	state = (sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN) ? 
	    PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED;
	sysmon_pswitch_event(&sc->sc_sm_lid, state);
}

static void
tctrl_ac_state(struct tctrl_softc *sc)
{
	int state;
	
	state = (sc->sc_ext_status & TS102_EXT_STATUS_MAIN_POWER_AVAILABLE) ? 
	    PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED;
	sysmon_pswitch_event(&sc->sc_sm_ac, state);
}

static int
tctrl_powerfail(void *arg)
{
	struct tctrl_softc *sc = (struct tctrl_softc *)arg;

	/*
	 * We lost power. Queue a callback with thread context to
	 * handle all the real work.
	 */
	if (sc->sc_powerpressed == 0) {
		sc->sc_powerpressed = 1;
		sysmon_task_queue_sched(0, tctrl_power_button_pressed, sc);
	}
	return (1);
}

static void
tctrl_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	/*struct tctrl_softc *sc = sme->sme_cookie;*/
	struct tctrl_req req;
	int sleepable;
	int i;

	i = edata->sensor;
	sleepable = curlwp ? 1 : 0;

	switch (i)
	{
		case 0:	/* case temperature */
			req.cmdbuf[0] = TS102_OP_RD_CURRENT_TEMP;
			req.cmdlen = 1;
			req.rsplen = 2;
			tadpole_request(&req, 0, sleepable);
			edata->value_cur =             /* 273160? */
			    (uint32_t)((int)((int)req.rspbuf[0] - 32) * 5000000
			    / 9 + 273150000);
			req.cmdbuf[0] = TS102_OP_RD_MAX_TEMP;
			req.cmdlen = 1;
			req.rsplen = 2;
			tadpole_request(&req, 0, sleepable);
			edata->value_max =
			    (uint32_t)((int)((int)req.rspbuf[0] - 32) * 5000000
			    / 9 + 273150000);
			edata->flags |= ENVSYS_FVALID_MAX;
			req.cmdbuf[0] = TS102_OP_RD_MIN_TEMP;
			req.cmdlen = 1;
			req.rsplen = 2;
			tadpole_request(&req, 0, sleepable);
			edata->value_min =
			    (uint32_t)((int)((int)req.rspbuf[0] - 32) * 5000000
			    / 9 + 273150000);
			edata->flags |= ENVSYS_FVALID_MIN;
			edata->units = ENVSYS_STEMP;
			break;

		case 1: /* battery voltage */
			{
				edata->units = ENVSYS_SVOLTS_DC;
				req.cmdbuf[0] = TS102_OP_RD_INT_BATT_VLT;
				req.cmdlen = 1;
				req.rsplen = 2;
				tadpole_request(&req, 0, sleepable);
				edata->value_cur = (int32_t)req.rspbuf[0] *
				    1000000 / 11;
			}
			break;
		case 2: /* DC voltage */
			{
				edata->units = ENVSYS_SVOLTS_DC;
				req.cmdbuf[0] = TS102_OP_RD_DC_IN_VLT;
				req.cmdlen = 1;
				req.rsplen = 2;
				tadpole_request(&req, 0, sleepable);
				edata->value_cur = (int32_t)req.rspbuf[0] *
				    1000000 / 11;
			}
			break;
	}
	edata->state = ENVSYS_SVALID;
}

static void
tctrl_event_thread(void *v)
{
	struct tctrl_softc *sc = v;
	device_t dv;
	struct sd_softc *sd = NULL;
	struct lance_softc *le = NULL;
	int ticks = hz/2;
	int rcount, wcount;
	int s;
	
	while (sd == NULL) {
		dv = device_find_by_xname("sd0");
		if (dv != NULL)
			sd = device_private(dv);
		else
			tsleep(&sc->sc_events, PWAIT, "probe_disk", hz);
	}			
	dv = device_find_by_xname("le0");
	if (dv != NULL)
		le = device_private(dv);
	printf("found %s\n", device_xname(sd->sc_dev));
	rcount = sd->sc_dk.dk_stats->io_rxfer;
	wcount = sd->sc_dk.dk_stats->io_wxfer;

	tctrl_read_event_status(sc);
	
	while (1) {
		tsleep(&sc->sc_events, PWAIT, "tctrl_event", ticks);
		s = splhigh();
		if ((rcount != sd->sc_dk.dk_stats->io_rxfer) || 
		    (wcount != sd->sc_dk.dk_stats->io_wxfer)) {
			rcount = sd->sc_dk.dk_stats->io_rxfer;
			wcount = sd->sc_dk.dk_stats->io_wxfer;
			sc->sc_lcdwanted |= TS102_LCD_DISK_ACTIVE;
		} else
			sc->sc_lcdwanted &= ~TS102_LCD_DISK_ACTIVE;
		if (le != NULL) {
			if (le->sc_havecarrier != 0) {
				sc->sc_lcdwanted |= TS102_LCD_LAN_ACTIVE;
			} else
				sc->sc_lcdwanted &= ~TS102_LCD_LAN_ACTIVE;
		}
		splx(s);
		tctrl_update_lcd(sc);
		if (sc->sc_ext_pending)
			tctrl_read_event_status(sc);
	}
}

void
tadpole_register_callback(void (*callback)(void *, int), void *cookie)
{
	struct tctrl_softc *sc;

	sc = device_lookup_private(&tctrl_cd, TCTRL_STD_DEV);
	sc->sc_video_callback = callback;
	sc->sc_video_callback_cookie = cookie;
	if (sc->sc_video_callback != NULL) {
		sc->sc_video_callback(sc->sc_video_callback_cookie,
		    sc->sc_extvga);
	}
}
