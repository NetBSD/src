/*	$NetBSD: if_umb.c,v 1.1 2018/07/31 16:44:29 khorben Exp $ */
/*	$OpenBSD: if_umb.c,v 1.18 2018/02/19 08:59:52 mpi Exp $ */

/*
 * Copyright (c) 2016 genua mbH
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

/*
 * Mobile Broadband Interface Model specification:
 * http://www.usb.org/developers/docs/devclass_docs/MBIM10Errata1_073013.zip
 * Compliance testing guide
 * http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_umb.c,v 1.1 2018/07/31 16:44:29 khorben Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/rndsource.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_types.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/mbim.h>
#include <dev/usb/if_umbreg.h>

#ifdef UMB_DEBUG
#define DPRINTF(x...)							\
		do { if (umb_debug) log(LOG_DEBUG, x); } while (0)

#define DPRINTFN(n, x...)						\
		do { if (umb_debug >= (n)) log(LOG_DEBUG, x); } while (0)

#define DDUMPN(n, b, l)							\
		do {							\
			if (umb_debug >= (n))				\
				umb_dump((b), (l));			\
		} while (0)

int	 umb_debug = 0;
Static char	*umb_uuid2str(uint8_t [MBIM_UUID_LEN]);
Static void	 umb_dump(void *, int);

#else
#define DPRINTF(x...)		do { } while (0)
#define DPRINTFN(n, x...)	do { } while (0)
#define DDUMPN(n, b, l)		do { } while (0)
#endif

#define DEVNAM(sc)		device_xname((sc)->sc_dev)

#ifndef notyet
#define usb_wait_task(dev, task)
#endif

/*
 * State change timeout
 */
#define UMB_STATE_CHANGE_TIMEOUT	30

/*
 * State change flags
 */
#define UMB_NS_DONT_DROP	0x0001	/* do not drop below current state */
#define UMB_NS_DONT_RAISE	0x0002	/* do not raise below current state */

/*
 * Diagnostic macros
 */
const struct umb_valdescr umb_regstates[] = MBIM_REGSTATE_DESCRIPTIONS;
const struct umb_valdescr umb_dataclasses[] = MBIM_DATACLASS_DESCRIPTIONS;
const struct umb_valdescr umb_simstate[] = MBIM_SIMSTATE_DESCRIPTIONS;
const struct umb_valdescr umb_messages[] = MBIM_MESSAGES_DESCRIPTIONS;
const struct umb_valdescr umb_status[] = MBIM_STATUS_DESCRIPTIONS;
const struct umb_valdescr umb_cids[] = MBIM_CID_DESCRIPTIONS;
const struct umb_valdescr umb_pktstate[] = MBIM_PKTSRV_STATE_DESCRIPTIONS;
const struct umb_valdescr umb_actstate[] = MBIM_ACTIVATION_STATE_DESCRIPTIONS;
const struct umb_valdescr umb_error[] = MBIM_ERROR_DESCRIPTIONS;
const struct umb_valdescr umb_pintype[] = MBIM_PINTYPE_DESCRIPTIONS;
const struct umb_valdescr umb_istate[] = UMB_INTERNAL_STATE_DESCRIPTIONS;

#define umb_regstate(c)		umb_val2descr(umb_regstates, (c))
#define umb_dataclass(c)	umb_val2descr(umb_dataclasses, (c))
#define umb_simstate(s)		umb_val2descr(umb_simstate, (s))
#define umb_request2str(m)	umb_val2descr(umb_messages, (m))
#define umb_status2str(s)	umb_val2descr(umb_status, (s))
#define umb_cid2str(c)		umb_val2descr(umb_cids, (c))
#define umb_packet_state(s)	umb_val2descr(umb_pktstate, (s))
#define umb_activation(s)	umb_val2descr(umb_actstate, (s))
#define umb_error2str(e)	umb_val2descr(umb_error, (e))
#define umb_pin_type(t)		umb_val2descr(umb_pintype, (t))
#define umb_istate(s)		umb_val2descr(umb_istate, (s))

Static int	 umb_match(device_t, cfdata_t, void *);
Static void	 umb_attach(device_t, device_t, void *);
Static int	 umb_detach(device_t, int);
Static int	 umb_activate(device_t, enum devact);
Static void	 umb_ncm_setup(struct umb_softc *);
Static int	 umb_alloc_xfers(struct umb_softc *);
Static void	 umb_free_xfers(struct umb_softc *);
Static int	 umb_alloc_bulkpipes(struct umb_softc *);
Static void	 umb_close_bulkpipes(struct umb_softc *);
Static int	 umb_ioctl(struct ifnet *, u_long, void *);
Static int	 umb_output(struct ifnet *, struct mbuf *,
		    const struct sockaddr *, const struct rtentry *);
Static void	 umb_input(struct ifnet *, struct mbuf *);
Static void	 umb_start(struct ifnet *);
Static void	 umb_watchdog(struct ifnet *);
Static void	 umb_statechg_timeout(void *);

Static int	 umb_mediachange(struct ifnet *);
Static void	 umb_mediastatus(struct ifnet *, struct ifmediareq *);

Static void	 umb_newstate(struct umb_softc *, enum umb_state, int);
Static void	 umb_state_task(void *);
Static void	 umb_up(struct umb_softc *);
Static void	 umb_down(struct umb_softc *, int);

Static void	 umb_get_response_task(void *);

Static void	 umb_decode_response(struct umb_softc *, void *, int);
Static void	 umb_handle_indicate_status_msg(struct umb_softc *, void *,
		    int);
Static void	 umb_handle_opendone_msg(struct umb_softc *, void *, int);
Static void	 umb_handle_closedone_msg(struct umb_softc *, void *, int);
Static int	 umb_decode_register_state(struct umb_softc *, void *, int);
Static int	 umb_decode_devices_caps(struct umb_softc *, void *, int);
Static int	 umb_decode_subscriber_status(struct umb_softc *, void *, int);
Static int	 umb_decode_radio_state(struct umb_softc *, void *, int);
Static int	 umb_decode_pin(struct umb_softc *, void *, int);
Static int	 umb_decode_packet_service(struct umb_softc *, void *, int);
Static int	 umb_decode_signal_state(struct umb_softc *, void *, int);
Static int	 umb_decode_connect_info(struct umb_softc *, void *, int);
Static int	 umb_decode_ip_configuration(struct umb_softc *, void *, int);
Static void	 umb_rx(struct umb_softc *);
Static void	 umb_rxeof(struct usbd_xfer *, void *, usbd_status);
Static int	 umb_encap(struct umb_softc *, struct mbuf *);
Static void	 umb_txeof(struct usbd_xfer *, void *, usbd_status);
Static void	 umb_decap(struct umb_softc *, struct usbd_xfer *);

Static usbd_status	 umb_send_encap_command(struct umb_softc *, void *, int);
Static int	 umb_get_encap_response(struct umb_softc *, void *, int *);
Static void	 umb_ctrl_msg(struct umb_softc *, uint32_t, void *, int);

Static void	 umb_open(struct umb_softc *);
Static void	 umb_close(struct umb_softc *);

Static int	 umb_setpin(struct umb_softc *, int, int, void *, int, void *,
		    int);
Static void	 umb_setdataclass(struct umb_softc *);
Static void	 umb_radio(struct umb_softc *, int);
Static void	 umb_allocate_cid(struct umb_softc *);
Static void	 umb_send_fcc_auth(struct umb_softc *);
Static void	 umb_packet_service(struct umb_softc *, int);
Static void	 umb_connect(struct umb_softc *);
Static void	 umb_disconnect(struct umb_softc *);
Static void	 umb_send_connect(struct umb_softc *, int);

Static void	 umb_qry_ipconfig(struct umb_softc *);
Static void	 umb_cmd(struct umb_softc *, int, int, const void *, int);
Static void	 umb_cmd1(struct umb_softc *, int, int, const void *, int, uint8_t *);
Static void	 umb_command_done(struct umb_softc *, void *, int);
Static void	 umb_decode_cid(struct umb_softc *, uint32_t, void *, int);
Static void	 umb_decode_qmi(struct umb_softc *, uint8_t *, int);

Static void	 umb_intr(struct usbd_xfer *, void *, usbd_status);

Static char	*umb_ntop(struct sockaddr *);

Static const char *
inet_ntop(int af, const void *src, char *dst, socklen_t size);
static const char *inet_ntop4(const u_char *src, char *dst, size_t size);
#ifdef INET6
static const char *inet_ntop6(const u_char *src, char *dst, size_t size);
#endif /* INET6 */

Static int	 umb_xfer_tout = USBD_DEFAULT_TIMEOUT;

Static uint8_t	 umb_uuid_basic_connect[] = MBIM_UUID_BASIC_CONNECT;
Static uint8_t	 umb_uuid_context_internet[] = MBIM_UUID_CONTEXT_INTERNET;
Static uint8_t	 umb_uuid_qmi_mbim[] = MBIM_UUID_QMI_MBIM;
Static uint32_t	 umb_session_id = 0;

CFATTACH_DECL_NEW(umb, sizeof(struct umb_softc), umb_match, umb_attach,
    umb_detach, umb_activate);

const int umb_delay = 4000;

/*
 * These devices require an "FCC Authentication" command.
 */
const struct usb_devno umb_fccauth_devs[] = {
	{ USB_VENDOR_SIERRA, USB_PRODUCT_SIERRA_EM7455 },
};

Static const uint8_t umb_qmi_alloc_cid[] = {
	0x01,
	0x0f, 0x00,		/* len */
	0x00,			/* QMUX flags */
	0x00,			/* service "ctl" */
	0x00,			/* CID */
	0x00,			/* QMI flags */
	0x01,			/* transaction */
	0x22, 0x00,		/* msg "Allocate CID" */
	0x04, 0x00,		/* TLV len */
	0x01, 0x01, 0x00, 0x02	/* TLV */
};

Static const uint8_t umb_qmi_fcc_auth[] = {
	0x01,
	0x0c, 0x00,		/* len */
	0x00,			/* QMUX flags */
	0x02,			/* service "dms" */
#define UMB_QMI_CID_OFFS	5
	0x00,			/* CID (filled in later) */
	0x00,			/* QMI flags */
	0x01, 0x00,		/* transaction */
	0x5f, 0x55,		/* msg "Send FCC Authentication" */
	0x00, 0x00		/* TLV len */
};

Static int
umb_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;
	usb_interface_descriptor_t *id;

	if (!uiaa->uiaa_iface)
		return UMATCH_NONE;
	if ((id = usbd_get_interface_descriptor(uiaa->uiaa_iface)) == NULL)
		return UMATCH_NONE;

	/*
	 * If this function implements NCM, check if alternate setting
	 * 1 implements MBIM.
	 */
	if (id->bInterfaceClass == UICLASS_CDC &&
	    id->bInterfaceSubClass ==
	    UISUBCLASS_NETWORK_CONTROL_MODEL)
		id = usbd_find_idesc(uiaa->uiaa_device->ud_cdesc, uiaa->uiaa_iface->ui_index, 1);
	if (id == NULL)
		return UMATCH_NONE;

	if (id->bInterfaceClass == UICLASS_CDC &&
	    id->bInterfaceSubClass ==
	    UISUBCLASS_MOBILE_BROADBAND_INTERFACE_MODEL &&
	    id->bInterfaceProtocol == 0)
		return UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO;

	return UMATCH_NONE;
}

Static void
umb_attach(device_t parent, device_t self, void *aux)
{
	struct umb_softc *sc = device_private(self);
	struct usbif_attach_arg *uiaa = aux;
	char *devinfop;
	usbd_status status;
	usbd_desc_iter_t iter;
	const usb_descriptor_t *desc;
	int	 v;
	const usb_cdc_union_descriptor_t *ud;
	const struct mbim_descriptor *md;
	int	 i;
	int	 ctrl_ep;
	const usb_interface_descriptor_t *id;
	usb_config_descriptor_t	*cd;
	usb_endpoint_descriptor_t *ed;
	const usb_interface_assoc_descriptor_t *ad;
	int	 current_ifaceno = -1;
	int	 data_ifaceno = -1;
	int	 altnum;
	int	 s;
	struct ifnet *ifp;
	int rv;

	sc->sc_dev = self;
	sc->sc_udev = uiaa->uiaa_device;

	aprint_naive("\n");
	aprint_normal("\n");

	devinfop = usbd_devinfo_alloc(sc->sc_udev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	sc->sc_ctrl_ifaceno = uiaa->uiaa_ifaceno;

	/*
	 * Some MBIM hardware does not provide the mandatory CDC Union
	 * Descriptor, so we also look at matching Interface
	 * Association Descriptors to find out the MBIM Data Interface
	 * number.
	 */
	sc->sc_ver_maj = sc->sc_ver_min = -1;
	sc->sc_maxpktlen = MBIM_MAXSEGSZ_MINVAL;
	usb_desc_iter_init(sc->sc_udev, &iter);
	while ((desc = usb_desc_iter_next(&iter))) {
		if (desc->bDescriptorType == UDESC_INTERFACE_ASSOC) {
			ad = (const usb_interface_assoc_descriptor_t *)desc;
			if (ad->bFirstInterface == uiaa->uiaa_ifaceno &&
			    ad->bInterfaceCount > 1)
				data_ifaceno = uiaa->uiaa_ifaceno + 1;
			continue;
		}
		if (desc->bDescriptorType == UDESC_INTERFACE) {
			id = (const usb_interface_descriptor_t *)desc;
			current_ifaceno = id->bInterfaceNumber;
			continue;
		}
		if (current_ifaceno != uiaa->uiaa_ifaceno)
			continue;
		if (desc->bDescriptorType != UDESC_CS_INTERFACE)
			continue;
		switch (desc->bDescriptorSubtype) {
		case UDESCSUB_CDC_UNION:
			ud = (const usb_cdc_union_descriptor_t *)desc;
			data_ifaceno = ud->bSlaveInterface[0];
			break;
		case UDESCSUB_MBIM:
			md = (const struct mbim_descriptor *)desc;
			v = UGETW(md->bcdMBIMVersion);
			sc->sc_ver_maj = MBIM_VER_MAJOR(v);
			sc->sc_ver_min = MBIM_VER_MINOR(v);
			sc->sc_ctrl_len = UGETW(md->wMaxControlMessage);
			/* Never trust a USB device! Could try to exploit us */
			if (sc->sc_ctrl_len < MBIM_CTRLMSG_MINLEN ||
			    sc->sc_ctrl_len > MBIM_CTRLMSG_MAXLEN) {
				DPRINTF("%s: control message len %d out of "
				    "bounds [%d .. %d]\n", DEVNAM(sc),
				    sc->sc_ctrl_len, MBIM_CTRLMSG_MINLEN,
				    MBIM_CTRLMSG_MAXLEN);
				/* cont. anyway */
			}
			sc->sc_maxpktlen = UGETW(md->wMaxSegmentSize);
			DPRINTFN(2, "%s: ctrl_len=%d, maxpktlen=%d, cap=0x%x\n",
			    DEVNAM(sc), sc->sc_ctrl_len, sc->sc_maxpktlen,
			    md->bmNetworkCapabilities);
			break;
		default:
			break;
		}
	}
	if (sc->sc_ver_maj < 0) {
		aprint_error_dev(self, "missing MBIM descriptor\n");
		goto fail;
	}

	aprint_normal_dev(self, "version %d.%d\n", sc->sc_ver_maj,
	    sc->sc_ver_min);

	if (usb_lookup(umb_fccauth_devs, uiaa->uiaa_vendor, uiaa->uiaa_product)) {
		sc->sc_flags |= UMBFLG_FCC_AUTH_REQUIRED;
		sc->sc_cid = -1;
	}

	for (i = 0; i < uiaa->uiaa_nifaces; i++) {
		id = usbd_get_interface_descriptor(uiaa->uiaa_ifaces[i]);
		if (id != NULL && id->bInterfaceNumber == data_ifaceno) {
			sc->sc_data_iface = uiaa->uiaa_ifaces[i];
		}
	}
	if (sc->sc_data_iface == NULL) {
		aprint_error_dev(self, "no data interface found\n");
		goto fail;
	}

	/*
	 * If this is a combined NCM/MBIM function, switch to
	 * alternate setting one to enable MBIM.
	 */
	id = usbd_get_interface_descriptor(uiaa->uiaa_iface);
	if (id->bInterfaceClass == UICLASS_CDC &&
	    id->bInterfaceSubClass ==
	    UISUBCLASS_NETWORK_CONTROL_MODEL)
		usbd_set_interface(uiaa->uiaa_iface, 1);

	id = usbd_get_interface_descriptor(uiaa->uiaa_iface);
	ctrl_ep = -1;
	for (i = 0; i < id->bNumEndpoints && ctrl_ep == -1; i++) {
		ed = usbd_interface2endpoint_descriptor(uiaa->uiaa_iface, i);
		if (ed == NULL)
			break;
		if (UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT &&
		    UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN)
			ctrl_ep = ed->bEndpointAddress;
	}
	if (ctrl_ep == -1) {
		aprint_error_dev(self, "missing interrupt endpoint\n");
		goto fail;
	}

	/*
	 * For the MBIM Data Interface, select the appropriate
	 * alternate setting by looking for a matching descriptor that
	 * has two endpoints.
	 */
	cd = usbd_get_config_descriptor(sc->sc_udev);
	altnum = usbd_get_no_alts(cd, data_ifaceno);
	for (i = 0; i < altnum; i++) {
		id = usbd_find_idesc(cd, sc->sc_data_iface->ui_index, i);
		if (id == NULL)
			continue;
		if (id->bInterfaceClass == UICLASS_CDC_DATA &&
		    id->bInterfaceSubClass == UISUBCLASS_DATA &&
		    id->bInterfaceProtocol == UIPROTO_DATA_MBIM &&
		    id->bNumEndpoints == 2)
			break;
	}
	if (i == altnum || id == NULL) {
		aprint_error_dev(self, "missing alt setting for interface #%d\n",
		    data_ifaceno);
		goto fail;
	}
	status = usbd_set_interface(sc->sc_data_iface, i);
	if (status) {
		aprint_error_dev(self, "select alt setting %d for interface #%d "
		    "failed: %s\n", i, data_ifaceno, usbd_errstr(status));
		goto fail;
	}

	id = usbd_get_interface_descriptor(sc->sc_data_iface);
	sc->sc_rx_ep = sc->sc_tx_ep = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		if ((ed = usbd_interface2endpoint_descriptor(sc->sc_data_iface,
		    i)) == NULL)
			break;
		if (UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK &&
		    UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN)
			sc->sc_rx_ep = ed->bEndpointAddress;
		else if (UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK &&
		    UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT)
			sc->sc_tx_ep = ed->bEndpointAddress;
	}
	if (sc->sc_rx_ep == -1 || sc->sc_tx_ep == -1) {
		aprint_error_dev(self, "missing bulk endpoints\n");
		goto fail;
	}

	DPRINTFN(2, "%s: ctrl-ifno#%d: ep-ctrl=%d, data-ifno#%d: ep-rx=%d, "
	    "ep-tx=%d\n", DEVNAM(sc), sc->sc_ctrl_ifaceno,
	    UE_GET_ADDR(ctrl_ep), data_ifaceno,
	    UE_GET_ADDR(sc->sc_rx_ep), UE_GET_ADDR(sc->sc_tx_ep));

	usb_init_task(&sc->sc_umb_task, umb_state_task, sc,
	    0);
	usb_init_task(&sc->sc_get_response_task, umb_get_response_task, sc,
	    0);
	callout_init(&sc->sc_statechg_timer, 0);
	callout_setfunc(&sc->sc_statechg_timer, umb_statechg_timeout, sc);

	if (usbd_open_pipe_intr(uiaa->uiaa_iface, ctrl_ep, USBD_SHORT_XFER_OK,
	    &sc->sc_ctrl_pipe, sc, &sc->sc_intr_msg, sizeof(sc->sc_intr_msg),
	    umb_intr, USBD_DEFAULT_INTERVAL)) {
		aprint_error_dev(self, "failed to open control pipe\n");
		goto fail;
	}
	sc->sc_resp_buf = kmem_alloc(sc->sc_ctrl_len, KM_NOSLEEP);
	if (sc->sc_resp_buf == NULL) {
		aprint_error_dev(self, "allocation of resp buffer failed\n");
		goto fail;
	}
	sc->sc_ctrl_msg = kmem_alloc(sc->sc_ctrl_len, KM_NOSLEEP);
	if (sc->sc_ctrl_msg == NULL) {
		aprint_error_dev(self, "allocation of ctrl msg buffer failed\n");
		goto fail;
	}

	sc->sc_info.regstate = MBIM_REGSTATE_UNKNOWN;
	sc->sc_info.pin_attempts_left = UMB_VALUE_UNKNOWN;
	sc->sc_info.rssi = UMB_VALUE_UNKNOWN;
	sc->sc_info.ber = UMB_VALUE_UNKNOWN;

	umb_ncm_setup(sc);
	DPRINTFN(2, "%s: rx/tx size %d/%d\n", DEVNAM(sc),
	    sc->sc_rx_bufsz, sc->sc_tx_bufsz);

	s = splnet();

	/* initialize the interface */
	ifp = GET_IFP(sc);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_SIMPLEX | IFF_MULTICAST | IFF_POINTOPOINT;
	ifp->if_ioctl = umb_ioctl;
	ifp->if_start = umb_start;

	ifp->if_watchdog = umb_watchdog;
	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_link_state = LINK_STATE_DOWN;
	ifmedia_init(&sc->sc_im, 0, umb_mediachange, umb_mediastatus);

	ifp->if_type = IFT_MBIM;
	ifp->if_addrlen = 0;
	ifp->if_hdrlen = sizeof(struct ncm_header16) +
	    sizeof(struct ncm_pointer16);
	ifp->if_mtu = 1500;		/* use a common default */
	ifp->if_mtu = sc->sc_maxpktlen;
	ifp->if_output = umb_output;
	ifp->_if_input = umb_input;
	IFQ_SET_READY(&ifp->if_snd);

	/* attach the interface */
	rv = if_initialize(ifp);
	if (rv != 0) {
		aprint_error_dev(self, "if_initialize failed(%d)\n", rv);
		splx(s);
		return;
	}
	if_register(ifp);
	if_alloc_sadl(ifp);

	bpf_attach(ifp, DLT_RAW, 0);
	rnd_attach_source(&sc->sc_rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	/*
	 * Open the device now so that we are able to query device information.
	 * XXX maybe close when done?
	 */
	umb_open(sc);

	sc->sc_attached = 1;
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;

fail:
	umb_activate(sc->sc_dev, DVACT_DEACTIVATE);
	return;
}

Static int
umb_detach(device_t self, int flags)
{
	struct umb_softc *sc = (struct umb_softc *)self;
	struct ifnet *ifp = GET_IFP(sc);
	int	 s;

	pmf_device_deregister(self);

	s = splnet();
	if (ifp->if_flags & IFF_RUNNING)
		umb_down(sc, 1);
	umb_close(sc);

	usb_rem_task(sc->sc_udev, &sc->sc_get_response_task);
	usb_wait_task(sc->sc_udev, &sc->sc_get_response_task);
	sc->sc_nresp = 0;
	if (sc->sc_rx_ep != -1 && sc->sc_tx_ep != -1) {
		callout_destroy(&sc->sc_statechg_timer);
		usb_rem_task(sc->sc_udev, &sc->sc_umb_task);
		usb_wait_task(sc->sc_udev, &sc->sc_umb_task);
	}
	if (sc->sc_ctrl_pipe) {
		usbd_close_pipe(sc->sc_ctrl_pipe);
		sc->sc_ctrl_pipe = NULL;
	}
	if (sc->sc_ctrl_msg) {
		kmem_free(sc->sc_ctrl_msg, sc->sc_ctrl_len);
		sc->sc_ctrl_msg = NULL;
	}
	if (sc->sc_resp_buf) {
		kmem_free(sc->sc_resp_buf, sc->sc_ctrl_len);
		sc->sc_resp_buf = NULL;
	}
	if (ifp->if_softc) {
		ifmedia_delete_instance(&sc->sc_im, IFM_INST_ANY);
	}
	if (sc->sc_attached) {
		rnd_detach_source(&sc->sc_rnd_source);
		bpf_detach(ifp);
		if_detach(ifp);
	}

	sc->sc_attached = 0;
	splx(s);
	return 0;
}

Static int
umb_activate(device_t self, enum devact act)
{
	struct umb_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(GET_IFP(sc));
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

Static void
umb_ncm_setup(struct umb_softc *sc)
{
	usb_device_request_t req;
	struct ncm_ntb_parameters np;

	/* Query NTB tranfers sizes */
	req.bmRequestType = UT_READ_CLASS_INTERFACE;
	req.bRequest = NCM_GET_NTB_PARAMETERS;
	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_ctrl_ifaceno);
	USETW(req.wLength, sizeof(np));
	if (usbd_do_request(sc->sc_udev, &req, &np) == USBD_NORMAL_COMPLETION &&
	    UGETW(np.wLength) == sizeof(np)) {
		sc->sc_rx_bufsz = UGETDW(np.dwNtbInMaxSize);
		sc->sc_tx_bufsz = UGETDW(np.dwNtbOutMaxSize);
	} else
		sc->sc_rx_bufsz = sc->sc_tx_bufsz = 8 * 1024;
}

Static int
umb_alloc_xfers(struct umb_softc *sc)
{
	int err = 0;

	if (!sc->sc_rx_xfer) {
		err |= usbd_create_xfer(sc->sc_rx_pipe,
		    sc->sc_rx_bufsz,
		    0, 0, &sc->sc_rx_xfer);
	}
	if (!sc->sc_tx_xfer) {
		err |= usbd_create_xfer(sc->sc_tx_pipe,
		    sc->sc_tx_bufsz,
		    0, 0, &sc->sc_tx_xfer);
	}
	if (err)
		return err;

	sc->sc_rx_buf = usbd_get_buffer(sc->sc_rx_xfer);
	sc->sc_tx_buf = usbd_get_buffer(sc->sc_tx_xfer);

	return 0;
}

Static void
umb_free_xfers(struct umb_softc *sc)
{
	if (sc->sc_rx_xfer) {
		/* implicit usbd_free_buffer() */
		usbd_destroy_xfer(sc->sc_rx_xfer);
		sc->sc_rx_xfer = NULL;
		sc->sc_rx_buf = NULL;
	}
	if (sc->sc_tx_xfer) {
		usbd_destroy_xfer(sc->sc_tx_xfer);
		sc->sc_tx_xfer = NULL;
		sc->sc_tx_buf = NULL;
	}
	if (sc->sc_tx_m) {
		m_freem(sc->sc_tx_m);
		sc->sc_tx_m = NULL;
	}
}

Static int
umb_alloc_bulkpipes(struct umb_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);
	int rv;

	if (!(ifp->if_flags & IFF_RUNNING)) {
		if ((rv = usbd_open_pipe(sc->sc_data_iface, sc->sc_rx_ep,
		    USBD_EXCLUSIVE_USE, &sc->sc_rx_pipe))) {
			DPRINTFN(4, "usbd_open_pipe() failed (RX) %d\n", rv);
			return 0;
		}
		if ((rv = usbd_open_pipe(sc->sc_data_iface, sc->sc_tx_ep,
		    USBD_EXCLUSIVE_USE, &sc->sc_tx_pipe))) {
			DPRINTFN(4, "usbd_open_pipe() failed (TX) %d\n", rv);
			return 0;
		}

		if ((rv = umb_alloc_xfers(sc)) != 0) {
			DPRINTFN(4, "umb_alloc_xfers() failed %d\n", rv);
			return 0;
		}

		ifp->if_flags |= IFF_RUNNING;
		ifp->if_flags &= ~IFF_OACTIVE;
		umb_rx(sc);
	}
	return 1;
}

Static void
umb_close_bulkpipes(struct umb_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
	if (sc->sc_rx_pipe) {
		usbd_close_pipe(sc->sc_rx_pipe);
		sc->sc_rx_pipe = NULL;
	}
	if (sc->sc_tx_pipe) {
		usbd_close_pipe(sc->sc_tx_pipe);
		sc->sc_tx_pipe = NULL;
	}
}

Static int
umb_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct umb_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;
	struct umb_parameter mp;

	if (sc->sc_dying)
		return EIO;

	s = splnet();
	switch (cmd) {
	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;
		usb_add_task(sc->sc_udev, &sc->sc_umb_task, USB_TASKQ_DRIVER);
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			break;
#endif /* INET */
#ifdef INET6
		case AF_INET6:
			break;
#endif /* INET6 */
		default:
			error = EAFNOSUPPORT;
			break;
		}
		ifa->ifa_rtrequest = p2p_rtrequest;
		break;
	case SIOCSIFFLAGS:
		error = ifioctl_common(ifp, cmd, data);
		if (error)
			break;
		usb_add_task(sc->sc_udev, &sc->sc_umb_task, USB_TASKQ_DRIVER);
		break;
	case SIOCGUMBINFO:
		error = copyout(&sc->sc_info, ifr->ifr_data,
		    sizeof(sc->sc_info));
		break;
	case SIOCSUMBPARAM:
		error = kauth_authorize_network(curlwp->l_cred,
		    KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp, KAUTH_ARG(cmd),
		    NULL);
		if (error)
			break;

		if ((error = copyin(ifr->ifr_data, &mp, sizeof(mp))) != 0)
			break;

		if ((error = umb_setpin(sc, mp.op, mp.is_puk, mp.pin, mp.pinlen,
		    mp.newpin, mp.newpinlen)) != 0)
			break;

		if (mp.apnlen < 0 || mp.apnlen > sizeof(sc->sc_info.apn)) {
			error = EINVAL;
			break;
		}
		sc->sc_roaming = mp.roaming ? 1 : 0;
		memset(sc->sc_info.apn, 0, sizeof(sc->sc_info.apn));
		memcpy(sc->sc_info.apn, mp.apn, mp.apnlen);
		sc->sc_info.apnlen = mp.apnlen;
		memset(sc->sc_info.username, 0, sizeof(sc->sc_info.username));
		memcpy(sc->sc_info.username, mp.username, mp.usernamelen);
		sc->sc_info.usernamelen = mp.usernamelen;
		memset(sc->sc_info.password, 0, sizeof(sc->sc_info.password));
		memcpy(sc->sc_info.password, mp.password, mp.passwordlen);
		sc->sc_info.passwordlen = mp.passwordlen;
		sc->sc_info.preferredclasses = mp.preferredclasses;
		umb_setdataclass(sc);
		break;
	case SIOCGUMBPARAM:
		memset(&mp, 0, sizeof(mp));
		memcpy(mp.apn, sc->sc_info.apn, sc->sc_info.apnlen);
		mp.apnlen = sc->sc_info.apnlen;
		mp.roaming = sc->sc_roaming;
		mp.preferredclasses = sc->sc_info.preferredclasses;
		error = copyout(&mp, ifr->ifr_data, sizeof(mp));
		break;
	case SIOCSIFMTU:
		/* Does this include the NCM headers and tail? */
		if (ifr->ifr_mtu > ifp->if_mtu) {
			error = EINVAL;
			break;
		}
		ifp->if_mtu = ifr->ifr_mtu;
		break;
	case SIOCSIFADDR:
	case SIOCAIFADDR:
	case SIOCSIFDSTADDR:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		break;
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_im, cmd);
		break;
	default:
		error = ifioctl_common(ifp, cmd, data);
		break;
	}
	splx(s);
	return error;
}

Static int
umb_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
    const struct rtentry *rtp)
{
	int error;

	DPRINTFN(10, "%s: %s: enter\n",
		     device_xname(((struct umb_softc *)ifp->if_softc)->sc_dev),
		     __func__);

	/*
	 * if the queueing discipline needs packet classification,
	 * do it now.
	 */
	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family);

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	error = if_transmit_lock(ifp, m);

	return error;
}

Static void
umb_input(struct ifnet *ifp, struct mbuf *m)
{
	size_t pktlen = m->m_len;
	int s;

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}
	if (pktlen < sizeof(struct ip)) {
		ifp->if_ierrors++;
		DPRINTFN(4, "%s: dropping short packet (len %zd)\n", __func__,
		    pktlen);
		m_freem(m);
		return;
	}
	s = splnet();
	if (__predict_false(!pktq_enqueue(ip_pktq, m, 0))) {
		ifp->if_iqdrops++;
		m_freem(m);
	} else {
		ifp->if_ipackets++;
		ifp->if_ibytes += pktlen;
	}
	splx(s);
}

Static void
umb_start(struct ifnet *ifp)
{
	struct umb_softc *sc = ifp->if_softc;
	struct mbuf *m_head = NULL;

	if (sc->sc_dying || (ifp->if_flags & IFF_OACTIVE))
		return;

	IFQ_POLL(&ifp->if_snd, m_head);
	if (m_head == NULL)
		return;

	if (!umb_encap(sc, m_head)) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}
	IFQ_DEQUEUE(&ifp->if_snd, m_head);

	bpf_mtap(ifp, m_head);

	ifp->if_flags |= IFF_OACTIVE;
	ifp->if_timer = (2 * umb_xfer_tout) / 1000;
}

Static void
umb_watchdog(struct ifnet *ifp)
{
	struct umb_softc *sc = ifp->if_softc;

	if (sc->sc_dying)
		return;

	ifp->if_oerrors++;
	printf("%s: watchdog timeout\n", DEVNAM(sc));
	usbd_abort_pipe(sc->sc_tx_pipe);
	return;
}

Static void
umb_statechg_timeout(void *arg)
{
	struct umb_softc *sc = arg;

	if (sc->sc_info.regstate == MBIM_REGSTATE_ROAMING && !sc->sc_roaming) {
		/*
		 * Query the registration state until we're with the home
		 * network again.
		 */
		umb_cmd(sc, MBIM_CID_REGISTER_STATE, MBIM_CMDOP_QRY, NULL, 0);
	} else
		printf("%s: state change timeout\n",DEVNAM(sc));
	usb_add_task(sc->sc_udev, &sc->sc_umb_task, USB_TASKQ_DRIVER);
}

Static int
umb_mediachange(struct ifnet * ifp)
{
	return 0;
}

Static void
umb_mediastatus(struct ifnet * ifp, struct ifmediareq * imr)
{
	switch (ifp->if_link_state) {
	case LINK_STATE_UP:
		imr->ifm_status = IFM_AVALID | IFM_ACTIVE;
		break;
	case LINK_STATE_DOWN:
		imr->ifm_status = IFM_AVALID;
		break;
	default:
		imr->ifm_status = 0;
		break;
	}
}

Static void
umb_newstate(struct umb_softc *sc, enum umb_state newstate, int flags)
{
	struct ifnet *ifp = GET_IFP(sc);

	if (newstate == sc->sc_state)
		return;
	if (((flags & UMB_NS_DONT_DROP) && newstate < sc->sc_state) ||
	    ((flags & UMB_NS_DONT_RAISE) && newstate > sc->sc_state))
		return;
	if (ifp->if_flags & IFF_DEBUG)
		log(LOG_DEBUG, "%s: state going %s from '%s' to '%s'\n",
		    DEVNAM(sc), newstate > sc->sc_state ? "up" : "down",
		    umb_istate(sc->sc_state), umb_istate(newstate));
	sc->sc_state = newstate;
	usb_add_task(sc->sc_udev, &sc->sc_umb_task, USB_TASKQ_DRIVER);
}

Static void
umb_state_task(void *arg)
{
	struct umb_softc *sc = arg;
	struct ifnet *ifp = GET_IFP(sc);
	struct ifreq ifr;
	int	 s;
	int	 state;

	s = splnet();
	if (ifp->if_flags & IFF_UP)
		umb_up(sc);
	else
		umb_down(sc, 0);

	state = sc->sc_state == UMB_S_UP ? LINK_STATE_UP : LINK_STATE_DOWN;
	if (ifp->if_link_state != state) {
		if (ifp->if_flags & IFF_DEBUG)
			log(LOG_DEBUG, "%s: link state changed from %s to %s\n",
			    DEVNAM(sc),
			    (ifp->if_link_state == LINK_STATE_UP)
			    ? "up" : "down",
			    (state == LINK_STATE_UP) ? "up" : "down");
		ifp->if_link_state = state;
		if (state != LINK_STATE_UP) {
			/*
			 * Purge any existing addresses
			 */
			memset(sc->sc_info.ipv4dns, 0,
			    sizeof(sc->sc_info.ipv4dns));
			if (in_control(NULL, SIOCGIFADDR, &ifr, ifp) == 0 &&
			    satosin(&ifr.ifr_addr)->sin_addr.s_addr !=
			    INADDR_ANY) {
				in_control(NULL, SIOCDIFADDR, &ifr, ifp);
			}
		}
		if_link_state_change(ifp, state);
	}
	splx(s);
}

Static void
umb_up(struct umb_softc *sc)
{
	switch (sc->sc_state) {
	case UMB_S_DOWN:
		DPRINTF("%s: init: opening ...\n", DEVNAM(sc));
		umb_open(sc);
		break;
	case UMB_S_OPEN:
		if (sc->sc_flags & UMBFLG_FCC_AUTH_REQUIRED) {
			if (sc->sc_cid == -1) {
				DPRINTF("%s: init: allocating CID ...\n",
				    DEVNAM(sc));
				umb_allocate_cid(sc);
				break;
			} else
				umb_newstate(sc, UMB_S_CID, UMB_NS_DONT_DROP);
		} else {
			DPRINTF("%s: init: turning radio on ...\n", DEVNAM(sc));
			umb_radio(sc, 1);
			break;
		}
		/*FALLTHROUGH*/
	case UMB_S_CID:
		DPRINTF("%s: init: sending FCC auth ...\n", DEVNAM(sc));
		umb_send_fcc_auth(sc);
		break;
	case UMB_S_RADIO:
		DPRINTF("%s: init: checking SIM state ...\n", DEVNAM(sc));
		umb_cmd(sc, MBIM_CID_SUBSCRIBER_READY_STATUS, MBIM_CMDOP_QRY,
		    NULL, 0);
		break;
	case UMB_S_SIMREADY:
		DPRINTF("%s: init: attaching ...\n", DEVNAM(sc));
		umb_packet_service(sc, 1);
		break;
	case UMB_S_ATTACHED:
		sc->sc_tx_seq = 0;
		DPRINTF("%s: init: connecting ...\n", DEVNAM(sc));
		umb_connect(sc);
		break;
	case UMB_S_CONNECTED:
		DPRINTF("%s: init: getting IP config ...\n", DEVNAM(sc));
		umb_qry_ipconfig(sc);
		break;
	case UMB_S_UP:
		DPRINTF("%s: init: reached state UP\n", DEVNAM(sc));
		if (!umb_alloc_bulkpipes(sc)) {
			printf("%s: opening bulk pipes failed\n", DEVNAM(sc));
			umb_down(sc, 1);
		}
		break;
	}
	if (sc->sc_state < UMB_S_UP)
		callout_schedule(&sc->sc_statechg_timer,
		    UMB_STATE_CHANGE_TIMEOUT * hz);
	else
		callout_stop(&sc->sc_statechg_timer);
	return;
}

Static void
umb_down(struct umb_softc *sc, int force)
{
	umb_close_bulkpipes(sc);
	if (sc->sc_state < UMB_S_CONNECTED)
		umb_free_xfers(sc);

	switch (sc->sc_state) {
	case UMB_S_UP:
	case UMB_S_CONNECTED:
		DPRINTF("%s: stop: disconnecting ...\n", DEVNAM(sc));
		umb_disconnect(sc);
		if (!force)
			break;
		/*FALLTHROUGH*/
	case UMB_S_ATTACHED:
		DPRINTF("%s: stop: detaching ...\n", DEVNAM(sc));
		umb_packet_service(sc, 0);
		if (!force)
			break;
		/*FALLTHROUGH*/
	case UMB_S_SIMREADY:
	case UMB_S_RADIO:
		DPRINTF("%s: stop: turning radio off ...\n", DEVNAM(sc));
		umb_radio(sc, 0);
		if (!force)
			break;
		/*FALLTHROUGH*/
	case UMB_S_CID:
	case UMB_S_OPEN:
	case UMB_S_DOWN:
		/* Do not close the device */
		DPRINTF("%s: stop: reached state DOWN\n", DEVNAM(sc));
		break;
	}
	if (force)
		sc->sc_state = UMB_S_OPEN;

	if (sc->sc_state > UMB_S_OPEN)
		callout_schedule(&sc->sc_statechg_timer,
		    UMB_STATE_CHANGE_TIMEOUT * hz);
	else
		callout_stop(&sc->sc_statechg_timer);
}

Static void
umb_get_response_task(void *arg)
{
	struct umb_softc *sc = arg;
	int	 len;
	int	 s;

	/*
	 * Function is required to send on RESPONSE_AVAILABLE notification for
	 * each encapsulated response that is to be processed by the host.
	 * But of course, we can receive multiple notifications before the
	 * response task is run.
	 */
	s = splusb();
	while (sc->sc_nresp > 0) {
		--sc->sc_nresp;
		len = sc->sc_ctrl_len;
		if (umb_get_encap_response(sc, sc->sc_resp_buf, &len))
			umb_decode_response(sc, sc->sc_resp_buf, len);
	}
	splx(s);
}

Static void
umb_decode_response(struct umb_softc *sc, void *response, int len)
{
	struct mbim_msghdr *hdr = response;
	struct mbim_fragmented_msg_hdr *fraghdr;
	uint32_t type;

	DPRINTFN(3, "%s: got response: len %d\n", DEVNAM(sc), len);
	DDUMPN(4, response, len);

	if (len < sizeof(*hdr) || le32toh(hdr->len) != len) {
		/*
		 * We should probably cancel a transaction, but since the
		 * message is too short, we cannot decode the transaction
		 * id (tid) and hence don't know, whom to cancel. Must wait
		 * for the timeout.
		 */
		DPRINTF("%s: received short response (len %d)\n",
		    DEVNAM(sc), len);
		return;
	}

	/*
	 * XXX FIXME: if message is fragmented, store it until last frag
	 *	is received and then re-assemble all fragments.
	 */
	type = le32toh(hdr->type);
	switch (type) {
	case MBIM_INDICATE_STATUS_MSG:
	case MBIM_COMMAND_DONE:
		fraghdr = response;
		if (le32toh(fraghdr->frag.nfrag) != 1) {
			DPRINTF("%s: discarding fragmented messages\n",
			    DEVNAM(sc));
			return;
		}
		break;
	default:
		break;
	}

	DPRINTF("%s: <- rcv %s (tid %u)\n", DEVNAM(sc), umb_request2str(type),
	    le32toh(hdr->tid));
	switch (type) {
	case MBIM_FUNCTION_ERROR_MSG:
	case MBIM_HOST_ERROR_MSG:
	{
		struct mbim_f2h_hosterr *e;
		int	 err;

		if (len >= sizeof(*e)) {
			e = response;
			err = le32toh(e->err);

			DPRINTF("%s: %s message, error %s (tid %u)\n",
			    DEVNAM(sc), umb_request2str(type),
			    umb_error2str(err), le32toh(hdr->tid));
			if (err == MBIM_ERROR_NOT_OPENED)
				umb_newstate(sc, UMB_S_DOWN, 0);
		}
		break;
	}
	case MBIM_INDICATE_STATUS_MSG:
		umb_handle_indicate_status_msg(sc, response, len);
		break;
	case MBIM_OPEN_DONE:
		umb_handle_opendone_msg(sc, response, len);
		break;
	case MBIM_CLOSE_DONE:
		umb_handle_closedone_msg(sc, response, len);
		break;
	case MBIM_COMMAND_DONE:
		umb_command_done(sc, response, len);
		break;
	default:
		DPRINTF("%s: discard messsage %s\n", DEVNAM(sc),
		    umb_request2str(type));
		break;
	}
}

Static void
umb_handle_indicate_status_msg(struct umb_softc *sc, void *data, int len)
{
	struct mbim_f2h_indicate_status *m = data;
	uint32_t infolen;
	uint32_t cid;

	if (len < sizeof(*m)) {
		DPRINTF("%s: discard short %s messsage\n", DEVNAM(sc),
		    umb_request2str(le32toh(m->hdr.type)));
		return;
	}
	if (memcmp(m->devid, umb_uuid_basic_connect, sizeof(m->devid))) {
		DPRINTF("%s: discard %s messsage for other UUID '%s'\n",
		    DEVNAM(sc), umb_request2str(le32toh(m->hdr.type)),
		    umb_uuid2str(m->devid));
		return;
	}
	infolen = le32toh(m->infolen);
	if (len < sizeof(*m) + infolen) {
		DPRINTF("%s: discard truncated %s messsage (want %d, got %d)\n",
		    DEVNAM(sc), umb_request2str(le32toh(m->hdr.type)),
		    (int)sizeof(*m) + infolen, len);
		return;
	}

	cid = le32toh(m->cid);
	DPRINTF("%s: indicate %s status\n", DEVNAM(sc), umb_cid2str(cid));
	umb_decode_cid(sc, cid, m->info, infolen);
}

Static void
umb_handle_opendone_msg(struct umb_softc *sc, void *data, int len)
{
	struct mbim_f2h_openclosedone *resp = data;
	struct ifnet *ifp = GET_IFP(sc);
	uint32_t status;

	status = le32toh(resp->status);
	if (status == MBIM_STATUS_SUCCESS) {
		if (sc->sc_maxsessions == 0) {
			umb_cmd(sc, MBIM_CID_DEVICE_CAPS, MBIM_CMDOP_QRY, NULL,
			    0);
			umb_cmd(sc, MBIM_CID_PIN, MBIM_CMDOP_QRY, NULL, 0);
			umb_cmd(sc, MBIM_CID_REGISTER_STATE, MBIM_CMDOP_QRY,
			    NULL, 0);
		}
		umb_newstate(sc, UMB_S_OPEN, UMB_NS_DONT_DROP);
	} else if (ifp->if_flags & IFF_DEBUG)
		log(LOG_ERR, "%s: open error: %s\n", DEVNAM(sc),
		    umb_status2str(status));
	return;
}

Static void
umb_handle_closedone_msg(struct umb_softc *sc, void *data, int len)
{
	struct mbim_f2h_openclosedone *resp = data;
	uint32_t status;

	status = le32toh(resp->status);
	if (status == MBIM_STATUS_SUCCESS)
		umb_newstate(sc, UMB_S_DOWN, 0);
	else
		DPRINTF("%s: close error: %s\n", DEVNAM(sc),
		    umb_status2str(status));
	return;
}

static inline void
umb_getinfobuf(char *in, int inlen, uint32_t offs, uint32_t sz,
    void *out, size_t outlen)
{
	offs = le32toh(offs);
	sz = le32toh(sz);
	if (inlen >= offs + sz) {
		memset(out, 0, outlen);
		memcpy(out, in + offs, MIN(sz, outlen));
	}
}

static inline int
umb_padding(void *data, int len, size_t sz)
{
	char *p = data;
	int np = 0;

	while (len < sz && (len % 4) != 0) {
		*p++ = '\0';
		len++;
		np++;
	}
	return np;
}

static inline int
umb_addstr(void *buf, size_t bufsz, int *offs, void *str, int slen,
    uint32_t *offsmember, uint32_t *sizemember)
{
	if (*offs + slen > bufsz)
		return 0;

	*sizemember = htole32((uint32_t)slen);
	if (slen && str) {
		*offsmember = htole32((uint32_t)*offs);
		memcpy((char *)buf + *offs, str, slen);
		*offs += slen;
		*offs += umb_padding(buf, *offs, bufsz);
	} else
		*offsmember = htole32(0);
	return 1;
}

static void
umb_in_len2mask(struct in_addr *mask, int len)
{
	int i;
	u_char *p;

	p = (u_char *)mask;
	memset(mask, 0, sizeof(*mask));
	for (i = 0; i < len / 8; i++)
		p[i] = 0xff;
	if (len % 8)
		p[i] = (0xff00 >> (len % 8)) & 0xff;
}

Static int
umb_decode_register_state(struct umb_softc *sc, void *data, int len)
{
	struct mbim_cid_registration_state_info *rs = data;
	struct ifnet *ifp = GET_IFP(sc);

	if (len < sizeof(*rs))
		return 0;
	sc->sc_info.nwerror = le32toh(rs->nwerror);
	sc->sc_info.regstate = le32toh(rs->regstate);
	sc->sc_info.regmode = le32toh(rs->regmode);
	sc->sc_info.cellclass = le32toh(rs->curcellclass);

	/* XXX should we remember the provider_id? */
	umb_getinfobuf(data, len, rs->provname_offs, rs->provname_size,
	    sc->sc_info.provider, sizeof(sc->sc_info.provider));
	umb_getinfobuf(data, len, rs->roamingtxt_offs, rs->roamingtxt_size,
	    sc->sc_info.roamingtxt, sizeof(sc->sc_info.roamingtxt));

	DPRINTFN(2, "%s: %s, availclass 0x%x, class 0x%x, regmode %d\n",
	    DEVNAM(sc), umb_regstate(sc->sc_info.regstate),
	    le32toh(rs->availclasses), sc->sc_info.cellclass,
	    sc->sc_info.regmode);

	if (sc->sc_info.regstate == MBIM_REGSTATE_ROAMING &&
	    !sc->sc_roaming &&
	    sc->sc_info.activation == MBIM_ACTIVATION_STATE_ACTIVATED) {
		if (ifp->if_flags & IFF_DEBUG)
			log(LOG_INFO,
			    "%s: disconnecting from roaming network\n",
			    DEVNAM(sc));
		umb_disconnect(sc);
	}
	return 1;
}

Static int
umb_decode_devices_caps(struct umb_softc *sc, void *data, int len)
{
	struct mbim_cid_device_caps *dc = data;

	if (len < sizeof(*dc))
		return 0;
	sc->sc_maxsessions = le32toh(dc->max_sessions);
	sc->sc_info.supportedclasses = le32toh(dc->dataclass);
	umb_getinfobuf(data, len, dc->devid_offs, dc->devid_size,
	    sc->sc_info.devid, sizeof(sc->sc_info.devid));
	umb_getinfobuf(data, len, dc->fwinfo_offs, dc->fwinfo_size,
	    sc->sc_info.fwinfo, sizeof(sc->sc_info.fwinfo));
	umb_getinfobuf(data, len, dc->hwinfo_offs, dc->hwinfo_size,
	    sc->sc_info.hwinfo, sizeof(sc->sc_info.hwinfo));
	DPRINTFN(2, "%s: max sessions %d, supported classes 0x%x\n",
	    DEVNAM(sc), sc->sc_maxsessions, sc->sc_info.supportedclasses);
	return 1;
}

Static int
umb_decode_subscriber_status(struct umb_softc *sc, void *data, int len)
{
	struct mbim_cid_subscriber_ready_info *si = data;
	struct ifnet *ifp = GET_IFP(sc);
	int	npn;

	if (len < sizeof(*si))
		return 0;
	sc->sc_info.sim_state = le32toh(si->ready);

	umb_getinfobuf(data, len, si->sid_offs, si->sid_size,
	    sc->sc_info.sid, sizeof(sc->sc_info.sid));
	umb_getinfobuf(data, len, si->icc_offs, si->icc_size,
	    sc->sc_info.iccid, sizeof(sc->sc_info.iccid));

	npn = le32toh(si->no_pn);
	if (npn > 0)
		umb_getinfobuf(data, len, si->pn[0].offs, si->pn[0].size,
		    sc->sc_info.pn, sizeof(sc->sc_info.pn));
	else
		memset(sc->sc_info.pn, 0, sizeof(sc->sc_info.pn));

	if (sc->sc_info.sim_state == MBIM_SIMSTATE_LOCKED)
		sc->sc_info.pin_state = UMB_PUK_REQUIRED;
	if (ifp->if_flags & IFF_DEBUG)
		log(LOG_INFO, "%s: SIM %s\n", DEVNAM(sc),
		    umb_simstate(sc->sc_info.sim_state));
	if (sc->sc_info.sim_state == MBIM_SIMSTATE_INITIALIZED)
		umb_newstate(sc, UMB_S_SIMREADY, UMB_NS_DONT_DROP);
	return 1;
}

Static int
umb_decode_radio_state(struct umb_softc *sc, void *data, int len)
{
	struct mbim_cid_radio_state_info *rs = data;
	struct ifnet *ifp = GET_IFP(sc);

	if (len < sizeof(*rs))
		return 0;

	sc->sc_info.hw_radio_on =
	    (le32toh(rs->hw_state) == MBIM_RADIO_STATE_ON) ? 1 : 0;
	sc->sc_info.sw_radio_on =
	    (le32toh(rs->sw_state) == MBIM_RADIO_STATE_ON) ? 1 : 0;
	if (!sc->sc_info.hw_radio_on) {
		printf("%s: radio is disabled by hardware switch\n",
		    DEVNAM(sc));
		/*
		 * XXX do we need a time to poll the state of the rfkill switch
		 *	or will the device send an unsolicited notification
		 *	in case the state changes?
		 */
		umb_newstate(sc, UMB_S_OPEN, 0);
	} else if (!sc->sc_info.sw_radio_on) {
		if (ifp->if_flags & IFF_DEBUG)
			log(LOG_INFO, "%s: radio is off\n", DEVNAM(sc));
		umb_newstate(sc, UMB_S_OPEN, 0);
	} else
		umb_newstate(sc, UMB_S_RADIO, UMB_NS_DONT_DROP);
	return 1;
}

Static int
umb_decode_pin(struct umb_softc *sc, void *data, int len)
{
	struct mbim_cid_pin_info *pi = data;
	struct ifnet *ifp = GET_IFP(sc);
	uint32_t	attempts_left;

	if (len < sizeof(*pi))
		return 0;

	attempts_left = le32toh(pi->remaining_attempts);
	if (attempts_left != 0xffffffff)
		sc->sc_info.pin_attempts_left = attempts_left;

	switch (le32toh(pi->state)) {
	case MBIM_PIN_STATE_UNLOCKED:
		sc->sc_info.pin_state = UMB_PIN_UNLOCKED;
		break;
	case MBIM_PIN_STATE_LOCKED:
		switch (le32toh(pi->type)) {
		case MBIM_PIN_TYPE_PIN1:
			sc->sc_info.pin_state = UMB_PIN_REQUIRED;
			break;
		case MBIM_PIN_TYPE_PUK1:
			sc->sc_info.pin_state = UMB_PUK_REQUIRED;
			break;
		case MBIM_PIN_TYPE_PIN2:
		case MBIM_PIN_TYPE_PUK2:
			/* Assume that PIN1 was accepted */
			sc->sc_info.pin_state = UMB_PIN_UNLOCKED;
			break;
		}
		break;
	}
	if (ifp->if_flags & IFF_DEBUG)
		log(LOG_INFO, "%s: %s state %s (%d attempts left)\n",
		    DEVNAM(sc), umb_pin_type(le32toh(pi->type)),
		    (le32toh(pi->state) == MBIM_PIN_STATE_UNLOCKED) ?
			"unlocked" : "locked",
		    le32toh(pi->remaining_attempts));

	/*
	 * In case the PIN was set after IFF_UP, retrigger the state machine
	 */
	usb_add_task(sc->sc_udev, &sc->sc_umb_task, USB_TASKQ_DRIVER);
	return 1;
}

Static int
umb_decode_packet_service(struct umb_softc *sc, void *data, int len)
{
	struct mbim_cid_packet_service_info *psi = data;
	int	 state, highestclass;
	uint64_t up_speed, down_speed;
	struct ifnet *ifp = GET_IFP(sc);

	if (len < sizeof(*psi))
		return 0;

	sc->sc_info.nwerror = le32toh(psi->nwerror);
	state = le32toh(psi->state);
	highestclass = le32toh(psi->highest_dataclass);
	up_speed = le64toh(psi->uplink_speed);
	down_speed = le64toh(psi->downlink_speed);
	if (sc->sc_info.packetstate  != state ||
	    sc->sc_info.uplink_speed != up_speed ||
	    sc->sc_info.downlink_speed != down_speed) {
		if (ifp->if_flags & IFF_DEBUG) {
			log(LOG_INFO, "%s: packet service ", DEVNAM(sc));
			if (sc->sc_info.packetstate  != state)
				addlog("changed from %s to ",
				    umb_packet_state(sc->sc_info.packetstate));
			addlog("%s, class %s, speed: %" PRIu64 " up / %" PRIu64 " down\n",
			    umb_packet_state(state), 
			    umb_dataclass(highestclass), up_speed, down_speed);
		}
	}
	sc->sc_info.packetstate = state;
	sc->sc_info.highestclass = highestclass;
	sc->sc_info.uplink_speed = up_speed;
	sc->sc_info.downlink_speed = down_speed;

	if (sc->sc_info.regmode == MBIM_REGMODE_AUTOMATIC) {
		/*
		 * For devices using automatic registration mode, just proceed,
		 * once registration has completed.
		 */
		if (ifp->if_flags & IFF_UP) {
			switch (sc->sc_info.regstate) {
			case MBIM_REGSTATE_HOME:
			case MBIM_REGSTATE_ROAMING:
			case MBIM_REGSTATE_PARTNER:
				umb_newstate(sc, UMB_S_ATTACHED,
				    UMB_NS_DONT_DROP);
				break;
			default:
				break;
			}
		} else
			umb_newstate(sc, UMB_S_SIMREADY, UMB_NS_DONT_RAISE);
	} else switch (sc->sc_info.packetstate) {
	case MBIM_PKTSERVICE_STATE_ATTACHED:
		umb_newstate(sc, UMB_S_ATTACHED, UMB_NS_DONT_DROP);
		break;
	case MBIM_PKTSERVICE_STATE_DETACHED:
		umb_newstate(sc, UMB_S_SIMREADY, UMB_NS_DONT_RAISE);
		break;
	}
	return 1;
}

Static int
umb_decode_signal_state(struct umb_softc *sc, void *data, int len)
{
	struct mbim_cid_signal_state *ss = data;
	struct ifnet *ifp = GET_IFP(sc);
	int	 rssi;

	if (len < sizeof(*ss))
		return 0;

	if (le32toh(ss->rssi) == 99)
		rssi = UMB_VALUE_UNKNOWN;
	else {
		rssi = -113 + 2 * le32toh(ss->rssi);
		if ((ifp->if_flags & IFF_DEBUG) && sc->sc_info.rssi != rssi &&
		    sc->sc_state >= UMB_S_CONNECTED)
			log(LOG_INFO, "%s: rssi %d dBm\n", DEVNAM(sc), rssi);
	}
	sc->sc_info.rssi = rssi;
	sc->sc_info.ber = le32toh(ss->err_rate);
	if (sc->sc_info.ber == -99)
		sc->sc_info.ber = UMB_VALUE_UNKNOWN;
	return 1;
}

Static int
umb_decode_connect_info(struct umb_softc *sc, void *data, int len)
{
	struct mbim_cid_connect_info *ci = data;
	struct ifnet *ifp = GET_IFP(sc);
	int	 act;

	if (len < sizeof(*ci))
		return 0;

	if (le32toh(ci->sessionid) != umb_session_id) {
		DPRINTF("%s: discard connection info for session %u\n",
		    DEVNAM(sc), le32toh(ci->sessionid));
		return 1;
	}
	if (memcmp(ci->context, umb_uuid_context_internet,
	    sizeof(ci->context))) {
		DPRINTF("%s: discard connection info for other context\n",
		    DEVNAM(sc));
		return 1;
	}
	act = le32toh(ci->activation);
	if (sc->sc_info.activation != act) {
		if (ifp->if_flags & IFF_DEBUG)
			log(LOG_INFO, "%s: connection %s\n", DEVNAM(sc),
			    umb_activation(act));
		if ((ifp->if_flags & IFF_DEBUG) &&
		    le32toh(ci->iptype) != MBIM_CONTEXT_IPTYPE_DEFAULT &&
		    le32toh(ci->iptype) != MBIM_CONTEXT_IPTYPE_IPV4)
			log(LOG_DEBUG, "%s: got iptype %d connection\n",
			    DEVNAM(sc), le32toh(ci->iptype));

		sc->sc_info.activation = act;
		sc->sc_info.nwerror = le32toh(ci->nwerror);

		if (sc->sc_info.activation == MBIM_ACTIVATION_STATE_ACTIVATED)
			umb_newstate(sc, UMB_S_CONNECTED, UMB_NS_DONT_DROP);
		else if (sc->sc_info.activation ==
		    MBIM_ACTIVATION_STATE_DEACTIVATED)
			umb_newstate(sc, UMB_S_ATTACHED, 0);
		/* else: other states are purely transitional */
	}
	return 1;
}

Static int
umb_decode_ip_configuration(struct umb_softc *sc, void *data, int len)
{
	struct mbim_cid_ip_configuration_info *ic = data;
	struct ifnet *ifp = GET_IFP(sc);
	int	 s;
	uint32_t avail;
	uint32_t val;
	int	 n, i;
	int	 off;
	struct mbim_cid_ipv4_element ipv4elem;
	struct in_aliasreq ifra;
	struct sockaddr_in *sin;
	int	 state = -1;
	int	 rv;

	if (len < sizeof(*ic))
		return 0;
	if (le32toh(ic->sessionid) != umb_session_id) {
		DPRINTF("%s: ignore IP configuration for session id %d\n",
		    DEVNAM(sc), le32toh(ic->sessionid));
		return 0;
	}
	s = splnet();

	/*
	 * IPv4 configuation
	 */
	avail = le32toh(ic->ipv4_available);
	if (avail & MBIM_IPCONF_HAS_ADDRINFO) {
		n = le32toh(ic->ipv4_naddr);
		off = le32toh(ic->ipv4_addroffs);

		if (n == 0 || off + sizeof(ipv4elem) > len)
			goto done;

		/* Only pick the first one */
		memcpy(&ipv4elem, (char *)data + off, sizeof(ipv4elem));
		ipv4elem.prefixlen = le32toh(ipv4elem.prefixlen);

		memset(&ifra, 0, sizeof(ifra));
		sin = (struct sockaddr_in *)&ifra.ifra_addr;
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(ifra.ifra_addr);
		sin->sin_addr.s_addr = ipv4elem.addr;

		sin = (struct sockaddr_in *)&ifra.ifra_dstaddr;
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(ifra.ifra_dstaddr);
		if (avail & MBIM_IPCONF_HAS_GWINFO) {
			off = le32toh(ic->ipv4_gwoffs);
			sin->sin_addr.s_addr = *((uint32_t *)((char *)data + off));
		}

		sin = (struct sockaddr_in *)&ifra.ifra_mask;
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(ifra.ifra_mask);
		umb_in_len2mask(&sin->sin_addr, ipv4elem.prefixlen);

		rv = in_control(NULL, SIOCAIFADDR, &ifra, ifp);
		if (rv == 0) {
			if (ifp->if_flags & IFF_DEBUG)
				log(LOG_INFO, "%s: IPv4 addr %s, mask %s, "
				    "gateway %s\n", device_xname(sc->sc_dev),
				    umb_ntop(sintosa(&ifra.ifra_addr)),
				    umb_ntop(sintosa(&ifra.ifra_mask)),
				    umb_ntop(sintosa(&ifra.ifra_dstaddr)));
			state = UMB_S_UP;
		} else
			printf("%s: unable to set IPv4 address, error %d\n",
			    device_xname(sc->sc_dev), rv);
	}

	memset(sc->sc_info.ipv4dns, 0, sizeof(sc->sc_info.ipv4dns));
	if (avail & MBIM_IPCONF_HAS_DNSINFO) {
		n = le32toh(ic->ipv4_ndnssrv);
		off = le32toh(ic->ipv4_dnssrvoffs);
		i = 0;
		while (n-- > 0) {
			if (off + sizeof(uint32_t) > len)
				break;
			val = *((uint32_t *)((char *)data + off));
			if (i < UMB_MAX_DNSSRV)
				sc->sc_info.ipv4dns[i++] = val;
			off += sizeof(uint32_t);
		}
	}

	if ((avail & MBIM_IPCONF_HAS_MTUINFO)) {
		val = le32toh(ic->ipv4_mtu);
		if (ifp->if_mtu != val && val <= sc->sc_maxpktlen) {
			ifp->if_mtu = val;
			if (ifp->if_mtu > val)
				ifp->if_mtu = val;
			if (ifp->if_flags & IFF_DEBUG)
				log(LOG_INFO, "%s: MTU %d\n", DEVNAM(sc), val);
		}
	}

	avail = le32toh(ic->ipv6_available);
	if ((ifp->if_flags & IFF_DEBUG) && avail & MBIM_IPCONF_HAS_ADDRINFO) {
		/* XXX FIXME: IPv6 configuration missing */
		log(LOG_INFO, "%s: ignoring IPv6 configuration\n", DEVNAM(sc));
	}
	if (state != -1)
		umb_newstate(sc, state, 0);

done:
	splx(s);
	return 1;
}

Static void
umb_rx(struct umb_softc *sc)
{
	usbd_setup_xfer(sc->sc_rx_xfer, sc, sc->sc_rx_buf,
	    sc->sc_rx_bufsz, USBD_SHORT_XFER_OK,
	    USBD_NO_TIMEOUT, umb_rxeof);
	usbd_transfer(sc->sc_rx_xfer);
}

Static void
umb_rxeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct umb_softc *sc = priv;
	struct ifnet *ifp = GET_IFP(sc);

	if (sc->sc_dying || !(ifp->if_flags & IFF_RUNNING))
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;
		DPRINTF("%s: rx error: %s\n", DEVNAM(sc), usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->sc_rx_pipe);
		if (++sc->sc_rx_nerr > 100) {
			log(LOG_ERR, "%s: too many rx errors, disabling\n",
			    DEVNAM(sc));
			umb_activate(sc->sc_dev, DVACT_DEACTIVATE);
		}
	} else {
		sc->sc_rx_nerr = 0;
		umb_decap(sc, xfer);
	}

	umb_rx(sc);
	return;
}

Static int
umb_encap(struct umb_softc *sc, struct mbuf *m)
{
	struct ncm_header16 *hdr;
	struct ncm_pointer16 *ptr;
	usbd_status  err;
	int len;

	/* All size constraints have been validated by the caller! */
	hdr = (struct ncm_header16 *)sc->sc_tx_buf;
	ptr = (struct ncm_pointer16 *)(hdr + 1);
	USETDW(hdr->dwSignature, NCM_HDR16_SIG);
	USETW(hdr->wHeaderLength, sizeof(*hdr));
	USETW(hdr->wSequence, sc->sc_tx_seq);
	sc->sc_tx_seq++;

	len = m->m_pkthdr.len;

	USETDW(ptr->dwSignature, MBIM_NCM_NTH16_SIG(umb_session_id));
	USETW(ptr->wLength, sizeof(*ptr));
	USETW(ptr->wNextNdpIndex, 0);
	USETW(ptr->dgram[0].wDatagramIndex, MBIM_HDR16_LEN);
	USETW(ptr->dgram[0].wDatagramLen, len);
	USETW(ptr->dgram[1].wDatagramIndex, 0);
	USETW(ptr->dgram[1].wDatagramLen, 0);

	KASSERT(len <= sc->sc_tx_bufsz - sizeof(*hdr) - sizeof(*ptr));
	m_copydata(m, 0, len, ptr + 1);
	sc->sc_tx_m = m;
	len += MBIM_HDR16_LEN;
	USETW(hdr->wBlockLength, len);

	DPRINTFN(3, "%s: encap %d bytes\n", DEVNAM(sc), len);
	DDUMPN(5, sc->sc_tx_buf, len);
	usbd_setup_xfer(sc->sc_tx_xfer, sc, sc->sc_tx_buf, len,
	    USBD_FORCE_SHORT_XFER, umb_xfer_tout, umb_txeof);
	err = usbd_transfer(sc->sc_tx_xfer);
	if (err != USBD_IN_PROGRESS) {
		DPRINTF("%s: start tx error: %s\n", DEVNAM(sc),
		    usbd_errstr(err));
		return 0;
	}
	return 1;
}

Static void
umb_txeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct umb_softc *sc = priv;
	struct ifnet *ifp = GET_IFP(sc);
	int	 s;

	s = splnet();
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;

	m_freem(sc->sc_tx_m);
	sc->sc_tx_m = NULL;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status != USBD_NOT_STARTED && status != USBD_CANCELLED) {
			ifp->if_oerrors++;
			DPRINTF("%s: tx error: %s\n", DEVNAM(sc),
			    usbd_errstr(status));
			if (status == USBD_STALLED)
				usbd_clear_endpoint_stall_async(sc->sc_tx_pipe);
		}
	}
	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		umb_start(ifp);

	splx(s);
}

Static void
umb_decap(struct umb_softc *sc, struct usbd_xfer *xfer)
{
	struct ifnet *ifp = GET_IFP(sc);
	int	 s;
	char	*buf;
	uint32_t len;
	char	*dp;
	struct ncm_header16 *hdr16;
	struct ncm_header32 *hdr32;
	struct ncm_pointer16 *ptr16;
	struct ncm_pointer16_dgram *dgram16;
	struct ncm_pointer32_dgram *dgram32;
	uint32_t hsig, psig;
	int	 hlen, blen;
	int	 ptrlen, ptroff, dgentryoff;
	uint32_t doff, dlen;
	struct mbuf *m;

	usbd_get_xfer_status(xfer, NULL, (void **)&buf, &len, NULL);
	DPRINTFN(4, "%s: recv %d bytes\n", DEVNAM(sc), len);
	DDUMPN(5, buf, len);
	s = splnet();
	if (len < sizeof(*hdr16))
		goto toosmall;

	hdr16 = (struct ncm_header16 *)buf;
	hsig = UGETDW(hdr16->dwSignature);
	hlen = UGETW(hdr16->wHeaderLength);
	if (len < hlen)
		goto toosmall;
	if (len > sc->sc_rx_bufsz) {
		DPRINTF("%s: packet too large (%d)\n", DEVNAM(sc), len);
		goto fail;
	}
	switch (hsig) {
	case NCM_HDR16_SIG:
		blen = UGETW(hdr16->wBlockLength);
		ptroff = UGETW(hdr16->wNdpIndex);
		if (hlen != sizeof(*hdr16)) {
			DPRINTF("%s: bad header len %d for NTH16 (exp %zu)\n",
			    DEVNAM(sc), hlen, sizeof(*hdr16));
			goto fail;
		}
		break;
	case NCM_HDR32_SIG:
		hdr32 = (struct ncm_header32 *)hdr16;
		blen = UGETDW(hdr32->dwBlockLength);
		ptroff = UGETDW(hdr32->dwNdpIndex);
		if (hlen != sizeof(*hdr32)) {
			DPRINTF("%s: bad header len %d for NTH32 (exp %zu)\n",
			    DEVNAM(sc), hlen, sizeof(*hdr32));
			goto fail;
		}
		break;
	default:
		DPRINTF("%s: unsupported NCM header signature (0x%08x)\n",
		    DEVNAM(sc), hsig);
		goto fail;
	}
	if (len < blen) {
		DPRINTF("%s: bad NTB len (%d) for %d bytes of data\n",
		    DEVNAM(sc), blen, len);
		goto fail;
	}

	ptr16 = (struct ncm_pointer16 *)(buf + ptroff);
	psig = UGETDW(ptr16->dwSignature);
	ptrlen = UGETW(ptr16->wLength);
	if (len < ptrlen + ptroff)
		goto toosmall;
	if (!MBIM_NCM_NTH16_ISISG(psig) && !MBIM_NCM_NTH32_ISISG(psig)) {
		DPRINTF("%s: unsupported NCM pointer signature (0x%08x)\n",
		    DEVNAM(sc), psig);
		goto fail;
	}

	switch (hsig) {
	case NCM_HDR16_SIG:
		dgentryoff = offsetof(struct ncm_pointer16, dgram);
		break;
	case NCM_HDR32_SIG:
		dgentryoff = offsetof(struct ncm_pointer32, dgram);
		break;
	default:
		goto fail;
	}

	while (dgentryoff < ptrlen) {
		switch (hsig) {
		case NCM_HDR16_SIG:
			if (ptroff + dgentryoff < sizeof(*dgram16))
				goto done;
			dgram16 = (struct ncm_pointer16_dgram *)
			    (buf + ptroff + dgentryoff);
			dgentryoff += sizeof(*dgram16);
			dlen = UGETW(dgram16->wDatagramLen);
			doff = UGETW(dgram16->wDatagramIndex);
			break;
		case NCM_HDR32_SIG:
			if (ptroff + dgentryoff < sizeof(*dgram32))
				goto done;
			dgram32 = (struct ncm_pointer32_dgram *)
			    (buf + ptroff + dgentryoff);
			dgentryoff += sizeof(*dgram32);
			dlen = UGETDW(dgram32->dwDatagramLen);
			doff = UGETDW(dgram32->dwDatagramIndex);
			break;
		default:
			ifp->if_ierrors++;
			goto done;
		}

		/* Terminating zero entry */
		if (dlen == 0 || doff == 0)
			break;
		if (len < dlen + doff) {
			/* Skip giant datagram but continue processing */
			DPRINTF("%s: datagram too large (%d @ off %d)\n",
			    DEVNAM(sc), dlen, doff);
			continue;
		}

		dp = buf + doff;
		DPRINTFN(3, "%s: decap %d bytes\n", DEVNAM(sc), dlen);
		m = m_devget(dp, dlen, 0, ifp, NULL);
		if (m == NULL) {
			ifp->if_iqdrops++;
			continue;
		}

		if_percpuq_enqueue((ifp)->if_percpuq, (m));
	}
done:
	splx(s);
	return;
toosmall:
	DPRINTF("%s: packet too small (%d)\n", DEVNAM(sc), len);
fail:
	ifp->if_ierrors++;
	splx(s);
}

Static usbd_status
umb_send_encap_command(struct umb_softc *sc, void *data, int len)
{
	struct usbd_xfer *xfer;
	usb_device_request_t req;
	char *buf;

	if (len > sc->sc_ctrl_len)
		return USBD_INVAL;

	if (usbd_create_xfer(sc->sc_udev->ud_pipe0, len, 0, 0, &xfer) != 0)
		return USBD_NOMEM;
	buf = usbd_get_buffer(xfer);
	memcpy(buf, data, len);

	/* XXX FIXME: if (total len > sc->sc_ctrl_len) => must fragment */
	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SEND_ENCAPSULATED_COMMAND;
	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_ctrl_ifaceno);
	USETW(req.wLength, len);
	DELAY(umb_delay);
	return usbd_request_async(sc->sc_udev, xfer, &req, NULL, NULL);
}

Static int
umb_get_encap_response(struct umb_softc *sc, void *buf, int *len)
{
	usb_device_request_t req;
	usbd_status err;

	req.bmRequestType = UT_READ_CLASS_INTERFACE;
	req.bRequest = UCDC_GET_ENCAPSULATED_RESPONSE;
	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_ctrl_ifaceno);
	USETW(req.wLength, *len);
	/* XXX FIXME: re-assemble fragments */

	DELAY(umb_delay);
	err = usbd_do_request_flags(sc->sc_udev, &req, buf, USBD_SHORT_XFER_OK,
	    len, umb_xfer_tout);
	if (err == USBD_NORMAL_COMPLETION)
		return 1;
	DPRINTF("%s: ctrl recv: %s\n", DEVNAM(sc), usbd_errstr(err));
	return 0;
}

Static void
umb_ctrl_msg(struct umb_softc *sc, uint32_t req, void *data, int len)
{
	struct ifnet *ifp = GET_IFP(sc);
	uint32_t tid;
	struct mbim_msghdr *hdr = data;
	usbd_status err;
	int	 s;

	if (sc->sc_dying)
		return;
	if (len < sizeof(*hdr))
		return;
	tid = ++sc->sc_tid;

	hdr->type = htole32(req);
	hdr->len = htole32(len);
	hdr->tid = htole32(tid);

#ifdef UMB_DEBUG
	if (umb_debug) {
		const char *op, *str;
		if (req == MBIM_COMMAND_MSG) {
			struct mbim_h2f_cmd *c = data;
			if (le32toh(c->op) == MBIM_CMDOP_SET)
				op = "set";
			else
				op = "qry";
			str = umb_cid2str(le32toh(c->cid));
		} else {
			op = "snd";
			str = umb_request2str(req);
		}
		DPRINTF("%s: -> %s %s (tid %u)\n", DEVNAM(sc), op, str, tid);
	}
#endif
	s = splusb();
	err = umb_send_encap_command(sc, data, len);
	splx(s);
	if (err != USBD_NORMAL_COMPLETION) {
		if (ifp->if_flags & IFF_DEBUG)
			log(LOG_ERR, "%s: send %s msg (tid %u) failed: %s\n",
			    DEVNAM(sc), umb_request2str(req), tid,
			    usbd_errstr(err));

		/* will affect other transactions, too */
		usbd_abort_pipe(sc->sc_udev->ud_pipe0);
	} else {
		DPRINTFN(2, "%s: sent %s (tid %u)\n", DEVNAM(sc),
		    umb_request2str(req), tid);
		DDUMPN(3, data, len);
	}
	return;
}

Static void
umb_open(struct umb_softc *sc)
{
	struct mbim_h2f_openmsg msg;

	memset(&msg, 0, sizeof(msg));
	msg.maxlen = htole32(sc->sc_ctrl_len);
	umb_ctrl_msg(sc, MBIM_OPEN_MSG, &msg, sizeof(msg));
	return;
}

Static void
umb_close(struct umb_softc *sc)
{
	struct mbim_h2f_closemsg msg;

	memset(&msg, 0, sizeof(msg));
	umb_ctrl_msg(sc, MBIM_CLOSE_MSG, &msg, sizeof(msg));
}

Static int
umb_setpin(struct umb_softc *sc, int op, int is_puk, void *pin, int pinlen,
    void *newpin, int newpinlen)
{
	struct mbim_cid_pin cp;
	int	 off;

	if (pinlen == 0)
		return 0;
	if (pinlen < 0 || pinlen > MBIM_PIN_MAXLEN ||
	    newpinlen < 0 || newpinlen > MBIM_PIN_MAXLEN ||
	    op < 0 || op > MBIM_PIN_OP_CHANGE ||
	    (is_puk && op != MBIM_PIN_OP_ENTER))
		return EINVAL;

	memset(&cp, 0, sizeof(cp));
	cp.type = htole32(is_puk ? MBIM_PIN_TYPE_PUK1 : MBIM_PIN_TYPE_PIN1);

	off = offsetof(struct mbim_cid_pin, data);
	if (!umb_addstr(&cp, sizeof(cp), &off, pin, pinlen,
	    &cp.pin_offs, &cp.pin_size))
		return EINVAL;

	cp.op  = htole32(op);
	if (newpinlen) {
		if (!umb_addstr(&cp, sizeof(cp), &off, newpin, newpinlen,
		    &cp.newpin_offs, &cp.newpin_size))
			return EINVAL;
	} else {
		if ((op == MBIM_PIN_OP_CHANGE) || is_puk)
			return EINVAL;
		if (!umb_addstr(&cp, sizeof(cp), &off, NULL, 0,
		    &cp.newpin_offs, &cp.newpin_size))
			return EINVAL;
	}
	umb_cmd(sc, MBIM_CID_PIN, MBIM_CMDOP_SET, &cp, off);
	return 0;
}

Static void
umb_setdataclass(struct umb_softc *sc)
{
	struct mbim_cid_registration_state rs;
	uint32_t	 classes;

	if (sc->sc_info.supportedclasses == MBIM_DATACLASS_NONE)
		return;

	memset(&rs, 0, sizeof(rs));
	rs.regaction = htole32(MBIM_REGACTION_AUTOMATIC);
	classes = sc->sc_info.supportedclasses;
	if (sc->sc_info.preferredclasses != MBIM_DATACLASS_NONE)
		classes &= sc->sc_info.preferredclasses;
	rs.data_class = htole32(classes);
	umb_cmd(sc, MBIM_CID_REGISTER_STATE, MBIM_CMDOP_SET, &rs, sizeof(rs));
}

Static void
umb_radio(struct umb_softc *sc, int on)
{
	struct mbim_cid_radio_state s;

	DPRINTF("%s: set radio %s\n", DEVNAM(sc), on ? "on" : "off");
	memset(&s, 0, sizeof(s));
	s.state = htole32(on ? MBIM_RADIO_STATE_ON : MBIM_RADIO_STATE_OFF);
	umb_cmd(sc, MBIM_CID_RADIO_STATE, MBIM_CMDOP_SET, &s, sizeof(s));
}

Static void
umb_allocate_cid(struct umb_softc *sc)
{
	umb_cmd1(sc, MBIM_CID_DEVICE_CAPS, MBIM_CMDOP_SET,
	    umb_qmi_alloc_cid, sizeof(umb_qmi_alloc_cid), umb_uuid_qmi_mbim);
}

Static void
umb_send_fcc_auth(struct umb_softc *sc)
{
	uint8_t	 fccauth[sizeof(umb_qmi_fcc_auth)];

	if (sc->sc_cid == -1) {
		DPRINTF("%s: missing CID, cannot send FCC auth\n", DEVNAM(sc));
		umb_allocate_cid(sc);
		return;
	}
	memcpy(fccauth, umb_qmi_fcc_auth, sizeof(fccauth));
	fccauth[UMB_QMI_CID_OFFS] = sc->sc_cid;
	umb_cmd1(sc, MBIM_CID_DEVICE_CAPS, MBIM_CMDOP_SET,
	    fccauth, sizeof(fccauth), umb_uuid_qmi_mbim);
}

Static void
umb_packet_service(struct umb_softc *sc, int attach)
{
	struct mbim_cid_packet_service	s;

	DPRINTF("%s: %s packet service\n", DEVNAM(sc),
	    attach ? "attach" : "detach");
	memset(&s, 0, sizeof(s));
	s.action = htole32(attach ?
	    MBIM_PKTSERVICE_ACTION_ATTACH : MBIM_PKTSERVICE_ACTION_DETACH);
	umb_cmd(sc, MBIM_CID_PACKET_SERVICE, MBIM_CMDOP_SET, &s, sizeof(s));
}

Static void
umb_connect(struct umb_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);

	if (sc->sc_info.regstate == MBIM_REGSTATE_ROAMING && !sc->sc_roaming) {
		log(LOG_INFO, "%s: connection disabled in roaming network\n",
		    DEVNAM(sc));
		return;
	}
	if (ifp->if_flags & IFF_DEBUG)
		log(LOG_DEBUG, "%s: connecting ...\n", DEVNAM(sc));
	umb_send_connect(sc, MBIM_CONNECT_ACTIVATE);
}

Static void
umb_disconnect(struct umb_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);

	if (ifp->if_flags & IFF_DEBUG)
		log(LOG_DEBUG, "%s: disconnecting ...\n", DEVNAM(sc));
	umb_send_connect(sc, MBIM_CONNECT_DEACTIVATE);
}

Static void
umb_send_connect(struct umb_softc *sc, int command)
{
	struct mbim_cid_connect *c;
	int	 off;

	/* Too large or the stack */
	c = kmem_zalloc(sizeof(*c), KM_SLEEP);
	c->sessionid = htole32(umb_session_id);
	c->command = htole32(command);
	off = offsetof(struct mbim_cid_connect, data);
	if (!umb_addstr(c, sizeof(*c), &off, sc->sc_info.apn,
	    sc->sc_info.apnlen, &c->access_offs, &c->access_size))
		goto done;
	if (!umb_addstr(c, sizeof(*c), &off, sc->sc_info.username,
	    sc->sc_info.usernamelen, &c->user_offs, &c->user_size))
		goto done;
	if (!umb_addstr(c, sizeof(*c), &off, sc->sc_info.password,
	    sc->sc_info.passwordlen, &c->passwd_offs, &c->passwd_size))
		goto done;
	c->authprot = htole32(MBIM_AUTHPROT_NONE);
	c->compression = htole32(MBIM_COMPRESSION_NONE);
	c->iptype = htole32(MBIM_CONTEXT_IPTYPE_IPV4);
	memcpy(c->context, umb_uuid_context_internet, sizeof(c->context));
	umb_cmd(sc, MBIM_CID_CONNECT, MBIM_CMDOP_SET, c, off);
done:
	kmem_free(c, sizeof(*c));
	return;
}

Static void
umb_qry_ipconfig(struct umb_softc *sc)
{
	struct mbim_cid_ip_configuration_info ipc;

	memset(&ipc, 0, sizeof(ipc));
	ipc.sessionid = htole32(umb_session_id);
	umb_cmd(sc, MBIM_CID_IP_CONFIGURATION, MBIM_CMDOP_QRY,
	    &ipc, sizeof(ipc));
}

Static void
umb_cmd(struct umb_softc *sc, int cid, int op, const void *data, int len)
{
	umb_cmd1(sc, cid, op, data, len, umb_uuid_basic_connect);
}

Static void
umb_cmd1(struct umb_softc *sc, int cid, int op, const void *data, int len,
    uint8_t *uuid)
{
	struct mbim_h2f_cmd *cmd;
	int	totlen;

	/* XXX FIXME support sending fragments */
	if (sizeof(*cmd) + len > sc->sc_ctrl_len) {
		DPRINTF("%s: set %s msg too long: cannot send\n",
		    DEVNAM(sc), umb_cid2str(cid));
		return;
	}
	cmd = sc->sc_ctrl_msg;
	memset(cmd, 0, sizeof(*cmd));
	cmd->frag.nfrag = htole32(1);
	memcpy(cmd->devid, uuid, sizeof(cmd->devid));
	cmd->cid = htole32(cid);
	cmd->op = htole32(op);
	cmd->infolen = htole32(len);
	totlen = sizeof(*cmd);
	if (len > 0) {
		memcpy(cmd + 1, data, len);
		totlen += len;
	}
	umb_ctrl_msg(sc, MBIM_COMMAND_MSG, cmd, totlen);
}

Static void
umb_command_done(struct umb_softc *sc, void *data, int len)
{
	struct mbim_f2h_cmddone *cmd = data;
	struct ifnet *ifp = GET_IFP(sc);
	uint32_t status;
	uint32_t cid;
	uint32_t infolen;
	int	 qmimsg = 0;

	if (len < sizeof(*cmd)) {
		DPRINTF("%s: discard short %s messsage\n", DEVNAM(sc),
		    umb_request2str(le32toh(cmd->hdr.type)));
		return;
	}
	cid = le32toh(cmd->cid);
	if (memcmp(cmd->devid, umb_uuid_basic_connect, sizeof(cmd->devid))) {
		if (memcmp(cmd->devid, umb_uuid_qmi_mbim,
		    sizeof(cmd->devid))) {
			DPRINTF("%s: discard %s messsage for other UUID '%s'\n",
			    DEVNAM(sc), umb_request2str(le32toh(cmd->hdr.type)),
			    umb_uuid2str(cmd->devid));
			return;
		} else
			qmimsg = 1;
	}

	status = le32toh(cmd->status);
	switch (status) {
	case MBIM_STATUS_SUCCESS:
		break;
	case MBIM_STATUS_NOT_INITIALIZED:
		if (ifp->if_flags & IFF_DEBUG)
			log(LOG_ERR, "%s: SIM not initialized (PIN missing)\n",
			    DEVNAM(sc));
		return;
	case MBIM_STATUS_PIN_REQUIRED:
		sc->sc_info.pin_state = UMB_PIN_REQUIRED;
		/*FALLTHROUGH*/
	default:
		if (ifp->if_flags & IFF_DEBUG)
			log(LOG_ERR, "%s: set/qry %s failed: %s\n", DEVNAM(sc),
			    umb_cid2str(cid), umb_status2str(status));
		return;
	}

	infolen = le32toh(cmd->infolen);
	if (len < sizeof(*cmd) + infolen) {
		DPRINTF("%s: discard truncated %s messsage (want %d, got %d)\n",
		    DEVNAM(sc), umb_cid2str(cid),
		    (int)sizeof(*cmd) + infolen, len);
		return;
	}
	if (qmimsg) {
		if (sc->sc_flags & UMBFLG_FCC_AUTH_REQUIRED)
			umb_decode_qmi(sc, cmd->info, infolen);
	} else {
		DPRINTFN(2, "%s: set/qry %s done\n", DEVNAM(sc),
		    umb_cid2str(cid));
		umb_decode_cid(sc, cid, cmd->info, infolen);
	}
}

Static void
umb_decode_cid(struct umb_softc *sc, uint32_t cid, void *data, int len)
{
	int	 ok = 1;

	switch (cid) {
	case MBIM_CID_DEVICE_CAPS:
		ok = umb_decode_devices_caps(sc, data, len);
		break;
	case MBIM_CID_SUBSCRIBER_READY_STATUS:
		ok = umb_decode_subscriber_status(sc, data, len);
		break;
	case MBIM_CID_RADIO_STATE:
		ok = umb_decode_radio_state(sc, data, len);
		break;
	case MBIM_CID_PIN:
		ok = umb_decode_pin(sc, data, len);
		break;
	case MBIM_CID_REGISTER_STATE:
		ok = umb_decode_register_state(sc, data, len);
		break;
	case MBIM_CID_PACKET_SERVICE:
		ok = umb_decode_packet_service(sc, data, len);
		break;
	case MBIM_CID_SIGNAL_STATE:
		ok = umb_decode_signal_state(sc, data, len);
		break;
	case MBIM_CID_CONNECT:
		ok = umb_decode_connect_info(sc, data, len);
		break;
	case MBIM_CID_IP_CONFIGURATION:
		ok = umb_decode_ip_configuration(sc, data, len);
		break;
	default:
		/*
		 * Note: the above list is incomplete and only contains
		 *	mandatory CIDs from the BASIC_CONNECT set.
		 *	So alternate values are not unusual.
		 */
		DPRINTFN(4, "%s: ignore %s\n", DEVNAM(sc), umb_cid2str(cid));
		break;
	}
	if (!ok)
		DPRINTF("%s: discard %s with bad info length %d\n",
		    DEVNAM(sc), umb_cid2str(cid), len);
	return;
}

Static void
umb_decode_qmi(struct umb_softc *sc, uint8_t *data, int len)
{
	uint8_t	srv;
	uint16_t msg, tlvlen;
	uint32_t val;

#define UMB_QMI_QMUXLEN		6
	if (len < UMB_QMI_QMUXLEN)
		goto tooshort;

	srv = data[4];
	data += UMB_QMI_QMUXLEN;
	len -= UMB_QMI_QMUXLEN;

#define UMB_GET16(p)	((uint16_t)*p | (uint16_t)*(p + 1) << 8)
#define UMB_GET32(p)	((uint32_t)*p | (uint32_t)*(p + 1) << 8 | \
			    (uint32_t)*(p + 2) << 16 |(uint32_t)*(p + 3) << 24)
	switch (srv) {
	case 0:	/* ctl */
#define UMB_QMI_CTLLEN		6
		if (len < UMB_QMI_CTLLEN)
			goto tooshort;
		msg = UMB_GET16(&data[2]);
		tlvlen = UMB_GET16(&data[4]);
		data += UMB_QMI_CTLLEN;
		len -= UMB_QMI_CTLLEN;
		break;
	case 2:	/* dms  */
#define UMB_QMI_DMSLEN		7
		if (len < UMB_QMI_DMSLEN)
			goto tooshort;
		msg = UMB_GET16(&data[3]);
		tlvlen = UMB_GET16(&data[5]);
		data += UMB_QMI_DMSLEN;
		len -= UMB_QMI_DMSLEN;
		break;
	default:
		DPRINTF("%s: discard QMI message for unknown service type %d\n",
		    DEVNAM(sc), srv);
		return;
	}

	if (len < tlvlen)
		goto tooshort;

#define UMB_QMI_TLVLEN		3
	while (len > 0) {
		if (len < UMB_QMI_TLVLEN)
			goto tooshort;
		tlvlen = UMB_GET16(&data[1]);
		if (len < UMB_QMI_TLVLEN + tlvlen)
			goto tooshort;
		switch (data[0]) {
		case 1:	/* allocation info */
			if (msg == 0x0022) {	/* Allocate CID */
				if (tlvlen != 2 || data[3] != 2) /* dms */
					break;
				sc->sc_cid = data[4];
				DPRINTF("%s: QMI CID %d allocated\n",
				    DEVNAM(sc), sc->sc_cid);
				umb_newstate(sc, UMB_S_CID, UMB_NS_DONT_DROP);
			}
			break;
		case 2:	/* response */
			if (tlvlen != sizeof(val))
				break;
			val = UMB_GET32(&data[3]);
			switch (msg) {
			case 0x0022:	/* Allocate CID */
				if (val != 0) {
					log(LOG_ERR, "%s: allocation of QMI CID"
					    " failed, error 0x%x\n", DEVNAM(sc),
					    val);
					/* XXX how to proceed? */
					return;
				}
				break;
			case 0x555f:	/* Send FCC Authentication */
				if (val == 0)
					log(LOG_INFO, "%s: send FCC "
					    "Authentication succeeded\n",
					    DEVNAM(sc));
				else if (val == 0x001a0001)
					log(LOG_INFO, "%s: FCC Authentication "
					    "not required\n", DEVNAM(sc));
				else
					log(LOG_INFO, "%s: send FCC "
					    "Authentication failed, "
					    "error 0x%x\n", DEVNAM(sc), val);

				/* FCC Auth is needed only once after power-on*/
				sc->sc_flags &= ~UMBFLG_FCC_AUTH_REQUIRED;

				/* Try to proceed anyway */
				DPRINTF("%s: init: turning radio on ...\n",
				    DEVNAM(sc));
				umb_radio(sc, 1);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		data += UMB_QMI_TLVLEN + tlvlen;
		len -= UMB_QMI_TLVLEN + tlvlen;
	}
	return;

tooshort:
	DPRINTF("%s: discard short QMI message\n", DEVNAM(sc));
	return;
}

Static void
umb_intr(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct umb_softc *sc = priv;
	struct ifnet *ifp = GET_IFP(sc);
	int	 total_len;

	if (status != USBD_NORMAL_COMPLETION) {
		DPRINTF("%s: notification error: %s\n", DEVNAM(sc),
		    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->sc_ctrl_pipe);
		return;
	}
	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);
	if (total_len < UCDC_NOTIFICATION_LENGTH) {
		DPRINTF("%s: short notification (%d<%d)\n", DEVNAM(sc),
		    total_len, UCDC_NOTIFICATION_LENGTH);
		    return;
	}
	if (sc->sc_intr_msg.bmRequestType != UCDC_NOTIFICATION) {
		DPRINTF("%s: unexpected notification (type=0x%02x)\n",
		    DEVNAM(sc), sc->sc_intr_msg.bmRequestType);
		return;
	}

	switch (sc->sc_intr_msg.bNotification) {
	case UCDC_N_NETWORK_CONNECTION:
		if (ifp->if_flags & IFF_DEBUG)
			log(LOG_DEBUG, "%s: network %sconnected\n", DEVNAM(sc),
			    UGETW(sc->sc_intr_msg.wValue) ? "" : "dis");
		break;
	case UCDC_N_RESPONSE_AVAILABLE:
		DPRINTFN(2, "%s: umb_intr: response available\n", DEVNAM(sc));
		++sc->sc_nresp;
		usb_add_task(sc->sc_udev, &sc->sc_get_response_task, USB_TASKQ_DRIVER);
		break;
	case UCDC_N_CONNECTION_SPEED_CHANGE:
		DPRINTFN(2, "%s: umb_intr: connection speed changed\n",
		    DEVNAM(sc));
		break;
	default:
		DPRINTF("%s: unexpected notifiation (0x%02x)\n",
		    DEVNAM(sc), sc->sc_intr_msg.bNotification);
		break;
	}
}

/*
 * Diagnostic routines
 */
Static char *
umb_ntop(struct sockaddr *sa)
{
#define NUMBUFS		4
	static char astr[NUMBUFS][INET_ADDRSTRLEN];
	static unsigned nbuf = 0;
	char	*s;

	s = astr[nbuf++];
	if (nbuf >= NUMBUFS)
		nbuf = 0;

	switch (sa->sa_family) {
	case AF_INET:
	default:
		inet_ntop(AF_INET, &satosin(sa)->sin_addr, s, sizeof(astr[0]));
		break;
	case AF_INET6:
		inet_ntop(AF_INET6, &satosin6(sa)->sin6_addr, s,
		    sizeof(astr[0]));
		break;
	}
	return s;
}

#ifdef UMB_DEBUG
Static char *
umb_uuid2str(uint8_t uuid[MBIM_UUID_LEN])
{
	static char uuidstr[2 * MBIM_UUID_LEN + 5];

#define UUID_BFMT	"%02X"
#define UUID_SEP	"-"
	snprintf(uuidstr, sizeof(uuidstr),
	    UUID_BFMT UUID_BFMT UUID_BFMT UUID_BFMT UUID_SEP
	    UUID_BFMT UUID_BFMT UUID_SEP
	    UUID_BFMT UUID_BFMT UUID_SEP
	    UUID_BFMT UUID_BFMT UUID_SEP
	    UUID_BFMT UUID_BFMT UUID_BFMT UUID_BFMT UUID_BFMT UUID_BFMT,
	    uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5],
	    uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11],
	    uuid[12], uuid[13], uuid[14], uuid[15]);
	return uuidstr;
}

Static void
umb_dump(void *buf, int len)
{
	int	 i = 0;
	uint8_t	*c = buf;

	if (len == 0)
		return;
	while (i < len) {
		if ((i % 16) == 0) {
			if (i > 0)
				addlog("\n");
			log(LOG_DEBUG, "%4d:  ", i);
		}
		addlog(" %02x", *c);
		c++;
		i++;
	}
	addlog("\n");
}
#endif /* UMB_DEBUG */

/* char *
 * inet_ntop(af, src, dst, size)
 *	convert a network format address to presentation format.
 * return:
 *	pointer to presentation format address (`dst'), or NULL (see errno).
 * author:
 *	Paul Vixie, 1996.
 */
Static const char *
inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	switch (af) {
	case AF_INET:
		return (inet_ntop4(src, dst, (size_t)size));
#ifdef INET6
	case AF_INET6:
		return (inet_ntop6(src, dst, (size_t)size));
#endif /* INET6 */
	default:
		return (NULL);
	}
	/* NOTREACHED */
}

/* const char *
 * inet_ntop4(src, dst, size)
 *	format an IPv4 address, more or less like inet_ntoa()
 * return:
 *	`dst' (as a const)
 * notes:
 *	(1) uses no statics
 *	(2) takes a u_char* not an in_addr as input
 * author:
 *	Paul Vixie, 1996.
 */
Static const char *
inet_ntop4(const u_char *src, char *dst, size_t size)
{
	char tmp[sizeof "255.255.255.255"];
	int l;

	l = snprintf(tmp, sizeof(tmp), "%u.%u.%u.%u",
	    src[0], src[1], src[2], src[3]);
	if (l <= 0 || l >= size) {
		return (NULL);
	}
	strlcpy(dst, tmp, size);
	return (dst);
}

#ifdef INET6
/* const char *
 * inet_ntop6(src, dst, size)
 *	convert IPv6 binary address into presentation (printable) format
 * author:
 *	Paul Vixie, 1996.
 */
Static const char *
inet_ntop6(const u_char *src, char *dst, size_t size)
{
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];
	char *tp, *ep;
	struct { int base, len; } best, cur;
#define IN6ADDRSZ	16
#define INT16SZ		2
	u_int words[IN6ADDRSZ / INT16SZ];
	int i;
	int advance;

	/*
	 * Preprocess:
	 *	Copy the input (bytewise) array into a wordwise array.
	 *	Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	memset(words, '\0', sizeof words);
	for (i = 0; i < IN6ADDRSZ; i++)
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	best.base = -1;
	best.len = 0;
	cur.base = -1;
	cur.len = 0;
	for (i = 0; i < (IN6ADDRSZ / INT16SZ); i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		} else {
			if (cur.base != -1) {
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	/*
	 * Format the result.
	 */
	tp = tmp;
	ep = tmp + sizeof(tmp);
	for (i = 0; i < (IN6ADDRSZ / INT16SZ) && tp < ep; i++) {
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
		    i < (best.base + best.len)) {
			if (i == best.base) {
				if (tp + 1 >= ep)
					return (NULL);
				*tp++ = ':';
			}
			continue;
		}
		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0) {
			if (tp + 1 >= ep)
				return (NULL);
			*tp++ = ':';
		}
		/* Is this address an encapsulated IPv4? */
		if (i == 6 && best.base == 0 &&
		    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
			if (!inet_ntop4(src+12, tp, (size_t)(ep - tp)))
				return (NULL);
			tp += strlen(tp);
			break;
		}
		advance = snprintf(tp, ep - tp, "%x", words[i]);
		if (advance <= 0 || advance >= ep - tp)
			return (NULL);
		tp += advance;
	}
	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && (best.base + best.len) == (IN6ADDRSZ / INT16SZ)) {
		if (tp + 1 >= ep)
			return (NULL);
		*tp++ = ':';
	}
	if (tp + 1 >= ep)
		return (NULL);
	*tp++ = '\0';

	/*
	 * Check for overflow, copy, and we're done.
	 */
	if ((size_t)(tp - tmp) > size) {
		return (NULL);
	}
	strlcpy(dst, tmp, size);
	return (dst);
}
#endif /* INET6 */
