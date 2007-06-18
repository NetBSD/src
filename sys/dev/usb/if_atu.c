/*	$NetBSD: if_atu.c,v 1.24.6.4 2007/06/18 13:38:14 itohy Exp $ */
/*	$OpenBSD: if_atu.c,v 1.48 2004/12/30 01:53:21 dlg Exp $ */
/*
 * Copyright (c) 2003, 2004
 *	Daan Vreeken <Danovitsch@Vitsch.net>.  All rights reserved.
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
 *	This product includes software developed by Daan Vreeken.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Daan Vreeken AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Daan Vreeken OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Atmel AT76c503 / AT76c503a / AT76c505 / AT76c505a  USB WLAN driver
 * version 0.5 - 2004-08-03
 *
 * Originally written by Daan Vreeken <Danovitsch @ Vitsch . net>
 *  http://vitsch.net/bsd/atuwi
 *
 * Contributed to by :
 *  Chris Whitehouse, Alistair Phillips, Peter Pilka, Martijn van Buul,
 *  Suihong Liang, Arjan van Leeuwen, Stuart Walsh
 *
 * Ported to OpenBSD by Theo de Raadt and David Gwynne.
 * Ported to NetBSD by Jesse Off
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_atu.c,v 1.24.6.4 2007/06/18 13:38:14 itohy Exp $");

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/queue.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>

#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_ethersubr.h>

#include <dev/microcode/atmel/atmel_intersil_fw.h>
#include <dev/microcode/atmel/atmel_rfmd2958-smc_fw.h>
#include <dev/microcode/atmel/atmel_rfmd2958_fw.h>
#include <dev/microcode/atmel/atmel_rfmd_fw.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_radiotap.h>

#ifdef USB_DEBUG
#define ATU_DEBUG
#endif

#include <dev/usb/if_atureg.h>

#ifdef ATU_DEBUG
#define DPRINTF(x)	do { if (atudebug) printf x; } while (0)
#define DPRINTFN(n,x)	do { if (atudebug>(n)) printf x; } while (0)
int atudebug = 1;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

/*
 * Various supported device vendors/products/radio type.
 */
struct atu_type atu_devs[] = {
	{ USB_VENDOR_ATMEL,	USB_PRODUCT_ATMEL_BW002,
	  RadioRFMD,		ATU_NO_QUIRK },
	{ USB_VENDOR_BELKIN,	USB_PRODUCT_BELKIN_F5D6050,
	  RadioRFMD,		ATU_NO_QUIRK },
	{ USB_VENDOR_ATMEL,	USB_PRODUCT_ATMEL_AT76C503A,
	  RadioIntersil,	ATU_NO_QUIRK },
	{ USB_VENDOR_LEXAR,	USB_PRODUCT_LEXAR_2662WAR,
	  RadioRFMD,		ATU_NO_QUIRK },
	/* Belkin F5D6050 */
	{ USB_VENDOR_SMC3,	USB_PRODUCT_SMC3_2662WUSB,
	  RadioRFMD,		ATU_NO_QUIRK },
	{ USB_VENDOR_LINKSYS2,	USB_PRODUCT_LINKSYS2_WUSB11,
	  RadioRFMD,		ATU_NO_QUIRK },
	{ USB_VENDOR_LINKSYS3,	USB_PRODUCT_LINKSYS3_WUSB11V28,
	  RadioRFMD2958,	ATU_NO_QUIRK },
	{ USB_VENDOR_NETGEAR2,	USB_PRODUCT_NETGEAR2_MA101B,
	  RadioRFMD,		ATU_NO_QUIRK },
	{ USB_VENDOR_ACERP,	USB_PRODUCT_ACERP_AWL400,
	  RadioRFMD,		ATU_NO_QUIRK },
	{ USB_VENDOR_ATMEL,	USB_PRODUCT_ATMEL_WL1130,
	  RadioRFMD2958,	ATU_NO_QUIRK },
	{ USB_VENDOR_LINKSYS3,	USB_PRODUCT_LINKSYS3_WUSB11V28,
	  RadioRFMD2958,	ATU_NO_QUIRK },
	{ USB_VENDOR_AINCOMM,	USB_PRODUCT_AINCOMM_AWU2000B,
	  RadioRFMD2958,	ATU_NO_QUIRK },
	/* SMC2662 V.4 */
	{ USB_VENDOR_ATMEL,	USB_PRODUCT_ATMEL_AT76C505A,
	  RadioRFMD2958_SMC,	ATU_QUIRK_NO_REMAP | ATU_QUIRK_FW_DELAY },
	{ USB_VENDOR_ACERP,	USB_PRODUCT_ACERP_AWL300,
	  RadioIntersil,	ATU_NO_QUIRK },
	{ USB_VENDOR_OQO,	USB_PRODUCT_OQO_WIFI01,
	  RadioRFMD2958_SMC,	ATU_QUIRK_NO_REMAP | ATU_QUIRK_FW_DELAY },
	{ USB_VENDOR_SMC3,	USB_PRODUCT_SMC3_2662WV1,
	  RadioIntersil,	ATU_NO_QUIRK },
};

struct atu_radfirm {
	enum	atu_radio_type atur_type;
	unsigned char	*atur_internal;
	size_t		atur_internal_sz;
	unsigned char	*atur_external;
	size_t		atur_external_sz;
} atu_radfirm[] = {
	{ RadioRFMD,
	  atmel_fw_rfmd_int,		sizeof(atmel_fw_rfmd_int),
	  atmel_fw_rfmd_ext,		sizeof(atmel_fw_rfmd_ext) },
	{ RadioRFMD2958,
	  atmel_fw_rfmd2958_int,	sizeof(atmel_fw_rfmd2958_int),
	  atmel_fw_rfmd2958_ext,	sizeof(atmel_fw_rfmd2958_ext) },
	{ RadioRFMD2958_SMC,
	  atmel_fw_rfmd2958_smc_int,	sizeof(atmel_fw_rfmd2958_smc_int),
	  atmel_fw_rfmd2958_smc_ext,	sizeof(atmel_fw_rfmd2958_smc_ext) },
	{ RadioIntersil,
	  atmel_fw_intersil_int,	sizeof(atmel_fw_intersil_int),
	  atmel_fw_intersil_ext,	sizeof(atmel_fw_intersil_ext) }
};

void	atu_rxeof(usbd_xfer_handle, usbd_private_handle, usbd_status);
void	atu_txeof(usbd_xfer_handle, usbd_private_handle, usbd_status);
void	atu_start(struct ifnet *);
int	atu_ioctl(struct ifnet *, u_long, usb_ioctlarg_t);
int	atu_init(struct ifnet *);
void	atu_stop(struct ifnet *, int);
void	atu_watchdog(struct ifnet *);
usbd_status atu_usb_request(struct atu_softc *sc, u_int8_t type,
	    u_int8_t request, u_int16_t value, u_int16_t index,
	    u_int16_t length, u_int8_t *data);
int	atu_send_command(struct atu_softc *sc, u_int8_t *command, int size);
int	atu_get_cmd_status(struct atu_softc *sc, u_int8_t cmd,
	    u_int8_t *status);
int	atu_wait_completion(struct atu_softc *sc, u_int8_t cmd,
	    u_int8_t *status);
int	atu_send_mib(struct atu_softc *sc, u_int8_t type,
	    u_int8_t size, u_int8_t index, void *data);
int	atu_get_mib(struct atu_softc *sc, u_int8_t type,
	    u_int8_t size, u_int8_t index, u_int8_t *buf);
#if 0
int	atu_start_ibss(struct atu_softc *sc);
#endif
int	atu_start_scan(struct atu_softc *sc);
int	atu_switch_radio(struct atu_softc *sc, int state);
int	atu_initial_config(struct atu_softc *sc);
int	atu_join(struct atu_softc *sc, struct ieee80211_node *node);
int8_t	atu_get_dfu_state(struct atu_softc *sc);
u_int8_t atu_get_opmode(struct atu_softc *sc, u_int8_t *mode);
void	atu_internal_firmware(struct device *);
void	atu_external_firmware(struct device *);
int	atu_get_card_config(struct atu_softc *sc);
int	atu_media_change(struct ifnet *ifp);
void	atu_media_status(struct ifnet *ifp, struct ifmediareq *req);

#ifdef ATU_DEBUG
void	atu_debug_print(struct atu_softc *sc);
#endif

void atu_task(void *);
int atu_newstate(struct ieee80211com *, enum ieee80211_state, int);
int atu_tx_start(struct atu_softc *, struct ieee80211_node *,
    struct ue_chain *, struct mbuf *);
void atu_complete_attach(struct atu_softc *);
u_int8_t atu_calculate_padding(int);

USB_DECLARE_DRIVER(atu);

usbd_status
atu_usb_request(struct atu_softc *sc, u_int8_t type,
    u_int8_t request, u_int16_t value, u_int16_t index, u_int16_t length,
    u_int8_t *data)
{
	usb_device_request_t	req;
	usbd_xfer_handle	xfer;
	usbd_status		err;
	int			total_len = 0, s;

	req.bmRequestType = type;
	req.bRequest = request;
	USETW(req.wValue, value);
	USETW(req.wIndex, index);
	USETW(req.wLength, length);

#ifdef ATU_DEBUG
	if (atudebug) {
		DPRINTFN(20, ("%s: req=%02x val=%02x ind=%02x "
		    "len=%02x\n", USBDEVNAME(sc->atu_dev), request,
		    value, index, length));
	}
#endif /* ATU_DEBUG */

	s = splnet();

	xfer = usbd_alloc_default_xfer(sc->atu_udev);
	usbd_setup_default_xfer(xfer, sc->atu_udev, 0, 500000, &req, data,
	    length, USBD_SHORT_XFER_OK, 0);

	err = usbd_sync_transfer(xfer);

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);

#ifdef ATU_DEBUG
	if (atudebug) {
		if (type & UT_READ) {
			DPRINTFN(20, ("%s: transfered 0x%x bytes in\n",
			    USBDEVNAME(sc->atu_dev), total_len));
		} else {
			if (total_len != length)
				DPRINTF(("%s: wrote only %x bytes\n",
				    USBDEVNAME(sc->atu_dev), total_len));
		}
	}
#endif /* ATU_DEBUG */

	usbd_free_xfer(xfer);

	splx(s);
	return(err);
}

int
atu_send_command(struct atu_softc *sc, u_int8_t *command, int size)
{
	return atu_usb_request(sc, UT_WRITE_VENDOR_DEVICE, 0x0e, 0x0000,
	    0x0000, size, command);
}

int
atu_get_cmd_status(struct atu_softc *sc, u_int8_t cmd, u_int8_t *status)
{
	/*
	 * all other drivers (including Windoze) request 40 bytes of status
	 * and get a short-xfer of just 6 bytes. we can save 34 bytes of
	 * buffer if we just request those 6 bytes in the first place :)
	 */
	/*
	return atu_usb_request(sc, UT_READ_VENDOR_INTERFACE, 0x22, cmd,
	    0x0000, 40, status);
	*/
	return atu_usb_request(sc, UT_READ_VENDOR_INTERFACE, 0x22, cmd,
	    0x0000, 6, status);
}

int
atu_wait_completion(struct atu_softc *sc, u_int8_t cmd, u_int8_t *status)
{
	int			idle_count = 0, err;
	u_int8_t		statusreq[6];

	DPRINTFN(15, ("%s: wait-completion: cmd=%02x\n",
	    USBDEVNAME(sc->atu_dev), cmd));

	while (1) {
		err = atu_get_cmd_status(sc, cmd, statusreq);
		if (err)
			return err;

#ifdef ATU_DEBUG
		if (atudebug) {
			DPRINTFN(20, ("%s: status=%s cmd=%02x\n",
			    USBDEVNAME(sc->atu_dev),
			ether_sprintf(statusreq), cmd));
		}
#endif /* ATU_DEBUG */

		/*
		 * during normal operations waiting on STATUS_IDLE
		 * will never happen more than once
		 */
		if ((statusreq[5] == STATUS_IDLE) && (idle_count++ > 20)) {
			DPRINTF(("%s: idle_count > 20!\n",
			    USBDEVNAME(sc->atu_dev)));
			return 0;
		}

		if ((statusreq[5] != STATUS_IN_PROGRESS) &&
		    (statusreq[5] != STATUS_IDLE)) {
			if (status != NULL)
				*status = statusreq[5];
			return 0;
		}
		usbd_delay_ms(sc->atu_udev, 25);
	}
}

int
atu_send_mib(struct atu_softc *sc, u_int8_t type, u_int8_t size,
    u_int8_t index, void *data)
{
	int				err;
	struct atu_cmd_set_mib		request;

	/*
	 * We don't construct a MIB packet first and then memcpy it into an
	 * Atmel-command-packet, we just construct it the right way at once :)
	 */

	memset(&request, 0, sizeof(request));

	request.AtCmd = CMD_SET_MIB;
	USETW(request.AtSize, size + 4);

	request.MIBType = type;
	request.MIBSize = size;
	request.MIBIndex = index;
	request.MIBReserved = 0;

	/*
	 * For 1 and 2 byte requests we assume a direct value,
	 * everything bigger than 2 bytes we assume a pointer to the data
	 */
	switch (size) {
	case 0:
		break;
	case 1:
		request.data[0]=(long)data & 0x000000ff;
		break;
	case 2:
		request.data[0]=(long)data & 0x000000ff;
		request.data[1]=(long)data >> 8;
		break;
	default:
		memcpy(request.data, data, size);
		break;
	}

	err = atu_usb_request(sc, UT_WRITE_VENDOR_DEVICE, 0x0e, 0x0000,
	    0x0000, size+8, (uByte *)&request);
	if (err)
		return (err);

	DPRINTFN(15, ("%s: sendmib : waitcompletion...\n",
	    USBDEVNAME(sc->atu_dev)));
	return atu_wait_completion(sc, CMD_SET_MIB, NULL);
}

int
atu_get_mib(struct atu_softc *sc, u_int8_t type, u_int8_t size,
    u_int8_t index, u_int8_t *buf)
{

	/* linux/at76c503.c - 478 */
	return atu_usb_request(sc, UT_READ_VENDOR_INTERFACE, 0x033,
	    type << 8, index, size, buf);
}

#if 0
int
atu_start_ibss(struct atu_softc *sc)
{
	struct ieee80211com		*ic = &sc->sc_ic;
	int				err;
	struct atu_cmd_start_ibss	Request;

	Request.Cmd = CMD_START_IBSS;
	Request.Reserved = 0;
	Request.Size = sizeof(Request) - 4;

	memset(Request.BSSID, 0x00, sizeof(Request.BSSID));
	memset(Request.SSID, 0x00, sizeof(Request.SSID));
	memcpy(Request.SSID, ic->ic_des_ssid, ic->ic_des_ssidlen);
	Request.SSIDSize = ic->ic_des_ssidlen;
	if (sc->atu_desired_channel != IEEE80211_CHAN_ANY)
		Request.Channel = (u_int8_t)sc->atu_desired_channel;
	else
		Request.Channel = ATU_DEFAULT_CHANNEL;
	Request.BSSType = AD_HOC_MODE;
	memset(Request.Res, 0x00, sizeof(Request.Res));

	/* Write config to adapter */
	err = atu_send_command(sc, (u_int8_t *)&Request, sizeof(Request));
	if (err) {
		DPRINTF(("%s: start ibss failed!\n",
		    USBDEVNAME(sc->atu_dev)));
		return err;
	}

	/* Wait for the adapter to do it's thing */
	err = atu_wait_completion(sc, CMD_START_IBSS, NULL);
	if (err) {
		DPRINTF(("%s: error waiting for start_ibss\n",
		    USBDEVNAME(sc->atu_dev)));
		return err;
	}

	/* Get the current BSSID */
	err = atu_get_mib(sc, MIB_MAC_MGMT__CURRENT_BSSID, sc->atu_bssid);
	if (err) {
		DPRINTF(("%s: could not get BSSID!\n",
		    USBDEVNAME(sc->atu_dev)));
		return err;
	}

	DPRINTF(("%s: started a new IBSS (BSSID=%s)\n",
	    USBDEVNAME(sc->atu_dev), ether_sprintf(sc->atu_bssid)));
	return 0;
}
#endif

int
atu_start_scan(struct atu_softc *sc)
{
	struct ieee80211com		*ic = &sc->sc_ic;
	struct atu_cmd_do_scan		Scan;
	usbd_status			err;
	int				Cnt;

	memset(&Scan, 0, sizeof(Scan));

	Scan.Cmd = CMD_START_SCAN;
	Scan.Reserved = 0;
	USETW(Scan.Size, sizeof(Scan) - 4);

	/* use the broadcast BSSID (in active scan) */
	for (Cnt=0; Cnt<6; Cnt++)
		Scan.BSSID[Cnt] = 0xff;

	memset(Scan.SSID, 0x00, sizeof(Scan.SSID));
	memcpy(Scan.SSID, ic->ic_des_essid, ic->ic_des_esslen);
	Scan.SSID_Len = ic->ic_des_esslen;

	/* default values for scan */
	Scan.ScanType = ATU_SCAN_ACTIVE;
	if (sc->atu_desired_channel != IEEE80211_CHAN_ANY)
		Scan.Channel = (u_int8_t)sc->atu_desired_channel;
	else
		Scan.Channel = sc->atu_channel;

	ic->ic_curchan = &ic->ic_channels[Scan.Channel];

	/* we like scans to be quick :) */
	/* the time we wait before sending probe's */
	USETW(Scan.ProbeDelay, 0);
	/* the time we stay on one channel */
	USETW(Scan.MinChannelTime, 100);
	USETW(Scan.MaxChannelTime, 200);
	/* wether or not we scan all channels */
	Scan.InternationalScan = 0xc1;

#ifdef ATU_DEBUG
	if (atudebug) {
		DPRINTFN(20, ("%s: scan cmd len=%02lx\n",
		    USBDEVNAME(sc->atu_dev), (unsigned long)sizeof(Scan)));
	}
#endif /* ATU_DEBUG */

	/* Write config to adapter */
	err = atu_send_command(sc, (u_int8_t *)&Scan, sizeof(Scan));
	if (err)
		return err;

	/*
	 * We don't wait for the command to finish... the mgmt-thread will do
	 * that for us
	 */
	/*
	err = atu_wait_completion(sc, CMD_START_SCAN, NULL);
	if (err)
		return err;
	*/
	return 0;
}

int
atu_switch_radio(struct atu_softc *sc, int state)
{
	usbd_status		err;
	struct atu_cmd		CmdRadio;

	if (sc->atu_radio == RadioIntersil) {
		/*
		 * Intersil doesn't seem to need/support switching the radio
		 * on/off
		 */
		return 0;
	}

	memset(&CmdRadio, 0, sizeof(CmdRadio));
	CmdRadio.Cmd = CMD_RADIO_ON;

	if (sc->atu_radio_on != state) {
		if (state == 0)
			CmdRadio.Cmd = CMD_RADIO_OFF;

		err = atu_send_command(sc, (u_int8_t *)&CmdRadio,
		    sizeof(CmdRadio));
		if (err)
			return err;

		err = atu_wait_completion(sc, CmdRadio.Cmd, NULL);
		if (err)
			return err;

		DPRINTFN(10, ("%s: radio turned %s\n",
		    USBDEVNAME(sc->atu_dev), state ? "on" : "off"));
		sc->atu_radio_on = state;
	}
	return 0;
}

int
atu_initial_config(struct atu_softc *sc)
{
	struct ieee80211com		*ic = &sc->sc_ic;
	u_int32_t			i;
	usbd_status			err;
/*	u_int8_t			rates[4] = {0x82, 0x84, 0x8B, 0x96};*/
	u_int8_t			rates[4] = {0x82, 0x04, 0x0B, 0x16};
	struct atu_cmd_card_config	cmd;
	u_int8_t			reg_domain;

	DPRINTFN(10, ("%s: sending mac-addr\n", USBDEVNAME(sc->atu_dev)));
	err = atu_send_mib(sc, MIB_MAC_ADDR__ADDR, ic->ic_myaddr);
	if (err) {
		DPRINTF(("%s: error setting mac-addr\n",
		    USBDEVNAME(sc->atu_dev)));
		return err;
	}

	/*
	DPRINTF(("%s: sending reg-domain\n", USBDEVNAME(sc->atu_dev)));
	err = atu_send_mib(sc, MIB_PHY__REG_DOMAIN, NR(0x30));
	if (err) {
		DPRINTF(("%s: error setting mac-addr\n",
		    USBDEVNAME(sc->atu_dev)));
		return err;
	}
	*/

	memset(&cmd, 0, sizeof(cmd));
	cmd.Cmd = CMD_STARTUP;
	cmd.Reserved = 0;
	USETW(cmd.Size, sizeof(cmd) - 4);

	if (sc->atu_desired_channel != IEEE80211_CHAN_ANY)
		cmd.Channel = (u_int8_t)sc->atu_desired_channel;
	else
		cmd.Channel = sc->atu_channel;
	cmd.AutoRateFallback = 1;
	memcpy(cmd.BasicRateSet, rates, 4);

	/* ShortRetryLimit should be 7 according to 802.11 spec */
	cmd.ShortRetryLimit = 7;
	USETW(cmd.RTS_Threshold, 2347);
	USETW(cmd.FragThreshold, 2346);

	/* Doesn't seem to work, but we'll set it to 1 anyway */
	cmd.PromiscuousMode = 1;

	/* this goes into the beacon we transmit */
	if (ic->ic_flags & IEEE80211_F_PRIVACY)
		cmd.PrivacyInvoked = 1;
	else
		cmd.PrivacyInvoked = 0;

	cmd.ExcludeUnencrypted = 0;

	if (ic->ic_flags & IEEE80211_F_PRIVACY) {
		switch (ic->ic_nw_keys[ic->ic_def_txkey].wk_keylen) {
		case 5:
			cmd.EncryptionType = ATU_WEP_40BITS;
			break;
		case 13:
			cmd.EncryptionType = ATU_WEP_104BITS;
			break;
		default:
			cmd.EncryptionType = ATU_WEP_OFF;
			break;
		}


		cmd.WEP_DefaultKeyID = ic->ic_def_txkey;
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			memcpy(cmd.WEP_DefaultKey[i], ic->ic_nw_keys[i].wk_key, 
			    ic->ic_nw_keys[i].wk_keylen); 
		}
	}

	/* Setting the SSID here doesn't seem to do anything */
	memset(cmd.SSID, 0x00, sizeof(cmd.SSID));
	memcpy(cmd.SSID, ic->ic_des_essid, ic->ic_des_esslen);
	cmd.SSID_Len = ic->ic_des_esslen;

	cmd.ShortPreamble = 0;
	USETW(cmd.BeaconPeriod, 100);
	/* cmd.BeaconPeriod = 65535; */

	/*
	 * TODO:
	 * read reg domain MIB_PHY @ 0x17 (1 byte), (reply = 0x30)
	 * we should do something usefull with this info. right now it's just
	 * ignored
	 */
	err = atu_get_mib(sc, MIB_PHY__REG_DOMAIN, &reg_domain);
	if (err) {
		DPRINTF(("%s: could not get regdomain!\n",
		    USBDEVNAME(sc->atu_dev)));
	} else {
		DPRINTF(("%s: in reg domain 0x%x according to the "
		    "adapter\n", USBDEVNAME(sc->atu_dev), reg_domain));
	}

#ifdef ATU_DEBUG
	if (atudebug) {
		DPRINTFN(20, ("%s: configlen=%02lx\n", USBDEVNAME(sc->atu_dev),
		    (unsigned long)sizeof(cmd)));
	}
#endif /* ATU_DEBUG */

	/* Windoze : driver says exclude-unencrypted=1 & encr-type=1 */

	err = atu_send_command(sc, (u_int8_t *)&cmd, sizeof(cmd));
	if (err)
		return err;
	err = atu_wait_completion(sc, CMD_STARTUP, NULL);
	if (err)
		return err;

	/* Turn on radio now */
	err = atu_switch_radio(sc, 1);
	if (err)
		return err;

	/* preamble type = short */
	err = atu_send_mib(sc, MIB_LOCAL__PREAMBLE, NR(PREAMBLE_SHORT));
	if (err)
		return err;

	/* frag = 1536 */
	err = atu_send_mib(sc, MIB_MAC__FRAG, NR(2346));
	if (err)
		return err;

	/* rts = 1536 */
	err = atu_send_mib(sc, MIB_MAC__RTS, NR(2347));
	if (err)
		return err;

	/* auto rate fallback = 1 */
	err = atu_send_mib(sc, MIB_LOCAL__AUTO_RATE_FALLBACK, NR(1));
	if (err)
		return err;

	/* power mode = full on, no power saving */
	err = atu_send_mib(sc, MIB_MAC_MGMT__POWER_MODE,
	    NR(POWER_MODE_ACTIVE));
	if (err)
		return err;

	DPRINTFN(10, ("%s: completed initial config\n",
	   USBDEVNAME(sc->atu_dev)));
	return 0;
}

int
atu_join(struct atu_softc *sc, struct ieee80211_node *node)
{
	struct atu_cmd_join		join;
	u_int8_t			status = 0;	/* XXX: GCC */
	usbd_status			err;

	memset(&join, 0, sizeof(join));

	join.Cmd = CMD_JOIN;
	join.Reserved = 0x00;
	USETW(join.Size, sizeof(join) - 4);

	DPRINTFN(15, ("%s: pre-join sc->atu_bssid=%s\n",
	    USBDEVNAME(sc->atu_dev), ether_sprintf(sc->atu_bssid)));
	DPRINTFN(15, ("%s: mode=%d\n", USBDEVNAME(sc->atu_dev),
	    sc->atu_mode));
	memcpy(join.bssid, node->ni_bssid, IEEE80211_ADDR_LEN);
	memset(join.essid, 0x00, 32);
	memcpy(join.essid, node->ni_essid, node->ni_esslen);
	join.essid_size = node->ni_esslen;
	if (node->ni_capinfo & IEEE80211_CAPINFO_IBSS)
		join.bss_type = AD_HOC_MODE;
	else
		join.bss_type = INFRASTRUCTURE_MODE;
	join.channel = ieee80211_chan2ieee(&sc->sc_ic, node->ni_chan);

	USETW(join.timeout, ATU_JOIN_TIMEOUT);
	join.reserved = 0x00;

	DPRINTFN(10, ("%s: trying to join BSSID=%s\n",
	    USBDEVNAME(sc->atu_dev), ether_sprintf(join.bssid)));
	err = atu_send_command(sc, (u_int8_t *)&join, sizeof(join));
	if (err) {
		DPRINTF(("%s: ERROR trying to join IBSS\n",
		    USBDEVNAME(sc->atu_dev)));
		return err;
	}
	err = atu_wait_completion(sc, CMD_JOIN, &status);
	if (err) {
		DPRINTF(("%s: error joining BSS!\n",
		    USBDEVNAME(sc->atu_dev)));
		return err;
	}
	if (status != STATUS_COMPLETE) {
		DPRINTF(("%s: error joining... [status=%02x]\n",
		    USBDEVNAME(sc->atu_dev), status));
		return status;
	} else {
		DPRINTFN(10, ("%s: joined BSS\n", USBDEVNAME(sc->atu_dev)));
	}
	return err;
}

/*
 * Get the state of the DFU unit
 */
int8_t
atu_get_dfu_state(struct atu_softc *sc)
{
	u_int8_t	state;

	if (atu_usb_request(sc, DFU_GETSTATE, 0, 0, 1, &state))
		return -1;
	return state;
}

/*
 * Get MAC opmode
 */
u_int8_t
atu_get_opmode(struct atu_softc *sc, u_int8_t *mode)
{

	return atu_usb_request(sc, UT_READ_VENDOR_INTERFACE, 0x33, 0x0001,
	    0x0000, 1, mode);
}

/*
 * Upload the internal firmware into the device
 */
void
atu_internal_firmware(struct device *arg)
{
	struct atu_softc *sc = (struct atu_softc *)arg;
	u_char	state, *ptr = NULL, *firm = NULL, status[6];
	int block_size, block = 0, err, i;
	size_t	bytes_left = 0;

	/*
	 * Uploading firmware is done with the DFU (Device Firmware Upgrade)
	 * interface. See "Universal Serial Bus - Device Class Specification
	 * for Device Firmware Upgrade" pdf for details of the protocol.
	 * Maybe this could be moved to a separate 'firmware driver' once more
	 * device drivers need it... For now we'll just do it here.
	 *
	 * Just for your information, the Atmel's DFU descriptor looks like
	 * this:
	 *
	 * 07		size
	 * 21		type
	 * 01		capabilities : only firmware download, need reset
	 *		  after download
	 * 13 05	detach timeout : max 1299ms between DFU_DETACH and
	 *		  reset
	 * 00 04	max bytes of firmware per transaction : 1024
	 */

	/* Choose the right firmware for the device */
	for (i = 0; i < sizeof(atu_radfirm)/sizeof(atu_radfirm[0]); i++)
		if (sc->atu_radio == atu_radfirm[i].atur_type) {
			firm = atu_radfirm[i].atur_internal;
			bytes_left = atu_radfirm[i].atur_internal_sz;
		}

	if (firm == NULL) {
		printf("%s: no firmware found\n", USBDEVNAME(sc->atu_dev));
		return;
	}

	ptr = firm;
	state = atu_get_dfu_state(sc);

	while (block >= 0 && state > 0) {
		switch (state) {
		case DFUState_DnLoadSync:
			/* get DFU status */
			err = atu_usb_request(sc, DFU_GETSTATUS, 0, 0 , 6,
			    status);
			if (err) {
				DPRINTF(("%s: dfu_getstatus failed!\n",
				    USBDEVNAME(sc->atu_dev)));
				return;
			}
			/* success means state => DnLoadIdle */
			state = DFUState_DnLoadIdle;
			continue;
			break;

		case DFUState_DFUIdle:
		case DFUState_DnLoadIdle:
			if (bytes_left>=DFU_MaxBlockSize)
				block_size = DFU_MaxBlockSize;
			else
				block_size = bytes_left;
			DPRINTFN(15, ("%s: firmware block %d\n",
			    USBDEVNAME(sc->atu_dev), block));

			err = atu_usb_request(sc, DFU_DNLOAD, block++, 0,
			    block_size, ptr);
			if (err) {
				DPRINTF(("%s: dfu_dnload failed\n",
				    USBDEVNAME(sc->atu_dev)));
				return;
			}

			ptr += block_size;
			bytes_left -= block_size;
			if (block_size == 0)
				block = -1;
			break;

		default:
			usbd_delay_ms(sc->atu_udev, 100);
			DPRINTFN(20, ("%s: sleeping for a while\n",
			    USBDEVNAME(sc->atu_dev)));
			break;
		}

		state = atu_get_dfu_state(sc);
	}

	if (state != DFUState_ManifestSync) {
		DPRINTF(("%s: state != manifestsync... eek!\n",
		    USBDEVNAME(sc->atu_dev)));
	}

	err = atu_usb_request(sc, DFU_GETSTATUS, 0, 0, 6, status);
	if (err) {
		DPRINTF(("%s: dfu_getstatus failed!\n",
		    USBDEVNAME(sc->atu_dev)));
		return;
	}

	DPRINTFN(15, ("%s: sending remap\n", USBDEVNAME(sc->atu_dev)));
	err = atu_usb_request(sc, DFU_REMAP, 0, 0, 0, NULL);
	if ((err) && (! sc->atu_quirk & ATU_QUIRK_NO_REMAP)) {
		DPRINTF(("%s: remap failed!\n", USBDEVNAME(sc->atu_dev)));
		return;
	}

	/* after a lot of trying and measuring I found out the device needs
	 * about 56 miliseconds after sending the remap command before
	 * it's ready to communicate again. So we'll wait just a little bit
	 * longer than that to be sure...
	 */
	usbd_delay_ms(sc->atu_udev, 56+100);

	printf("%s: reattaching after firmware upload\n",
	    USBDEVNAME(sc->atu_dev));
	usb_needs_reattach(sc->atu_udev);
}

void
atu_external_firmware(struct device *arg)
{
	struct atu_softc *sc = (struct atu_softc *)arg;
	u_char	*ptr = NULL, *firm = NULL;
	int	block_size, block = 0, err, i;
	size_t	bytes_left = 0;

	for (i = 0; i < sizeof(atu_radfirm)/sizeof(atu_radfirm[0]); i++)
		if (sc->atu_radio == atu_radfirm[i].atur_type) {
			firm = atu_radfirm[i].atur_external;
			bytes_left = atu_radfirm[i].atur_external_sz;
		}

	if (firm == NULL) {
		printf("%s: no firmware found\n", USBDEVNAME(sc->atu_dev));
		return;
	}
	ptr = firm;

	while (bytes_left) {
		if (bytes_left > 1024)
			block_size = 1024;
		else
			block_size = bytes_left;

		DPRINTFN(15, ("%s: block:%d size:%d\n",
		    USBDEVNAME(sc->atu_dev), block, block_size));
		err = atu_usb_request(sc, UT_WRITE_VENDOR_DEVICE, 0x0e,
		    0x0802, block, block_size, ptr);
		if (err) {
			DPRINTF(("%s: could not load external firmware "
			    "block\n", USBDEVNAME(sc->atu_dev)));
			return;
		}

		ptr += block_size;
		block++;
		bytes_left -= block_size;
	}

	err = atu_usb_request(sc, UT_WRITE_VENDOR_DEVICE, 0x0e, 0x0802,
	    block, 0, NULL);
	if (err) {
		DPRINTF(("%s: could not load last zero-length firmware "
		    "block\n", USBDEVNAME(sc->atu_dev)));
		return;
	}

	/*
	 * The SMC2662w V.4 seems to require some time to do it's thing with
	 * the external firmware... 20 ms isn't enough, but 21 ms works 100
	 * times out of 100 tries. We'll wait a bit longer just to be sure
	 */
	if (sc->atu_quirk & ATU_QUIRK_FW_DELAY)
		usbd_delay_ms(sc->atu_udev, 21 + 100);

	DPRINTFN(10, ("%s: external firmware upload done\n",
	    USBDEVNAME(sc->atu_dev)));
	/* complete configuration after the firmwares have been uploaded */
	atu_complete_attach(sc);
}

int
atu_get_card_config(struct atu_softc *sc)
{
	struct ieee80211com		*ic = &sc->sc_ic;
	struct atu_rfmd_conf		rfmd_conf;
	struct atu_intersil_conf	intersil_conf;
	int				err;

	switch (sc->atu_radio) {

	case RadioRFMD:
	case RadioRFMD2958:
	case RadioRFMD2958_SMC:
		err = atu_usb_request(sc, UT_READ_VENDOR_INTERFACE, 0x33,
		    0x0a02, 0x0000, sizeof(rfmd_conf),
		    (u_int8_t *)&rfmd_conf);
		if (err) {
			DPRINTF(("%s: could not get rfmd config!\n",
			    USBDEVNAME(sc->atu_dev)));
			return err;
		}
		memcpy(ic->ic_myaddr, rfmd_conf.MACAddr, IEEE80211_ADDR_LEN);
		break;

	case RadioIntersil:
		err = atu_usb_request(sc, UT_READ_VENDOR_INTERFACE, 0x33,
		    0x0902, 0x0000, sizeof(intersil_conf),
		    (u_int8_t *)&intersil_conf);
		if (err) {
			DPRINTF(("%s: could not get intersil config!\n",
			    USBDEVNAME(sc->atu_dev)));
			return err;
		}
		memcpy(ic->ic_myaddr, intersil_conf.MACAddr,
		    IEEE80211_ADDR_LEN);
		break;
	}
	return 0;
}

/*
 * Probe for an AT76c503 chip.
 */
USB_MATCH(atu)
{
	USB_MATCH_START(atu, uaa);
	int			i;

#ifndef USB_USE_IFATTACH
	if (!uaa->iface)
		return(UMATCH_NONE);
#endif /* USB_USE_IFATTACH */

	for (i = 0; i < sizeof(atu_devs)/sizeof(atu_devs[0]); i++) {
		struct atu_type *t = &atu_devs[i];

		if (uaa->vendor == t->atu_vid &&
		    uaa->product == t->atu_pid) {
			return(UMATCH_VENDOR_PRODUCT);
		}
	}
	return(UMATCH_NONE);
}

int
atu_media_change(struct ifnet *ifp)
{
	struct atu_softc	*sc = ifp->if_softc;
	struct ieee80211com	*ic = &sc->sc_ic;
	int			err, s;

	DPRINTFN(10, ("%s: atu_media_change\n", USBDEVNAME(sc->atu_dev)));

	err = ieee80211_media_change(ifp);
	if (err == ENETRESET) {
		if ((ifp->if_flags & (IFF_RUNNING|IFF_UP)) ==
		    (IFF_RUNNING|IFF_UP)) {
			s = splnet();
			ieee80211_new_state(ic, IEEE80211_S_INIT, -1);
			atu_initial_config(sc);
			ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
			splx(s);
		}
		err = 0;
	}

	return (err);
}

void
atu_media_status(struct ifnet *ifp, struct ifmediareq *req)
{
#ifdef ATU_DEBUG
	struct atu_softc	*sc = ifp->if_softc;
#endif /* ATU_DEBUG */

	DPRINTFN(10, ("%s: atu_media_status\n", USBDEVNAME(sc->atu_dev)));

	ieee80211_media_status(ifp, req);
}

void
atu_task(void *arg)
{
	struct atu_softc	*sc = (struct atu_softc *)arg;
	struct ieee80211com	*ic = &sc->sc_ic;
	usbd_status		err;
	int			s;

	DPRINTFN(10, ("%s: atu_task\n", USBDEVNAME(sc->atu_dev)));

	if (sc->sc_state != ATU_S_OK)
		return;

	switch (sc->sc_cmd) {
	case ATU_C_SCAN:

		err = atu_start_scan(sc);
		if (err) {
			DPRINTFN(1, ("%s: atu_task: couldn't start scan!\n",
			    USBDEVNAME(sc->atu_dev)));
			return;
		}

		err = atu_wait_completion(sc, CMD_START_SCAN, NULL);
		if (err) {
			DPRINTF(("%s: atu_task: error waiting for scan\n",
			    USBDEVNAME(sc->atu_dev)));
			return;
		}

		DPRINTF(("%s: ==========================> END OF SCAN!\n",
		    USBDEVNAME(sc->atu_dev)));

		s = splnet();
		ieee80211_next_scan(ic);
		splx(s);

		DPRINTF(("%s: ----------------------======> END OF SCAN2!\n",
		    USBDEVNAME(sc->atu_dev)));
		break;

	case ATU_C_JOIN:
		atu_join(sc, ic->ic_bss);
	}
}

int
atu_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct ifnet		*ifp = ic->ic_ifp;
	struct atu_softc	*sc = ifp->if_softc;
	enum ieee80211_state	ostate = ic->ic_state;

	DPRINTFN(10, ("%s: atu_newstate: %s -> %s\n", USBDEVNAME(sc->atu_dev),
	    ieee80211_state_name[ostate], ieee80211_state_name[nstate]));

	switch (nstate) {
	case IEEE80211_S_SCAN:
		memcpy(ic->ic_chan_scan, ic->ic_chan_active,
		    sizeof(ic->ic_chan_active));
		ieee80211_node_table_reset(&ic->ic_scan);

		/* tell the event thread that we want a scan */
		sc->sc_cmd = ATU_C_SCAN;
		usb_add_task(sc->atu_udev, &sc->sc_task, USB_TASKQ_DRIVER);

		/* handle this ourselves */
		ic->ic_state = nstate;
		return (0);

	case IEEE80211_S_AUTH:
	case IEEE80211_S_RUN:
		if (ostate == IEEE80211_S_SCAN) {
			sc->sc_cmd = ATU_C_JOIN;
			usb_add_task(sc->atu_udev, &sc->sc_task,
			    USB_TASKQ_DRIVER);
		}
		break;
	default:
		/* nothing to do */
		break;
	}

	return (*sc->sc_newstate)(ic, nstate, arg);
}

/*
 * Attach the interface. Allocate softc structures, do
 * setup and ethernet/BPF attach.
 */
USB_ATTACH(atu)
{
	USB_ATTACH_START(atu, sc, uaa);
	char				*devinfop;
	usbd_status			err;
	usbd_device_handle		dev = uaa->device;
	u_int8_t			mode, channel;
	int i;

	sc->sc_state = ATU_S_UNCONFIG;

	devinfop = usbd_devinfo_alloc(dev, 0);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", USBDEVNAME(sc->atu_dev), devinfop);
	usbd_devinfo_free(devinfop);

	err = usbd_set_config_no(dev, ATU_CONFIG_NO, 1);
	if (err) {
		printf("%s: setting config no failed\n",
		    USBDEVNAME(sc->atu_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	err = usbd_device2interface_handle(dev, ATU_IFACE_IDX, &sc->atu_iface);
	if (err) {
		printf("%s: getting interface handle failed\n",
		    USBDEVNAME(sc->atu_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	sc->atu_unit = device_unit(self);
	sc->atu_udev = dev;

	/*
	 * look up the radio_type for the device
	 * basically does the same as USB_MATCH
	 */
	for (i = 0; i < sizeof(atu_devs)/sizeof(atu_devs[0]); i++) {
		struct atu_type *t = &atu_devs[i];

		if (uaa->vendor == t->atu_vid &&
		    uaa->product == t->atu_pid) {
			sc->atu_radio = t->atu_radio;
			sc->atu_quirk = t->atu_quirk;
		}
	}

	/*
	 * Check in the interface descriptor if we're in DFU mode
	 * If we're in DFU mode, we upload the external firmware
	 * If we're not, the PC must have rebooted without power-cycling
	 * the device.. I've tried this out, a reboot only requeres the
	 * external firmware to be reloaded :)
	 *
	 * Hmm. The at76c505a doesn't report a DFU descriptor when it's
	 * in DFU mode... Let's just try to get the opmode
	 */
	err = atu_get_opmode(sc, &mode);
	DPRINTFN(20, ("%s: opmode: %d\n", USBDEVNAME(sc->atu_dev), mode));
	if (err || (mode != MODE_NETCARD && mode != MODE_NOFLASHNETCARD)) {
		DPRINTF(("%s: starting internal firmware download\n",
		    USBDEVNAME(sc->atu_dev)));

		atu_internal_firmware((struct device *)sc);
		/*
		 * atu_internal_firmware will cause a reset of the device
		 * so we don't want to do any more configuration after this
		 * point.
		 */
		USB_ATTACH_SUCCESS_RETURN;
	}

#ifndef USB_USE_IFATTACH
	uaa->iface = sc->atu_iface;
#endif /* USB_USE_IFATTACH */

	if (mode != MODE_NETCARD) {
		DPRINTFN(15, ("%s: device needs external firmware\n",
		    USBDEVNAME(sc->atu_dev)));

		if (mode != MODE_NOFLASHNETCARD) {
			DPRINTF(("%s: unexpected opmode=%d\n",
			    USBDEVNAME(sc->atu_dev), mode));
		}

		/*
		 * There is no difference in opmode before and after external
		 * firmware upload with the SMC2662 V.4 . So instead we'll try
		 * to read the channel number. If we succeed, external
		 * firmwaremust have been already uploaded...
		 */
		if (sc->atu_radio != RadioIntersil) {
			err = atu_get_mib(sc, MIB_PHY__CHANNEL, &channel);
			if (!err) {
				DPRINTF(("%s: external firmware has already"
				    " been downloaded\n",
				    USBDEVNAME(sc->atu_dev)));
				atu_complete_attach(sc);
				USB_ATTACH_SUCCESS_RETURN;
			}
		}

		atu_external_firmware((struct device *)sc);

		/*
		 * atu_external_firmware will call atu_complete_attach after
		 * it's finished so we can just return.
		 */
	} else {
		/* all the firmwares are in place, so complete the attach */
		atu_complete_attach(sc);
	}

	USB_ATTACH_SUCCESS_RETURN;
}

void
atu_complete_attach(struct atu_softc *sc)
{
	struct ieee80211com		*ic = &sc->sc_ic;
	struct ifnet			*ifp = &sc->sc_if;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	usbd_status			err;
	int				i;
#ifdef ATU_DEBUG
	struct atu_fw			fw;
#endif

	id = usbd_get_interface_descriptor(sc->atu_iface);

	/* Find endpoints. */
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->atu_iface, i);
		if (!ed) {
			DPRINTF(("%s: num_endp:%d\n", USBDEVNAME(sc->atu_dev),
			    sc->atu_iface->idesc->bNumEndpoints));
			DPRINTF(("%s: couldn't get ep %d\n",
			    USBDEVNAME(sc->atu_dev), i));
			return;
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->atu_ed[ATU_ENDPT_RX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->atu_ed[ATU_ENDPT_TX] = ed->bEndpointAddress;
		}
	}

	/* read device config & get MAC address */
	err = atu_get_card_config(sc);
	if (err) {
		printf("\n%s: could not get card cfg!\n",
		    USBDEVNAME(sc->atu_dev));
		return;
	}

#ifdef ATU_DEBUG
	/* DEBUG : try to get firmware version */
	err = atu_get_mib(sc, MIB_FW_VERSION, sizeof(fw), 0,
	    (u_int8_t *)&fw);
	if (!err) {
		DPRINTFN(15, ("%s: firmware: maj:%d min:%d patch:%d "
		    "build:%d\n", USBDEVNAME(sc->atu_dev), fw.major, fw.minor,
		    fw.patch, fw.build));
	} else {
		DPRINTF(("%s: get firmware version failed\n",
		    USBDEVNAME(sc->atu_dev)));
	}
#endif /* ATU_DEBUG */

	/* Show the world our MAC address */
	printf("%s: MAC address %s\n", USBDEVNAME(sc->atu_dev),
	    ether_sprintf(ic->ic_myaddr));

	sc->atu_cdata.atu_tx_inuse = 0;
	sc->atu_encrypt = ATU_WEP_OFF;
	sc->atu_wepkeylen = ATU_WEP_104BITS;
	sc->atu_wepkey = 0;

	bzero(sc->atu_bssid, ETHER_ADDR_LEN);
	sc->atu_channel = ATU_DEFAULT_CHANNEL;
	sc->atu_desired_channel = IEEE80211_CHAN_ANY;
	sc->atu_mode = INFRASTRUCTURE_MODE;

	ic->ic_ifp = ifp;
	ic->ic_phytype = IEEE80211_T_DS;
	ic->ic_opmode = IEEE80211_M_STA;
	ic->ic_state = IEEE80211_S_INIT;
#ifdef FIXME
	ic->ic_caps = IEEE80211_C_IBSS | IEEE80211_C_WEP | IEEE80211_C_SCANALL;
#else
	ic->ic_caps = IEEE80211_C_IBSS | IEEE80211_C_WEP;
#endif

	i = 0;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates[i++] = 2;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates[i++] = 4;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates[i++] = 11;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates[i++] = 22;
	ic->ic_sup_rates[IEEE80211_MODE_11B].rs_nrates = i;

	for (i = 1; i <= 14; i++) {
		ic->ic_channels[i].ic_flags = IEEE80211_CHAN_B |
		    IEEE80211_CHAN_PASSIVE;
		ic->ic_channels[i].ic_freq = ieee80211_ieee2mhz(i,
		    ic->ic_channels[i].ic_flags);
	}

	ic->ic_ibss_chan = &ic->ic_channels[0];

	ifp->if_softc = sc;
	memcpy(ifp->if_xname, USBDEVNAME(sc->atu_dev), IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = atu_init;
	ifp->if_stop = atu_stop;
	ifp->if_start = atu_start;
	ifp->if_ioctl = atu_ioctl;
	ifp->if_watchdog = atu_watchdog;
	ifp->if_mtu = ATU_DEFAULT_MTU;
	IFQ_SET_READY(&ifp->if_snd);

	/* Call MI attach routine. */
	if_attach(ifp);
	ieee80211_ifattach(ic);

	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = atu_newstate;

	/* setup ifmedia interface */
	ieee80211_media_init(ic, atu_media_change, atu_media_status);

	usb_init_task(&sc->sc_task, atu_task, sc);

	sc->sc_state = ATU_S_OK;
}

USB_DETACH(atu)
{
	USB_DETACH_START(atu, sc);
	struct ifnet		*ifp = &sc->sc_if;

	DPRINTFN(10, ("%s: atu_detach state=%d\n", USBDEVNAME(sc->atu_dev),
	    sc->sc_state));

	if (sc->sc_state != ATU_S_UNCONFIG) {
		atu_stop(ifp, 1);

		ieee80211_ifdetach(&sc->sc_ic);
		if_detach(ifp);
	}

	return(0);
}

int
atu_activate(device_ptr_t self, enum devact act)
{
	struct atu_softc *sc = (struct atu_softc *)self;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;
	case DVACT_DEACTIVATE:
		if (sc->sc_state != ATU_S_UNCONFIG) {
			if_deactivate(&sc->atu_ec.ec_if);
			sc->sc_state = ATU_S_DEAD;
		}
		break;
	}
	return (0);
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
void
atu_rxeof(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct ue_chain		*c = priv;
	struct atu_softc	*sc = (void *)c->ue_dev;
	struct ieee80211com	*ic = &sc->sc_ic;
	struct ifnet		*ifp = &sc->sc_if;
	struct atu_rx_hdr	*h;
	struct ieee80211_frame_min	*wh;
	struct ieee80211_node	*ni;
	struct mbuf		*m;
	u_int32_t		len;
	int			s;
	int			rssi;
	u_int32_t		rx_time;

	DPRINTFN(25, ("%s: atu_rxeof\n", USBDEVNAME(sc->atu_dev)));

	if (sc->sc_state != ATU_S_OK)
		return;

	usbd_unmap_buffer(xfer);

	if ((ifp->if_flags & (IFF_RUNNING|IFF_UP)) != (IFF_RUNNING|IFF_UP))
		goto done;

	if (status != USBD_NORMAL_COMPLETION) {
		DPRINTF(("%s: status != USBD_NORMAL_COMPLETION\n",
		    USBDEVNAME(sc->atu_dev)));
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			return;
		}
#if 0
		if (status == USBD_IOERROR) {
			DPRINTF(("%s: rx: EEK! lost device?\n",
			    USBDEVNAME(sc->atu_dev)));

			/*
			 * My experience with USBD_IOERROR is that trying to
			 * restart the transfer will always fail and we'll
			 * keep on looping restarting transfers untill someone
			 * pulls the plug of the device.
			 * So we don't restart the transfer, but just let it
			 * die... If someone knows of a situation where we can
			 * recover from USBD_IOERROR, let me know.
			 */
			splx(s);
			return;
		}
#endif /* 0 */

		if (usbd_ratecheck(&sc->atu_rx_notice)) {
			DPRINTF(("%s: usb error on rx: %s\n",
			    USBDEVNAME(sc->atu_dev), usbd_errstr(status)));
		}
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(
			    sc->atu_ep[ATU_ENDPT_RX]);
		goto done;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &len, NULL);

	if (len <= 1) {
		DPRINTF(("%s: atu_rxeof: too short\n",
		    USBDEVNAME(sc->atu_dev)));
		goto done;
	}

	m = c->ue_mbuf;

	/*
	 * Allocate new mbuf cluster for the next transfer.
	 * If that failed, discard current packet and recycle the mbuf.
	 */
	if ((c->ue_mbuf = usb_ether_newbuf(NULL)) == NULL) {
		printf("%s: no memory for rx list -- packet dropped!\n",
		    USBDEVNAME(sc->atu_dev));
		ifp->if_ierrors++;
		c->ue_mbuf = usb_ether_newbuf(m);
		goto done;
	}

	h = mtod(m, struct atu_rx_hdr *);
	len = UGETW(h->length) + ATU_RX_HDRLEN - 4; /* XXX magic number */
	rssi = h->rssi;
	rx_time = UGETDW(h->rx_time);

	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len = len;
	m_adj(m, ATU_RX_HDRLEN);	/* strip header */

	wh = mtod(m, struct ieee80211_frame_min *);
	ni = ieee80211_find_rxnode(ic, wh);

	ifp->if_ipackets++;

	s = splnet();

	if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
		/*
		 * WEP is decrypted by hardware. Clear WEP bit
		 * header for ieee80211_input().
		 */
		wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
	}

	ieee80211_input(ic, m, ni, rssi, rx_time);

	ieee80211_free_node(ni);
	splx(s);
done:
	/* Setup new transfer. */
	(void)usbd_map_buffer_mbuf(c->ue_xfer, c->ue_mbuf);
	usbd_setup_xfer(c->ue_xfer, sc->atu_ep[ATU_ENDPT_RX], c, NULL /* XXX buf */,
	    ATU_RX_BUFSZ, USBD_SHORT_XFER_OK | USBD_NO_COPY, USBD_NO_TIMEOUT,
		atu_rxeof);
	usbd_transfer(c->ue_xfer);
}

/*
 * A frame was downloaded to the chip. It's safe for us to clean up
 * the list buffers.
 */
void
atu_txeof(usbd_xfer_handle xfer, usbd_private_handle priv,
    usbd_status status)
{
	struct ue_chain		*c = priv;
	struct atu_softc	*sc = (void *)c->ue_dev;
	struct ifnet		*ifp = &sc->sc_if;
	usbd_status		err;
	int			s;

	DPRINTFN(25, ("%s: atu_txeof status=%d\n", USBDEVNAME(sc->atu_dev),
	    status));

	if (c->ue_mbuf) {
		usbd_unmap_buffer(xfer);
		m_freem(c->ue_mbuf);
		c->ue_mbuf = NULL;
	}

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;

		DPRINTF(("%s: usb error on tx: %s\n", USBDEVNAME(sc->atu_dev),
		    usbd_errstr(status)));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->atu_ep[ATU_ENDPT_TX]);
		return;
	}

	usbd_get_xfer_status(c->ue_xfer, NULL, NULL, NULL, &err);

	if (err)
		ifp->if_oerrors++;
	else
		ifp->if_opackets++;

	s = splnet();
	SLIST_INSERT_HEAD(&sc->atu_cdata.atu_tx_free, c, ue_next);
	sc->atu_cdata.atu_tx_inuse--;
	if (sc->atu_cdata.atu_tx_inuse == 0)
		ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;
	splx(s);

	atu_start(ifp);
}

u_int8_t
atu_calculate_padding(int size)
{
	size %= 64;

	if (size < 50)
		return (50 - size);
	if (size >=61)
		return (64 + 50 - size);
	return (0);
}

int
atu_tx_start(struct atu_softc *sc, struct ieee80211_node *ni,
    struct ue_chain *c, struct mbuf *m)
{
	int			len;
	struct atu_tx_hdr	*h;
	usbd_status		err;
	u_int8_t		pad;
	int			ret;

	DPRINTFN(25, ("%s: atu_tx_start\n", USBDEVNAME(sc->atu_dev)));

	/* Don't try to send when we're shutting down the driver */
	if (sc->sc_state != ATU_S_OK) {
		m_freem(m);
		return(EIO);
	}

	len = m->m_pkthdr.len;

	/* Prepend atmel headers */
	M_PREPEND(m, ATU_TX_HDRLEN, M_DONTWAIT);
	if (m != NULL)
		m = m_pullup(m, ATU_TX_HDRLEN);	/* just in case */
	if (m == NULL) {
		sc->sc_if.if_oerrors++;
		return (ENOBUFS);
	}

	h = mtod(m, struct atu_tx_hdr *);
	memset(h, 0, ATU_TX_HDRLEN);
	USETW(h->length, len);
	h->tx_rate = 4; /* XXX rate = auto */
	len += ATU_TX_HDRLEN;

	pad = atu_calculate_padding(len);
	len += pad;
	h->padding = pad;

	if (pad > 0) {
		m_copyback(m, len - 1, 1, "");	/* expand mbuf chain */
		if (m->m_pkthdr.len != len) {
			m_freem(m);
			return (ENOBUFS);
		}
	}

	ret = usb_ether_map_tx_buffer_mbuf(c, m);
	if (ret) {
		m_freem(m);
		return (ret);
	}

	usbd_setup_xfer(c->ue_xfer, sc->atu_ep[ATU_ENDPT_TX],
	    c, NULL /* XXX buf */, len, USBD_NO_COPY, ATU_TX_TIMEOUT,
	    atu_txeof);

	/* Let's get this thing into the air! */
	err = usbd_transfer(c->ue_xfer);
	if (err != USBD_IN_PROGRESS) {
		DPRINTFN(25, ("%s: atu_tx_start, err=%d",
		    USBDEVNAME(sc->atu_dev), err));
		c->ue_mbuf = NULL;
		m_freem(m);
		return(EIO);
	}

	return (0);
}

void
atu_start(struct ifnet *ifp)
{
	struct atu_softc	*sc = ifp->if_softc;
	struct ieee80211com	*ic = &sc->sc_ic;
	struct atu_cdata	*cd = &sc->atu_cdata;
	struct ieee80211_node	*ni;
	struct ue_chain		*c;
	struct mbuf		*m = NULL;
	int			s;

	DPRINTFN(25, ("%s: atu_start: enter\n", USBDEVNAME(sc->atu_dev)));

	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		return;
	}
	if (ifp->if_flags & IFF_OACTIVE) {
		DPRINTFN(30, ("%s: atu_start: IFF_OACTIVE\n",
		    USBDEVNAME(sc->atu_dev)));
		return;
	}

	for (;;) {
		/* grab a TX buffer */
		s = splnet();
		c = SLIST_FIRST(&cd->atu_tx_free);
		if (c != NULL) {
			SLIST_REMOVE_HEAD(&cd->atu_tx_free, ue_next);
			cd->atu_tx_inuse++;
			if (cd->atu_tx_inuse == ATU_TX_LIST_CNT)
				ifp->if_flags |= IFF_OACTIVE;
		}
		splx(s);
		if (c == NULL) {
			DPRINTFN(10, ("%s: out of tx xfers\n",
			    USBDEVNAME(sc->atu_dev)));
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		/*
		 * Poll the management queue for frames, it has priority over
		 * normal data frames.
		 */
		IF_DEQUEUE(&ic->ic_mgtq, m);
		if (m == NULL) {
			DPRINTFN(10, ("%s: atu_start: data packet\n",
			    USBDEVNAME(sc->atu_dev)));
			if (ic->ic_state != IEEE80211_S_RUN) {
				DPRINTFN(25, ("%s: no data till running\n",
				    USBDEVNAME(sc->atu_dev)));
				/* put the xfer back on the list */
				s = splnet();
				SLIST_INSERT_HEAD(&cd->atu_tx_free, c,
				    ue_next);
				cd->atu_tx_inuse--;
				splx(s);
				break;
			}

			IFQ_DEQUEUE(&ifp->if_snd, m);
			if (m == NULL) {
				DPRINTFN(25, ("%s: nothing to send\n",
				    USBDEVNAME(sc->atu_dev)));
				s = splnet();
				SLIST_INSERT_HEAD(&cd->atu_tx_free, c,
				    ue_next);
				cd->atu_tx_inuse--;
				splx(s);
				break;
			}
#if NBPFILTER > 0
			if (ifp->if_bpf)
				bpf_mtap(ifp->if_bpf, m);
#endif
			ni = ieee80211_find_txnode(ic,
			    mtod(m, struct ether_header *)->ether_dhost);
			if (ni == NULL) {
				m_freem(m);
				goto bad;
			}
			m = ieee80211_encap(ic, m, ni);
			if (m == NULL)
				goto bad;
		} else {
			DPRINTFN(25, ("%s: atu_start: mgmt packet\n",
			    USBDEVNAME(sc->atu_dev)));

			/*
			 * Hack!  The referenced node pointer is in the
			 * rcvif field of the packet header.  This is
			 * placed there by ieee80211_mgmt_output because
			 * we need to hold the reference with the frame
			 * and there's no other way (other than packet
			 * tags which we consider too expensive to use)
			 * to pass it along.
			 */
			ni = (struct ieee80211_node *)m->m_pkthdr.rcvif;
			m->m_pkthdr.rcvif = NULL;

			/* sc->sc_stats.ast_tx_mgmt++; */
		}

#if NBPFILTER > 0
		if (ic->ic_rawbpf)
			bpf_mtap(ic->ic_rawbpf, m);
#endif

		if (atu_tx_start(sc, ni, c, m)) {
bad:
			s = splnet();
			SLIST_INSERT_HEAD(&cd->atu_tx_free, c,
			    ue_next);
			cd->atu_tx_inuse--;
			splx(s);
			/* ifp_if_oerrors++; */
			if (ni != NULL)
				ieee80211_free_node(ni);
			continue;
		}
		ifp->if_timer = 5;
	}
}

int
atu_init(struct ifnet *ifp)
{
	struct atu_softc	*sc = ifp->if_softc;
	struct ieee80211com	*ic = &sc->sc_ic;
	struct ue_chain		*c;
	usbd_status		err;
	int			i, s;

	s = splnet();

	DPRINTFN(10, ("%s: atu_init\n", USBDEVNAME(sc->atu_dev)));

	if (ifp->if_flags & IFF_RUNNING) {
		splx(s);
		return(0);
	}

	/* Open RX and TX pipes. */
	err = usbd_open_pipe(sc->atu_iface, sc->atu_ed[ATU_ENDPT_RX],
	    USBD_EXCLUSIVE_USE, &sc->atu_ep[ATU_ENDPT_RX]);
	if (err) {
		DPRINTF(("%s: open rx pipe failed: %s\n",
		    USBDEVNAME(sc->atu_dev), usbd_errstr(err)));
		splx(s);
		return(EIO);
	}

	err = usbd_open_pipe(sc->atu_iface, sc->atu_ed[ATU_ENDPT_TX],
	    USBD_EXCLUSIVE_USE, &sc->atu_ep[ATU_ENDPT_TX]);
	if (err) {
		DPRINTF(("%s: open tx pipe failed: %s\n",
		    USBDEVNAME(sc->atu_dev), usbd_errstr(err)));
		splx(s);
		return(EIO);
	}

	/* Init TX ring */
	if ((i = usb_ether_tx_list_init(USBDEV(sc->atu_dev),
	    sc->atu_cdata.atu_tx_chain, ATU_TX_LIST_CNT,
	    sc->atu_udev, sc->atu_ep[ATU_ENDPT_TX],
	    &sc->atu_cdata.atu_tx_free))) {
		printf("%s: tx list init failed\n", USBDEVNAME(sc->atu_dev));
		splx(s);
		return (i);
	}

	/* Init RX ring */
	if ((i = usb_ether_rx_list_init(USBDEV(sc->atu_dev),
	    sc->atu_cdata.atu_rx_chain, ATU_RX_LIST_CNT,
	    sc->atu_udev, sc->atu_ep[ATU_ENDPT_RX]))) {
		printf("%s: rx list init failed\n", USBDEVNAME(sc->atu_dev));
		splx(s);
		return (i);
	}

	/* Load the multicast filter. */
	/*atu_setmulti(sc); */

	/* Start up the receive pipe. */
	for (i = 0; i < ATU_RX_LIST_CNT; i++) {
		c = &sc->atu_cdata.atu_rx_chain[i];

		(void)usbd_map_buffer_mbuf(c->ue_xfer, c->ue_mbuf);
		usbd_setup_xfer(c->ue_xfer, sc->atu_ep[ATU_ENDPT_RX], c,
		    NULL /* XXX unused */, ATU_RX_BUFSZ, USBD_SHORT_XFER_OK | USBD_NO_COPY,
		    USBD_NO_TIMEOUT, atu_rxeof);
		usbd_transfer(c->ue_xfer);
	}

	DPRINTFN(10, ("%s: starting up using MAC=%s\n",
	    USBDEVNAME(sc->atu_dev), ether_sprintf(ic->ic_myaddr)));

	/* Do initial setup */
	err = atu_initial_config(sc);
	if (err) {
		DPRINTF(("%s: initial config failed!\n",
		    USBDEVNAME(sc->atu_dev)));
		splx(s);
		return(EIO);
	}
	DPRINTFN(10, ("%s: initialised transceiver\n",
	    USBDEVNAME(sc->atu_dev)));

	/* sc->atu_rxfilt = ATU_RXFILT_UNICAST|ATU_RXFILT_BROADCAST; */

	/* If we want promiscuous mode, set the allframes bit. */
	/*
	if (ifp->if_flags & IFF_PROMISC)
		sc->atu_rxfilt |= ATU_RXFILT_PROMISC;
	*/

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	splx(s);

	/* XXX the following HAS to be replaced */
	s = splnet();
	err = ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
	if (err) {
		DPRINTFN(1, ("%s: atu_init: error calling "
		    "ieee80211_net_state", USBDEVNAME(sc->atu_dev)));
	}
	splx(s);

	return 0;
}

#ifdef ATU_DEBUG
void
atu_debug_print(struct atu_softc *sc)
{
	usbd_status		err;
	u_int8_t		tmp[32];

	/* DEBUG */
	if ((err = atu_get_mib(sc, MIB_MAC_MGMT__CURRENT_BSSID, tmp)))
		return;
	DPRINTF(("%s: DEBUG: current BSSID=%s\n", USBDEVNAME(sc->atu_dev),
	    ether_sprintf(tmp)));

	if ((err = atu_get_mib(sc, MIB_MAC_MGMT__BEACON_PERIOD, tmp)))
		return;
	DPRINTF(("%s: DEBUG: beacon period=%d\n", USBDEVNAME(sc->atu_dev),
	    tmp[0]));

	if ((err = atu_get_mib(sc, MIB_MAC_WEP__PRIVACY_INVOKED, tmp)))
		return;
	DPRINTF(("%s: DEBUG: privacy invoked=%d\n", USBDEVNAME(sc->atu_dev),
	    tmp[0]));

	if ((err = atu_get_mib(sc, MIB_MAC_WEP__ENCR_LEVEL, tmp)))
		return;
	DPRINTF(("%s: DEBUG: encr_level=%d\n", USBDEVNAME(sc->atu_dev),
	    tmp[0]));

	if ((err = atu_get_mib(sc, MIB_MAC_WEP__ICV_ERROR_COUNT, tmp)))
		return;
	DPRINTF(("%s: DEBUG: icv error count=%d\n", USBDEVNAME(sc->atu_dev),
	    *(short *)tmp));

	if ((err = atu_get_mib(sc, MIB_MAC_WEP__EXCLUDED_COUNT, tmp)))
		return;
	DPRINTF(("%s: DEBUG: wep excluded count=%d\n",
	    USBDEVNAME(sc->atu_dev), *(short *)tmp));

	if ((err = atu_get_mib(sc, MIB_MAC_MGMT__POWER_MODE, tmp)))
		return;
	DPRINTF(("%s: DEBUG: power mode=%d\n", USBDEVNAME(sc->atu_dev),
	    tmp[0]));

	if ((err = atu_get_mib(sc, MIB_PHY__CHANNEL, tmp)))
		return;
	DPRINTF(("%s: DEBUG: channel=%d\n", USBDEVNAME(sc->atu_dev), tmp[0]));

	if ((err = atu_get_mib(sc, MIB_PHY__REG_DOMAIN, tmp)))
		return;
	DPRINTF(("%s: DEBUG: reg domain=%d\n", USBDEVNAME(sc->atu_dev),
	    tmp[0]));

	if ((err = atu_get_mib(sc, MIB_LOCAL__SSID_SIZE, tmp)))
		return;
	DPRINTF(("%s: DEBUG: ssid size=%d\n", USBDEVNAME(sc->atu_dev),
	    tmp[0]));

	if ((err = atu_get_mib(sc, MIB_LOCAL__BEACON_ENABLE, tmp)))
		return;
	DPRINTF(("%s: DEBUG: beacon enable=%d\n", USBDEVNAME(sc->atu_dev),
	    tmp[0]));

	if ((err = atu_get_mib(sc, MIB_LOCAL__AUTO_RATE_FALLBACK, tmp)))
		return;
	DPRINTF(("%s: DEBUG: auto rate fallback=%d\n",
	    USBDEVNAME(sc->atu_dev), tmp[0]));

	if ((err = atu_get_mib(sc, MIB_MAC_ADDR__ADDR, tmp)))
		return;
	DPRINTF(("%s: DEBUG: mac addr=%s\n", USBDEVNAME(sc->atu_dev),
	    ether_sprintf(tmp)));

	if ((err = atu_get_mib(sc, MIB_MAC__DESIRED_SSID, tmp)))
		return;
	DPRINTF(("%s: DEBUG: desired ssid=%s\n", USBDEVNAME(sc->atu_dev),
	    tmp));

	if ((err = atu_get_mib(sc, MIB_MAC_MGMT__CURRENT_ESSID, tmp)))
		return;
	DPRINTF(("%s: DEBUG: current ESSID=%s\n", USBDEVNAME(sc->atu_dev),
	    tmp));
}
#endif /* ATU_DEBUG */

int
atu_ioctl(struct ifnet *ifp, u_long command, usb_ioctlarg_t data)
{
	struct atu_softc	*sc = ifp->if_softc;
	struct ifreq		*ifr = (struct ifreq *)data;
	struct ieee80211com	*ic = &sc->sc_ic;
	int			err = 0, s;

	s = splnet();
	switch (command) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		err = ifmedia_ioctl(ifp, ifr, &ic->ic_media, command);
		break;

	default:
		DPRINTFN(15, ("%s: ieee80211_ioctl (%lu)\n",
		    USBDEVNAME(sc->atu_dev), command));
		err = ieee80211_ioctl(ic, command, data);
		break;
	}

	if (err == ENETRESET) {
		if ((ifp->if_flags & (IFF_RUNNING|IFF_UP)) ==
		    (IFF_RUNNING|IFF_UP)) {
			DPRINTF(("%s: atu_ioctl(): netreset %lu\n",
			    USBDEVNAME(sc->atu_dev), command));
			ieee80211_new_state(ic, IEEE80211_S_INIT, -1);
			atu_initial_config(sc);
			ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);
		}
		err = 0;
	}

	splx(s);
	return (err);
}

void
atu_watchdog(struct ifnet *ifp)
{
	struct atu_softc	*sc = ifp->if_softc;
	struct ue_chain		*c;
	usbd_status		stat;
	int			cnt, s;

	DPRINTF(("%s: atu_watchdog\n", USBDEVNAME(sc->atu_dev)));

	ifp->if_timer = 0;

	if (sc->sc_state != ATU_S_OK || (ifp->if_flags & IFF_RUNNING) == 0)
		return;

	sc = ifp->if_softc;
	s = splnet();
	ifp->if_oerrors++;
	DPRINTF(("%s: watchdog timeout\n", USBDEVNAME(sc->atu_dev)));

	/*
	 * TODO:
	 * we should change this since we have multiple TX tranfers...
	 */
	for (cnt = 0; cnt < ATU_TX_LIST_CNT; cnt++) {
		c = &sc->atu_cdata.atu_tx_chain[cnt];
		if (c->ue_mbuf) {
			usbd_get_xfer_status(c->ue_xfer, NULL, NULL, NULL,
			    &stat);
			atu_txeof(c->ue_xfer, c, stat);
		}
	}

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		atu_start(ifp);
	splx(s);

	ieee80211_watchdog(&sc->sc_ic);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
void
atu_stop(struct ifnet *ifp, int disable)
{
	struct atu_softc	*sc = ifp->if_softc;
	struct ieee80211com	*ic = &sc->sc_ic;
	struct atu_cdata	*cd;
	usbd_status		err;
	int s;

	s = splnet();
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;

	usb_rem_task(sc->atu_udev, &sc->sc_task);
	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);

	/* Stop transfers. */
	if (sc->atu_ep[ATU_ENDPT_RX] != NULL) {
		err = usbd_abort_pipe(sc->atu_ep[ATU_ENDPT_RX]);
		if (err) {
			DPRINTF(("%s: abort rx pipe failed: %s\n",
			    USBDEVNAME(sc->atu_dev), usbd_errstr(err)));
		}
	}

	if (sc->atu_ep[ATU_ENDPT_TX] != NULL) {
		err = usbd_abort_pipe(sc->atu_ep[ATU_ENDPT_TX]);
		if (err) {
			DPRINTF(("%s: abort tx pipe failed: %s\n",
			    USBDEVNAME(sc->atu_dev), usbd_errstr(err)));
		}
	}

	/* Free RX/TX/MGMT list resources. */
	cd = &sc->atu_cdata;
	usb_ether_rx_list_free(cd->atu_rx_chain, ATU_RX_LIST_CNT);
	usb_ether_tx_list_free(cd->atu_tx_chain, ATU_TX_LIST_CNT);

	/* Close pipes. */
	if (sc->atu_ep[ATU_ENDPT_RX] != NULL) {
		err = usbd_close_pipe(sc->atu_ep[ATU_ENDPT_RX]);
		if (err) {
			DPRINTF(("%s: close rx pipe failed: %s\n",
			    USBDEVNAME(sc->atu_dev), usbd_errstr(err)));
		}
		sc->atu_ep[ATU_ENDPT_RX] = NULL;
	}

	if (sc->atu_ep[ATU_ENDPT_TX] != NULL) {
		err = usbd_close_pipe(sc->atu_ep[ATU_ENDPT_TX]);
		if (err) {
			DPRINTF(("%s: close tx pipe failed: %s\n",
			    USBDEVNAME(sc->atu_dev), usbd_errstr(err)));
		}
		sc->atu_ep[ATU_ENDPT_TX] = NULL;
	}

	/* Let's be nice and turn off the radio before we leave */
	atu_switch_radio(sc, 0);

	splx(s);
}
