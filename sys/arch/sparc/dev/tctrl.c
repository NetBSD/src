/*	$NetBSD: tctrl.c,v 1.1 1999/08/09 18:39:58 matt Exp $	*/

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

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/bus.h>

#include <sparc/dev/ts102reg.h>
#include <sparc/dev/tctrlvar.h>

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
	struct device sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	unsigned int sc_junk;
	unsigned int sc_ext_status;
	unsigned int sc_pending;
#define	TCTRL_SEND_BITPORT		0x0001
#define	TCTRL_SEND_POWEROFF		0x0002
#define	TCTRL_SEND_RD_EXT_STATUS	0x0004
#define	TCTRL_SEND_RD_EVENT_STATUS	0x0008
	enum { TCTRL_IDLE, TCTRL_ARGS,
		TCTRL_ACK, TCTRL_DATA } sc_state;
	u_int8_t sc_cmdbuf[16];
	u_int8_t sc_rspbuf[16];
	u_int8_t sc_bitport;
	u_int8_t sc_tft_on;
	u_int8_t sc_op;
	u_int8_t sc_cmdoff;
	u_int8_t sc_cmdlen;
	u_int8_t sc_rspoff;
	u_int8_t sc_rsplen;

	struct evcnt sc_intrcnt;	/* interrupt counting */
};

static int tctrl_match(struct device *parent, struct cfdata *cf, void *aux);
static void tctrl_attach(struct device *parent, struct device *self, void *aux);

static void tctrl_write(struct tctrl_softc *sc, bus_size_t off, u_int8_t v);
static u_int8_t tctrl_read(struct tctrl_softc *sc, bus_size_t off);
static void tctrl_write_data(struct tctrl_softc *sc, u_int8_t v);
static u_int8_t tctrl_read_data(struct tctrl_softc *sc);
static int tctrl_intr(void *arg);
static void tctrl_setup_bitport(struct tctrl_softc *sc);
static void tctrl_process_response(struct tctrl_softc *sc);

struct cfattach tctrl_ca = {
	sizeof(struct tctrl_softc), tctrl_match, tctrl_attach
};

extern struct cfdriver tctrl_cd;

static int
tctrl_match(struct device *parent, struct cfdata *cf, void *aux)
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
tctrl_attach(struct device *parent, struct device *self, void *aux)
{
	struct tctrl_softc *sc = (void *)self;
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	unsigned int ack, msb, lsb;
	unsigned int i, v;
	const char *sep;

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

	sc->sc_tft_on = 1;
	/* clear any pending data.
	 */
	for (i = 0; i < 10000; i++) {
		if ((TS102_UCTRL_STS_RXNE_REQ & tctrl_read(sc, TS102_REG_UCTRL_STS)) == 0) {
			break;
		}
		v = tctrl_read(sc, TS102_REG_UCTRL_DATA);
		tctrl_write(sc, TS102_REG_UCTRL_STS, TS102_UCTRL_STS_RXNE_REQ);
	}

	printf("\n");

	tctrl_write_data(sc, TS102_OP_RD_EXT_STATUS);
	ack = tctrl_read_data(sc);
	msb = tctrl_read_data(sc);
	lsb = tctrl_read_data(sc);
	sc->sc_ext_status = v = msb * 256 + lsb;

	if (sc->sc_ext_status) {
		printf("%s: ", sc->sc_dev.dv_xname);
		for (i = 0, sep = ""; v != 0; i++, v >>= 1) {
			if (v & 1) {
				printf("%s%s", sep, tctrl_ext_statuses[i]);
				sep = ", ";
			}
		}
		printf("\n");
	}

	sc->sc_pending |= TCTRL_SEND_BITPORT;

	(void)bus_intr_establish(sc->sc_memt, sa->sa_pri, 0, tctrl_intr, sc);
	evcnt_attach(&sc->sc_dev, "intr", &sc->sc_intrcnt);

	tctrl_write(sc, TS102_REG_UCTRL_STS,
		    tctrl_read(sc, TS102_REG_UCTRL_STS)
		    |TS102_UCTRL_INT_RXNE_MSK|TS102_UCTRL_INT_TXNF_MSK);
	tctrl_write(sc, TS102_REG_UCTRL_INT,
		    TS102_UCTRL_INT_RXNE_REQ|TS102_UCTRL_INT_TXNF_REQ|
		    TS102_UCTRL_INT_RXNE_MSK|TS102_UCTRL_INT_TXNF_MSK);
#if 0
	printf("%s: int=0x%02x sts=0x%02x\n", sc->sc_dev.dv_xname,
		tctrl_read(sc, TS102_REG_UCTRL_INT),
		tctrl_read(sc, TS102_REG_UCTRL_STS));
#endif
}

static int
tctrl_intr(void *arg)
{
	struct tctrl_softc *sc = arg;
	unsigned int v, d;
	int progress = 0;

    again:
	/* find out the cause(s) of the interrupt */
	v = tctrl_read(sc, TS102_REG_UCTRL_STS);

	/* clear the cause(s) of the interrupt */
	tctrl_write(sc, TS102_REG_UCTRL_STS, v);

	v &= ~(TS102_UCTRL_STS_RXO_REQ|TS102_UCTRL_STS_TXE_REQ);
	if (sc->sc_cmdoff >= sc->sc_cmdlen) {
		v &= ~TS102_UCTRL_STS_TXNF_REQ;
	}
	if ((v == 0) && (sc->sc_pending == 0 || sc->sc_state != TCTRL_IDLE)) {
		return progress;
	}

	progress = 1;
	if (v & TS102_UCTRL_STS_RXNE_REQ) {
		d = tctrl_read_data(sc);
		switch (sc->sc_state) {
		case TCTRL_IDLE:
			if (d == 0xfb) {
				sc->sc_pending |= TCTRL_SEND_RD_EVENT_STATUS;
			}
			break;
		case TCTRL_ACK:
			if (d != 0xfe) {
				printf("%s: (op=0x%02x): unexpected value for ack (0x%02x)\n",
					sc->sc_dev.dv_xname, sc->sc_op, d);
			}
			sc->sc_state = sc->sc_rsplen ? TCTRL_DATA : TCTRL_IDLE;
			sc->sc_rspoff = 0;
			sc->sc_rsplen--;
			break;
		case TCTRL_DATA:
			sc->sc_rspbuf[sc->sc_rspoff++] = d;
			if (sc->sc_rspoff == sc->sc_rsplen) {
				sc->sc_state = TCTRL_IDLE;
				if (sc->sc_rsplen > 0) {
					tctrl_process_response(sc);
				}
			}
			break;
		default:
			printf("%s: (op=0x%02x): unexpected data (0x%02x) in state %d\n",
			       sc->sc_dev.dv_xname, sc->sc_op, d, sc->sc_state);
			break;
		}
	}
	if (sc->sc_state == TCTRL_IDLE) {
		sc->sc_cmdoff = 0;
		if (sc->sc_pending & TCTRL_SEND_POWEROFF) {
			sc->sc_pending &= ~TCTRL_SEND_POWEROFF;
			sc->sc_cmdbuf[0] = TS102_OP_ADMIN_POWER_OFF;
			sc->sc_cmdlen = 1;
			sc->sc_rsplen = 0;
		} else if (sc->sc_pending & TCTRL_SEND_RD_EVENT_STATUS) {
			sc->sc_pending &= ~TCTRL_SEND_RD_EVENT_STATUS;
			sc->sc_cmdbuf[0] = TS102_OP_RD_EVENT_STATUS;
			sc->sc_cmdlen = 1;
			sc->sc_rsplen = 3;
		} else if (sc->sc_pending & TCTRL_SEND_RD_EXT_STATUS) {
			sc->sc_pending &= ~TCTRL_SEND_RD_EXT_STATUS;
			sc->sc_cmdbuf[0] = TS102_OP_RD_EXT_STATUS;
			sc->sc_cmdlen = 1;
			sc->sc_rsplen = 3;
		} else if (sc->sc_pending & TCTRL_SEND_BITPORT) {
			sc->sc_pending &= ~TCTRL_SEND_BITPORT;
			tctrl_setup_bitport(sc);
		} 
		if (sc->sc_cmdlen > 0) {
			tctrl_write(sc, TS102_REG_UCTRL_INT,
				tctrl_read(sc, TS102_REG_UCTRL_INT)
				|TS102_UCTRL_INT_TXNF_MSK
				|TS102_UCTRL_INT_TXNF_REQ);
			v = tctrl_read(sc, TS102_REG_UCTRL_STS);
		}
	}
	if ((sc->sc_cmdoff < sc->sc_cmdlen) && (v & TS102_UCTRL_STS_TXNF_REQ)) {
		tctrl_write_data(sc, sc->sc_cmdbuf[sc->sc_cmdoff++]);
		if (sc->sc_cmdoff == sc->sc_cmdlen) {
			sc->sc_state = sc->sc_rsplen ? TCTRL_ACK : TCTRL_IDLE;
			tctrl_write(sc, TS102_REG_UCTRL_INT,
				tctrl_read(sc, TS102_REG_UCTRL_INT)
				& (~TS102_UCTRL_INT_TXNF_MSK
				   |TS102_UCTRL_INT_TXNF_REQ));
		} else if (sc->sc_state == TCTRL_IDLE) {
			sc->sc_op = sc->sc_cmdbuf[0];
			sc->sc_state = TCTRL_ARGS;
		}
	}
	goto again;
}

static void
tctrl_setup_bitport(struct tctrl_softc *sc)
{
	sc->sc_cmdbuf[0] = TS102_OP_CTL_BITPORT;
	sc->sc_cmdbuf[1] = ~TS102_BITPORT_TFTPWR;
	if (sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN) {
		sc->sc_cmdbuf[2] = 0;
	} else {
		sc->sc_cmdbuf[2] = sc->sc_tft_on ? TS102_BITPORT_TFTPWR : 0;
	}
	sc->sc_cmdlen = 3;
	sc->sc_rsplen = 2;
}

static void
tctrl_process_response(struct tctrl_softc *sc)
{
	switch (sc->sc_op) {
	case TS102_OP_RD_EXT_STATUS: {
		sc->sc_ext_status = sc->sc_rspbuf[0] * 256 + sc->sc_rspbuf[1];
		break;
	}
	case TS102_OP_RD_EVENT_STATUS: {
		unsigned int v = sc->sc_rspbuf[0] * 256 + sc->sc_rspbuf[1];
		if (v & TS102_EVENT_STATUS_SHUTDOWN_REQUEST) {
			printf("%s: SHUTDOWN REQUEST!\n", sc->sc_dev.dv_xname);
		}
		if (v & TS102_EVENT_STATUS_VERY_LOW_POWER_WARNING) {
			printf("%s: VERY LOW POWER WARNING!\n", sc->sc_dev.dv_xname);
		}
		if (v & TS102_EVENT_STATUS_LOW_POWER_WARNING) {
			printf("%s: LOW POWER WARNING!\n", sc->sc_dev.dv_xname);
		}
		if (v & TS102_EVENT_STATUS_DC_STATUS_CHANGE) {
			sc->sc_pending |= TCTRL_SEND_RD_EXT_STATUS;
			printf("%s: main power %s\n", sc->sc_dev.dv_xname,
			       (sc->sc_ext_status & TS102_EXT_STATUS_MAIN_POWER_AVAILABLE) ? "removed" : "restored");
		}
		if (v & TS102_EVENT_STATUS_LID_STATUS_CHANGE) {
			sc->sc_pending |= TCTRL_SEND_RD_EXT_STATUS;
			sc->sc_pending |= TCTRL_SEND_BITPORT;
		}
		break;
	}
	case TS102_OP_CTL_BITPORT:
		sc->sc_bitport = sc->sc_rspbuf[0];
		break;
	default:
		break;
	}
}

void
tadpole_powerdown(void)
{
	struct tctrl_softc *sc;
	int i, s;

	if (tctrl_cd.cd_devs == NULL
	    || tctrl_cd.cd_ndevs == 0
	    || tctrl_cd.cd_devs[0] == NULL) {
		return;
	}

	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[0];
	s = splhigh();
	sc->sc_pending |= TCTRL_SEND_POWEROFF;
	for (i = 0; i < 10000; i++) {
		tctrl_intr(sc);
		DELAY(1);
	}
	splx(s);
}

void
tadpole_set_video(int enabled)
{
	struct tctrl_softc *sc;
	int s;

	if (tctrl_cd.cd_devs == NULL
	    || tctrl_cd.cd_ndevs == 0
	    || tctrl_cd.cd_devs[0] == NULL) {
		return;
	}

	sc = (struct tctrl_softc *) tctrl_cd.cd_devs[0];
	s = splhigh();
	if ((sc->sc_tft_on && !enabled) || (!sc->sc_tft_on && enabled)) {
		sc->sc_tft_on = enabled;
		if (sc->sc_ext_status & TS102_EXT_STATUS_LID_DOWN) {
			splx(s);
			return;
		}
		sc->sc_pending |= TCTRL_SEND_BITPORT;
		tctrl_intr(sc);
	}
	splx(s);
}

static void
tctrl_write_data(struct tctrl_softc *sc, u_int8_t v)
{
	unsigned int i;
	for (i = 0; i < 100; i++)  {
		if (TS102_UCTRL_STS_TXNF_REQ & tctrl_read(sc, TS102_REG_UCTRL_STS))
			break;
	}
	tctrl_write(sc, TS102_REG_UCTRL_DATA, v);
}

static u_int8_t
tctrl_read_data(struct tctrl_softc *sc)
{ 
	unsigned int i, v;

	for (i = 0; i < 100000; i++) {
		if (TS102_UCTRL_STS_RXNE_REQ & tctrl_read(sc, TS102_REG_UCTRL_STS))
			break;
		DELAY(1);
	}

	v = tctrl_read(sc, TS102_REG_UCTRL_DATA);
	tctrl_write(sc, TS102_REG_UCTRL_STS, TS102_UCTRL_STS_RXNE_REQ);
	return v;
}

static u_int8_t
tctrl_read(struct tctrl_softc *sc, bus_size_t off)
{
	sc->sc_junk = bus_space_read_4(sc->sc_memt, sc->sc_memh, off) >> 24;
	return sc->sc_junk;
}

static void
tctrl_write(struct tctrl_softc *sc, bus_size_t off, u_int8_t v)
{
	sc->sc_junk = v;
	bus_space_write_4(sc->sc_memt, sc->sc_memh, off, v << 24);
}
