/*	$NetBSD: hvkbd.c,v 1.9 2021/08/07 16:19:11 thorpej Exp $	*/

/*-
 * Copyright (c) 2017 Microsoft Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
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
 *
 * $FreeBSD: head/sys/dev/hyperv/input/hv_kbd.c 317821 2017-05-05 03:28:30Z sephe $
 * $FreeBSD: head/sys/dev/hyperv/input/hv_kbdc.c 320490 2017-06-30 03:01:22Z sephe $
 * $FreeBSD: head/sys/dev/hyperv/input/hv_kbdc.h 316515 2017-04-05 05:01:23Z sephe $
 */

#ifdef _KERNEL_OPT
#include "opt_pckbd_layout.h"
#include "opt_wsdisplay_compat.h"
#endif /* _KERNEL_OPT */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hvkbd.c,v 1.9 2021/08/07 16:19:11 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/pmf.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <dev/hyperv/vmbusvar.h>
#include <dev/hyperv/hvkbdvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/pckbport/wskbdmap_mfii.h>

#define HVKBD_BUFSIZE		(4 * PAGE_SIZE)
#define HVKBD_TX_RING_SIZE	(10 * PAGE_SIZE)
#define HVKBD_RX_RING_SIZE	(10 * PAGE_SIZE)

#define HVKBD_VER_MAJOR		(1)
#define HVKBD_VER_MINOR		(0)
#define HVKBD_VERSION		((HVKBD_VER_MAJOR << 16) | HVKBD_VER_MINOR)

enum hvkbd_msg_type {
	HVKBD_PROTO_REQUEST		= 1,
	HVKBD_PROTO_RESPONSE		= 2,
	HVKBD_PROTO_EVENT		= 3,
	HVKBD_PROTO_LED_INDICATORS	= 4
};

struct hvkbd_msg_hdr {
	uint32_t		type;
} __packed;

struct hvkbd_proto_req {
	struct hvkbd_msg_hdr	hdr;
	uint32_t		ver;
} __packed;

struct hvkbd_proto_resp {
	struct hvkbd_msg_hdr	hdr;
	uint32_t		status;
#define	RESP_STATUS_ACCEPTED	__BIT(0)
} __packed;

struct keystroke {
	uint16_t		makecode;
	uint16_t		pad0;
	uint32_t		info;
#define KS_INFO_UNICODE		__BIT(0)
#define KS_INFO_BREAK		__BIT(1)
#define KS_INFO_E0		__BIT(2)
#define KS_INFO_E1		__BIT(3)
} __packed;

struct hvkbd_keystroke {
	struct hvkbd_msg_hdr	hdr;
	struct keystroke	ks;
} __packed;

struct hvkbd_keystroke_info {
	LIST_ENTRY(hvkbd_keystroke_info)	link;
	STAILQ_ENTRY(hvkbd_keystroke_info)	slink;
	struct keystroke			ks;
};

#define	HVKBD_KEYBUF_SIZE	16

struct hvkbd_softc {
	device_t				sc_dev;

	struct vmbus_channel    		*sc_chan;
	void					*sc_buf;

	kmutex_t				sc_ks_lock;
	LIST_HEAD(, hvkbd_keystroke_info)	sc_ks_free;
	STAILQ_HEAD(, hvkbd_keystroke_info)	sc_ks_queue;

	int					sc_enabled;
	int					sc_polling;
	int					sc_console_keyboard;
#if defined(WSDISPLAY_COMPAT_RAWKBD)
	int					sc_rawkbd;
#endif

	int					sc_connected;
	uint32_t				sc_connect_status;

	device_t				sc_wskbddev;
};

static int	hvkbd_match(device_t, cfdata_t, void *);
static void	hvkbd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(hvkbd, sizeof(struct hvkbd_softc),
    hvkbd_match, hvkbd_attach, NULL, NULL);

static int	hvkbd_alloc_keybuf(struct hvkbd_softc *);
static void	hvkbd_free_keybuf(struct hvkbd_softc *);

static int	hvkbd_enable(void *, int);
static void	hvkbd_set_leds(void *, int);
static int	hvkbd_ioctl(void *, u_long, void *, int, struct lwp *);

static const struct wskbd_accessops hvkbd_accessops = {
	hvkbd_enable,
	hvkbd_set_leds,
	hvkbd_ioctl,
};

static const struct wskbd_mapdata hvkbd_keymapdata = {
	pckbd_keydesctab,
#if defined(PCKBD_LAYOUT)
	PCKBD_LAYOUT,
#else
	KB_US,
#endif
};

static int	hvkbd_connect(struct hvkbd_softc *);
static void	hvkbd_intr(void *);

static void	hvkbd_cngetc(void *, u_int *, int *);
static void	hvkbd_cnpollc(void *, int);

static const struct wskbd_consops hvkbd_consops = {
	.getc =  hvkbd_cngetc,
	.pollc = hvkbd_cnpollc,
	.bell =  NULL,
};

static int	hvkbd_is_console;

static int
hvkbd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct vmbus_attach_args *aa = aux;

	if (memcmp(aa->aa_type, &hyperv_guid_kbd, sizeof(*aa->aa_type)) != 0)
		return 0;

	/* If hvkbd(4) is not console, we use pckbd(4) in Gen.1 VM. */
	if (!hvkbd_is_console && hyperv_is_gen1())
		return 0;

	return 1;
}

static void
hvkbd_attach(device_t parent, device_t self, void *aux)
{
	struct hvkbd_softc *sc = device_private(self);
	struct vmbus_attach_args *aa = aux;
	struct wskbddev_attach_args a;

	sc->sc_dev = self;
	sc->sc_chan = aa->aa_chan;

	aprint_naive("\n");
	aprint_normal(": Hyper-V Synthetic Keyboard\n");

	mutex_init(&sc->sc_ks_lock, MUTEX_DEFAULT, IPL_TTY);
	LIST_INIT(&sc->sc_ks_free);
	STAILQ_INIT(&sc->sc_ks_queue);
	hvkbd_alloc_keybuf(sc);

	sc->sc_buf = kmem_zalloc(HVKBD_BUFSIZE, KM_SLEEP);

	sc->sc_chan->ch_flags &= ~CHF_BATCHED;
	if (vmbus_channel_open(sc->sc_chan,
	    HVKBD_TX_RING_SIZE + HVKBD_RX_RING_SIZE, NULL, 0, hvkbd_intr, sc)) {
		aprint_error_dev(self, "failed to open channel\n");
		goto free_buf;
	}

	if (hvkbd_connect(sc))
		goto free_buf;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	sc->sc_console_keyboard = hvkbd_is_console;
	if (hvkbd_is_console)
		hvkbd_is_console = 0;

	if (sc->sc_console_keyboard) {
		wskbd_cnattach(&hvkbd_consops, sc, &hvkbd_keymapdata);
		hvkbd_enable(sc, 1);
	}

	a.console = sc->sc_console_keyboard;
	a.keymap = &hvkbd_keymapdata;
	a.accessops = &hvkbd_accessops;
	a.accesscookie = sc;
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint, CFARGS_NONE);
	return;

free_buf:
	if (sc->sc_buf != NULL) {
		kmem_free(sc->sc_buf, HVKBD_BUFSIZE);
		sc->sc_buf = NULL;
	}
	hvkbd_free_keybuf(sc);
}

static int
hvkbd_alloc_keybuf(struct hvkbd_softc *sc)
{
	struct hvkbd_keystroke_info *ksi;
	int i;

	for (i = 0; i < HVKBD_KEYBUF_SIZE; i++) {
		ksi = kmem_zalloc(sizeof(*ksi), KM_SLEEP);
		LIST_INSERT_HEAD(&sc->sc_ks_free, ksi, link);
	}

	return 0;
}

static void
hvkbd_free_keybuf(struct hvkbd_softc *sc)
{
	struct hvkbd_keystroke_info *ksi;

	while ((ksi = STAILQ_FIRST(&sc->sc_ks_queue)) != NULL) {
		STAILQ_REMOVE(&sc->sc_ks_queue, ksi, hvkbd_keystroke_info,
		    slink);
		kmem_free(ksi, sizeof(*ksi));
	}
	while ((ksi = LIST_FIRST(&sc->sc_ks_free)) != NULL) {
		LIST_REMOVE(ksi, link);
		kmem_free(ksi, sizeof(*ksi));
	}
}

int
hvkbd_enable(void *v, int on)
{
	struct hvkbd_softc *sc = v;

	sc->sc_enabled = on;

	return 0;
}

static void
hvkbd_set_leds(void *v, int leds)
{
}

static int
hvkbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
#if defined(WSDISPLAY_COMPAT_RAWKBD)
	struct hvkbd_softc *sc = v;
#endif

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_HYPERV;
		return 0;

	case WSKBDIO_SETLEDS:
		hvkbd_set_leds(v, *(int *)data);
		return 0;

	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;

#if defined(WSDISPLAY_COMPAT_RAWKBD)
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = (*(int *)data == WSKBD_RAW);
		return 0;
#endif
	}

	return EPASSTHROUGH;
}

static int
hvkbd_connect(struct hvkbd_softc *sc)
{
	struct hvkbd_proto_req req;
	int timo = 100;
	int error, s;

	sc->sc_connected = 0;

	memset(&req, 0, sizeof(req));
	req.hdr.type = HVKBD_PROTO_REQUEST;
	req.ver = HVKBD_VERSION;
	error = vmbus_channel_send(sc->sc_chan, &req, sizeof(req),
	    0, VMBUS_CHANPKT_TYPE_INBAND, VMBUS_CHANPKT_FLAG_RC);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to send connect: %d\n",
		    error);
		return error;
	}

	do {
		if (cold) {
			delay(1000);
			s = spltty();
			hvkbd_intr(sc);
			splx(s);
		} else
			tsleep(sc, PRIBIO | PCATCH, "hvkbdcon",
			    uimax(1, mstohz(1)));
	} while (--timo > 0 && sc->sc_connected == 0);

	if (timo == 0 && sc->sc_connected == 0) {
		aprint_error_dev(sc->sc_dev, "connect timed out\n");
		return ETIMEDOUT;
	}

	if (!(sc->sc_connect_status & RESP_STATUS_ACCEPTED)) {
		aprint_error_dev(sc->sc_dev, "protocol request failed\n");
		return ENODEV;
	}

	return 0;
}

static int
hvkbd_keybuf_add_keystroke(struct hvkbd_softc *sc, const struct keystroke *ks)
{
	struct hvkbd_keystroke_info *ksi;

	mutex_enter(&sc->sc_ks_lock);
	ksi = LIST_FIRST(&sc->sc_ks_free);
	if (ksi != NULL) {
		LIST_REMOVE(ksi, link);
		ksi->ks = *ks;
		STAILQ_INSERT_TAIL(&sc->sc_ks_queue, ksi, slink);
	}
	mutex_exit(&sc->sc_ks_lock);

	return (ksi != NULL) ? 0 : 1;
}

static int
hvkbd_decode(struct hvkbd_softc *sc, u_int *type, int *scancode)
{
	struct hvkbd_keystroke_info *ksi;
	struct keystroke ks;

	mutex_enter(&sc->sc_ks_lock);
	ksi = STAILQ_FIRST(&sc->sc_ks_queue);
	if (ksi != NULL) {
		STAILQ_REMOVE_HEAD(&sc->sc_ks_queue, slink);
		ks = ksi->ks;
		LIST_INSERT_HEAD(&sc->sc_ks_free, ksi, link);
	}
	mutex_exit(&sc->sc_ks_lock);

	if (ksi == NULL)
		return 0;

	/*
	 * XXX: Hyper-V host send unicode to VM through 'Type clipboard text',
	 * the mapping from unicode to scancode depends on the keymap.
	 * It is so complicated that we do not plan to support it yet.
	 */
	if (ks.info & KS_INFO_UNICODE)
		return 0;

	*type = (ks.info & KS_INFO_BREAK) ?
	    WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN;
	*scancode = ks.makecode;
	return 1;
}

#if defined(WSDISPLAY_COMPAT_RAWKBD)
static int
hvkbd_encode(struct hvkbd_softc *sc, u_char *buf, int *len)
{
	struct hvkbd_keystroke_info *ksi;
	struct keystroke ks;
	int i;

	mutex_enter(&sc->sc_ks_lock);
	ksi = STAILQ_FIRST(&sc->sc_ks_queue);
	if (ksi != NULL) {
		STAILQ_REMOVE_HEAD(&sc->sc_ks_queue, slink);
		ks = ksi->ks;
		LIST_INSERT_HEAD(&sc->sc_ks_free, ksi, link);
	}
	mutex_exit(&sc->sc_ks_lock);

	if (ksi == NULL)
		return 0;

	/*
	 * XXX: Hyper-V host send unicode to VM through 'Type clipboard text',
	 * the mapping from unicode to scancode depends on the keymap.
	 * It is so complicated that we do not plan to support it yet.
	 */
	if (ks.info & KS_INFO_UNICODE)
		return 0;

	i = 0;
	if (ks.info & (KS_INFO_E0|KS_INFO_E1)) {
		if (ks.info & KS_INFO_E0)
			buf[i++] = 0xe0;
		else
			buf[i++] = 0xe1;
	}
	if (ks.info & KS_INFO_BREAK)
		buf[i++] = (u_char)ks.makecode & 0x80;
	else
		buf[i++] = (u_char)ks.makecode;

	KDASSERT(i <= *len);
	*len = i;

	return 1;
}
#endif

static void
hvkbd_intr(void *xsc)
{
	struct hvkbd_softc *sc = xsc;
	struct vmbus_chanpkt_hdr *cph;
	const struct hvkbd_msg_hdr *hdr;
	const struct hvkbd_proto_resp *rsp;
	const struct hvkbd_keystroke *ks;
	uint64_t rid;
	uint32_t rlen;
	u_int type;
	int key, error;

	for (;;) {
		error = vmbus_channel_recv(sc->sc_chan, sc->sc_buf,
		    HVKBD_BUFSIZE, &rlen, &rid, 1);
		if (error != 0 || rlen == 0) {
			if (error != EAGAIN)
				device_printf(sc->sc_dev,
				    "failed to receive a reply packet\n");
			return;
		}

		cph = (struct vmbus_chanpkt_hdr *)sc->sc_buf;
		switch (cph->cph_type) {
		case VMBUS_CHANPKT_TYPE_INBAND:
			hdr = VMBUS_CHANPKT_CONST_DATA(cph);
			if (rlen < sizeof(*hdr)) {
				device_printf(sc->sc_dev, "Illegal packet\n");
				continue;
			}

			switch (hdr->type) {
			case HVKBD_PROTO_RESPONSE:
				if (!sc->sc_connected) {
					rsp = VMBUS_CHANPKT_CONST_DATA(cph);
					if (rlen < sizeof(*rsp)) {
						device_printf(sc->sc_dev,
						    "Illegal resp packet\n");
						break;
					}
					sc->sc_connect_status = rsp->status;
					sc->sc_connected = 1;
					wakeup(sc);
				}
				break;

			case HVKBD_PROTO_EVENT:
				if (sc->sc_wskbddev == NULL || !sc->sc_enabled)
					break;

				ks = VMBUS_CHANPKT_CONST_DATA(cph);
				hvkbd_keybuf_add_keystroke(sc, &ks->ks);
				if (sc->sc_polling)
					break;

#if defined(WSDISPLAY_COMPAT_RAWKBD)
				if (sc->sc_rawkbd) {
					u_char buf[2];
					int len;

					len = sizeof(buf);
					if (hvkbd_encode(sc, buf, &len)) {
						wskbd_rawinput(sc->sc_wskbddev,
						    buf, len);
					}
					break;
				}
#endif
				if (hvkbd_decode(sc, &type, &key))
					wskbd_input(sc->sc_wskbddev, type, key);
				break;

			case HVKBD_PROTO_REQUEST:
			case HVKBD_PROTO_LED_INDICATORS:
				device_printf(sc->sc_dev,
				    "unhandled message: %d\n", hdr->type);
				break;

			default:
				device_printf(sc->sc_dev,
				    "unknown message: %d\n", hdr->type);
				break;
			}
			break;

		case VMBUS_CHANPKT_TYPE_COMP:
		case VMBUS_CHANPKT_TYPE_RXBUF:
			device_printf(sc->sc_dev, "unhandled event: %d\n",
			    cph->cph_type);
			break;

		default:
			device_printf(sc->sc_dev, "unknown event: %d\n",
			    cph->cph_type);
			break;
		}
	}
}

int
hvkbd_cnattach(void)
{

	hvkbd_is_console = 1;

	return 0;
}

static void
hvkbd_cngetc(void *v, u_int *type, int *data)
{
	struct hvkbd_softc *sc = v;

	while (!hvkbd_decode(sc, type, data))
		hvkbd_intr(sc);
}

static void
hvkbd_cnpollc(void *v, int on)
{
	struct hvkbd_softc *sc = v;

	sc->sc_polling = on;
}

MODULE(MODULE_CLASS_DRIVER, hvkbd, "vmbus");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
hvkbd_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_hvkbd,
		    cfattach_ioconf_hvkbd, cfdata_ioconf_hvkbd);
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_hvtkbd,
		    cfattach_ioconf_hvkbd, cfdata_ioconf_hvkbd);
#endif
		break;

	case MODULE_CMD_AUTOUNLOAD:
		error = EBUSY;
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}
