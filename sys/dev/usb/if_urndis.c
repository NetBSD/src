/*	$NetBSD: if_urndis.c,v 1.42 2022/03/03 05:55:19 riastradh Exp $ */
/*	$OpenBSD: if_urndis.c,v 1.31 2011/07/03 15:47:17 matthew Exp $ */

/*
 * Copyright (c) 2010 Jonathan Armani <armani@openbsd.org>
 * Copyright (c) 2010 Fabien Romano <fabien@openbsd.org>
 * Copyright (c) 2010 Michael Knudsen <mk@openbsd.org>
 * All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_urndis.c,v 1.42 2022/03/03 05:55:19 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/kmem.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbcdc.h>

#include <dev/ic/rndisreg.h>

#define RNDIS_RX_LIST_CNT	1
#define RNDIS_TX_LIST_CNT	1
#define RNDIS_BUFSZ		1562

struct urndis_softc {
	struct usbnet			sc_un;

	int				sc_ifaceno_ctl;

	/* RNDIS device info */
	uint32_t			sc_filter;
	uint32_t			sc_maxppt;
	uint32_t			sc_maxtsz;
	uint32_t			sc_palign;
};

#ifdef URNDIS_DEBUG
#define DPRINTF(x)      do { printf x; } while (0)
#else
#define DPRINTF(x)
#endif

#define DEVNAME(un)	(device_xname(un->un_dev))

#define URNDIS_RESPONSE_LEN 0x400

#if 0
static void urndis_watchdog(struct ifnet *);
#endif

static int urndis_uno_init(struct ifnet *);
static void urndis_uno_rx_loop(struct usbnet *, struct usbnet_chain *,
			       uint32_t);
static unsigned urndis_uno_tx_prepare(struct usbnet *, struct mbuf *,
				      struct usbnet_chain *);

static int urndis_init_un(struct ifnet *, struct usbnet *);

static uint32_t urndis_ctrl_handle_init(struct usbnet *,
    const struct rndis_comp_hdr *);
static uint32_t urndis_ctrl_handle_query(struct usbnet *,
    const struct rndis_comp_hdr *, void **, size_t *);
static uint32_t urndis_ctrl_handle_reset(struct usbnet *,
    const struct rndis_comp_hdr *);
static uint32_t urndis_ctrl_handle_status(struct usbnet *,
    const struct rndis_comp_hdr *);

static uint32_t urndis_ctrl_set(struct usbnet *, uint32_t, void *,
    size_t);

static int urndis_match(device_t, cfdata_t, void *);
static void urndis_attach(device_t, device_t, void *);

static const struct usbnet_ops urndis_ops = {
	.uno_init = urndis_uno_init,
	.uno_tx_prepare = urndis_uno_tx_prepare,
	.uno_rx_loop = urndis_uno_rx_loop,
};

CFATTACH_DECL_NEW(urndis, sizeof(struct urndis_softc),
    urndis_match, urndis_attach, usbnet_detach, usbnet_activate);

/*
 * Supported devices that we can't match by class IDs.
 */
static const struct usb_devno urndis_devs[] = {
	{ USB_VENDOR_HTC,	USB_PRODUCT_HTC_ANDROID },
	{ USB_VENDOR_SAMSUNG,	USB_PRODUCT_SAMSUNG_ANDROID2 },
	{ USB_VENDOR_SAMSUNG,	USB_PRODUCT_SAMSUNG_ANDROID },
};

static usbd_status
urndis_ctrl_msg(struct usbnet *un, uint8_t rt, uint8_t r,
    uint16_t index, uint16_t value, void *buf, size_t buflen)
{
	usb_device_request_t req;

	req.bmRequestType = rt;
	req.bRequest = r;
	USETW(req.wValue, value);
	USETW(req.wIndex, index);
	USETW(req.wLength, buflen);

	return usbd_do_request(un->un_udev, &req, buf);
}

static usbd_status
urndis_ctrl_send(struct usbnet *un, void *buf, size_t len)
{
	struct urndis_softc	*sc = usbnet_softc(un);
	usbd_status err;

	if (usbnet_isdying(un))
		return(0);

	err = urndis_ctrl_msg(un, UT_WRITE_CLASS_INTERFACE, UR_GET_STATUS,
	    sc->sc_ifaceno_ctl, 0, buf, len);

	if (err != USBD_NORMAL_COMPLETION)
		printf("%s: %s\n", DEVNAME(un), usbd_errstr(err));

	return err;
}

static struct rndis_comp_hdr *
urndis_ctrl_recv(struct usbnet *un)
{
	struct urndis_softc	*sc = usbnet_softc(un);
	struct rndis_comp_hdr	*hdr;
	char			*buf;
	usbd_status		 err;

	if (usbnet_isdying(un))
		return(0);

	buf = kmem_alloc(URNDIS_RESPONSE_LEN, KM_SLEEP);
	err = urndis_ctrl_msg(un, UT_READ_CLASS_INTERFACE, UR_CLEAR_FEATURE,
	    sc->sc_ifaceno_ctl, 0, buf, URNDIS_RESPONSE_LEN);

	if (err != USBD_NORMAL_COMPLETION && err != USBD_SHORT_XFER) {
		printf("%s: %s\n", DEVNAME(un), usbd_errstr(err));
		kmem_free(buf, URNDIS_RESPONSE_LEN);
		return NULL;
	}

	hdr = (struct rndis_comp_hdr *)buf;
	DPRINTF(("%s: urndis_ctrl_recv: type %#x len %u\n",
	    DEVNAME(un),
	    le32toh(hdr->rm_type),
	    le32toh(hdr->rm_len)));

	if (le32toh(hdr->rm_len) > URNDIS_RESPONSE_LEN) {
		printf("%s: ctrl message error: wrong size %u > %u\n",
		    DEVNAME(un),
		    le32toh(hdr->rm_len),
		    URNDIS_RESPONSE_LEN);
		kmem_free(buf, URNDIS_RESPONSE_LEN);
		return NULL;
	}

	return hdr;
}

static uint32_t
urndis_ctrl_handle(struct usbnet *un, struct rndis_comp_hdr *hdr,
    void **buf, size_t *bufsz)
{
	uint32_t rval;

	DPRINTF(("%s: urndis_ctrl_handle\n", DEVNAME(un)));

	if (buf && bufsz) {
		*buf = NULL;
		*bufsz = 0;
	}

	switch (le32toh(hdr->rm_type)) {
		case REMOTE_NDIS_INITIALIZE_CMPLT:
			rval = urndis_ctrl_handle_init(un, hdr);
			break;

		case REMOTE_NDIS_QUERY_CMPLT:
			rval = urndis_ctrl_handle_query(un, hdr, buf, bufsz);
			break;

		case REMOTE_NDIS_RESET_CMPLT:
			rval = urndis_ctrl_handle_reset(un, hdr);
			break;

		case REMOTE_NDIS_KEEPALIVE_CMPLT:
		case REMOTE_NDIS_SET_CMPLT:
			rval = le32toh(hdr->rm_status);
			break;

		case REMOTE_NDIS_INDICATE_STATUS_MSG:
			rval = urndis_ctrl_handle_status(un, hdr);
			break;

		default:
			printf("%s: ctrl message error: unknown event %#x\n",
			    DEVNAME(un), le32toh(hdr->rm_type));
			rval = RNDIS_STATUS_FAILURE;
	}

	kmem_free(hdr, URNDIS_RESPONSE_LEN);

	return rval;
}

static uint32_t
urndis_ctrl_handle_init(struct usbnet *un, const struct rndis_comp_hdr *hdr)
{
	struct urndis_softc		*sc = usbnet_softc(un);
	const struct rndis_init_comp	*msg;

	msg = (const struct rndis_init_comp *) hdr;

	DPRINTF(("%s: urndis_ctrl_handle_init: len %u rid %u status %#x "
	    "ver_major %u ver_minor %u devflags %#x medium %#x pktmaxcnt %u "
	    "pktmaxsz %u align %u aflistoffset %u aflistsz %u\n",
	    DEVNAME(un),
	    le32toh(msg->rm_len),
	    le32toh(msg->rm_rid),
	    le32toh(msg->rm_status),
	    le32toh(msg->rm_ver_major),
	    le32toh(msg->rm_ver_minor),
	    le32toh(msg->rm_devflags),
	    le32toh(msg->rm_medium),
	    le32toh(msg->rm_pktmaxcnt),
	    le32toh(msg->rm_pktmaxsz),
	    le32toh(msg->rm_align),
	    le32toh(msg->rm_aflistoffset),
	    le32toh(msg->rm_aflistsz)));

	if (le32toh(msg->rm_status) != RNDIS_STATUS_SUCCESS) {
		printf("%s: init failed %#x\n",
		    DEVNAME(un),
		    le32toh(msg->rm_status));

		return le32toh(msg->rm_status);
	}

	if (le32toh(msg->rm_devflags) != RNDIS_DF_CONNECTIONLESS) {
		printf("%s: wrong device type (current type: %#x)\n",
		    DEVNAME(un),
		    le32toh(msg->rm_devflags));

		return RNDIS_STATUS_FAILURE;
	}

	if (le32toh(msg->rm_medium) != RNDIS_MEDIUM_802_3) {
		printf("%s: medium not 802.3 (current medium: %#x)\n",
		    DEVNAME(un), le32toh(msg->rm_medium));

		return RNDIS_STATUS_FAILURE;
	}

	if (le32toh(msg->rm_ver_major) != RNDIS_MAJOR_VERSION ||
	    le32toh(msg->rm_ver_minor) != RNDIS_MINOR_VERSION) {
		printf("%s: version not %u.%u (current version: %u.%u)\n",
		    DEVNAME(un), RNDIS_MAJOR_VERSION, RNDIS_MINOR_VERSION,
		    le32toh(msg->rm_ver_major), le32toh(msg->rm_ver_minor));

		return RNDIS_STATUS_FAILURE;
	}

	sc->sc_maxppt = le32toh(msg->rm_pktmaxcnt);
	sc->sc_maxtsz = le32toh(msg->rm_pktmaxsz);
	sc->sc_palign = 1U << le32toh(msg->rm_align);

	return le32toh(msg->rm_status);
}

static uint32_t
urndis_ctrl_handle_query(struct usbnet *un,
    const struct rndis_comp_hdr *hdr, void **buf, size_t *bufsz)
{
	const struct rndis_query_comp	*msg;

	msg = (const struct rndis_query_comp *) hdr;

	DPRINTF(("%s: urndis_ctrl_handle_query: len %u rid %u status %#x "
	    "buflen %u bufoff %u\n",
	    DEVNAME(un),
	    le32toh(msg->rm_len),
	    le32toh(msg->rm_rid),
	    le32toh(msg->rm_status),
	    le32toh(msg->rm_infobuflen),
	    le32toh(msg->rm_infobufoffset)));

	if (buf && bufsz) {
		*buf = NULL;
		*bufsz = 0;
	}

	if (le32toh(msg->rm_status) != RNDIS_STATUS_SUCCESS) {
		printf("%s: query failed %#x\n",
		    DEVNAME(un),
		    le32toh(msg->rm_status));

		return le32toh(msg->rm_status);
	}

	if (le32toh(msg->rm_infobuflen) + le32toh(msg->rm_infobufoffset) +
	    RNDIS_HEADER_OFFSET > le32toh(msg->rm_len)) {
		printf("%s: ctrl message error: invalid query info "
		    "len/offset/end_position(%u/%u/%u) -> "
		    "go out of buffer limit %u\n",
		    DEVNAME(un),
		    le32toh(msg->rm_infobuflen),
		    le32toh(msg->rm_infobufoffset),
		    le32toh(msg->rm_infobuflen) +
		    le32toh(msg->rm_infobufoffset) + (uint32_t)RNDIS_HEADER_OFFSET,
		    le32toh(msg->rm_len));
		return RNDIS_STATUS_FAILURE;
	}

	if (buf && bufsz) {
		const char *p;

		*buf = kmem_alloc(le32toh(msg->rm_infobuflen), KM_SLEEP);
		*bufsz = le32toh(msg->rm_infobuflen);

		p = (const char *)&msg->rm_rid;
		p += le32toh(msg->rm_infobufoffset);
		memcpy(*buf, p, le32toh(msg->rm_infobuflen));
	}

	return le32toh(msg->rm_status);
}

static uint32_t
urndis_ctrl_handle_reset(struct usbnet *un, const struct rndis_comp_hdr *hdr)
{
	struct urndis_softc		*sc = usbnet_softc(un);
	const struct rndis_reset_comp	*msg;
	uint32_t			 rval;

	msg = (const struct rndis_reset_comp *) hdr;

	rval = le32toh(msg->rm_status);

	DPRINTF(("%s: urndis_ctrl_handle_reset: len %u status %#x "
	    "adrreset %u\n",
	    DEVNAME(un),
	    le32toh(msg->rm_len),
	    rval,
	    le32toh(msg->rm_adrreset)));

	if (rval != RNDIS_STATUS_SUCCESS) {
		printf("%s: reset failed %#x\n", DEVNAME(un), rval);
		return rval;
	}

	if (le32toh(msg->rm_adrreset) != 0) {
		uint32_t filter;

		filter = htole32(sc->sc_filter);
		rval = urndis_ctrl_set(un, OID_GEN_CURRENT_PACKET_FILTER,
		    &filter, sizeof(filter));
		if (rval != RNDIS_STATUS_SUCCESS) {
			printf("%s: unable to reset data filters\n",
			    DEVNAME(un));
			return rval;
		}
	}

	return rval;
}

static uint32_t
urndis_ctrl_handle_status(struct usbnet *un,
    const struct rndis_comp_hdr *hdr)
{
	const struct rndis_status_msg	*msg;
	uint32_t			rval;

	msg = (const struct rndis_status_msg *)hdr;

	rval = le32toh(msg->rm_status);

	DPRINTF(("%s: urndis_ctrl_handle_status: len %u status %#x "
	    "stbuflen %u\n",
	    DEVNAME(un),
	    le32toh(msg->rm_len),
	    rval,
	    le32toh(msg->rm_stbuflen)));

	switch (rval) {
		case RNDIS_STATUS_MEDIA_CONNECT:
		case RNDIS_STATUS_MEDIA_DISCONNECT:
		case RNDIS_STATUS_OFFLOAD_CURRENT_CONFIG:
			rval = RNDIS_STATUS_SUCCESS;
			break;

		default:
		        printf("%s: status %#x\n", DEVNAME(un), rval);
	}

	return rval;
}

static uint32_t
urndis_ctrl_init(struct usbnet *un)
{
	struct rndis_init_req	*msg;
	uint32_t		 rval;
	struct rndis_comp_hdr	*hdr;

	msg = kmem_alloc(sizeof(*msg), KM_SLEEP);
	msg->rm_type = htole32(REMOTE_NDIS_INITIALIZE_MSG);
	msg->rm_len = htole32(sizeof(*msg));
	msg->rm_rid = htole32(0);
	msg->rm_ver_major = htole32(RNDIS_MAJOR_VERSION);
	msg->rm_ver_minor = htole32(RNDIS_MINOR_VERSION);
	msg->rm_max_xfersz = htole32(RNDIS_BUFSZ);

	DPRINTF(("%s: urndis_ctrl_init send: type %u len %u rid %u ver_major %u "
	    "ver_minor %u max_xfersz %u\n",
	    DEVNAME(un),
	    le32toh(msg->rm_type),
	    le32toh(msg->rm_len),
	    le32toh(msg->rm_rid),
	    le32toh(msg->rm_ver_major),
	    le32toh(msg->rm_ver_minor),
	    le32toh(msg->rm_max_xfersz)));

	rval = urndis_ctrl_send(un, msg, sizeof(*msg));
	kmem_free(msg, sizeof(*msg));

	if (rval != RNDIS_STATUS_SUCCESS) {
		printf("%s: init failed\n", DEVNAME(un));
		return rval;
	}

	if ((hdr = urndis_ctrl_recv(un)) == NULL) {
		printf("%s: unable to get init response\n", DEVNAME(un));
		return RNDIS_STATUS_FAILURE;
	}
	rval = urndis_ctrl_handle(un, hdr, NULL, NULL);

	return rval;
}

#if 0
static uint32_t
urndis_ctrl_halt(struct usbnet *un)
{
	struct rndis_halt_req	*msg;
	uint32_t		 rval;

	msg = kmem_alloc(sizeof(*msg), KM_SLEEP);
	msg->rm_type = htole32(REMOTE_NDIS_HALT_MSG);
	msg->rm_len = htole32(sizeof(*msg));
	msg->rm_rid = 0;

	DPRINTF(("%s: urndis_ctrl_halt send: type %u len %u rid %u\n",
	    DEVNAME(un),
	    le32toh(msg->rm_type),
	    le32toh(msg->rm_len),
	    le32toh(msg->rm_rid)));

	rval = urndis_ctrl_send(un, msg, sizeof(*msg));
	kmem_free(msg, sizeof(*msg));

	if (rval != RNDIS_STATUS_SUCCESS)
		printf("%s: halt failed\n", DEVNAME(un));

	return rval;
}
#endif

static uint32_t
urndis_ctrl_query(struct usbnet *un, uint32_t oid,
    void *qbuf, size_t qlen,
    void **rbuf, size_t *rbufsz)
{
	struct rndis_query_req	*msg;
	uint32_t		 rval;
	struct rndis_comp_hdr	*hdr;

	msg = kmem_alloc(sizeof(*msg) + qlen, KM_SLEEP);
	msg->rm_type = htole32(REMOTE_NDIS_QUERY_MSG);
	msg->rm_len = htole32(sizeof(*msg) + qlen);
	msg->rm_rid = 0; /* XXX */
	msg->rm_oid = htole32(oid);
	msg->rm_infobuflen = htole32(qlen);
	if (qlen != 0) {
		msg->rm_infobufoffset = htole32(20);
		memcpy((char*)msg + 20, qbuf, qlen);
	} else
		msg->rm_infobufoffset = 0;
	msg->rm_devicevchdl = 0;

	DPRINTF(("%s: urndis_ctrl_query send: type %u len %u rid %u oid %#x "
	    "infobuflen %u infobufoffset %u devicevchdl %u\n",
	    DEVNAME(un),
	    le32toh(msg->rm_type),
	    le32toh(msg->rm_len),
	    le32toh(msg->rm_rid),
	    le32toh(msg->rm_oid),
	    le32toh(msg->rm_infobuflen),
	    le32toh(msg->rm_infobufoffset),
	    le32toh(msg->rm_devicevchdl)));

	rval = urndis_ctrl_send(un, msg, sizeof(*msg));
	kmem_free(msg, sizeof(*msg) + qlen);

	if (rval != RNDIS_STATUS_SUCCESS) {
		printf("%s: query failed\n", DEVNAME(un));
		return rval;
	}

	if ((hdr = urndis_ctrl_recv(un)) == NULL) {
		printf("%s: unable to get query response\n", DEVNAME(un));
		return RNDIS_STATUS_FAILURE;
	}
	rval = urndis_ctrl_handle(un, hdr, rbuf, rbufsz);

	return rval;
}

static uint32_t
urndis_ctrl_set(struct usbnet *un, uint32_t oid, void *buf, size_t len)
{
	struct rndis_set_req	*msg;
	uint32_t		 rval;
	struct rndis_comp_hdr	*hdr;

	msg = kmem_alloc(sizeof(*msg) + len, KM_SLEEP);
	msg->rm_type = htole32(REMOTE_NDIS_SET_MSG);
	msg->rm_len = htole32(sizeof(*msg) + len);
	msg->rm_rid = 0; /* XXX */
	msg->rm_oid = htole32(oid);
	msg->rm_infobuflen = htole32(len);
	if (len != 0) {
		msg->rm_infobufoffset = htole32(20);
		memcpy((char*)msg + 20, buf, len);
	} else
		msg->rm_infobufoffset = 0;
	msg->rm_devicevchdl = 0;

	DPRINTF(("%s: urndis_ctrl_set send: type %u len %u rid %u oid %#x "
	    "infobuflen %u infobufoffset %u devicevchdl %u\n",
	    DEVNAME(un),
	    le32toh(msg->rm_type),
	    le32toh(msg->rm_len),
	    le32toh(msg->rm_rid),
	    le32toh(msg->rm_oid),
	    le32toh(msg->rm_infobuflen),
	    le32toh(msg->rm_infobufoffset),
	    le32toh(msg->rm_devicevchdl)));

	rval = urndis_ctrl_send(un, msg, sizeof(*msg));
	kmem_free(msg, sizeof(*msg) + len);

	if (rval != RNDIS_STATUS_SUCCESS) {
		printf("%s: set failed\n", DEVNAME(un));
		return rval;
	}

	if ((hdr = urndis_ctrl_recv(un)) == NULL) {
		printf("%s: unable to get set response\n", DEVNAME(un));
		return RNDIS_STATUS_FAILURE;
	}
	rval = urndis_ctrl_handle(un, hdr, NULL, NULL);
	if (rval != RNDIS_STATUS_SUCCESS)
		printf("%s: set failed %#x\n", DEVNAME(un), rval);

	return rval;
}

#if 0
static uint32_t
urndis_ctrl_set_param(struct urndis_softc *un,
    const char *name,
    uint32_t type,
    void *buf,
    size_t len)
{
	struct rndis_set_parameter	*param;
	uint32_t			 rval;
	size_t				 namelen, tlen;

	if (name)
		namelen = strlen(name);
	else
		namelen = 0;
	tlen = sizeof(*param) + len + namelen;
	param = kmem_alloc(tlen, KM_SLEEP);
	param->rm_namelen = htole32(namelen);
	param->rm_valuelen = htole32(len);
	param->rm_type = htole32(type);
	if (namelen != 0) {
		param->rm_nameoffset = htole32(20);
		memcpy(param + 20, name, namelen);
	} else
		param->rm_nameoffset = 0;
	if (len != 0) {
		param->rm_valueoffset = htole32(20 + namelen);
		memcpy(param + 20 + namelen, buf, len);
	} else
		param->rm_valueoffset = 0;

	DPRINTF(("%s: urndis_ctrl_set_param send: nameoffset %u namelen %u "
	    "type %#x valueoffset %u valuelen %u\n",
	    DEVNAME(un),
	    le32toh(param->rm_nameoffset),
	    le32toh(param->rm_namelen),
	    le32toh(param->rm_type),
	    le32toh(param->rm_valueoffset),
	    le32toh(param->rm_valuelen)));

	rval = urndis_ctrl_set(un, OID_GEN_RNDIS_CONFIG_PARAMETER, param, tlen);
	kmem_free(param, tlen);
	if (rval != RNDIS_STATUS_SUCCESS)
		printf("%s: set param failed %#x\n", DEVNAME(un), rval);

	return rval;
}

/* XXX : adrreset, get it from response */
static uint32_t
urndis_ctrl_reset(struct usbnet *un)
{
	struct rndis_reset_req		*reset;
	uint32_t			 rval;
	struct rndis_comp_hdr		*hdr;

	reset = kmem_alloc(sizeof(*reset), KM_SLEEP);
	reset->rm_type = htole32(REMOTE_NDIS_RESET_MSG);
	reset->rm_len = htole32(sizeof(*reset));
	reset->rm_rid = 0; /* XXX rm_rid == reserved ... remove ? */

	DPRINTF(("%s: urndis_ctrl_reset send: type %u len %u rid %u\n",
	    DEVNAME(un),
	    le32toh(reset->rm_type),
	    le32toh(reset->rm_len),
	    le32toh(reset->rm_rid)));

	rval = urndis_ctrl_send(un, reset, sizeof(*reset));
	kmem_free(reset, sizeof(*reset));

	if (rval != RNDIS_STATUS_SUCCESS) {
		printf("%s: reset failed\n", DEVNAME(un));
		return rval;
	}

	if ((hdr = urndis_ctrl_recv(un)) == NULL) {
		printf("%s: unable to get reset response\n", DEVNAME(un));
		return RNDIS_STATUS_FAILURE;
	}
	rval = urndis_ctrl_handle(un, hdr, NULL, NULL);

	return rval;
}

static uint32_t
urndis_ctrl_keepalive(struct usbnet *un)
{
	struct rndis_keepalive_req	*keep;
	uint32_t			 rval;
	struct rndis_comp_hdr		*hdr;

	keep = kmem_alloc(sizeof(*keep), KM_SLEEP);
	keep->rm_type = htole32(REMOTE_NDIS_KEEPALIVE_MSG);
	keep->rm_len = htole32(sizeof(*keep));
	keep->rm_rid = 0; /* XXX rm_rid == reserved ... remove ? */

	DPRINTF(("%s: urndis_ctrl_keepalive: type %u len %u rid %u\n",
	    DEVNAME(un),
	    le32toh(keep->rm_type),
	    le32toh(keep->rm_len),
	    le32toh(keep->rm_rid)));

	rval = urndis_ctrl_send(un, keep, sizeof(*keep));
	kmem_free(keep, sizeof(*keep));

	if (rval != RNDIS_STATUS_SUCCESS) {
		printf("%s: keepalive failed\n", DEVNAME(un));
		return rval;
	}

	if ((hdr = urndis_ctrl_recv(un)) == NULL) {
		printf("%s: unable to get keepalive response\n", DEVNAME(un));
		return RNDIS_STATUS_FAILURE;
	}
	rval = urndis_ctrl_handle(un, hdr, NULL, NULL);
	if (rval != RNDIS_STATUS_SUCCESS) {
		printf("%s: keepalive failed %#x\n", DEVNAME(un), rval);
		urndis_ctrl_reset(un);
	}

	return rval;
}
#endif

static unsigned
urndis_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	struct rndis_packet_msg		*msg;

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - sizeof(*msg))
		return 0;

	msg = (struct rndis_packet_msg *)c->unc_buf;

	memset(msg, 0, sizeof(*msg));
	msg->rm_type = htole32(REMOTE_NDIS_PACKET_MSG);
	msg->rm_len = htole32(sizeof(*msg) + m->m_pkthdr.len);

	msg->rm_dataoffset = htole32(RNDIS_DATA_OFFSET);
	msg->rm_datalen = htole32(m->m_pkthdr.len);

	m_copydata(m, 0, m->m_pkthdr.len,
	    ((char*)msg + RNDIS_DATA_OFFSET + RNDIS_HEADER_OFFSET));

	DPRINTF(("%s: %s type %#x len %u data(off %u len %u)\n",
	    __func__,
	    DEVNAME(un),
	    le32toh(msg->rm_type),
	    le32toh(msg->rm_len),
	    le32toh(msg->rm_dataoffset),
	    le32toh(msg->rm_datalen)));

	return le32toh(msg->rm_len);
}

static void
urndis_uno_rx_loop(struct usbnet * un, struct usbnet_chain *c,
		   uint32_t total_len)
{
	struct rndis_packet_msg	*msg;
	struct ifnet		*ifp = usbnet_ifp(un);
	int			 offset;

	offset = 0;

	while (total_len > 1) {
		msg = (struct rndis_packet_msg *)((char*)c->unc_buf + offset);

		DPRINTF(("%s: %s buffer size left %u\n", DEVNAME(un), __func__,
		    total_len));

		if (total_len < sizeof(*msg)) {
			printf("%s: urndis_decap invalid buffer total_len %u < "
			    "minimum header %zu\n",
			    DEVNAME(un),
			    total_len,
			    sizeof(*msg));
			return;
		}

		DPRINTF(("%s: urndis_decap total_len %u data(off:%u len:%u) "
		    "oobdata(off:%u len:%u nb:%u) perpacket(off:%u len:%u)\n",
		    DEVNAME(un),
		    le32toh(msg->rm_len),
		    le32toh(msg->rm_dataoffset),
		    le32toh(msg->rm_datalen),
		    le32toh(msg->rm_oobdataoffset),
		    le32toh(msg->rm_oobdatalen),
		    le32toh(msg->rm_oobdataelements),
		    le32toh(msg->rm_pktinfooffset),
		    le32toh(msg->rm_pktinfooffset)));

		if (le32toh(msg->rm_type) != REMOTE_NDIS_PACKET_MSG) {
			printf("%s: urndis_decap invalid type %#x != %#x\n",
			    DEVNAME(un),
			    le32toh(msg->rm_type),
			    REMOTE_NDIS_PACKET_MSG);
			return;
		}
		if (le32toh(msg->rm_len) < sizeof(*msg)) {
			printf("%s: urndis_decap invalid msg len %u < %zu\n",
			    DEVNAME(un),
			    le32toh(msg->rm_len),
			    sizeof(*msg));
			return;
		}
		if (le32toh(msg->rm_len) > total_len) {
			printf("%s: urndis_decap invalid msg len %u > buffer "
			    "total_len %u\n",
			    DEVNAME(un),
			    le32toh(msg->rm_len),
			    total_len);
			return;
		}

		if (le32toh(msg->rm_dataoffset) +
		    le32toh(msg->rm_datalen) + RNDIS_HEADER_OFFSET
		        > le32toh(msg->rm_len)) {
			printf("%s: urndis_decap invalid data "
			    "len/offset/end_position(%u/%u/%u) -> "
			    "go out of receive buffer limit %u\n",
			    DEVNAME(un),
			    le32toh(msg->rm_datalen),
			    le32toh(msg->rm_dataoffset),
			    le32toh(msg->rm_dataoffset) +
			    le32toh(msg->rm_datalen) + (uint32_t)RNDIS_HEADER_OFFSET,
			    le32toh(msg->rm_len));
			return;
		}

		if (le32toh(msg->rm_datalen) < sizeof(struct ether_header)) {
			if_statinc(ifp, if_ierrors);
			printf("%s: urndis_decap invalid ethernet size "
			    "%d < %zu\n",
			    DEVNAME(un),
			    le32toh(msg->rm_datalen),
			    sizeof(struct ether_header));
			return;
		}

		usbnet_enqueue(un,
		    ((char*)&msg->rm_dataoffset + le32toh(msg->rm_dataoffset)),
		    le32toh(msg->rm_datalen), 0, 0, 0);

		offset += le32toh(msg->rm_len);
		total_len -= le32toh(msg->rm_len);
	}
}

#if 0
static void
urndis_watchdog(struct ifnet *ifp)
{
	struct urndis_softc	*sc = usbnet_softc(un);

	if (un->un_dying)
		return;

	if_statinc(ifp, if_oerrors);
	printf("%s: watchdog timeout\n", DEVNAME(un));

	urndis_ctrl_keepalive(un);
}
#endif

static int
urndis_init_un(struct ifnet *ifp, struct usbnet *un)
{
	int 			 err;

	err = urndis_ctrl_init(un);
	if (err != RNDIS_STATUS_SUCCESS)
		return EIO;

	return err;
}

static int
urndis_uno_init(struct ifnet *ifp)
{
	struct usbnet *un = ifp->if_softc;
	int error;

	KASSERT(IFNET_LOCKED(ifp));

	error = urndis_init_un(ifp, un);
	if (error)
		return EIO;	/* XXX */
	error = usbnet_init_rx_tx(un);
	usbnet_set_link(un, error == 0);

	return error;
}

static int
urndis_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg		*uiaa = aux;
	usb_interface_descriptor_t	*id;

	if (!uiaa->uiaa_iface)
		return UMATCH_NONE;

	id = usbd_get_interface_descriptor(uiaa->uiaa_iface);
	if (id == NULL)
		return UMATCH_NONE;

	if (id->bInterfaceClass == UICLASS_WIRELESS &&
	    id->bInterfaceSubClass == UISUBCLASS_RF &&
	    id->bInterfaceProtocol == UIPROTO_RNDIS)
		return UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO;

	return usb_lookup(urndis_devs, uiaa->uiaa_vendor, uiaa->uiaa_product) != NULL ?
	    UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
urndis_attach(device_t parent, device_t self, void *aux)
{
	struct urndis_softc		*sc = device_private(self);
	struct usbnet * const		 un = &sc->sc_un;
	struct usbif_attach_arg		*uiaa = aux;
	struct usbd_device	        *dev = uiaa->uiaa_device;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	usb_config_descriptor_t		*cd;
	struct usbd_interface		*iface_ctl;
	const usb_cdc_union_descriptor_t *ud;
	const usb_cdc_header_descriptor_t *desc;
	usbd_desc_iter_t		 iter;
	int				 if_ctl, if_data;
	int				 i, j, altcnt;
	void				*buf;
	size_t				 bufsz;
	uint32_t			 filter;
	char				*devinfop;

	KASSERT((void *)sc == un);

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = sc;
	un->un_ops = &urndis_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = RNDIS_RX_LIST_CNT;
	un->un_tx_list_cnt = RNDIS_TX_LIST_CNT;
	un->un_rx_bufsz = RNDIS_BUFSZ;
	un->un_tx_bufsz = RNDIS_BUFSZ;

	iface_ctl = uiaa->uiaa_iface;
	un->un_iface = uiaa->uiaa_iface;
	id = usbd_get_interface_descriptor(iface_ctl);
	if_ctl = id->bInterfaceNumber;
	sc->sc_ifaceno_ctl = if_ctl;
	if_data = -1;

	usb_desc_iter_init(un->un_udev, &iter);
	while ((desc = (const void *)usb_desc_iter_next(&iter)) != NULL) {

		if (desc->bDescriptorType != UDESC_CS_INTERFACE) {
			continue;
		}
		switch (desc->bDescriptorSubtype) {
		case UDESCSUB_CDC_UNION:
			/* XXX bail out when found first? */
			ud = (const usb_cdc_union_descriptor_t *)desc;
			if (if_data == -1)
				if_data = ud->bSlaveInterface[0];
			break;
		}
	}

	if (if_data == -1) {
		DPRINTF(("urndis_attach: no union interface\n"));
		un->un_iface = iface_ctl;
	} else {
		DPRINTF(("urndis_attach: union interface: ctl %u, data %u\n",
		    if_ctl, if_data));
		for (i = 0; i < uiaa->uiaa_nifaces; i++) {
			if (uiaa->uiaa_ifaces[i] != NULL) {
				id = usbd_get_interface_descriptor(
				    uiaa->uiaa_ifaces[i]);
				if (id != NULL && id->bInterfaceNumber ==
				    if_data) {
					un->un_iface = uiaa->uiaa_ifaces[i];
					uiaa->uiaa_ifaces[i] = NULL;
				}
			}
		}
	}

	if (un->un_iface == NULL) {
		aprint_error("%s: no data interface\n", DEVNAME(un));
		return;
	}

	id = usbd_get_interface_descriptor(un->un_iface);
	cd = usbd_get_config_descriptor(un->un_udev);
	altcnt = usbd_get_no_alts(cd, id->bInterfaceNumber);

	for (j = 0; j < altcnt; j++) {
		if (usbd_set_interface(un->un_iface, j)) {
			aprint_error("%s: interface alternate setting %u "
			    "failed\n", DEVNAME(un), j);
			return;
		}
		/* Find endpoints. */
		id = usbd_get_interface_descriptor(un->un_iface);
		un->un_ed[USBNET_ENDPT_RX] = un->un_ed[USBNET_ENDPT_TX] = 0;
		for (i = 0; i < id->bNumEndpoints; i++) {
			ed = usbd_interface2endpoint_descriptor(
			    un->un_iface, i);
			if (!ed) {
				aprint_error("%s: no descriptor for bulk "
				    "endpoint %u\n", DEVNAME(un), i);
				return;
			}
			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
				un->un_ed[USBNET_ENDPT_RX] = ed->bEndpointAddress;
			}
			else if (
			    UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
				un->un_ed[USBNET_ENDPT_TX] = ed->bEndpointAddress;
			}
		}

		if (un->un_ed[USBNET_ENDPT_RX] != 0 && un->un_ed[USBNET_ENDPT_TX] != 0) {
			DPRINTF(("%s: in=%#x, out=%#x\n",
			    DEVNAME(un),
			    un->un_ed[USBNET_ENDPT_RX],
			    un->un_ed[USBNET_ENDPT_TX]));
			break;
		}
	}

	if (un->un_ed[USBNET_ENDPT_RX] == 0)
		aprint_error("%s: could not find data bulk in\n", DEVNAME(un));
	if (un->un_ed[USBNET_ENDPT_TX] == 0)
		aprint_error("%s: could not find data bulk out\n",DEVNAME(un));
	if (un->un_ed[USBNET_ENDPT_RX] == 0 || un->un_ed[USBNET_ENDPT_TX] == 0)
		return;

#if 0
	ifp->if_watchdog = urndis_watchdog;
#endif

	usbnet_attach(un, "urndisdet");

	struct ifnet *ifp = usbnet_ifp(un);
	urndis_init_un(ifp, un);

	if (urndis_ctrl_query(un, OID_802_3_PERMANENT_ADDRESS, NULL, 0,
	    &buf, &bufsz) != RNDIS_STATUS_SUCCESS) {
		aprint_error("%s: unable to get hardware address\n",
		    DEVNAME(un));
		return;
	}

	if (bufsz == ETHER_ADDR_LEN) {
		memcpy(un->un_eaddr, buf, ETHER_ADDR_LEN);
		kmem_free(buf, bufsz);
	} else {
		aprint_error("%s: invalid address\n", DEVNAME(un));
		if (buf && bufsz)
			kmem_free(buf, bufsz);
		return;
	}

	/* Initialize packet filter */
	sc->sc_filter = RNDIS_PACKET_TYPE_BROADCAST;
	sc->sc_filter |= RNDIS_PACKET_TYPE_ALL_MULTICAST;
	filter = htole32(sc->sc_filter);
	if (urndis_ctrl_set(un, OID_GEN_CURRENT_PACKET_FILTER, &filter,
	    sizeof(filter)) != RNDIS_STATUS_SUCCESS) {
		aprint_error("%s: unable to set data filters\n", DEVNAME(un));
		return;
	}

	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
            0, NULL);
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(urndis)
