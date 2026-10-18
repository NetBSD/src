/*	$NetBSD: if_rsu.c,v 1.1 2026/10/18 20:17:42 nia Exp $ */
/*	$OpenBSD: if_rsu.c,v 1.53 2024/05/23 03:21:08 jsg Exp $	*/

/*-
 * Copyright (c) 2010 Damien Bergamini <damien.bergamini@free.fr>
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
 * Driver for Realtek RTL8188SU/RTL8191SU/RTL8192SU.
 */

/*
 * XXX: This driver is unfinished. It attaches but is not yet usable.
 */

#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/module.h>
#include <sys/device.h>
#include <sys/endian.h>

#include <machine/intr.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <netinet/in.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_radiotap.h>

#include <dev/firmload.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/if_rsureg.h>

#ifdef RSU_DEBUG
#define DPRINTF(x)	do { if (rsu_debug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (rsu_debug >= (n)) printf x; } while (0)
int rsu_debug = 4;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

/*
 * NB: When updating this list of devices, beware to also update the list
 * of devices that have HT support disabled below, if applicable.
 */
static const struct usb_devno rsu_devs[] = {
	{ USB_VENDOR_ACCTON,		USB_PRODUCT_ACCTON_RTL8192SU },
	{ USB_VENDOR_ASUSTEK,		USB_PRODUCT_ASUSTEK_USBN10 },
	{ USB_VENDOR_ASUSTEK,		USB_PRODUCT_ASUSTEK_RTL8192SU_1 },
	{ USB_VENDOR_AZUREWAVE,		USB_PRODUCT_AZUREWAVE_RTL8192SU_1 },
	{ USB_VENDOR_AZUREWAVE,		USB_PRODUCT_AZUREWAVE_RTL8192SU_2 },
	{ USB_VENDOR_AZUREWAVE,		USB_PRODUCT_AZUREWAVE_RTL8192SU_3 },
	{ USB_VENDOR_AZUREWAVE,		USB_PRODUCT_AZUREWAVE_RTL8192SU_4 },
	{ USB_VENDOR_AZUREWAVE,		USB_PRODUCT_AZUREWAVE_RTL8192SU_5 },
	{ USB_VENDOR_BELKIN,		USB_PRODUCT_BELKIN_RTL8192SU_1 },
	{ USB_VENDOR_BELKIN,		USB_PRODUCT_BELKIN_RTL8192SU_2 },
	{ USB_VENDOR_BELKIN,		USB_PRODUCT_BELKIN_RTL8192SU_3 },
	{ USB_VENDOR_CONCEPTRONIC,	USB_PRODUCT_CONCEPTRONIC_RTL8192SU_1 },
	{ USB_VENDOR_CONCEPTRONIC,	USB_PRODUCT_CONCEPTRONIC_RTL8192SU_2 },
	{ USB_VENDOR_CONCEPTRONIC,	USB_PRODUCT_CONCEPTRONIC_RTL8192SU_3 },
	{ USB_VENDOR_COREGA,		USB_PRODUCT_COREGA_CGWLUSB300GNM },
	{ USB_VENDOR_DLINK2,		USB_PRODUCT_DLINK2_DWA131A1 },
	{ USB_VENDOR_DLINK2,		USB_PRODUCT_DLINK2_RTL8192SU_1 },
	{ USB_VENDOR_DLINK2,		USB_PRODUCT_DLINK2_RTL8192SU_2 },
	{ USB_VENDOR_EDIMAX,		USB_PRODUCT_EDIMAX_RTL8192SU_1 },
	{ USB_VENDOR_EDIMAX,		USB_PRODUCT_EDIMAX_RTL8192SU_2 },
	{ USB_VENDOR_EDIMAX,		USB_PRODUCT_EDIMAX_RTL8192SU_3 },
	{ USB_VENDOR_GUILLEMOT,		USB_PRODUCT_GUILLEMOT_HWGUN54 },
	{ USB_VENDOR_GUILLEMOT,		USB_PRODUCT_GUILLEMOT_HWNUM300 },
	{ USB_VENDOR_HAWKING,		USB_PRODUCT_HAWKING_RTL8192SU_1 },
	{ USB_VENDOR_HAWKING,		USB_PRODUCT_HAWKING_RTL8192SU_2 },
	{ USB_VENDOR_PLANEX2,		USB_PRODUCT_PLANEX2_GWUSNANO },
	{ USB_VENDOR_REALTEK,		USB_PRODUCT_REALTEK_RTL8171 },
	{ USB_VENDOR_REALTEK,		USB_PRODUCT_REALTEK_RTL8172 },
	{ USB_VENDOR_REALTEK,		USB_PRODUCT_REALTEK_RTL8173 },
	{ USB_VENDOR_REALTEK,		USB_PRODUCT_REALTEK_RTL8174 },
	{ USB_VENDOR_REALTEK,		USB_PRODUCT_REALTEK_RTL8192SU },
	{ USB_VENDOR_REALTEK,		USB_PRODUCT_REALTEK_RTL8712 },
	{ USB_VENDOR_REALTEK,		USB_PRODUCT_REALTEK_RTL8713 },
	{ USB_VENDOR_SENAO,		USB_PRODUCT_SENAO_RTL8192SU_1 },
	{ USB_VENDOR_SENAO,		USB_PRODUCT_SENAO_RTL8192SU_2 },
	{ USB_VENDOR_SITECOMEU,		USB_PRODUCT_SITECOMEU_WL349V1 },
	{ USB_VENDOR_SITECOMEU,		USB_PRODUCT_SITECOMEU_WL353 },
	{ USB_VENDOR_SITECOMEU,		USB_PRODUCT_SITECOMEU_RTL8188S },
	{ USB_VENDOR_SWEEX2,		USB_PRODUCT_SWEEX2_LW154 }
};

#ifdef notyet
/* List of devices that have HT support disabled. */
static const struct usb_devno rsu_devs_noht[] = {
	{ USB_VENDOR_ASUSTEK,		USB_PRODUCT_ASUS_RTL8192SU_1 },
	{ USB_VENDOR_AZUREWAVE,		USB_PRODUCT_AZUREWAVE_RTL8192SU_4 }
};
#endif

static int	rsu_match(device_t, cfdata_t, void *);
static void	rsu_attach(device_t, device_t, void *);
static int	rsu_detach(device_t, int);
static int	rsu_activate(device_t, enum devact);

int		rsu_open_pipes(struct rsu_softc *);
void		rsu_close_pipes(struct rsu_softc *);
int		rsu_alloc_rx_list(struct rsu_softc *);
void		rsu_free_rx_list(struct rsu_softc *);
int		rsu_alloc_tx_list(struct rsu_softc *);
void		rsu_free_tx_list(struct rsu_softc *);
void		rsu_task(void *);
void		rsu_do_async(struct rsu_softc *,
		    void (*)(struct rsu_softc *, void *), void *, int);
void		rsu_wait_async(struct rsu_softc *);
int		rsu_write_region_1(struct rsu_softc *, uint16_t, uint8_t *,
		    int);
void		rsu_write_1(struct rsu_softc *, uint16_t, uint8_t);
void		rsu_write_2(struct rsu_softc *, uint16_t, uint16_t);
void		rsu_write_4(struct rsu_softc *, uint16_t, uint32_t);
int		rsu_read_region_1(struct rsu_softc *, uint16_t, uint8_t *,
		    int);
uint8_t		rsu_read_1(struct rsu_softc *, uint16_t);
uint16_t	rsu_read_2(struct rsu_softc *, uint16_t);
uint32_t	rsu_read_4(struct rsu_softc *, uint16_t);
int		rsu_fw_iocmd(struct rsu_softc *, uint32_t);
uint8_t		rsu_efuse_read_1(struct rsu_softc *, uint16_t);
int		rsu_read_rom(struct rsu_softc *);
int		rsu_fw_cmd(struct rsu_softc *, uint8_t, void *, int);
int		rsu_media_change(struct ifnet *);
void		rsu_calib_to(void *);
void		rsu_calib_cb(struct rsu_softc *, void *);
int		rsu_newstate(struct ieee80211com *, enum ieee80211_state, int);
void		rsu_newstate_cb(struct rsu_softc *, void *);
int		rsu_site_survey(struct rsu_softc *);
int		rsu_join_bss(struct rsu_softc *, struct ieee80211_node *);
int		rsu_disconnect(struct rsu_softc *);
void		rsu_event_survey(struct rsu_softc *, uint8_t *, int);
void		rsu_event_join_bss(struct rsu_softc *, uint8_t *, int);
void		rsu_rx_event(struct rsu_softc *, uint8_t, uint8_t *, int);
void		rsu_rx_multi_event(struct rsu_softc *, uint8_t *, int);
int8_t		rsu_get_rssi(struct rsu_softc *, int, void *);
void		rsu_rx_frame(struct rsu_softc *, uint8_t *, int);
void		rsu_rx_multi_frame(struct rsu_softc *, uint8_t *, int);
void		rsu_rxeof(struct usbd_xfer *, void *, usbd_status);
void		rsu_txeof(struct usbd_xfer *, void *, usbd_status);
int		rsu_tx(struct rsu_softc *, struct mbuf *,
		    struct ieee80211_node *);
int		rsu_send_mgmt(struct ieee80211com *, struct ieee80211_node *,
		    int, int);
void		rsu_start(struct ifnet *);
void		rsu_watchdog(struct ifnet *);
int		rsu_ioctl(struct ifnet *, u_long, void *);
void		rsu_power_on_acut(struct rsu_softc *);
void		rsu_power_on_bcut(struct rsu_softc *);
void		rsu_power_off(struct rsu_softc *);
int		rsu_fw_loadsection(struct rsu_softc *, uint8_t *, int);
int		rsu_load_firmware(struct rsu_softc *);
int		rsu_init(struct ifnet *);
void		rsu_stop(struct ifnet *);

CFATTACH_DECL_NEW(rsu, sizeof(struct rsu_softc), rsu_match,
    rsu_attach, rsu_detach, rsu_activate);

static int
rsu_activate(device_t self, enum devact act)
{
	struct rsu_softc *sc = device_private(self);
	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(sc->sc_ic.ic_ifp);
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static int
rsu_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return ((usb_lookup(rsu_devs, uaa->uaa_vendor, uaa->uaa_product) != NULL) ?
	    UMATCH_VENDOR_PRODUCT_CONF_IFACE : UMATCH_NONE);
}

static void
rsu_attach(device_t parent, device_t self, void *aux)
{
	struct rsu_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int i, error;

	sc->sc_dev = self;
	sc->sc_udev = uaa->uaa_device;

	cv_init(&sc->sc_task_cv, "rsutsk");
	mutex_init(&sc->sc_task_mtx, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_tx_mtx, MUTEX_DEFAULT, IPL_NONE);

	usb_init_task(&sc->sc_task, rsu_task, sc, 0);

	callout_init(&sc->calib_to, 0);
	callout_setfunc(&sc->calib_to, rsu_calib_to, sc);

	/* Read chip revision. */
	sc->cut = MS(rsu_read_4(sc, R92S_PMC_FSM), R92S_PMC_FSM_CUT);
	if (sc->cut != 3)
		sc->cut = (sc->cut >> 1) + 1;

	KASSERT(sc->sc_dev != NULL);

	error = usbd_set_config_no(sc->sc_udev, 1, 0);
	if (error != 0) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(error));
		return;
	}

	/* Get the first interface handle. */
	error = usbd_device2interface_handle(sc->sc_udev, 0, &sc->sc_iface);
	if (error != 0) {
		aprint_error_dev(self, "could not get interface handle\n");
		return;
	}

	error = rsu_read_rom(sc);
	if (error != 0) {
		printf("%s: could not read ROM\n", device_xname(sc->sc_dev));
		return;
	}
	IEEE80211_ADDR_COPY(ic->ic_myaddr, &sc->rom[0x12]);

	printf("%s: MAC/BB RTL8712 cut %d, address %s\n",
	    device_xname(sc->sc_dev), sc->cut, ether_sprintf(ic->ic_myaddr));

	if (rsu_open_pipes(sc) != 0)
		return;

	KASSERT(ifp != NULL);

	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_OFDM;	/* Not only, but not used. */
	ic->ic_opmode = IEEE80211_M_STA;	/* Default to BSS mode. */
	ic->ic_state = IEEE80211_S_INIT;

	/* Set device capabilities. */
	ic->ic_caps =
	    IEEE80211_C_MONITOR |	/* Monitor mode. */
	    IEEE80211_C_SHPREAMBLE |	/* Short preamble supported. */
	    IEEE80211_C_SHSLOT |	/* Short slot time supported. */
	    IEEE80211_C_WPA;		/* WPA/RSN. */

#ifdef notyet
	ic->ic_caps +=
	    IEEE80211_C_WEP;		/* WEP. */
#endif

#ifdef notyet
	/* Check if HT support is present. */
	if (usb_lookup(rsu_devs_noht, uaa->uaa_vendor, uaa->uaa_product) == NULL) {
		/* Set HT capabilities. */
		ic->ic_htcaps =
		    IEEE80211_HTCAP_CBW20_40 |
		    IEEE80211_HTCAP_DSSSCCK40;
		/* Set supported HT rates. */
		for (i = 0; i < 2; i++)
			ic->ic_sup_mcs[i] = 0xff;
	}
#endif

	/* Set supported .11b and .11g rates. */
	ic->ic_sup_rates[IEEE80211_MODE_11B] = ieee80211_std_rateset_11b;
	ic->ic_sup_rates[IEEE80211_MODE_11G] = ieee80211_std_rateset_11g;

	/* Set supported .11b and .11g channels (1 through 14). */
	for (i = 1; i <= 14; i++) {
		ic->ic_channels[i].ic_freq =
		    ieee80211_ieee2mhz(i, IEEE80211_CHAN_2GHZ);
		ic->ic_channels[i].ic_flags =
		    IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM |
		    IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ;
	}

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = rsu_init;
	ifp->if_ioctl = rsu_ioctl;
	ifp->if_start = rsu_start;
	ifp->if_watchdog = rsu_watchdog;
	memcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);

	if_initialize(ifp);
	ieee80211_ifattach(ic);

	/* Override state transition machine. */
	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = rsu_newstate;

	/* XXX media locking needs revisiting */
	mutex_init(&sc->sc_media_mtx, MUTEX_DEFAULT, IPL_SOFTUSB);
	ieee80211_media_init_with_lock(ic,
	    rsu_media_change, ieee80211_media_status, &sc->sc_media_mtx);

	if_register(ifp);

	ieee80211_announce(ic);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);
}

static int
rsu_detach(device_t self, int flags)
{
	struct rsu_softc *sc = device_private(self);
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	int s;

	s = splusb();

	if (ifp->if_flags & IFF_RUNNING) {
		rsu_stop(ifp);
		callout_halt(&sc->calib_to, NULL);
		usb_rem_task_wait(sc->sc_udev, &sc->sc_task, USB_TASKQ_DRIVER,
		   NULL);
	}

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ieee80211_ifdetach(ic);
	if_detach(ifp);

	mutex_destroy(&sc->sc_media_mtx);

	/* Abort and close Tx/Rx pipes. */
	rsu_close_pipes(sc);

	/* Free Tx/Rx buffers. */
	rsu_free_tx_list(sc);
	rsu_free_rx_list(sc);
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev, sc->sc_dev);

	callout_destroy(&sc->calib_to);

	mutex_destroy(&sc->sc_task_mtx);
	mutex_destroy(&sc->sc_tx_mtx);

	return (0);
}

int
rsu_open_pipes(struct rsu_softc *sc)
{
	usb_interface_descriptor_t *id;
	int i, error;

	/*
	 * Determine the number of Tx/Rx endpoints (there are chips with
	 * 4, 6 or 11 endpoints).
	 */
	id = usbd_get_interface_descriptor(sc->sc_iface);
	sc->npipes = id->bNumEndpoints;
	if (sc->npipes == 4)
		sc->qid2idx = rsu_qid2idx_4ep;
	else if (sc->npipes == 6)
		sc->qid2idx = rsu_qid2idx_6ep;
	else	/* Assume npipes==11; will fail below otherwise. */
		sc->qid2idx = rsu_qid2idx_11ep;
	DPRINTF(("%d endpoints configuration\n", sc->npipes));

	/* Open all pipes. */
	for (i = 0; i < MIN(sc->npipes, (int)__arraycount(r92s_epaddr)); i++) {
		error = usbd_open_pipe(sc->sc_iface, r92s_epaddr[i], 0,
		    &sc->pipe[i]);
		if (error != 0) {
			printf("%s: could not open bulk pipe 0x%02x\n",
			    device_xname(sc->sc_dev), r92s_epaddr[i]);
			break;
		}
	}
	if (error != 0)
		rsu_close_pipes(sc);
	return (error);
}

void
rsu_close_pipes(struct rsu_softc *sc)
{
	struct usbd_pipe *pipe;
	int i;

	/* Close all pipes. */
	for (i = 0; i < sc->npipes; i++) {
		pipe = atomic_swap_ptr(&sc->pipe[i], NULL);
		if (pipe != NULL) {
			usbd_close_pipe(pipe);
		}
	}
}

int
rsu_alloc_rx_list(struct rsu_softc *sc)
{
	struct rsu_rx_data *data;
	int i, error = 0;

	for (i = 0; i < RSU_RX_LIST_COUNT; i++) {
		data = &sc->rx_data[i];

		data->sc = sc;	/* Backpointer for callbacks. */

		error = usbd_create_xfer(data->pipe, RSU_RXBUFSZ,
			0, 0, &data->xfer);
		if (error) {
			printf("%s: could not allocate xfer\n",
			    device_xname(sc->sc_dev));
			error = ENOMEM;
			break;
		}
		data->buf = usbd_get_buffer(data->xfer);
	}
	if (error != 0)
		rsu_free_rx_list(sc);
	return (error);
}

void
rsu_free_rx_list(struct rsu_softc *sc)
{
	int i;

	/* NB: Caller must abort pipe first. */
	for (i = 0; i < RSU_RX_LIST_COUNT; i++) {
		if (sc->rx_data[i].xfer != NULL)
			usbd_destroy_xfer(sc->rx_data[i].xfer);
		sc->rx_data[i].xfer = NULL;
	}
}

int
rsu_alloc_tx_list(struct rsu_softc *sc)
{
	struct rsu_tx_data *data;
	int i, error = 0;

	mutex_enter(&sc->sc_tx_mtx);
	TAILQ_INIT(&sc->tx_free_list);
	for (i = 0; i < RSU_TX_LIST_COUNT; i++) {
		data = &sc->tx_data[i];

		data->sc = sc;	/* Backpointer for callbacks. */

		error = usbd_create_xfer(data->pipe,
			    RSU_TXBUFSZ, USBD_FORCE_SHORT_XFER, 0,
			    &data->xfer);
		if (error) {
			printf("%s: could not allocate xfer\n",
			    device_xname(sc->sc_dev));
			error = ENOMEM;
			break;
		}
		data->buf = usbd_get_buffer(data->xfer);

		/* Append this Tx buffer to our free list. */
		TAILQ_INSERT_TAIL(&sc->tx_free_list, data, next);
	}
	if (error != 0)
		rsu_free_tx_list(sc);

	mutex_exit(&sc->sc_tx_mtx);
	return (error);
}

void
rsu_free_tx_list(struct rsu_softc *sc)
{
	struct usbd_xfer *xfer;
	int i;

	/* NB: Caller must abort pipe first. */
	for (i = 0; i < RSU_TX_LIST_COUNT; i++) {
		xfer = atomic_swap_ptr(&sc->tx_data[i].xfer, NULL);
		if (xfer != NULL)
			usbd_destroy_xfer(xfer);
	}
}

void
rsu_task(void *arg)
{
	struct rsu_softc *sc = arg;
	struct rsu_host_cmd_ring *ring = &sc->cmdq;
	struct rsu_host_cmd *cmd;
	int s;

	/* Process host commands. */
	s = splusb();
	mutex_spin_enter(&sc->sc_task_mtx);

	while (ring->next != ring->cur) {
		cmd = &ring->cmd[ring->next];
		splx(s);
		membar_consumer();
		/* Invoke callback. */
		cmd->cb(sc, cmd->data);
		s = splusb();
		atomic_dec_uint(&ring->queued);
		ring->next = (ring->next + 1) % RSU_HOST_CMD_RING_COUNT;
	}

	mutex_spin_exit(&sc->sc_task_mtx);
	splx(s);
}

void
rsu_do_async(struct rsu_softc *sc,
    void (*cb)(struct rsu_softc *, void *), void *arg, int len)
{
	struct rsu_host_cmd_ring *ring = &sc->cmdq;
	struct rsu_host_cmd *cmd;
	bool schedtask = false;
	int s;

	s = splusb();
	mutex_spin_enter(&sc->sc_task_mtx);

	cmd = &ring->cmd[ring->cur];
	cmd->cb = cb;
	KASSERT(len <= sizeof(cmd->data));
	memcpy(cmd->data, arg, len);
	ring->cur = (ring->cur + 1) % RSU_HOST_CMD_RING_COUNT;

	/* If there is no pending command already, schedule a task. */
	if (atomic_inc_uint_nv(&ring->queued) == 1)
		schedtask = true;

	mutex_spin_exit(&sc->sc_task_mtx);
	splx(s);

	if (schedtask)
		usb_add_task(sc->sc_udev, &sc->sc_task, USB_TASKQ_DRIVER);
}

void
rsu_wait_async(struct rsu_softc *sc)
{
	/* Wait for all queued asynchronous commands to complete. */
	mutex_spin_enter(&sc->sc_task_mtx);
	while (sc->cmdq.queued > 0)
		cv_wait(&sc->sc_task_cv, &sc->sc_task_mtx);
	mutex_spin_exit(&sc->sc_task_mtx);
}

int
rsu_write_region_1(struct rsu_softc *sc, uint16_t addr, uint8_t *buf,
    int len)
{
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = R92S_REQ_REGS;
	USETW(req.wValue, addr);
	USETW(req.wIndex, 0);
	USETW(req.wLength, len);
	return (usbd_do_request(sc->sc_udev, &req, buf));
}

void
rsu_write_1(struct rsu_softc *sc, uint16_t addr, uint8_t val)
{
	rsu_write_region_1(sc, addr, &val, 1);
}

void
rsu_write_2(struct rsu_softc *sc, uint16_t addr, uint16_t val)
{
	val = htole16(val);
	rsu_write_region_1(sc, addr, (uint8_t *)&val, 2);
}

void
rsu_write_4(struct rsu_softc *sc, uint16_t addr, uint32_t val)
{
	val = htole32(val);
	rsu_write_region_1(sc, addr, (uint8_t *)&val, 4);
}

int
rsu_read_region_1(struct rsu_softc *sc, uint16_t addr, uint8_t *buf,
    int len)
{
	usb_device_request_t req;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = R92S_REQ_REGS;
	USETW(req.wValue, addr);
	USETW(req.wIndex, 0);
	USETW(req.wLength, len);
	return (usbd_do_request(sc->sc_udev, &req, buf));
}

uint8_t
rsu_read_1(struct rsu_softc *sc, uint16_t addr)
{
	uint8_t val;

	if (rsu_read_region_1(sc, addr, &val, 1) != 0)
		return (0xff);
	return (val);
}

uint16_t
rsu_read_2(struct rsu_softc *sc, uint16_t addr)
{
	uint16_t val;

	if (rsu_read_region_1(sc, addr, (uint8_t *)&val, 2) != 0)
		return (0xffff);
	return (le16toh(val));
}

uint32_t
rsu_read_4(struct rsu_softc *sc, uint16_t addr)
{
	uint32_t val;

	if (rsu_read_region_1(sc, addr, (uint8_t *)&val, 4) != 0)
		return (0xffffffff);
	return (le32toh(val));
}

int
rsu_fw_iocmd(struct rsu_softc *sc, uint32_t iocmd)
{
	int ntries;

	rsu_write_4(sc, R92S_IOCMD_CTRL, iocmd);
	DELAY(100);
	for (ntries = 0; ntries < 50; ntries++) {
		if (rsu_read_4(sc, R92S_IOCMD_CTRL) == 0)
			return (0);
		DELAY(10);
	}
	return (ETIMEDOUT);
}

uint8_t
rsu_efuse_read_1(struct rsu_softc *sc, uint16_t addr)
{
	uint32_t reg;
	int ntries;

	reg = rsu_read_4(sc, R92S_EFUSE_CTRL);
	reg = RW(reg, R92S_EFUSE_CTRL_ADDR, addr);
	reg &= ~R92S_EFUSE_CTRL_VALID;
	rsu_write_4(sc, R92S_EFUSE_CTRL, reg);
	/* Wait for read operation to complete. */
	for (ntries = 0; ntries < 100; ntries++) {
		reg = rsu_read_4(sc, R92S_EFUSE_CTRL);
		if (reg & R92S_EFUSE_CTRL_VALID)
			return (MS(reg, R92S_EFUSE_CTRL_DATA));
		DELAY(5);
	}
	printf("%s: could not read efuse byte at address 0x%x\n",
	    device_xname(sc->sc_dev), addr);
	return (0xff);
}

int
rsu_read_rom(struct rsu_softc *sc)
{
	uint8_t *rom = sc->rom;
	uint16_t addr = 0;
	uint32_t reg;
	uint8_t off, msk;
	int i;

	/* Make sure that ROM type is eFuse and that autoload succeeded. */
	reg = rsu_read_1(sc, R92S_EE_9346CR);
	if ((reg & (R92S_9356SEL | R92S_EEPROM_EN)) != R92S_EEPROM_EN)
		return (EIO);

	/* Turn on 2.5V to prevent eFuse leakage. */
	reg = rsu_read_1(sc, R92S_EFUSE_TEST + 3);
	rsu_write_1(sc, R92S_EFUSE_TEST + 3, reg | 0x80);
	usbd_delay_ms(sc->sc_udev, 10);
	rsu_write_1(sc, R92S_EFUSE_TEST + 3, reg & ~0x80);

	/* Read full ROM image. */
	memset(&sc->rom, 0xff, sizeof(sc->rom));
	while (addr < 512) {
		reg = rsu_efuse_read_1(sc, addr);
		if (reg == 0xff)
			break;
		addr++;
		off = reg >> 4;
		msk = reg & 0xf;
		for (i = 0; i < 4; i++) {
			if (msk & (1 << i))
				continue;
			rom[off * 8 + i * 2 + 0] =
			    rsu_efuse_read_1(sc, addr);
			addr++;
			rom[off * 8 + i * 2 + 1] =
			    rsu_efuse_read_1(sc, addr);
			addr++;
		}
	}

#ifdef RSU_DEBUG
	if (rsu_debug >= 5) {
		/* Dump ROM content. */
		printf("\n");
		for (i = 0; i < sizeof(sc->rom); i++)
			printf("%02x:", rom[i]);
		printf("\n");
	}
#endif
	return (0);
}

int
rsu_fw_cmd(struct rsu_softc *sc, uint8_t code, void *buf, int len)
{
	struct rsu_tx_data *data;
	struct r92s_tx_desc *txd;
	struct r92s_fw_cmd_hdr *cmd;
	int cmdsz, xferlen;

	data = sc->fwcmd_data;

	/* Round-up command length to a multiple of 8 bytes. */
	cmdsz = (len + 7) & ~7;

	xferlen = sizeof(*txd) + sizeof(*cmd) + cmdsz;
	KASSERT(xferlen <= RSU_TXBUFSZ);
	memset(data->buf, 0, xferlen);

	/* Setup Tx descriptor. */
	txd = (struct r92s_tx_desc *)data->buf;
	txd->txdw0 = htole32(
	    SM(R92S_TXDW0_OFFSET, sizeof(*txd)) |
	    SM(R92S_TXDW0_PKTLEN, sizeof(*cmd) + cmdsz) |
	    R92S_TXDW0_OWN | R92S_TXDW0_FSG | R92S_TXDW0_LSG);
	txd->txdw1 = htole32(SM(R92S_TXDW1_QSEL, R92S_TXDW1_QSEL_H2C));

	/* Setup command header. */
	cmd = (struct r92s_fw_cmd_hdr *)&txd[1];
	cmd->len = htole16(cmdsz);
	cmd->code = code;
	cmd->seq = sc->cmd_seq;
	sc->cmd_seq = (sc->cmd_seq + 1) & 0x7f;

	/* Copy command payload. */
	memcpy(&cmd[1], buf, len);

	DPRINTFN(2, ("Tx cmd code=%d len=%d\n", code, cmdsz));
	/*pipe = sc->pipe[sc->qid2idx[RSU_QID_H2C]];*/
	usbd_setup_xfer(data->xfer, data, data->buf, xferlen,
	    USBD_SHORT_XFER_OK | USBD_SYNCHRONOUS,
	    RSU_CMD_TIMEOUT, NULL);
	return (usbd_transfer(data->xfer));
}

int
rsu_media_change(struct ifnet *ifp)
{
	int error;

	error = ieee80211_media_change(ifp);
	if (error != ENETRESET)
		return (error);

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
	    (IFF_UP | IFF_RUNNING)) {
		rsu_stop(ifp);
		error = rsu_init(ifp);
	}
	return (error);
}

void
rsu_calib_to(void *arg)
{
	struct rsu_softc *sc = arg;

	/* Do it in a process context. */
	rsu_do_async(sc, rsu_calib_cb, NULL, 0);
}

void
rsu_calib_cb(struct rsu_softc *sc, void *arg)
{
	uint32_t reg;

#ifdef notyet
	/* Read WPS PBC status. */
	rsu_write_1(sc, R92S_MAC_PINMUX_CTRL,
	    R92S_GPIOMUX_EN | SM(R92S_GPIOSEL_GPIO, R92S_GPIOSEL_GPIO_JTAG));
	rsu_write_1(sc, R92S_GPIO_IO_SEL,
	    rsu_read_1(sc, R92S_GPIO_IO_SEL) & ~R92S_GPIO_WPS);
	reg = rsu_read_1(sc, R92S_GPIO_CTRL);
	if (reg != 0xff && (reg & R92S_GPIO_WPS))
		DPRINTF(("WPS PBC is pushed\n"));
#endif

	/* Read current signal level. */
	if (rsu_fw_iocmd(sc, 0xf4000001) == 0) {
		reg = rsu_read_4(sc, R92S_IOCMD_DATA);
		DPRINTFN(8, ("RSSI=%d%%\n", reg >> 4));
	}
	(void)reg;

	callout_schedule(&sc->calib_to, hz);
}

int
rsu_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct rsu_softc *sc = ic->ic_ifp->if_softc;
	struct rsu_cmd_newstate cmd;

	callout_stop(&sc->calib_to);

	/* Do it in a process context. */
	cmd.state = nstate;
	cmd.arg = arg;
	rsu_do_async(sc, rsu_newstate_cb, &cmd, sizeof(cmd));
	return (0);
}

void
rsu_newstate_cb(struct rsu_softc *sc, void *arg)
{
	struct rsu_cmd_newstate *cmd = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	enum ieee80211_state ostate;
	int error, s;

	s = splnet();
	ostate = ic->ic_state;

	if (ostate == IEEE80211_S_RUN) {
		/* Stop calibration. */
		callout_halt(&sc->calib_to, NULL);
		/* Disassociate from our current BSS. */
		(void)rsu_disconnect(sc);
	}
	switch (cmd->state) {
	case IEEE80211_S_INIT:
		break;
	case IEEE80211_S_SCAN:
		error = rsu_site_survey(sc);
		if (error != 0) {
			printf("%s: could not send site survey command\n",
			    device_xname(sc->sc_dev));
		}
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: %s -> %s\n", ifp->if_xname,
			    ieee80211_state_name[ic->ic_state],
			    ieee80211_state_name[cmd->state]);
		ic->ic_state = cmd->state;
		splx(s);
		return;
	case IEEE80211_S_AUTH:
		ic->ic_state = cmd->state;
		splx(s);
		return;
	case IEEE80211_S_ASSOC:
		/* No-op for this driver. See rsu_event_join_bss(). */
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: %s -> %s\n", ifp->if_xname,
			    ieee80211_state_name[ic->ic_state],
			    ieee80211_state_name[cmd->state]);
		ic->ic_state = cmd->state;
		splx(s);
		return;
	case IEEE80211_S_RUN:
		/* Indicate highest supported rate. */
		ic->ic_bss->ni_txrate = ic->ic_bss->ni_rates.rs_nrates - 1;

		/* Start periodic calibration. */
		callout_schedule(&sc->calib_to, hz);
		break;
	}
	(void)sc->sc_newstate(ic, cmd->state, cmd->arg);
	splx(s);
}

int
rsu_site_survey(struct rsu_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r92s_fw_cmd_sitesurvey cmd;

	memset(&cmd, 0, sizeof(cmd));
	if ((ic->ic_flags & IEEE80211_F_ASCAN) || sc->scan_pass == 1)
		cmd.active = htole32(1);
	cmd.limit = htole32(48);
	if (sc->scan_pass == 1) {
		/* Do a directed scan for second pass. */
		cmd.ssidlen = htole32(ic->ic_des_esslen);
		memcpy(cmd.ssid, ic->ic_des_essid, ic->ic_des_esslen);
	}
	DPRINTF(("sending site survey command, pass=%d\n", sc->scan_pass));
	return (rsu_fw_cmd(sc, R92S_CMD_SITE_SURVEY, &cmd, sizeof(cmd)));
}

int
rsu_join_bss(struct rsu_softc *sc, struct ieee80211_node *ni)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ndis_wlan_bssid_ex *bss;
	struct ndis_802_11_fixed_ies *fixed;
	struct r92s_fw_cmd_auth auth;
	uint8_t buf[sizeof(*bss) + 128], *frm;
	uint8_t opmode;
	int error;

	/* Let the FW decide the opmode based on the capinfo field. */
	opmode = NDIS802_11AUTOUNKNOWN;
	DPRINTF(("setting operating mode to %d\n", opmode));
	error = rsu_fw_cmd(sc, R92S_CMD_SET_OPMODE, &opmode, sizeof(opmode));
	if (error != 0)
		return (error);

	memset(&auth, 0, sizeof(auth));
	auth.mode = R92S_AUTHMODE_OPEN;
	DPRINTF(("setting auth mode to %d\n", auth.mode));
	error = rsu_fw_cmd(sc, R92S_CMD_SET_AUTH, &auth, sizeof(auth));
	if (error != 0)
		return (error);

	memset(buf, 0, sizeof(buf));
	bss = (struct ndis_wlan_bssid_ex *)buf;
	IEEE80211_ADDR_COPY(bss->macaddr, ni->ni_bssid);
	bss->ssid.ssidlen = htole32(ni->ni_esslen);
	memcpy(bss->ssid.ssid, ni->ni_essid, ni->ni_esslen);
	bss->rssi = htole32(ni->ni_rssi);
	if (ic->ic_curmode == IEEE80211_MODE_11B)
		bss->networktype = htole32(NDIS802_11DS);
	else
		bss->networktype = htole32(NDIS802_11OFDM24);
	bss->config.len = htole32(sizeof(bss->config));
	bss->config.bintval = htole32(ni->ni_intval);
	bss->config.dsconfig = htole32(ieee80211_chan2ieee(ic, ni->ni_chan));
	bss->inframode = htole32(NDIS802_11INFRASTRUCTURE);
	memcpy(bss->supprates, ni->ni_rates.rs_rates,
	    ni->ni_rates.rs_nrates);
	/* Write the fixed fields of the beacon frame. */
	fixed = (struct ndis_802_11_fixed_ies *)&bss[1];
	memcpy(&fixed->tstamp, ni->ni_tstamp.data, 8);
	fixed->bintval = htole16(ni->ni_intval);
	fixed->capabilities = htole16(ni->ni_capinfo);
	/* Write IEs to be included in the association request. */
	frm = (uint8_t *)&fixed[1];
	bss->ieslen = htole32(frm - (uint8_t *)fixed);
	bss->len = htole32(((frm - buf) + 3) & ~3);
	DPRINTF(("sending join bss command to %s chan %d\n",
	    ether_sprintf(bss->macaddr), le32toh(bss->config.dsconfig)));
	return (rsu_fw_cmd(sc, R92S_CMD_JOIN_BSS, buf, sizeof(buf)));
}

int
rsu_disconnect(struct rsu_softc *sc)
{
	uint32_t zero = 0;	/* :-) */

	/* Disassociate from our current BSS. */
	DPRINTF(("sending disconnect command\n"));
	return (rsu_fw_cmd(sc, R92S_CMD_DISCONNECT, &zero, sizeof(zero)));
}

void
rsu_event_survey(struct rsu_softc *sc, uint8_t *buf, int len)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	struct ieee80211_frame *wh;
	struct ndis_wlan_bssid_ex *bss;
	struct mbuf *m;
	uint32_t pktlen, ieslen;

	if (__predict_false(len < sizeof(*bss)))
		return;
	bss = (struct ndis_wlan_bssid_ex *)buf;
	ieslen = le32toh(bss->ieslen);
	if (ieslen > len - sizeof(*bss))
		return;

	DPRINTFN(2, ("found BSS %s: len=%d chan=%d inframode=%d "
	    "networktype=%d privacy=%d\n",
	    ether_sprintf(bss->macaddr), le32toh(bss->len),
	    le32toh(bss->config.dsconfig), le32toh(bss->inframode),
	    le32toh(bss->networktype), le32toh(bss->privacy)));

	/* Build a fake beacon frame to let net80211 do all the parsing. */
	pktlen = sizeof(*wh) + ieslen;
	if (__predict_false(pktlen > MCLBYTES))
		return;
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (__predict_false(m == NULL))
		return;
	if (pktlen > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if (!(m->m_flags & M_EXT)) {
			m_free(m);
			return;
		}
	}
	wh = mtod(m, struct ieee80211_frame *);
	wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_MGT |
	    IEEE80211_FC0_SUBTYPE_BEACON;
	wh->i_fc[1] = IEEE80211_FC1_DIR_NODS;
	*(uint16_t *)wh->i_dur = 0;
	IEEE80211_ADDR_COPY(wh->i_addr1, etherbroadcastaddr);
	IEEE80211_ADDR_COPY(wh->i_addr2, bss->macaddr);
	IEEE80211_ADDR_COPY(wh->i_addr3, bss->macaddr);
	*(uint16_t *)wh->i_seq = 0;
	memcpy(&wh[1], (uint8_t *)&bss[1], ieslen);

	/* Finalize mbuf. */
	m->m_pkthdr.len = m->m_len = pktlen;

	ni = ieee80211_find_rxnode(ic, (struct ieee80211_frame_min *)wh);
	ieee80211_input(ic, m, ni, le32toh(bss->rssi), 0);
	/* Node is no longer needed. */
	ieee80211_free_node(ni);
}

void
rsu_event_join_bss(struct rsu_softc *sc, uint8_t *buf, int len)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	struct r92s_event_join_bss *rsp;
	int res;

	if (__predict_false(len < sizeof(*rsp)))
		return;
	rsp = (struct r92s_event_join_bss *)buf;
	res = (int)le32toh(rsp->join_res);

	DPRINTF(("Rx join BSS event len=%d res=%d\n", len, res));
	if (res <= 0) {
		ic->ic_stats.is_rx_auth_fail++;
		ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
		return;
	}
	DPRINTF(("associated with %s associd=%d\n",
	    ether_sprintf(rsp->bss.macaddr), le32toh(rsp->associd)));

	ni->ni_associd = le32toh(rsp->associd) | 0xc000;

	/* Force an ASSOC->RUN transition. AUTH->RUN is invalid. */
	ic->ic_state = IEEE80211_S_ASSOC;
	ieee80211_new_state(ic, IEEE80211_S_RUN,
	    IEEE80211_FC0_SUBTYPE_ASSOC_RESP);
}

void
rsu_rx_event(struct rsu_softc *sc, uint8_t code, uint8_t *buf, int len)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;

	DPRINTFN(4, ("Rx event code=%d len=%d\n", code, len));
	switch (code) {
	case R92S_EVT_SURVEY:
		if (ic->ic_state == IEEE80211_S_SCAN)
			rsu_event_survey(sc, buf, len);
		break;
	case R92S_EVT_SURVEY_DONE:
		DPRINTF(("site survey pass %d done, found %d BSS\n",
		    sc->scan_pass, le32toh(*(uint32_t *)buf)));
		if (ic->ic_state != IEEE80211_S_SCAN)
			break;	/* Ignore if not scanning. */
		if (sc->scan_pass == 0 && ic->ic_des_esslen != 0) {
			/* Schedule a directed scan for hidden APs. */
			sc->scan_pass = 1;
			ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
			break;
		}
		sc->scan_pass = 0;
		break;
	case R92S_EVT_JOIN_BSS:
		if (ic->ic_state == IEEE80211_S_AUTH)
			rsu_event_join_bss(sc, buf, len);
		break;
	case R92S_EVT_DEL_STA:
		DPRINTF(("disassociated from %s\n", ether_sprintf(buf)));
		if (ic->ic_state == IEEE80211_S_RUN &&
		    IEEE80211_ADDR_EQ(ic->ic_bss->ni_bssid, buf))
			ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
		break;
	case R92S_EVT_WPS_PBC:
		DPRINTF(("WPS PBC pushed.\n"));
		break;
	case R92S_EVT_FWDBG:
		if (ifp->if_flags & IFF_DEBUG) {
			buf[60] = '\0';
			printf("FWDBG: %s\n", (char *)buf);
		}
		break;
	}
}

void
rsu_rx_multi_event(struct rsu_softc *sc, uint8_t *buf, int len)
{
	struct r92s_fw_cmd_hdr *cmd;
	int cmdsz;

	DPRINTFN(6, ("Rx events len=%d\n", len));

	/* Skip Rx status. */
	buf += sizeof(struct r92s_rx_stat);
	len -= sizeof(struct r92s_rx_stat);

	/* Process all events. */
	for (;;) {
		/* Check that command header fits. */
		if (__predict_false(len < sizeof(*cmd)))
			break;
		cmd = (struct r92s_fw_cmd_hdr *)buf;
		/* Check that command payload fits. */
		cmdsz = le16toh(cmd->len);
		if (__predict_false(len < sizeof(*cmd) + cmdsz))
			break;
		if (cmdsz > len)
			break;

		/* Process firmware event. */
		rsu_rx_event(sc, cmd->code, (uint8_t *)&cmd[1], cmdsz);

		if (!(cmd->seq & R92S_FW_CMD_MORE))
			break;
		buf += sizeof(*cmd) + cmdsz;
		len -= sizeof(*cmd) + cmdsz;
	}
}

int8_t
rsu_get_rssi(struct rsu_softc *sc, int rate, void *physt)
{
	static const int8_t cckoff[] = { 14, -2, -20, -40 };
	struct r92s_rx_phystat *phy;
	struct r92s_rx_cck *cck;
	uint8_t rpt;
	int8_t rssi;

	if (rate <= 3) {
		cck = (struct r92s_rx_cck *)physt;
		rpt = (cck->agc_rpt >> 6) & 0x3;
		rssi = cck->agc_rpt & 0x3e;
		rssi = cckoff[rpt] - rssi;
	} else {	/* OFDM/HT. */
		phy = (struct r92s_rx_phystat *)physt;
		rssi = ((le32toh(phy->phydw1) >> 1) & 0x7f) - 106;
	}
	return (rssi);
}

void
rsu_rx_frame(struct rsu_softc *sc, uint8_t *buf, int pktlen)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	struct ieee80211_frame *wh;
	struct ieee80211_node *ni;
	struct r92s_rx_stat *stat;
	uint32_t rxdw0, rxdw3;
	struct mbuf *m;
	uint8_t rate;
	int8_t rssi = 0;
	int s, infosz;

	stat = (struct r92s_rx_stat *)buf;
	rxdw0 = le32toh(stat->rxdw0);
	rxdw3 = le32toh(stat->rxdw3);

	if (__predict_false(rxdw0 & R92S_RXDW0_CRCERR)) {
		if_statinc(ifp, if_ierrors);
		return;
	}
	if (__predict_false(pktlen < sizeof(*wh) || pktlen > MCLBYTES)) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	rate = MS(rxdw3, R92S_RXDW3_RATE);
	infosz = MS(rxdw0, R92S_RXDW0_INFOSZ) * 8;

	/* Get RSSI from PHY status descriptor if present. */
	if (infosz != 0)
		rssi = rsu_get_rssi(sc, rate, &stat[1]);

	DPRINTFN(5, ("Rx frame len=%d rate=%d infosz=%d rssi=%d\n",
	    pktlen, rate, infosz, rssi));

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (__predict_false(m == NULL)) {
		if_statinc(ifp, if_ierrors);
		return;
	}
	if (pktlen > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if (__predict_false(!(m->m_flags & M_EXT))) {
			if_statinc(ifp, if_ierrors);
			m_freem(m);
			return;
		}
	}
	/* Finalize mbuf. */
	m_set_rcvif(m, ifp);

	/* Hardware does Rx TCP checksum offload. */
	if (rxdw3 & R92S_RXDW3_TCPCHKVALID) {
		if (__predict_true(rxdw3 & R92S_RXDW3_TCPCHKRPT))
			m->m_pkthdr.csum_flags |= M_CSUM_TCPv4;
		else
			m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
	}
	wh = (struct ieee80211_frame *)((uint8_t *)&stat[1] + infosz);
	memcpy(mtod(m, uint8_t *), wh, pktlen);
	m->m_pkthdr.len = m->m_len = pktlen;

	s = splnet();

	ni = ieee80211_find_rxnode(ic, (struct ieee80211_frame_min *)wh);
	ieee80211_input(ic, m, ni, rssi, 0);
	/* Node is no longer needed. */
	ieee80211_free_node(ni);
	splx(s);
}

void
rsu_rx_multi_frame(struct rsu_softc *sc, uint8_t *buf, int len)
{
	struct r92s_rx_stat *stat;
	uint32_t rxdw0;
	int totlen, pktlen, infosz, npkts;

	/* Get the number of encapsulated frames. */
	stat = (struct r92s_rx_stat *)buf;
	npkts = MS(le32toh(stat->rxdw2), R92S_RXDW2_PKTCNT);
	DPRINTFN(6, ("Rx %d frames in one chunk\n", npkts));

	/* Process all of them. */
	while (npkts-- > 0) {
		if (__predict_false(len < sizeof(*stat)))
			break;
		stat = (struct r92s_rx_stat *)buf;
		rxdw0 = le32toh(stat->rxdw0);

		pktlen = MS(rxdw0, R92S_RXDW0_PKTLEN);
		if (__predict_false(pktlen == 0))
			break;

		infosz = MS(rxdw0, R92S_RXDW0_INFOSZ) * 8;

		/* Make sure everything fits in xfer. */
		totlen = sizeof(*stat) + infosz + pktlen;
		if (__predict_false(totlen > len))
			break;

		/* Process 802.11 frame. */
		rsu_rx_frame(sc, buf, pktlen);

		/* Next chunk is 128-byte aligned. */
		totlen = (totlen + 127) & ~127;
		buf += totlen;
		len -= totlen;
	}
}

void
rsu_rxeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct rsu_rx_data *data = priv;
	struct rsu_softc *sc = data->sc;
	struct r92s_rx_stat *stat;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	int len;

	if (__predict_false(status != USBD_NORMAL_COMPLETION)) {
		DPRINTF(("RX status=%d\n", status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(data->pipe);
		if (status != USBD_CANCELLED)
			goto resubmit;
		return;
	}
	usbd_get_xfer_status(xfer, NULL, NULL, &len, NULL);

	if (__predict_false(len < sizeof(*stat))) {
		DPRINTF(("xfer too short %d\n", len));
		if_statinc(ifp, if_ierrors);
		goto resubmit;
	}
	if (len > RSU_RXBUFSZ) {
		DPRINTF(("xfer too large %d\n", len));
		if_statinc(ifp, if_ierrors);
		goto resubmit;
	}
		
	/* Determine if it is a firmware C2H event or an 802.11 frame. */
	stat = (struct r92s_rx_stat *)data->buf;
	if ((le32toh(stat->rxdw1) & 0x1ff) == 0x1ff)
		rsu_rx_multi_event(sc, data->buf, len);
	else
		rsu_rx_multi_frame(sc, data->buf, len);

 resubmit:
	/* Setup a new transfer. */
	usbd_setup_xfer(xfer, data, data->buf, RSU_RXBUFSZ,
	    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, rsu_rxeof);
	(void)usbd_transfer(xfer);
}

void
rsu_txeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct rsu_tx_data *data = priv;
	struct rsu_softc *sc = data->sc;
	struct ifnet *ifp = sc->sc_ic.ic_ifp;
	int s;

	s = splnet();

	/* Put this Tx buffer back to our free list. */
	mutex_enter(&sc->sc_tx_mtx);
	TAILQ_INSERT_TAIL(&sc->tx_free_list, data, next);
	mutex_exit(&sc->sc_tx_mtx);

	if (__predict_false(status != USBD_NORMAL_COMPLETION)) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;

		DPRINTF(("TX status=%d\n", status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(data->pipe);
		if_statinc(ifp, if_oerrors);
		splx(s);
		return;
	}
	sc->sc_tx_timer = 0;

	/* We just released a Tx buffer, notify Tx. */
	if (ifp->if_flags & IFF_OACTIVE) {
		ifp->if_flags &= ~IFF_OACTIVE;
		rsu_start(ifp);
	}
	splx(s);
}

int
rsu_tx(struct rsu_softc *sc, struct mbuf *m, struct ieee80211_node *ni)
{
	struct ieee80211_frame *wh;
	struct rsu_tx_data *data;
	struct r92s_tx_desc *txd;
	struct usbd_pipe *pipe;
	uint16_t qos;
	uint8_t qid, tid = 0;
	int hasqos, xferlen, error;

	wh = mtod(m, struct ieee80211_frame *);

#ifdef notyet
	if (wh->i_fc[1] & IEEE80211_FC1_PROTECTED) {
		k = ieee80211_get_txkey(ic, wh, ni);
		if ((m = ieee80211_encrypt(ic, m, k)) == NULL)
			return (ENOBUFS);
		wh = mtod(m, struct ieee80211_frame *);
	}
#endif

	if ((hasqos = ieee80211_has_qos(wh))) {
		qos = ieee80211_get_qos(wh);
		tid = qos & IEEE80211_QOS_TID;
		qid = rsu_ac2qid[TID_TO_WME_AC(tid)];
	} else
		qid = RSU_QID_BE;

	/* Get the USB pipe to use for this queue id. */
	pipe = sc->pipe[sc->qid2idx[qid]];

	/* Grab a Tx buffer from our free list. */
	mutex_enter(&sc->sc_tx_mtx);
	data = TAILQ_FIRST(&sc->tx_free_list);
	TAILQ_REMOVE(&sc->tx_free_list, data, next);
	mutex_exit(&sc->sc_tx_mtx);

	/* Fill Tx descriptor. */
	txd = (struct r92s_tx_desc *)data->buf;
	memset(txd, 0, sizeof(*txd));

	txd->txdw0 |= htole32(
	    SM(R92S_TXDW0_PKTLEN, m->m_pkthdr.len) |
	    SM(R92S_TXDW0_OFFSET, sizeof(*txd)) |
	    R92S_TXDW0_OWN | R92S_TXDW0_FSG | R92S_TXDW0_LSG);

	txd->txdw1 |= htole32(
	    SM(R92S_TXDW1_MACID, R92S_MACID_BSS) |
	    SM(R92S_TXDW1_QSEL, R92S_TXDW1_QSEL_BE));
	if (!hasqos)
		txd->txdw1 |= htole32(R92S_TXDW1_NONQOS);
#ifdef notyet
	if (k != NULL) {
		switch (k->k_cipher) {
		case IEEE80211_CIPHER_WEP:
			k->wk_flags |= IEEE80211_KEY_GROUP; /* XXX */
			if (k->wk_keylen == 5)
				mode = RT2860_MODE_WEP40;
			else
				mode = RT2860_MODE_WEP104;
			break;
		case IEEE80211_CIPHER_TKIP:
			cipher = R92S_TXDW1_CIPHER_TKIP;
			break;
		case IEEE80211_CIPHER_AES_CCM:
			cipher = R92S_TXDW1_CIPHER_AES;
			break;
		default:
			cipher = R92S_TXDW1_CIPHER_NONE;
		}
		txd->txdw1 |= htole32(
		    SM(R92S_TXDW1_CIPHER, cipher) |
		    SM(R92S_TXDW1_KEYIDX, k->k_id));
	}
#endif
	txd->txdw2 |= htole32(R92S_TXDW2_BK);
	if (IEEE80211_IS_MULTICAST(wh->i_addr1))
		txd->txdw2 |= htole32(R92S_TXDW2_BMCAST);
	/*
	 * Firmware will use and increment the sequence number for the
	 * specified TID.
	 */
	txd->txdw3 |= htole32(SM(R92S_TXDW3_SEQ, tid));

	xferlen = sizeof(*txd) + m->m_pkthdr.len;
	m_copydata(m, 0, m->m_pkthdr.len, &txd[1]);
	m_freem(m);

	data->pipe = pipe;
	usbd_setup_xfer(data->xfer, data, data->buf, xferlen,
	    USBD_FORCE_SHORT_XFER, RSU_TX_TIMEOUT,
	    rsu_txeof);
	error = usbd_transfer(data->xfer);
	if (__predict_false(error != USBD_IN_PROGRESS && error != 0)) {
		/* Put this Tx buffer back to our free list. */
		mutex_enter(&sc->sc_tx_mtx);
		TAILQ_INSERT_TAIL(&sc->tx_free_list, data, next);
		mutex_exit(&sc->sc_tx_mtx);
		return (error);
	}
	ieee80211_free_node(ni);
	return (0);
}

void
rsu_start(struct ifnet *ifp)
{
	struct rsu_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ether_header *eh;
	struct ieee80211_node *ni;
	struct mbuf *m;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		if (TAILQ_EMPTY(&sc->tx_free_list)) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		if (ic->ic_state != IEEE80211_S_RUN)
			break;

		/* Encapsulate and send data frames. */
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		eh = mtod(m, struct ether_header *);
		ni = ieee80211_find_txnode(ic, eh->ether_dhost);
		if (ni == NULL) {
			m_freem(m);
			if_statinc(ifp, if_oerrors);
			continue;
		}

		if ((m = ieee80211_encap(ic, m, ni)) == NULL) {
			ieee80211_free_node(ni);
			if_statinc(ifp, if_oerrors);
			continue;
		}

		if (rsu_tx(sc, m, ni) != 0) {
			ieee80211_free_node(ni);
			if_statinc(ifp, if_oerrors);
			continue;
		}

		sc->sc_tx_timer = 5;
		ifp->if_timer = 1;
	}
}

void
rsu_watchdog(struct ifnet *ifp)
{
	struct rsu_softc *sc = ifp->if_softc;

	ifp->if_timer = 0;

	if (sc->sc_tx_timer > 0) {
		if (--sc->sc_tx_timer == 0) {
			printf("%s: device timeout\n",
			    device_xname(sc->sc_dev));
			/* rsu_init(ifp); XXX needs a process context! */
			if_statinc(ifp, if_oerrors);
			return;
		}
		ifp->if_timer = 1;
	}
	ieee80211_watchdog(&sc->sc_ic);
}

int
rsu_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct rsu_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		/* FALLTHROUGH */
	case SIOCSIFFLAGS:
		switch (ifp->if_flags & (IFF_UP|IFF_RUNNING)) {
		case IFF_UP|IFF_RUNNING:
			break;
		case IFF_UP:
			rsu_init(ifp);
			break;
		case IFF_RUNNING:
			rsu_stop(ifp);
			break;
		case 0:
			break;
                }
		break;
	default:
		error = ieee80211_ioctl(ic, cmd, data);
	}

	if (error == ENETRESET) {
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
		    (IFF_UP | IFF_RUNNING)) {
			rsu_stop(ifp);
			rsu_init(ifp);
		}
		error = 0;
	}
	splx(s);

	return (error);
}

/*
 * Power on sequence for A-cut adapters.
 */
void
rsu_power_on_acut(struct rsu_softc *sc)
{
	uint32_t reg;

	rsu_write_1(sc, R92S_SPS0_CTRL + 1, 0x53);
	rsu_write_1(sc, R92S_SPS0_CTRL + 0, 0x57);

	/* Enable AFE macro block's bandgap and Mbias. */
	rsu_write_1(sc, R92S_AFE_MISC,
	    rsu_read_1(sc, R92S_AFE_MISC) |
	    R92S_AFE_MISC_BGEN | R92S_AFE_MISC_MBEN);
	/* Enable LDOA15 block. */
	rsu_write_1(sc, R92S_LDOA15_CTRL,
	    rsu_read_1(sc, R92S_LDOA15_CTRL) | R92S_LDA15_EN);

	rsu_write_1(sc, R92S_SPS1_CTRL,
	    rsu_read_1(sc, R92S_SPS1_CTRL) | R92S_SPS1_LDEN);
	usbd_delay_ms(sc->sc_udev, 2);
	/* Enable switch regulator block. */
	rsu_write_1(sc, R92S_SPS1_CTRL,
	    rsu_read_1(sc, R92S_SPS1_CTRL) | R92S_SPS1_SWEN);

	rsu_write_4(sc, R92S_SPS1_CTRL, 0x00a7b267);

	rsu_write_1(sc, R92S_SYS_ISO_CTRL + 1,
	    rsu_read_1(sc, R92S_SYS_ISO_CTRL + 1) | 0x08);

	rsu_write_1(sc, R92S_SYS_FUNC_EN + 1,
	    rsu_read_1(sc, R92S_SYS_FUNC_EN + 1) | 0x20);

	rsu_write_1(sc, R92S_SYS_ISO_CTRL + 1,
	    rsu_read_1(sc, R92S_SYS_ISO_CTRL + 1) & ~0x90);

	/* Enable AFE clock. */
	rsu_write_1(sc, R92S_AFE_XTAL_CTRL + 1,
	    rsu_read_1(sc, R92S_AFE_XTAL_CTRL + 1) & ~0x04);
	/* Enable AFE PLL macro block. */
	rsu_write_1(sc, R92S_AFE_PLL_CTRL,
	    rsu_read_1(sc, R92S_AFE_PLL_CTRL) | 0x11);
	/* Attach AFE PLL to MACTOP/BB. */
	rsu_write_1(sc, R92S_SYS_ISO_CTRL,
	    rsu_read_1(sc, R92S_SYS_ISO_CTRL) & ~0x11);

	/* Switch to 40MHz clock instead of 80MHz. */
	rsu_write_2(sc, R92S_SYS_CLKR,
	    rsu_read_2(sc, R92S_SYS_CLKR) & ~R92S_SYS_CLKSEL);

	/* Enable MAC clock. */
	rsu_write_2(sc, R92S_SYS_CLKR,
	    rsu_read_2(sc, R92S_SYS_CLKR) |
	    R92S_MAC_CLK_EN | R92S_SYS_CLK_EN);

	rsu_write_1(sc, R92S_PMC_FSM, 0x02);

	/* Enable digital core and IOREG R/W. */
	rsu_write_1(sc, R92S_SYS_FUNC_EN + 1,
	    rsu_read_1(sc, R92S_SYS_FUNC_EN + 1) | 0x08);

	rsu_write_1(sc, R92S_SYS_FUNC_EN + 1,
	    rsu_read_1(sc, R92S_SYS_FUNC_EN + 1) | 0x80);

	/* Switch the control path to firmware. */
	reg = rsu_read_2(sc, R92S_SYS_CLKR);
	reg = (reg & ~R92S_SWHW_SEL) | R92S_FWHW_SEL;
	rsu_write_2(sc, R92S_SYS_CLKR, reg);

	rsu_write_2(sc, R92S_CR, 0x37fc);

	/* Fix USB RX FIFO issue. */
	rsu_write_1(sc, 0xfe5c,
	    rsu_read_1(sc, 0xfe5c) | 0x80);
	rsu_write_1(sc, 0x00ab,
	    rsu_read_1(sc, 0x00ab) | 0xc0);

	rsu_write_1(sc, R92S_SYS_CLKR,
	    rsu_read_1(sc, R92S_SYS_CLKR) & ~R92S_SYS_CPU_CLKSEL);
}

/*
 * Power on sequence for B-cut and C-cut adapters.
 */
void
rsu_power_on_bcut(struct rsu_softc *sc)
{
	uint32_t reg;
	int ntries;

	/* Prevent eFuse leakage. */
	rsu_write_1(sc, 0x37, 0xb0);
	usbd_delay_ms(sc->sc_udev, 10);
	rsu_write_1(sc, 0x37, 0x30);

	/* Switch the control path to hardware. */
	reg = rsu_read_2(sc, R92S_SYS_CLKR);
	if (reg & R92S_FWHW_SEL) {
		rsu_write_2(sc, R92S_SYS_CLKR,
		    reg & ~(R92S_SWHW_SEL | R92S_FWHW_SEL));
	}
	rsu_write_1(sc, R92S_SYS_FUNC_EN + 1,
	    rsu_read_1(sc, R92S_SYS_FUNC_EN + 1) & ~0x8c);
	usbd_delay_ms(sc->sc_udev, 10);

	rsu_write_1(sc, R92S_SPS0_CTRL + 1, 0x53);
	rsu_write_1(sc, R92S_SPS0_CTRL + 0, 0x57);

	reg = rsu_read_1(sc, R92S_AFE_MISC);
	rsu_write_1(sc, R92S_AFE_MISC, reg | R92S_AFE_MISC_BGEN);
	rsu_write_1(sc, R92S_AFE_MISC, reg | R92S_AFE_MISC_BGEN |
	    R92S_AFE_MISC_MBEN | R92S_AFE_MISC_I32_EN);

	/* Enable PLL. */
	rsu_write_1(sc, R92S_LDOA15_CTRL,
	    rsu_read_1(sc, R92S_LDOA15_CTRL) | R92S_LDA15_EN);

	rsu_write_1(sc, R92S_LDOV12D_CTRL,
	    rsu_read_1(sc, R92S_LDOV12D_CTRL) | R92S_LDV12_EN);

	rsu_write_1(sc, R92S_SYS_ISO_CTRL + 1,
	    rsu_read_1(sc, R92S_SYS_ISO_CTRL + 1) | 0x08);

	rsu_write_1(sc, R92S_SYS_FUNC_EN + 1,
	    rsu_read_1(sc, R92S_SYS_FUNC_EN + 1) | 0x20);

	/* Support 64KB IMEM. */
	rsu_write_1(sc, R92S_SYS_ISO_CTRL + 1,
	    rsu_read_1(sc, R92S_SYS_ISO_CTRL + 1) & ~0x97);

	/* Enable AFE clock. */
	rsu_write_1(sc, R92S_AFE_XTAL_CTRL + 1,
	    rsu_read_1(sc, R92S_AFE_XTAL_CTRL + 1) & ~0x04);
	/* Enable AFE PLL macro block. */
	reg = rsu_read_1(sc, R92S_AFE_PLL_CTRL);
	rsu_write_1(sc, R92S_AFE_PLL_CTRL, reg | 0x11);
	usbd_delay_ms(sc->sc_udev, 5);
	rsu_write_1(sc, R92S_AFE_PLL_CTRL, reg | 0x51);
	usbd_delay_ms(sc->sc_udev, 5);
	rsu_write_1(sc, R92S_AFE_PLL_CTRL, reg | 0x11);
	usbd_delay_ms(sc->sc_udev, 5);

	/* Attach AFE PLL to MACTOP/BB. */
	rsu_write_1(sc, R92S_SYS_ISO_CTRL,
	    rsu_read_1(sc, R92S_SYS_ISO_CTRL) & ~0x11);

	/* Switch to 40MHz clock. */
	rsu_write_1(sc, R92S_SYS_CLKR, 0x00);
	/* Disable CPU clock and 80MHz SSC. */
	rsu_write_1(sc, R92S_SYS_CLKR,
	    rsu_read_1(sc, R92S_SYS_CLKR) | 0xa0);
	/* Enable MAC clock. */
	rsu_write_2(sc, R92S_SYS_CLKR,
	    rsu_read_2(sc, R92S_SYS_CLKR) |
	    R92S_MAC_CLK_EN | R92S_SYS_CLK_EN);

	rsu_write_1(sc, R92S_PMC_FSM, 0x02);

	/* Enable digital core and IOREG R/W. */
	rsu_write_1(sc, R92S_SYS_FUNC_EN + 1,
	    rsu_read_1(sc, R92S_SYS_FUNC_EN + 1) | 0x08);

	rsu_write_1(sc, R92S_SYS_FUNC_EN + 1,
	    rsu_read_1(sc, R92S_SYS_FUNC_EN + 1) | 0x80);

	/* Switch the control path to firmware. */
	reg = rsu_read_2(sc, R92S_SYS_CLKR);
	reg = (reg & ~R92S_SWHW_SEL) | R92S_FWHW_SEL;
	rsu_write_2(sc, R92S_SYS_CLKR, reg);

	rsu_write_2(sc, R92S_CR, 0x37fc);

	/* Fix USB RX FIFO issue. */
	rsu_write_1(sc, 0xfe5c,
	    rsu_read_1(sc, 0xfe5c) | 0x80);

	rsu_write_1(sc, R92S_SYS_CLKR,
	    rsu_read_1(sc, R92S_SYS_CLKR) & ~R92S_SYS_CPU_CLKSEL);

	rsu_write_1(sc, 0xfe1c, 0x80);

	/* Make sure TxDMA is ready to download firmware. */
	for (ntries = 0; ntries < 20; ntries++) {
		reg = rsu_read_1(sc, R92S_TCR);
		if ((reg & (R92S_TCR_IMEM_CHK_RPT | R92S_TCR_EMEM_CHK_RPT)) ==
		    (R92S_TCR_IMEM_CHK_RPT | R92S_TCR_EMEM_CHK_RPT))
			break;
		DELAY(5);
	}
	if (ntries == 20) {
		/* Reset TxDMA. */
		reg = rsu_read_1(sc, R92S_CR);
		rsu_write_1(sc, R92S_CR, reg & ~R92S_CR_TXDMA_EN);
		DELAY(2);
		rsu_write_1(sc, R92S_CR, reg | R92S_CR_TXDMA_EN);
	}
}

void
rsu_power_off(struct rsu_softc *sc)
{
	/* Turn RF off. */
	rsu_write_1(sc, R92S_RF_CTRL, 0x00);
	usbd_delay_ms(sc->sc_udev, 5);

	/* Turn MAC off. */
	/* Switch control path. */
	rsu_write_1(sc, R92S_SYS_CLKR + 1, 0x38);
	/* Reset MACTOP. */
	rsu_write_1(sc, R92S_SYS_FUNC_EN + 1, 0x70);
	rsu_write_1(sc, R92S_PMC_FSM, 0x06);
	rsu_write_1(sc, R92S_SYS_ISO_CTRL + 0, 0xf9);
	rsu_write_1(sc, R92S_SYS_ISO_CTRL + 1, 0xe8);

	/* Disable AFE PLL. */
	rsu_write_1(sc, R92S_AFE_PLL_CTRL, 0x00);
	/* Disable A15V. */
	rsu_write_1(sc, R92S_LDOA15_CTRL, 0x54);
	/* Disable eFuse 1.2V. */
	rsu_write_1(sc, R92S_SYS_FUNC_EN + 1, 0x50);
	rsu_write_1(sc, R92S_LDOV12D_CTRL, 0x24);
	/* Enable AFE macro block's bandgap and Mbias. */
	rsu_write_1(sc, R92S_AFE_MISC, 0x30);
	/* Disable 1.6V LDO. */
	rsu_write_1(sc, R92S_SPS0_CTRL + 0, 0x56);
	rsu_write_1(sc, R92S_SPS0_CTRL + 1, 0x43);
}

int
rsu_fw_loadsection(struct rsu_softc *sc, uint8_t *buf, int len)
{
	struct rsu_tx_data *data;
	struct r92s_tx_desc *txd;
	/*struct usbd_pipe *pipe;*/
	int mlen, error;

	data = sc->fwcmd_data;
	/*pipe = sc->pipe[sc->qid2idx[RSU_QID_VO]];*/
	txd = (struct r92s_tx_desc *)data->buf;
	while (len > 0) {
		memset(txd, 0, sizeof(*txd));
		if (len <= RSU_TXBUFSZ - sizeof(*txd)) {
			/* Last chunk. */
			txd->txdw0 |= htole32(R92S_TXDW0_LINIP);
			mlen = len;
		} else
			mlen = RSU_TXBUFSZ - sizeof(*txd);
		txd->txdw0 |= htole32(SM(R92S_TXDW0_PKTLEN, mlen));
		memcpy(&txd[1], buf, mlen);

		usbd_setup_xfer(data->xfer, data, data->buf,
		    sizeof(*txd) + mlen,
		    USBD_SHORT_XFER_OK | USBD_SYNCHRONOUS,
		    RSU_TX_TIMEOUT, NULL);
		error = usbd_transfer(data->xfer);
		if (error != 0)
			return (error);
		buf += mlen;
		len -= mlen;
	}
	return (0);
}

int
rsu_load_firmware(struct rsu_softc *sc)
{
	struct r92s_fw_hdr *hdr;
	struct r92s_fw_priv *dmem;
	uint8_t *imem, *emem;
	int imemsz, ememsz;
	u_char *fw;
	size_t size;
	uint32_t reg;
	int ntries, error;
	firmware_handle_t fh;

	if ((error = firmware_open("rsu", "rsu-rtl8712", &fh)) != 0) {
		printf("%s: failed firmware_open of file %s (error %d)\n",
		    device_xname(sc->sc_dev), "rsu-rtl8712", error);
		return error;
	}

	size = firmware_get_size(fh);
	if ((fw = firmware_malloc(size)) == NULL) {
		firmware_close(fh);
		return ENOMEM;
	}

	if ((error = firmware_read(fh, 0, fw, size)) != 0) {
		firmware_close(fh);
		goto fail;
	}

	firmware_close(fh);

	if (size < sizeof(*hdr)) {
		printf("%s: firmware too short\n", device_xname(sc->sc_dev));
		error = EINVAL;
		goto fail;
	}

	hdr = (struct r92s_fw_hdr *)fw;
	if (hdr->signature != htole16(0x8712) &&
	    hdr->signature != htole16(0x8192)) {
		printf("%s: invalid firmware signature 0x%x\n",
		    device_xname(sc->sc_dev), le16toh(hdr->signature));
		error = EINVAL;
		goto fail;
	}
	DPRINTF(("FW V%d %02x-%02x %02x:%02x\n", le16toh(hdr->version),
	    hdr->month, hdr->day, hdr->hour, hdr->minute));

	/* Make sure that driver and firmware are in sync. */
	if (hdr->privsz != htole32(sizeof(*dmem))) {
		printf("%s: unsupported firmware image\n",
		    device_xname(sc->sc_dev));
		error = EINVAL;
		goto fail;
	}
	/* Get FW sections sizes. */
	imemsz = le32toh(hdr->imemsz);
	ememsz = le32toh(hdr->sramsz);
	/* Check that all FW sections fit in image. */
	if (size < sizeof(*hdr) + imemsz + ememsz) {
		printf("%s: firmware too short\n", device_xname(sc->sc_dev));
		error = EINVAL;
		goto fail;
	}
	imem = (uint8_t *)&hdr[1];
	emem = imem + imemsz;

	/* Load IMEM section. */
	error = rsu_fw_loadsection(sc, imem, imemsz);
	if (error != 0) {
		printf("%s: could not load firmware section %s\n",
		    device_xname(sc->sc_dev), "IMEM");
		goto fail;
	}
	/* Wait for load to complete. */
	for (ntries = 0; ntries < 10; ntries++) {
		reg = rsu_read_2(sc, R92S_TCR);
		if (reg & R92S_TCR_IMEM_CODE_DONE)
			break;
		DELAY(10);
	}
	if (ntries == 10 || !(reg & R92S_TCR_IMEM_CHK_RPT)) {
		printf("%s: timeout waiting for %s transfer\n",
		    device_xname(sc->sc_dev), "IMEM");
		error = ETIMEDOUT;
		goto fail;
	}

	/* Load EMEM section. */
	error = rsu_fw_loadsection(sc, emem, ememsz);
	if (error != 0) {
		printf("%s: could not load firmware section %s\n",
		    device_xname(sc->sc_dev), "EMEM");
		goto fail;
	}
	/* Wait for load to complete. */
	for (ntries = 0; ntries < 10; ntries++) {
		reg = rsu_read_2(sc, R92S_TCR);
		if (reg & R92S_TCR_EMEM_CODE_DONE)
			break;
		DELAY(10);
	}
	if (ntries == 10 || !(reg & R92S_TCR_EMEM_CHK_RPT)) {
		printf("%s: timeout waiting for %s transfer\n",
		    device_xname(sc->sc_dev), "EMEM");
		error = ETIMEDOUT;
		goto fail;
	}

	/* Enable CPU. */
	rsu_write_1(sc, R92S_SYS_CLKR,
	    rsu_read_1(sc, R92S_SYS_CLKR) | R92S_SYS_CPU_CLKSEL);
	if (!(rsu_read_1(sc, R92S_SYS_CLKR) & R92S_SYS_CPU_CLKSEL)) {
		printf("%s: could not enable system clock\n",
		    device_xname(sc->sc_dev));
		error = EIO;
		goto fail;
	}
	rsu_write_2(sc, R92S_SYS_FUNC_EN,
	    rsu_read_2(sc, R92S_SYS_FUNC_EN) | R92S_FEN_CPUEN);
	if (!(rsu_read_2(sc, R92S_SYS_FUNC_EN) & R92S_FEN_CPUEN)) {
		printf("%s: could not enable microcontroller\n",
		    device_xname(sc->sc_dev));
		error = EIO;
		goto fail;
	}

	/* Wait for CPU to initialize. */
	for (ntries = 0; ntries < 100; ntries++) {
		if (rsu_read_2(sc, R92S_TCR) & R92S_TCR_IMEM_RDY)
			break;
		usbd_delay_ms(sc->sc_udev, 10);
	}
	if (ntries == 100) {
		printf("%s: timeout waiting for microcontroller\n",
		    device_xname(sc->sc_dev));
		error = ETIMEDOUT;
		goto fail;
	}

	/* Update DMEM section before loading. */
	dmem = &hdr->priv;
	memset(dmem, 0, sizeof(*dmem));
	dmem->hci_sel = R92S_HCI_SEL_USB | R92S_HCI_SEL_8172;
	dmem->nendpoints = sc->npipes;
	dmem->rf_config = 0x12;	/* 1T2R */
	dmem->vcs_type = R92S_VCS_TYPE_AUTO;
	dmem->vcs_mode = R92S_VCS_MODE_RTS_CTS;
#ifdef notyet
	dmem->bw40_en = (ic->ic_htcaps & IEEE80211_HTCAP_CBW20_40) != 0;
#else
	dmem->bw40_en = 0;
#endif
	dmem->turbo_mode = 1;
	/* Load DMEM section. */
	error = rsu_fw_loadsection(sc, (uint8_t *)dmem, sizeof(*dmem));
	if (error != 0) {
		printf("%s: could not load firmware section %s\n",
		    device_xname(sc->sc_dev), "DMEM");
		goto fail;
	}
	/* Wait for load to complete. */
	for (ntries = 0; ntries < 100; ntries++) {
		if (rsu_read_2(sc, R92S_TCR) & R92S_TCR_DMEM_CODE_DONE)
			break;
		DELAY(1000);
	}
	if (ntries == 100) {
		printf("%s: timeout waiting for %s transfer\n",
		    device_xname(sc->sc_dev), "DMEM");
		error = ETIMEDOUT;
		goto fail;
	}
	/* Wait for firmware readiness. */
	for (ntries = 0; ntries < 60; ntries++) {
		if (!(rsu_read_2(sc, R92S_TCR) & R92S_TCR_FWRDY))
			break;
		usbd_delay_ms(sc->sc_udev, 10);
	}
	if (ntries == 60) {
		printf("%s: timeout waiting for firmware readiness\n",
		    device_xname(sc->sc_dev));
		error = ETIMEDOUT;
		goto fail;
	}
 fail:
	firmware_free(fw, size);
	return (error);
}

int
rsu_init(struct ifnet *ifp)
{
	struct rsu_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct r92s_set_pwr_mode cmd;
	struct rsu_rx_data *data;
	int i, error;

	/* Init host async commands ring. */

	mutex_enter(&sc->sc_task_mtx);
	sc->cmdq.cur = sc->cmdq.next = sc->cmdq.queued = 0;
	mutex_exit(&sc->sc_task_mtx);

	/* Allocate Tx/Rx buffers. */
	error = rsu_alloc_rx_list(sc);
	if (error != 0) {
		printf("%s: could not allocate Rx buffers\n",
		    device_xname(sc->sc_dev));
		goto fail;
	}
	error = rsu_alloc_tx_list(sc);
	if (error != 0) {
		printf("%s: could not allocate Tx buffers\n",
		    device_xname(sc->sc_dev));
		goto fail;
	}

	/* Reserve one Tx buffer for firmware commands. */
	mutex_enter(&sc->sc_tx_mtx);
	sc->fwcmd_data = TAILQ_FIRST(&sc->tx_free_list);
	TAILQ_REMOVE(&sc->tx_free_list, sc->fwcmd_data, next);
	mutex_exit(&sc->sc_tx_mtx);

	/* Power on adapter. */
	if (sc->cut == 1)
		rsu_power_on_acut(sc);
	else
		rsu_power_on_bcut(sc);
	/* Load firmware. */
	error = rsu_load_firmware(sc);
	if (error != 0)
		goto fail;

	/* Enable Rx TCP checksum offload. */
	rsu_write_4(sc, R92S_RCR,
	    rsu_read_4(sc, R92S_RCR) | 0x04000000);
	/* Append PHY status. */
	rsu_write_4(sc, R92S_RCR,
	    rsu_read_4(sc, R92S_RCR) | 0x02000000);

	rsu_write_4(sc, R92S_CR,
	    rsu_read_4(sc, R92S_CR) & ~0xff000000);

	/* Use 128 bytes pages. */
	rsu_write_1(sc, 0x00b5,
	    rsu_read_1(sc, 0x00b5) | 0x01);
	/* Enable USB Rx aggregation. */
	rsu_write_1(sc, 0x00bd,
	    rsu_read_1(sc, 0x00bd) | 0x80);
	/* Set USB Rx aggregation threshold. */
	rsu_write_1(sc, 0x00d9, 0x01);
	/* Set USB Rx aggregation timeout (1.7ms/4). */
	rsu_write_1(sc, 0xfe5b, 0x04);
	/* Fix USB Rx FIFO issue. */
	rsu_write_1(sc, 0xfe5c,
	    rsu_read_1(sc, 0xfe5c) | 0x80);

	/* Set MAC address. */
	IEEE80211_ADDR_COPY(ic->ic_myaddr, CLLADDR(ifp->if_sadl));
	rsu_write_region_1(sc, R92S_MACID, ic->ic_myaddr, IEEE80211_ADDR_LEN);		

	/* Queue Rx xfers (XXX C2H pipe for 11-pipe configurations?) */
	for (i = 0; i < RSU_RX_LIST_COUNT; i++) {
		data = &sc->rx_data[i];

		data->pipe = sc->pipe[sc->qid2idx[RSU_QID_RXOFF]];
		usbd_setup_xfer(data->xfer, data, data->buf,
		    RSU_RXBUFSZ, USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT,
		    rsu_rxeof);
		error = usbd_transfer(data->xfer);
		if (error != 0 && error != USBD_IN_PROGRESS)
			goto fail;
	}

	/* NB: it really takes that long for firmware to boot. */
	usbd_delay_ms(sc->sc_udev, 1500);

	DPRINTF(("setting MAC address to %s\n", ether_sprintf(ic->ic_myaddr)));
	error = rsu_fw_cmd(sc, R92S_CMD_SET_MAC_ADDRESS, ic->ic_myaddr,
	    IEEE80211_ADDR_LEN);
	if (error != 0) {
		printf("%s: could not set MAC address\n",
		    device_xname(sc->sc_dev));
		goto fail;
	}

	rsu_write_1(sc, R92S_USB_HRPWM,
	    R92S_USB_HRPWM_PS_ST_ACTIVE | R92S_USB_HRPWM_PS_ALL_ON);

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = R92S_PS_MODE_ACTIVE;
	DPRINTF(("setting ps mode to %d\n", cmd.mode));
	error = rsu_fw_cmd(sc, R92S_CMD_SET_PWR_MODE, &cmd, sizeof(cmd));
	if (error != 0) {
		printf("%s: could not set PS mode\n",
		    device_xname(sc->sc_dev));
		goto fail;
	}

#ifdef notyet
	if (ic->ic_htcaps & IEEE80211_HTCAP_CBW20_40) {
		/* Enable 40MHz mode. */
		error = rsu_fw_iocmd(sc,
		    SM(R92S_IOCMD_CLASS, 0xf4) |
		    SM(R92S_IOCMD_INDEX, 0x00) |
		    SM(R92S_IOCMD_VALUE, 0x0007));
		if (error != 0) {
			printf("%s: could not enable 40MHz mode\n",
			    device_xname(sc->sc_dev));
			goto fail;
		}
	}
#endif

	/* Set default channel. */
	ic->ic_bss->ni_chan = ic->ic_ibss_chan;

	/* We're ready to go. */
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_flags |= IFF_RUNNING;

	sc->scan_pass = 0;
	if (ic->ic_opmode == IEEE80211_M_MONITOR)
		ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
	else
		ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
	return (0);
 fail:
	rsu_stop(ifp);
	return (error);
}

void
rsu_stop(struct ifnet *ifp)
{
	struct rsu_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	int i, s;

	sc->sc_tx_timer = 0;
	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	s = splusb();
	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);
	/* Wait for all async commands to complete. */
	rsu_wait_async(sc);
	splx(s);

	callout_destroy(&sc->calib_to);

	/* Power off adapter. */
	rsu_power_off(sc);

	/* Abort Tx/Rx. */
	for (i = 0; i < sc->npipes; i++)
		usbd_abort_pipe(sc->pipe[i]);

	/* Free Tx/Rx buffers. */
	rsu_free_tx_list(sc);
	rsu_free_rx_list(sc);
}

MODULE(MODULE_CLASS_DRIVER, if_rsu, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
if_rsu_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_rsu,
		    cfattach_ioconf_rsu, cfdata_ioconf_rsu);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_rsu,
		    cfattach_ioconf_rsu, cfdata_ioconf_rsu);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
