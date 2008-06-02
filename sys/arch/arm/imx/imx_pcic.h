/*	$Id: imx_pcic.h,v 1.1.20.1 2008/06/02 13:21:54 mjf Exp $	*/

/*
 * IMX CF interface to pcic/pcmcia
 * derived from pxa2x0_pcic
 * Sun Apr  1 21:42:37 PDT 2007
 */

/*	$NetBSD: imx_pcic.h,v 1.1.20.1 2008/06/02 13:21:54 mjf Exp $	*/
/*	$OpenBSD: pxapcicvar.h,v 1.7 2005/12/14 15:08:51 uwe Exp $ */

/*
 * Copyright (c) 2005 Dale Rahn <drahn@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef	_IMX_PCIC_H_
#define	_IMX_PCIC_H_

struct imx_pcic_socket {
	struct imx_pcic_softc *sc;
	int socket;		/* socket number */
	struct device *pcmcia;
	struct lwp *event_thread;

	int flags;
	int power_capability;	/* IMX_PCIC_POWER_3V | IMX_PCIC_POWER_5V */

	int irqpin;
	void *irq;

	void *pcictag_cookie;	/* opaque data for pcictag functions */
	struct imx_pcic_tag *pcictag;
};

/* event */
#define IMX_PCIC_EVENT_INSERTION	0
#define IMX_PCIC_EVENT_REMOVAL	1

/* flags */
#define IMX_PCIC_FLAG_CARDD	0
#define IMX_PCIC_FLAG_CARDP	1

struct imx_pcic_tag {
	u_int (*read)(struct imx_pcic_socket *, int);
	void (*write)(struct imx_pcic_socket *, int, u_int);
	void (*set_power)(struct imx_pcic_socket *, int);
	void (*clear_intr)(struct imx_pcic_socket *);
	void *(*intr_establish)(struct imx_pcic_socket *, int,
	    int (*)(void *), void *);
	void (*intr_disestablish)(struct imx_pcic_socket *, void *);
};

#ifdef NOTYET
/* pcictag registers and their values */
#define IMX_PCIC_CARD_STATUS	0
#define  IMX_PCIC_CARD_INVALID	0
#define  IMX_PCIC_CARD_VALID	1
#define IMX_PCIC_CARD_READY	1
#define IMX_PCIC_CARD_POWER	2
#define  IMX_PCIC_POWER_OFF	0
#define  IMX_PCIC_POWER_3V	1
#define  IMX_PCIC_POWER_5V	2
#define IMX_PCIC_CARD_RESET	3
#endif

#define IMX_PCIC_NSLOT	1			/* ??? */

struct imx_pcic_softc {
	struct device sc_dev;
	struct imx_pcic_socket sc_socket[IMX_PCIC_NSLOT];

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_memctl_ioh;

	bus_addr_t sc_pa;

	void *sc_irq;
	int sc_shutdown;
	int sc_nslots;
	int sc_irqpin[IMX_PCIC_NSLOT];
	int sc_irqcfpin[IMX_PCIC_NSLOT];

	u_int sc_flags;
#define	PPF_REVERSE_ORDER	(1 << 0)
};

void	imx_pcic_attach_common(struct imx_pcic_softc *,
	    void (*socket_setup_hook)(struct imx_pcic_socket *));
int	imx_pcic_intr(void *);
void	imx_pcic_create_event_thread(void *);

#endif	/* _IMX_PCIC_H_ */
