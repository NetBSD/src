/* $NetBSD: nextkbd.c,v 1.8 2003/07/15 02:59:32 lukem Exp $ */
/*
 * Copyright (c) 1998 Matt DeBergalis
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
 *      This product includes software developed by Matt DeBergalis
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nextkbd.c,v 1.8 2003/07/15 02:59:32 lukem Exp $");

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <next68k/dev/nextkbdvar.h>
#include <next68k/dev/wskbdmap_next.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <next68k/next68k/isr.h>

#include <next68k/dev/intiovar.h>

struct nextkbd_internal {
	int num_ints; /* interrupt total */
	int polling;
	int isconsole;

	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct nextkbd_softc *t_sc; /* back pointer */
	u_int32_t mods;
};

struct mon_regs {
	u_int32_t mon_csr;
	u_int32_t mon_1;
	u_int32_t mon_data;
};

static int attached = 0;

int nextkbd_match __P((struct device *, struct cfdata *, void *));
void nextkbd_attach __P((struct device *, struct device *, void *));

int nextkbc_cnattach __P((bus_space_tag_t));

CFATTACH_DECL(nextkbd, sizeof(struct nextkbd_softc),
    nextkbd_match, nextkbd_attach, NULL, NULL);

int	nextkbd_enable __P((void *, int));
void	nextkbd_set_leds __P((void *, int));
int	nextkbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

const struct wskbd_accessops nextkbd_accessops = {
	nextkbd_enable,
	nextkbd_set_leds,
	nextkbd_ioctl,
};

void	nextkbd_cngetc __P((void *, u_int *, int *));
void	nextkbd_cnpollc __P((void *, int));

const struct wskbd_consops nextkbd_consops = {
	nextkbd_cngetc,
	nextkbd_cnpollc,
};

const struct wskbd_mapdata nextkbd_keymapdata = {
	nextkbd_keydesctab,
	KB_US,
};

static int nextkbd_read_data __P((struct nextkbd_internal *));
static int nextkbd_decode __P((struct nextkbd_internal *, int, u_int *, int *));

static struct nextkbd_internal nextkbd_consdata;
static int nextkbd_is_console __P((bus_space_tag_t bst));

int nextkbdhard __P((void *));

static int
nextkbd_is_console(bst)
	bus_space_tag_t bst;
{
	return (nextkbd_consdata.isconsole
			&& (bst == nextkbd_consdata.iot));
}

int
nextkbd_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct intio_attach_args *ia = (struct intio_attach_args *)aux;

	if (attached)
		return(0);

	ia->ia_addr = (void *)NEXT_P_MON;

	return(1);
}

void
nextkbd_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct nextkbd_softc *sc = (struct nextkbd_softc *)self;
	struct intio_attach_args *ia = (struct intio_attach_args *)aux;
	int isconsole;
	struct wskbddev_attach_args a;

	printf("\n");

	isconsole = nextkbd_is_console(ia->ia_bst); /* XXX */

	if (isconsole) {
		sc->id = &nextkbd_consdata;
	} else {
		sc->id = malloc(sizeof(struct nextkbd_internal), 
				M_DEVBUF, M_WAITOK);

		memset(sc->id, 0, sizeof(struct nextkbd_internal));
		sc->id->iot = ia->ia_bst;
		if (bus_space_map(sc->id->iot, NEXT_P_MON,
				sizeof(struct mon_regs),
				0, &sc->id->ioh)) {
			printf("%s: can't map mon status control register\n",
					sc->sc_dev.dv_xname);
			return;
		}
	}

	sc->id->t_sc = sc; /* set back pointer */

	isrlink_autovec(nextkbdhard, sc, NEXT_I_IPL(NEXT_I_KYBD_MOUSE), 0, NULL);

	INTR_ENABLE(NEXT_I_KYBD_MOUSE);

	a.console = isconsole;
	a.keymap = &nextkbd_keymapdata;
	a.accessops = &nextkbd_accessops;
	a.accesscookie = sc;

	/*
	 * Attach the wskbd, saving a handle to it.
	 * XXX XXX XXX
	 */
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);

	attached = 1;
}

int
nextkbd_enable(v, on)
	void *v;
	int on;
{
	/* XXX not sure if this should do anything */
	/* printf("nextkbd_enable %d\n", on); */
	return 0;
}

void
nextkbd_set_leds(v, leds)
	void *v;
	int leds;
{
	struct nextkbd_softc *sc = v;
	uint32_t hw_leds = 0;
	int s;

	sc->sc_leds &= ~ NEXT_WSKBD_LEDS;
	sc->sc_leds |= (leds & NEXT_WSKBD_LEDS);

	if (sc->sc_leds & WSKBD_LED_CAPS) {
		hw_leds |= 0x30000;
	}

	s = spltty();
	bus_space_write_1(sc->id->iot, sc->id->ioh, 3, 0xc5);
	/* @@@ need to add:
	   if bit 7 of @ioh+0 set:
	     repeat 2
	       wait until bit 6 of @ioh+2 clears
	*/
	bus_space_write_4(sc->id->iot, sc->id->ioh, 4, hw_leds);
	/* @@@ need to add:
	   wait until bit 4 of @ioh+0 (@ioh+2 if bit 7 was set above)
	     clears
	*/
	splx(s);

	return;
}

int
nextkbd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct nextkbd_softc *sc = v;
		 
	switch (cmd) {
	case WSKBDIO_GTYPE:
		/* XXX */
		*(int *)data = WSKBD_TYPE_NEXT;
		return (0);
	case WSKBDIO_SETLEDS:
		nextkbd_set_leds (sc, *(int *)data);
		return (0);
	case WSKBDIO_GETLEDS:
		*(int *)data = sc->sc_leds & NEXT_WSKBD_LEDS;
		return (0);
	case WSKBDIO_COMPLEXBELL:
		return (0);
	}
	return EPASSTHROUGH;
}

int
nextkbdhard(arg)
	void *arg;
{
	register struct nextkbd_softc *sc = arg;
	int type, key, val;

	if (!INTR_OCCURRED(NEXT_I_KYBD_MOUSE)) return 0;

#define CSR_INT 0x00800000
#define CSR_DATA 0x00400000

#define KD_KEYMASK			0x007f
#define KD_DIRECTION		0x0080 /* pressed or released */
#define KD_CNTL					0x0100
#define KD_LSHIFT				0x0200
#define KD_RSHIFT				0x0400
#define KD_LCOMM				0x0800
#define KD_RCOMM				0x1000
#define KD_LALT					0x2000
#define KD_RALT					0x4000
#define KD_VALID				0x8000 /* only set for scancode keys ? */
#define KD_MODS					0x4f00

	val = nextkbd_read_data(sc->id);
	if ((val != -1) && nextkbd_decode(sc->id, val, &type, &key)) {
		wskbd_input(sc->sc_wskbddev, type, key);
	}
	return(1);
}

int
nextkbd_cnattach(bst)
	bus_space_tag_t bst;
{
	bus_space_handle_t bsh;

	if (bus_space_map(bst, NEXT_P_MON, sizeof(struct mon_regs),
			0, &bsh))
		return (ENXIO);

	memset(&nextkbd_consdata, 0, sizeof(nextkbd_consdata));

	nextkbd_consdata.iot = bst;
	nextkbd_consdata.ioh = bsh;
	nextkbd_consdata.isconsole = 1;

	wskbd_cnattach(&nextkbd_consops, &nextkbd_consdata, 
			&nextkbd_keymapdata);

	return (0);
}

void
nextkbd_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
	struct nextkbd_internal *t = v;
	int val;

	for (;;) {
		if (INTR_OCCURRED(NEXT_I_KYBD_MOUSE)) {
			val = nextkbd_read_data(t);
			if ((val != -1) && nextkbd_decode(t, val, type, data))
				return;
		}
	}
}

void
nextkbd_cnpollc(v, on)
	void *v;
	int on;
{
	struct nextkbd_internal *t = v;

	t->polling = on;
	if (on) {
		INTR_DISABLE(NEXT_I_KYBD_MOUSE);
	} else {
		INTR_ENABLE(NEXT_I_KYBD_MOUSE);
	}

}

static int
nextkbd_read_data(struct nextkbd_internal *id)
{
	unsigned char device;
	struct mon_regs stat;
				
	bus_space_read_region_4(id->iot, id->ioh, 0, &stat, 3);
	if ((stat.mon_csr & CSR_INT) && (stat.mon_csr & CSR_DATA)) {
		stat.mon_csr &= ~CSR_INT;
		id->num_ints++;
		bus_space_write_4(id->iot, id->ioh, 0, stat.mon_csr);
		device = stat.mon_data >> 28;
		if (device != 1) return (-1); /* XXX: mouse */
		return (stat.mon_data & 0xffff);
	}
	return (-1);
}

static int
nextkbd_decode(id, datain, type, dataout)
	struct nextkbd_internal *id;
	int datain;
	u_int *type;
	int *dataout;
{
	/* printf("datain %08x mods %08x\n", datain, id->mods); */

	if ((datain ^ id->mods) & KD_LSHIFT) {
		id->mods ^= KD_LSHIFT;
		*dataout = 90;
		if (datain & KD_LSHIFT)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_RSHIFT) {
		id->mods ^= KD_RSHIFT;
		*dataout = 91;
		if (datain & KD_RSHIFT)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_LALT) {
		id->mods ^= KD_LALT;
		*dataout = 92;
		if (datain & KD_LALT)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_RALT) {
		id->mods ^= KD_RALT;
		*dataout = 93;
		if (datain & KD_RALT)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_CNTL) {
		id->mods ^= KD_CNTL;
		*dataout = 94;
		if (datain & KD_CNTL)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_LCOMM) {
		id->mods ^= KD_LCOMM;
		*dataout = 95;
		if (datain & KD_LCOMM)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_RCOMM) {
		id->mods ^= KD_RCOMM;
		*dataout = 96;
		if (datain & KD_RCOMM)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if (datain & KD_KEYMASK) {
		if (datain & KD_DIRECTION)
			*type = WSCONS_EVENT_KEY_UP;
		else
			*type = WSCONS_EVENT_KEY_DOWN;
								
		*dataout = (datain & KD_KEYMASK);
	} else {
		*dataout = 0;
	}

	return 1;
}
