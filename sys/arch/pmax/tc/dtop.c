/* $Id: dtop.c,v 1.1.2.1 1998/10/15 02:48:59 nisimura Exp $ */
/* $NetBSD: dtop.c,v 1.1.2.1 1998/10/15 02:48:59 nisimura Exp $ */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <dev/cons.h>

#include <machine/autoconf.h>
#include <pmax/pmax/pmaxtype.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <pmax/tc/ioasicreg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsmousevar.h>

/* XXX XXX XXX */
#include <pmax/pmax/maxine.h>
#define	DTOP_IDLE		0
#define	DTOP_SRC_RECEIVED	1
#define	DTOP_BODY_RECEIVING	2
#define	DTOP_BODY_RECEIVED	3
#define	DATA(x)	(*(x) >> 8)
/* XXX XXX XXX */

#include <dev/dec/lk201.h>

struct dtop_softc {
	struct device	sc_dv;

	u_int32_t	*sc_reg;
	u_int32_t	*sc_intr;
	struct device	*sc_wskbddev;
	struct device	*sc_wsmousedev;
	int		sc_mouse_enabled;
	int		sc_state;
	u_int8_t	sc_msgbuf[127 + 4];
	u_int8_t	*sc_rcvp;
	int		sc_len;
};
extern struct cfdriver ioasic_cd;

int  dtopmatch	__P((struct device *, struct cfdata *, void *));
void dtopattach	__P((struct device *, struct device *, void *));

struct cfattach dtop_ca = {
	sizeof(struct dtop_softc), dtopmatch, dtopattach
};

int dtopintr __P((void *));

int
dtopmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;

	if (systype != DS_MAXINE)
		return (0);
	if (strcmp(d->iada_modname, "dtop") != 0)
		return (0);
	return (1);
}

void
dtopattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;
	struct dtop_softc *sc = (struct dtop_softc*)self;

	sc->sc_reg = (void *)(ioasic_base + IOASIC_SLOT_10_START);
	sc->sc_intr = (void *)(ioasic_base + IOASIC_INTR);

	sc->sc_state = 0;
	sc->sc_len = 0;
	sc->sc_rcvp = sc->sc_msgbuf;

	ioasic_intr_establish(parent,
			d->iada_cookie, TC_IPL_TTY, dtopintr, sc);
	printf("\n");

#if WSKBDDEV
	{
	struct wskbd_dev_attach_args a;

	a.layout = KB_US;
	a.getc = dtop_cngetc;
	a.pollc = dtop_cnpollc;
	a.set_leds = /* XXX */ 0;
	a.ioctl = dtop_ioctl;
	a.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
	}
#endif
#if NWSMOUSEDEV
	{
	struct wskbd_dev_attach_args a;

	a.accessops = &vsxxx_accessops;
	a.accesscookie = sc;

	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);
	}
#endif
}

int	vsxxx_enable __P((void *));
int	vsxxx_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
void	vsxxx_disable __P((void *));

const struct wsmouse_accessops vsxxx_accessops = {
	vsxxx_enable,
	vsxxx_ioctl,
	vsxxx_disable,
};

int
vsxxx_enable(v)
	void *v;
{
	struct dtop_softc *sc = v;
	sc->sc_mouse_enabled = 1;
	return 1;
}

void
vsxxx_disable(v)
	void *v;
{
	struct dtop_softc *sc = v;
	sc->sc_mouse_enabled = 0;
}

int
vsxxx_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = 0x12345678;
		return 0;
	}
	return -1;
}

struct packet {
	u_int8_t src;
	u_int8_t ctl;
	u_int8_t body[1];
};

int
dtopintr(v)
	void *v;
{
	struct dtop_softc *sc = v;
#if 1 
	int npoll;

	npoll = 20;
	do {
		switch (sc->sc_state) {
		case DTOP_IDLE:
			*sc->sc_rcvp++ = DATA(sc->sc_reg);
			sc->sc_state = DTOP_SRC_RECEIVED;
			break;

		case DTOP_SRC_RECEIVED:
			*sc->sc_rcvp++ = sc->sc_len = DATA(sc->sc_reg);
			if (sc->sc_len == 0xf8)
				sc->sc_len = 0;
			sc->sc_len &= 0x7f;
			sc->sc_len += 1;
			sc->sc_state = DTOP_BODY_RECEIVING;
			break;

		case DTOP_BODY_RECEIVING:
			*sc->sc_rcvp++ = DATA(sc->sc_reg);
			if (--sc->sc_len == 0)
				goto gotall;
			break;
		}
	} while (--npoll > 0 && (*sc->sc_intr & XINE_INTR_DTOP_RX));
	return 1;

gotall:
#else
	*sc->sc_rcvp++ = DATA(sc->sc_reg);
#endif
	sc->sc_state = DTOP_IDLE;
	sc->sc_rcvp = sc->sc_msgbuf;

	{
	int i;
	struct packet *pkt = (void *)sc->sc_msgbuf;
	npoll = (pkt->ctl == 0xf8) ? 1 : pkt->ctl & 0x7f;
	printf("src=%d ctl=%x", pkt->src, pkt->ctl);
	for (i = 0; i < npoll; i++)
		printf(" %02x", pkt->body[i]);
	printf("\n");
	}

	return 1;
}
