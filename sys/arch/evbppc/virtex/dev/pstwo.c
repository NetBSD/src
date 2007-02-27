/* 	$NetBSD: pstwo.c,v 1.1.8.1 2007/02/27 16:50:14 yamt Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pstwo.c,v 1.1.8.1 2007/02/27 16:50:14 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/syslog.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/pckbport/pckbportvar.h>

#include <evbppc/virtex/virtex.h>
#include <evbppc/virtex/dev/xcvbusvar.h>
#include <evbppc/virtex/dev/pstworeg.h>


#define PSTWO_RXBUF_SIZE 	32
#define NEXT(idx) 		(void)((idx) = ((idx) + 1) % PSTWO_RXBUF_SIZE)

struct pstwo_softc {
	struct device 		sc_dev;
	void 			*sc_ih;

	bus_space_tag_t 	sc_iot;
	bus_space_handle_t 	sc_ioh;

	pckbport_tag_t 		sc_pt;
	pckbport_slot_t 	sc_slot;
};

static int 	pstwo_intr(void *);
static void 	pstwo_reset(bus_space_tag_t, bus_space_handle_t, bool);
#if 0
static void 	pstwo_printreg(struct pstwo_softc *);
#endif

/* Interface to pckbport. */
static int 	pstwo_xt_translation(void *, pckbport_slot_t, int);
static int 	pstwo_send_devcmd(void *, pckbport_slot_t, u_char);
static int 	pstwo_poll_data1(void *, pckbport_slot_t);
static void 	pstwo_slot_enable(void *, pckbport_slot_t, int);
static void 	pstwo_intr_establish(void *, pckbport_slot_t);
static void 	pstwo_set_poll(void *, pckbport_slot_t, int);

/* Generic device. */
static void 	pstwo_attach(struct device *, struct device *, void *);

CFATTACH_DECL(pstwo, sizeof(struct pstwo_softc),
    xcvbus_child_match, pstwo_attach, NULL, NULL);

static struct pckbport_accessops pstwo_ops = {
	.t_xt_translation 	= pstwo_xt_translation,
	.t_send_devcmd 		= pstwo_send_devcmd,
	.t_poll_data1 		= pstwo_poll_data1,
	.t_slot_enable 		= pstwo_slot_enable,
	.t_intr_establish 	= pstwo_intr_establish,
	.t_set_poll 		= pstwo_set_poll,
};

static void
pstwo_attach(struct device *parent, struct device *self, void *aux)
{
	struct xcvbus_attach_args 	*vaa = aux;
	struct pstwo_softc 		*sc = device_private(self);
	int 				i;

	printf(": PS2 port\n");

	if ((sc->sc_ih = intr_establish(vaa->vaa_intr, IST_LEVEL, IPL_TTY,
	    pstwo_intr, sc)) == NULL) {
		printf("%s: could not establish interrupt\n",
		    device_xname(self));
		return ;
	}

	sc->sc_iot = vaa->vaa_iot;

	if (bus_space_map(vaa->vaa_iot, vaa->vaa_addr, PSTWO_SIZE, 0,
	    &sc->sc_ioh) != 0) {
		printf("%s: could not map registers\n", device_xname(self));
		return ;
	}

	pstwo_reset(sc->sc_iot, sc->sc_ioh, 1);

	sc->sc_pt = pckbport_attach(sc, &pstwo_ops);

	/* Emulate the concept of "slot" to make pms(4)/pckbd(4) happy. */
	for (i = 0; i < PCKBPORT_NSLOTS; i++)
		if (pckbport_attach_slot(self, sc->sc_pt, i) != NULL) {
			sc->sc_slot = i;
			break; 			/* only one device allowed */
		}
}

#if 0
static void
pstwo_printreg(struct pstwo_softc *sc)
{
#define PRINTREG(name, reg) \
	printf("%s: [0x%08x] %s -> 0x%08x\n", device_xname(&sc->sc_dev), \
	    reg, name, bus_space_read_4(sc->sc_iot, sc->sc_ioh, reg))

	PRINTREG("status   ", PSTWO_STAT);
	PRINTREG("recv byte", PSTWO_RX_DATA);
	PRINTREG("intr stat", PSTWO_INTR_STAT);
	PRINTREG("intr mask", PSTWO_INTR_MSET);

#undef PRINTREG
}
#endif

static int
pstwo_intr(void *arg)
{
	struct pstwo_softc 	*sc = arg;
	uint32_t 		stat;

	stat = bus_space_read_4(sc->sc_iot, sc->sc_ioh, PSTWO_INTR_STAT);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PSTWO_INTR_ACK, stat);

	if (stat & INTR_RX_FULL) {
		uint32_t 	val;

		val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, PSTWO_RX_DATA);
		pckbportintr(sc->sc_pt, sc->sc_slot, DATA_RECV(val));
	}
	return (0);
}

static void
pstwo_reset(bus_space_tag_t iot, bus_space_handle_t ioh, bool restart)
{
	bus_space_write_4(iot, ioh, PSTWO_CTRL, CTRL_RESET);

	delay(10);

	if (restart) {
		bus_space_write_4(iot, ioh, PSTWO_CTRL, ~CTRL_RESET);
		bus_space_write_4(iot, ioh, PSTWO_INTR_MSET, INTR_ANY);
	}
}

/*
 * pstwo_wait:
 *
 * 	Wait while status bits indicated by ${mask} have the value ${set}.
 * 	Return zero on success, one on timeout.
 */
static int
pstwo_wait(struct pstwo_softc *sc, uint32_t mask, bool set)
{
	uint32_t 		val = (set ? mask : 0);
	int 			i = 1000;

	while (i--) {
		if ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, PSTWO_STAT) &
		    mask) == val) {
			delay(10);
			continue;
		}

		return (0);
	}

	return (1);
}

/*
 * pckbport
 */

static int
pstwo_xt_translation(void *arg, pckbport_slot_t port, int enable)
{
	/* Can't do translation, so disable succeeds & enable fails. */
	return (!enable);
}

static int
pstwo_send_devcmd(void *arg, pckbport_slot_t slot, u_char byte)
{
	struct pstwo_softc 	*sc = arg;

	if (pstwo_wait(sc, STAT_TX_BUSY, 1))
		return (0);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PSTWO_TX_DATA,
	    DATA_SEND(byte));
	return (1);
}

static int
pstwo_poll_data1(void *arg, pckbport_slot_t slot)
{
	struct pstwo_softc 	*sc = arg;
	uint32_t 		val;

	if (pstwo_wait(sc, STAT_RX_DONE, 0))
		return (-1);

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, PSTWO_RX_DATA);
	return DATA_RECV(val);
}

static void
pstwo_slot_enable(void *arg, pckbport_slot_t slot, int enable)
{
	/* Nothing to do */
}

static void
pstwo_intr_establish(void *arg, pckbport_slot_t slot)
{
	/* XXX no-op because we don't support polled mode */
}

static void
pstwo_set_poll(void *arg, pckbport_slot_t slot, int enable)
{
	/* XXX only needed by console */
}
