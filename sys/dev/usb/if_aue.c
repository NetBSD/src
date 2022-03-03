/*	$NetBSD: if_aue.c,v 1.181 2022/03/03 05:53:23 riastradh Exp $	*/

/*
 * Copyright (c) 1997, 1998, 1999, 2000
 *	Bill Paul <wpaul@ee.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/usb/if_aue.c,v 1.11 2000/01/14 01:36:14 wpaul Exp $
 */

/*
 * ADMtek AN986 Pegasus and AN8511 Pegasus II USB to ethernet driver.
 * Datasheet is available from http://www.admtek.com.tw.
 *
 * Written by Bill Paul <wpaul@ee.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

/*
 * The Pegasus chip uses four USB "endpoints" to provide 10/100 ethernet
 * support: the control endpoint for reading/writing registers, burst
 * read endpoint for packet reception, burst write for packet transmission
 * and one for "interrupts." The chip uses the same RX filter scheme
 * as the other ADMtek ethernet parts: one perfect filter entry for the
 * the station address and a 64-bit multicast hash table. The chip supports
 * both MII and HomePNA attachments.
 *
 * Since the maximum data transfer speed of USB is supposed to be 12Mbps,
 * you're never really going to get 100Mbps speeds from this device. I
 * think the idea is to allow the device to connect to 10 or 100Mbps
 * networks, not necessarily to provide 100Mbps performance. Also, since
 * the controller uses an external PHY chip, it's possible that board
 * designers might simply choose a 10Mbps PHY.
 *
 * Registers are accessed using usbd_do_request(). Packet transfers are
 * done using usbd_transfer() and friends.
 */

/*
 * Ported to NetBSD and somewhat rewritten by Lennart Augustsson.
 */

/*
 * TODO:
 * better error messages from rxstat
 * more error checks
 * investigate short rx problem
 * proper cleanup on errors
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_aue.c,v 1.181 2022/03/03 05:53:23 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#include "opt_inet.h"
#endif

#include <sys/param.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/usbhist.h>
#include <dev/usb/if_auereg.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef USB_DEBUG
#ifndef AUE_DEBUG
#define auedebug 0
#else
static int auedebug = 10;

SYSCTL_SETUP(sysctl_hw_aue_setup, "sysctl hw.aue setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "aue",
	    SYSCTL_DESCR("aue global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &auedebug, sizeof(auedebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* AUE_DEBUG */
#endif /* USB_DEBUG */

#define DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(auedebug,1,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(auedebug,N,FMT,A,B,C,D)
#define AUEHIST_FUNC()		USBHIST_FUNC()
#define AUEHIST_CALLED(name)	USBHIST_CALLED(auedebug)
#define AUEHIST_CALLARGS(FMT,A,B,C,D) \
				USBHIST_CALLARGS(auedebug,FMT,A,B,C,D)
#define AUEHIST_CALLARGSN(N,FMT,A,B,C,D) \
				USBHIST_CALLARGSN(auedebug,N,FMT,A,B,C,D)

#define AUE_TX_LIST_CNT		1
#define AUE_RX_LIST_CNT		1

struct aue_softc {
	struct usbnet		aue_un;
	struct usbnet_intr	aue_intr;
	struct aue_intrpkt	aue_ibuf;
};

#define AUE_TIMEOUT		1000
#define AUE_BUFSZ		1536
#define AUE_MIN_FRAMELEN	60
#define AUE_TX_TIMEOUT		10000 /* ms */
#define AUE_INTR_INTERVAL	100 /* ms */

/*
 * Various supported device vendors/products.
 */
struct aue_type {
	struct usb_devno	aue_dev;
	uint16_t		aue_flags;
#define LSYS	0x0001		/* use Linksys reset */
#define PNA	0x0002		/* has Home PNA */
#define PII	0x0004		/* Pegasus II chip */
};

static const struct aue_type aue_devs[] = {
 {{ USB_VENDOR_3COM,		USB_PRODUCT_3COM_3C460B},	  PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX1},	  PNA | PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX2},	  PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_UFE1000},	  LSYS },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX4},	  PNA },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX5},	  PNA },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX6},	  PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX7},	  PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX8},	  PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX9},	  PNA },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX10},	  0 },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_DSB650TX_PNA}, 0 },
 {{ USB_VENDOR_ACCTON,		USB_PRODUCT_ACCTON_USB320_EC},	  0 },
 {{ USB_VENDOR_ACCTON,		USB_PRODUCT_ACCTON_SS1001},	  PII },
 {{ USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUS},	  PNA },
 {{ USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUSII},	  PII },
 {{ USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUSII_2},  PII },
 {{ USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUSII_3},  PII },
 {{ USB_VENDOR_AEI,		USB_PRODUCT_AEI_USBTOLAN},	  PII },
 {{ USB_VENDOR_BELKIN,		USB_PRODUCT_BELKIN_USB2LAN},	  PII },
 {{ USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USB100},	  0 },
 {{ USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USBLP100}, PNA },
 {{ USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USBEL100}, 0 },
 {{ USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USBE100},  PII },
 {{ USB_VENDOR_COMPAQ,		USB_PRODUCT_COMPAQ_HNE200},	  PII },
 {{ USB_VENDOR_COREGA,		USB_PRODUCT_COREGA_FETHER_USB_TX}, 0 },
 {{ USB_VENDOR_COREGA,		USB_PRODUCT_COREGA_FETHER_USB_TXS},PII },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX4},	  LSYS | PII },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX1},	  LSYS },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX},	  LSYS },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX_PNA},  PNA },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX3},	  LSYS | PII },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX2},	  LSYS | PII },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650},	  0 },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX0},	  0 },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX1},	  LSYS },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX2},	  0 },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX3},	  LSYS },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBLTX},	  PII },
 {{ USB_VENDOR_ELSA,		USB_PRODUCT_ELSA_USB2ETHERNET},	  0 },
 {{ USB_VENDOR_HAWKING,		USB_PRODUCT_HAWKING_UF100},	  PII },
 {{ USB_VENDOR_HP,		USB_PRODUCT_HP_HN210E},		  PII },
 {{ USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_USBETTX},	  0 },
 {{ USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_USBETTXS},	  PII },
 {{ USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_ETXUS2},	  PII },
 {{ USB_VENDOR_KINGSTON,	USB_PRODUCT_KINGSTON_KNU101TX},	  0 },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10TX1},	  LSYS | PII },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10T},	  LSYS },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB100TX},	  LSYS },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB100H1},	  LSYS | PNA },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10TA},	  LSYS },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10TX2},	  LSYS | PII },
 {{ USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUATX1},	  0 },
 {{ USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUATX5},	  0 },
 {{ USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUA2TX5},	  PII },
 {{ USB_VENDOR_MICROSOFT,	USB_PRODUCT_MICROSOFT_MN110},	  PII },
 {{ USB_VENDOR_NETGEAR,		USB_PRODUCT_NETGEAR_FA101},	  PII },
 {{ USB_VENDOR_SIEMENS,		USB_PRODUCT_SIEMENS_SPEEDSTREAM}, PII },
 {{ USB_VENDOR_SMARTBRIDGES,	USB_PRODUCT_SMARTBRIDGES_SMARTNIC},PII },
 {{ USB_VENDOR_SMC,		USB_PRODUCT_SMC_2202USB},	  0 },
 {{ USB_VENDOR_SMC,		USB_PRODUCT_SMC_2206USB},	  PII },
 {{ USB_VENDOR_SOHOWARE,	USB_PRODUCT_SOHOWARE_NUB100},	  0 },
};
#define aue_lookup(v, p) ((const struct aue_type *)usb_lookup(aue_devs, v, p))

static int aue_match(device_t, cfdata_t, void *);
static void aue_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(aue, sizeof(struct aue_softc), aue_match, aue_attach,
    usbnet_detach, usbnet_activate);

static void aue_reset_pegasus_II(struct aue_softc *);

static void aue_uno_stop(struct ifnet *, int);
static void aue_uno_mcast(struct ifnet *);
static int aue_uno_mii_read_reg(struct usbnet *, int, int, uint16_t *);
static int aue_uno_mii_write_reg(struct usbnet *, int, int, uint16_t);
static void aue_uno_mii_statchg(struct ifnet *);
static unsigned aue_uno_tx_prepare(struct usbnet *, struct mbuf *,
				   struct usbnet_chain *);
static void aue_uno_rx_loop(struct usbnet *, struct usbnet_chain *, uint32_t);
static int aue_uno_init(struct ifnet *);
static void aue_uno_intr(struct usbnet *, usbd_status);

static const struct usbnet_ops aue_ops = {
	.uno_stop = aue_uno_stop,
	.uno_mcast = aue_uno_mcast,
	.uno_read_reg = aue_uno_mii_read_reg,
	.uno_write_reg = aue_uno_mii_write_reg,
	.uno_statchg = aue_uno_mii_statchg,
	.uno_tx_prepare = aue_uno_tx_prepare,
	.uno_rx_loop = aue_uno_rx_loop,
	.uno_init = aue_uno_init,
	.uno_intr = aue_uno_intr,
};

static uint32_t aue_crc(void *);
static void aue_reset(struct aue_softc *);

static int aue_csr_read_1(struct aue_softc *, int);
static int aue_csr_write_1(struct aue_softc *, int, int);
static int aue_csr_read_2(struct aue_softc *, int);
static int aue_csr_write_2(struct aue_softc *, int, int);

#define AUE_SETBIT(sc, reg, x)				\
	aue_csr_write_1(sc, reg, aue_csr_read_1(sc, reg) | (x))

#define AUE_CLRBIT(sc, reg, x)				\
	aue_csr_write_1(sc, reg, aue_csr_read_1(sc, reg) & ~(x))

static int
aue_csr_read_1(struct aue_softc *sc, int reg)
{
	struct usbnet * const	un = &sc->aue_un;
	usb_device_request_t	req;
	usbd_status		err;
	uByte			val = 0;

	if (usbnet_isdying(un))
		return 0;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = AUE_UR_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 1);

	err = usbd_do_request(un->un_udev, &req, &val);

	if (err) {
		AUEHIST_FUNC();
		AUEHIST_CALLARGS("aue%jd: reg=%#jx err=%jd",
		    device_unit(un->un_dev), reg, err, 0);
		return 0;
	}

	return val;
}

static int
aue_csr_read_2(struct aue_softc *sc, int reg)
{
	struct usbnet * const	un = &sc->aue_un;
	usb_device_request_t	req;
	usbd_status		err;
	uWord			val;

	if (usbnet_isdying(un))
		return 0;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = AUE_UR_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 2);

	err = usbd_do_request(un->un_udev, &req, &val);

	if (err) {
		AUEHIST_FUNC();
		AUEHIST_CALLARGS("aue%jd: reg=%#jx err=%jd",
		    device_unit(un->un_dev), reg, err, 0);
		return 0;
	}

	return UGETW(val);
}

static int
aue_csr_write_1(struct aue_softc *sc, int reg, int aval)
{
	struct usbnet * const	un = &sc->aue_un;
	usb_device_request_t	req;
	usbd_status		err;
	uByte			val;

	if (usbnet_isdying(un))
		return 0;

	val = aval;
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = AUE_UR_WRITEREG;
	USETW(req.wValue, val);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 1);

	err = usbd_do_request(un->un_udev, &req, &val);

	if (err) {
		AUEHIST_FUNC();
		AUEHIST_CALLARGS("aue%jd: reg=%#jx err=%jd",
		    device_unit(un->un_dev), reg, err, 0);
		return -1;
	}

	return 0;
}

static int
aue_csr_write_2(struct aue_softc *sc, int reg, int aval)
{
	struct usbnet * const	un = &sc->aue_un;
	usb_device_request_t	req;
	usbd_status		err;
	uWord			val;

	if (usbnet_isdying(un))
		return 0;

	USETW(val, aval);
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = AUE_UR_WRITEREG;
	USETW(req.wValue, aval);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 2);

	err = usbd_do_request(un->un_udev, &req, &val);

	if (err) {
		AUEHIST_FUNC();
		AUEHIST_CALLARGS("aue%jd: reg=%#jx err=%jd",
		    device_unit(un->un_dev), reg, err, 0);
		return -1;
	}

	return 0;
}

/*
 * Read a word of data stored in the EEPROM at address 'addr.'
 */
static int
aue_eeprom_getword(struct aue_softc *sc, int addr)
{
	struct usbnet * const	un = &sc->aue_un;
	int			i;

	AUEHIST_FUNC(); AUEHIST_CALLED();

	aue_csr_write_1(sc, AUE_EE_REG, addr);
	aue_csr_write_1(sc, AUE_EE_CTL, AUE_EECTL_READ);

	for (i = 0; i < AUE_TIMEOUT; i++) {
		if (aue_csr_read_1(sc, AUE_EE_CTL) & AUE_EECTL_DONE)
			break;
	}

	if (i == AUE_TIMEOUT) {
		printf("%s: EEPROM read timed out\n",
		    device_xname(un->un_dev));
	}

	return aue_csr_read_2(sc, AUE_EE_DATA);
}

/*
 * Read the MAC from the EEPROM.  It's at offset 0.
 */
static void
aue_read_mac(struct usbnet *un)
{
	struct aue_softc	*sc = usbnet_softc(un);
	int			i;
	int			off = 0;
	int			word;

	usbnet_isowned_core(un);

	AUEHIST_FUNC();
	AUEHIST_CALLARGS("aue%jd: enter",
	    device_unit(un->un_dev), 0, 0, 0);

	for (i = 0; i < 3; i++) {
		word = aue_eeprom_getword(sc, off + i);
		un->un_eaddr[2 * i] =     (u_char)word;
		un->un_eaddr[2 * i + 1] = (u_char)(word >> 8);
	}
}

static int
aue_uno_mii_read_reg(struct usbnet *un, int phy, int reg, uint16_t *val)
{
	struct aue_softc	*sc = usbnet_softc(un);
	int			i;

	AUEHIST_FUNC();

#if 0
	/*
	 * The Am79C901 HomePNA PHY actually contains
	 * two transceivers: a 1Mbps HomePNA PHY and a
	 * 10Mbps full/half duplex ethernet PHY with
	 * NWAY autoneg. However in the ADMtek adapter,
	 * only the 1Mbps PHY is actually connected to
	 * anything, so we ignore the 10Mbps one. It
	 * happens to be configured for MII address 3,
	 * so we filter that out.
	 */
	if (sc->aue_vendor == USB_VENDOR_ADMTEK &&
	    sc->aue_product == USB_PRODUCT_ADMTEK_PEGASUS) {
		if (phy == 3)
			return EINVAL;
	}
#endif

	aue_csr_write_1(sc, AUE_PHY_ADDR, phy);
	aue_csr_write_1(sc, AUE_PHY_CTL, reg | AUE_PHYCTL_READ);

	for (i = 0; i < AUE_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return ENXIO;
		if (aue_csr_read_1(sc, AUE_PHY_CTL) & AUE_PHYCTL_DONE)
			break;
	}

	if (i == AUE_TIMEOUT) {
		AUEHIST_CALLARGS("aue%jd: phy=%#jx reg=%#jx read timed out",
		    device_unit(un->un_dev), phy, reg, 0);
		return ETIMEDOUT;
	}

	*val = aue_csr_read_2(sc, AUE_PHY_DATA);

	AUEHIST_CALLARGSN(11, "aue%jd: phy=%#jx reg=%#jx => 0x%04jx",
	    device_unit(un->un_dev), phy, reg, *val);

	return 0;
}

static int
aue_uno_mii_write_reg(struct usbnet *un, int phy, int reg, uint16_t val)
{
	struct aue_softc	*sc = usbnet_softc(un);
	int			i;

	AUEHIST_FUNC();
	AUEHIST_CALLARGSN(11, "aue%jd: phy=%jd reg=%jd data=0x%04jx",
	    device_unit(un->un_dev), phy, reg, val);

#if 0
	if (sc->aue_vendor == USB_VENDOR_ADMTEK &&
	    sc->aue_product == USB_PRODUCT_ADMTEK_PEGASUS) {
		if (phy == 3)
			return EINVAL;
	}
#endif

	aue_csr_write_2(sc, AUE_PHY_DATA, val);
	aue_csr_write_1(sc, AUE_PHY_ADDR, phy);
	aue_csr_write_1(sc, AUE_PHY_CTL, reg | AUE_PHYCTL_WRITE);

	for (i = 0; i < AUE_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return ENXIO;
		if (aue_csr_read_1(sc, AUE_PHY_CTL) & AUE_PHYCTL_DONE)
			break;
	}

	if (i == AUE_TIMEOUT) {
		DPRINTF("aue%jd: phy=%#jx reg=%#jx val=%#jx write timed out",
		    device_unit(un->un_dev), phy, reg, val);
		return ETIMEDOUT;
	}

	return 0;
}

static void
aue_uno_mii_statchg(struct ifnet *ifp)
{
	struct usbnet *un = ifp->if_softc;
	struct aue_softc *sc = usbnet_softc(un);
	struct mii_data	*mii = usbnet_mii(un);
	const bool hadlink __diagused = usbnet_havelink(un);

	AUEHIST_FUNC(); AUEHIST_CALLED();
	AUEHIST_CALLARGSN(5, "aue%jd: ifp=%#jx link=%jd",
	    device_unit(un->un_dev), (uintptr_t)ifp, hadlink, 0);

	AUE_CLRBIT(sc, AUE_CTL0, AUE_CTL0_RX_ENB | AUE_CTL0_TX_ENB);

	if (IFM_SUBTYPE(mii->mii_media_active) == IFM_100_TX) {
		AUE_SETBIT(sc, AUE_CTL1, AUE_CTL1_SPEEDSEL);
	} else {
		AUE_CLRBIT(sc, AUE_CTL1, AUE_CTL1_SPEEDSEL);
	}

	if ((mii->mii_media_active & IFM_FDX) != 0)
		AUE_SETBIT(sc, AUE_CTL1, AUE_CTL1_DUPLEX);
	else
		AUE_CLRBIT(sc, AUE_CTL1, AUE_CTL1_DUPLEX);

	AUE_SETBIT(sc, AUE_CTL0, AUE_CTL0_RX_ENB | AUE_CTL0_TX_ENB);

	if (mii->mii_media_status & IFM_ACTIVE &&
	    IFM_SUBTYPE(mii->mii_media_active) != IFM_NONE) {
		usbnet_set_link(un, true);
	}

	/*
	 * Set the LED modes on the LinkSys adapter.
	 * This turns on the 'dual link LED' bin in the auxmode
	 * register of the Broadcom PHY.
	 */
	if (!usbnet_isdying(un) && (un->un_flags & LSYS)) {
		uint16_t auxmode;
		aue_uno_mii_read_reg(un, 0, 0x1b, &auxmode);
		aue_uno_mii_write_reg(un, 0, 0x1b, auxmode | 0x04);
	}

	if (usbnet_havelink(un) != hadlink) {
		DPRINTFN(5, "aue%jd: exit link %jd",
		    device_unit(un->un_dev), usbnet_havelink(un), 0, 0);
	}
}

#define AUE_POLY	0xEDB88320
#define AUE_BITS	6

static uint32_t
aue_crc(void *addrv)
{
	uint32_t		idx, bit, data, crc;
	char *addr = addrv;

	/* Compute CRC for the address value. */
	crc = 0xFFFFFFFF; /* initial value */

	for (idx = 0; idx < 6; idx++) {
		for (data = *addr++, bit = 0; bit < 8; bit++, data >>= 1)
			crc = (crc >> 1) ^ (((crc ^ data) & 1) ? AUE_POLY : 0);
	}

	return crc & ((1 << AUE_BITS) - 1);
}

static void
aue_uno_mcast(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	struct aue_softc * const sc = usbnet_softc(un);
	struct ethercom *	ec = usbnet_ec(un);
	struct ether_multi	*enm;
	struct ether_multistep	step;
	uint32_t		h = 0, i;
	uint8_t hashtbl[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	AUEHIST_FUNC();
	AUEHIST_CALLARGSN(5, "aue%jd: enter", device_unit(un->un_dev), 0, 0, 0);

	if (ifp->if_flags & IFF_PROMISC) {
		ETHER_LOCK(ec);
allmulti:
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		AUE_SETBIT(sc, AUE_CTL0, AUE_CTL0_ALLMULTI);
		return;
	}

	/* now program new ones */
	ETHER_LOCK(ec);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo,
		    enm->enm_addrhi, ETHER_ADDR_LEN) != 0) {
			goto allmulti;
		}

		h = aue_crc(enm->enm_addrlo);
		hashtbl[h >> 3] |= 1 << (h & 0x7);
		ETHER_NEXT_MULTI(step, enm);
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_UNLOCK(ec);

	AUE_CLRBIT(sc, AUE_CTL0, AUE_CTL0_ALLMULTI);

	/* write the hashtable */
	for (i = 0; i < 8; i++)
		aue_csr_write_1(sc, AUE_MAR0 + i, hashtbl[i]);
}

static void
aue_reset_pegasus_II(struct aue_softc *sc)
{
	/* Magic constants taken from Linux driver. */
	aue_csr_write_1(sc, AUE_REG_1D, 0);
	aue_csr_write_1(sc, AUE_REG_7B, 2);
#if 0
	if ((un->un_flags & PNA) && mii_mode)
		aue_csr_write_1(sc, AUE_REG_81, 6);
	else
#endif
		aue_csr_write_1(sc, AUE_REG_81, 2);
}

static void
aue_reset(struct aue_softc *sc)
{
	struct usbnet * const un = &sc->aue_un;
	int		i;

	AUEHIST_FUNC();
	AUEHIST_CALLARGSN(2, "aue%jd: enter", device_unit(un->un_dev), 0, 0, 0);

	AUE_SETBIT(sc, AUE_CTL1, AUE_CTL1_RESETMAC);

	for (i = 0; i < AUE_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return;
		if (!(aue_csr_read_1(sc, AUE_CTL1) & AUE_CTL1_RESETMAC))
			break;
	}

	if (i == AUE_TIMEOUT)
		printf("%s: reset failed\n", device_xname(un->un_dev));

#if 0
	/* XXX what is mii_mode supposed to be */
	if (sc->sc_mii_mode && (un->un_flags & PNA))
		aue_csr_write_1(sc, AUE_GPIO1, 0x34);
	else
		aue_csr_write_1(sc, AUE_GPIO1, 0x26);
#endif

	/*
	 * The PHY(s) attached to the Pegasus chip may be held
	 * in reset until we flip on the GPIO outputs. Make sure
	 * to set the GPIO pins high so that the PHY(s) will
	 * be enabled.
	 *
	 * Note: We force all of the GPIO pins low first, *then*
	 * enable the ones we want.
	 */
	if (un->un_flags & LSYS) {
		/* Grrr. LinkSys has to be different from everyone else. */
		aue_csr_write_1(sc, AUE_GPIO0,
		    AUE_GPIO_SEL0 | AUE_GPIO_SEL1);
	} else {
		aue_csr_write_1(sc, AUE_GPIO0,
		    AUE_GPIO_OUT0 | AUE_GPIO_SEL0);
	}
	aue_csr_write_1(sc, AUE_GPIO0,
	    AUE_GPIO_OUT0 | AUE_GPIO_SEL0 | AUE_GPIO_SEL1);

	if (un->un_flags & PII)
		aue_reset_pegasus_II(sc);

	/* Wait a little while for the chip to get its brains in order. */
	delay(10000);	/* XXX */
	//usbd_delay_ms(un->un_udev, 10);	/* XXX */

	DPRINTFN(2, "aue%jd: exit", device_unit(un->un_dev), 0, 0, 0);
}

/*
 * Probe for a Pegasus chip.
 */
static int
aue_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	/*
	 * Some manufacturers use the same vendor and product id for
	 * different devices. We need to sanity check the DeviceClass
	 * in this case
	 * Currently known guilty products:
	 * 0x050d/0x0121 Belkin Bluetooth and USB2LAN
	 *
	 * If this turns out to be more common, we could use a quirk
	 * table.
	 */
	if (uaa->uaa_vendor == USB_VENDOR_BELKIN &&
		uaa->uaa_product == USB_PRODUCT_BELKIN_USB2LAN) {
		usb_device_descriptor_t *dd;

		dd = usbd_get_device_descriptor(uaa->uaa_device);
		if (dd != NULL &&
			dd->bDeviceClass != UDCLASS_IN_INTERFACE)
			return UMATCH_NONE;
	}

	return aue_lookup(uaa->uaa_vendor, uaa->uaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
static void
aue_attach(device_t parent, device_t self, void *aux)
{
	USBNET_MII_DECL_DEFAULT(unm);
	struct aue_softc * const sc = device_private(self);
	struct usbnet * const un = &sc->aue_un;
	struct usb_attach_arg *uaa = aux;
	char			*devinfop;
	struct usbd_device	*dev = uaa->uaa_device;
	usbd_status		err;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	int			i;

	AUEHIST_FUNC();
	AUEHIST_CALLARGSN(2, "aue%jd: enter sc=%#jx",
	    device_unit(self), (uintptr_t)sc, 0, 0);

	KASSERT((void *)sc == un);

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(uaa->uaa_device, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = sc;
	un->un_ops = &aue_ops;
	un->un_intr = &sc->aue_intr;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = AUE_RX_LIST_CNT;
	un->un_tx_list_cnt = AUE_RX_LIST_CNT;
	un->un_rx_bufsz = AUE_BUFSZ;
	un->un_tx_bufsz = AUE_BUFSZ;

	sc->aue_intr.uni_buf = &sc->aue_ibuf;
	sc->aue_intr.uni_bufsz = sizeof(sc->aue_ibuf);
	sc->aue_intr.uni_interval = AUE_INTR_INTERVAL;

	err = usbd_set_config_no(dev, AUE_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	err = usbd_device2interface_handle(dev, AUE_IFACE_IDX, &un->un_iface);
	if (err) {
		aprint_error_dev(self, "getting interface handle failed\n");
		return;
	}

	un->un_flags = aue_lookup(uaa->uaa_vendor, uaa->uaa_product)->aue_flags;

	id = usbd_get_interface_descriptor(un->un_iface);

	/* Find endpoints. */
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(un->un_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self,
			    "couldn't get endpoint descriptor %d\n", i);
			return;
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			un->un_ed[USBNET_ENDPT_RX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			un->un_ed[USBNET_ENDPT_TX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			un->un_ed[USBNET_ENDPT_INTR] = ed->bEndpointAddress;
		}
	}

	if (un->un_ed[USBNET_ENDPT_RX] == 0 ||
	    un->un_ed[USBNET_ENDPT_TX] == 0 ||
	    un->un_ed[USBNET_ENDPT_INTR] == 0) {
		aprint_error_dev(self, "missing endpoint\n");
		return;
	}

	/* First level attach. */
	usbnet_attach(un, "auedet");

	usbnet_lock_core(un);

	/* Reset the adapter and get station address from the EEPROM.  */
	aue_reset(sc);
	aue_read_mac(un);

	usbnet_unlock_core(un);

	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, &unm);
}

static void
aue_uno_intr(struct usbnet *un, usbd_status status)
{
	struct ifnet		*ifp = usbnet_ifp(un);
	struct aue_softc	*sc = usbnet_softc(un);
	struct aue_intrpkt	*p = &sc->aue_ibuf;

	AUEHIST_FUNC();
	AUEHIST_CALLARGSN(20, "aue%jd: enter txstat0 %#jx\n",
	    device_unit(un->un_dev), p->aue_txstat0, 0, 0);

	if (p->aue_txstat0)
		if_statinc(ifp, if_oerrors);

	if (p->aue_txstat0 & (AUE_TXSTAT0_LATECOLL | AUE_TXSTAT0_EXCESSCOLL))
		if_statinc(ifp, if_collisions);
}

static void
aue_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet		*ifp = usbnet_ifp(un);
	uint8_t			*buf = c->unc_buf;
	struct aue_rxpkt	r;
	uint32_t		pktlen;

	AUEHIST_FUNC();
	AUEHIST_CALLARGSN(10, "aue%jd: enter len %ju",
	    device_unit(un->un_dev), total_len, 0, 0);

	if (total_len <= 4 + ETHER_CRC_LEN) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	memcpy(&r, buf + total_len - 4, sizeof(r));

	/* Turn off all the non-error bits in the rx status word. */
	r.aue_rxstat &= AUE_RXSTAT_MASK;
	if (r.aue_rxstat) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	/* No errors; receive the packet. */
	pktlen = total_len - ETHER_CRC_LEN - 4;

	usbnet_enqueue(un, buf, pktlen, 0, 0, 0);
}

static unsigned
aue_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	uint8_t			*buf = c->unc_buf;
	int			total_len;

	AUEHIST_FUNC();
	AUEHIST_CALLARGSN(10, "aue%jd: enter pktlen=%jd",
	    device_unit(un->un_dev), m->m_pkthdr.len, 0, 0);

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - 2)
		return 0;

	/*
	 * Copy the mbuf data into a contiguous buffer, leaving two
	 * bytes at the beginning to hold the frame length.
	 */
	m_copydata(m, 0, m->m_pkthdr.len, buf + 2);

	/*
	 * The ADMtek documentation says that the packet length is
	 * supposed to be specified in the first two bytes of the
	 * transfer, however it actually seems to ignore this info
	 * and base the frame size on the bulk transfer length.
	 */
	buf[0] = (uint8_t)m->m_pkthdr.len;
	buf[1] = (uint8_t)(m->m_pkthdr.len >> 8);
	total_len = m->m_pkthdr.len + 2;

	DPRINTFN(5, "aue%jd: send %jd bytes",
	    device_unit(un->un_dev), total_len, 0, 0);

	return total_len;
}

static int
aue_uno_init(struct ifnet *ifp)
{
	struct usbnet * const	un = ifp->if_softc;
	struct aue_softc	*sc = usbnet_softc(un);
	int			i, rv;
	const u_char		*eaddr;

	AUEHIST_FUNC();
	AUEHIST_CALLARGSN(5, "aue%jd: enter link=%jd",
	    device_unit(un->un_dev), usbnet_havelink(un), 0, 0);

	if (usbnet_isdying(un))
		return EIO;

	/* Cancel pending I/O */
	if (ifp->if_flags & IFF_RUNNING)
		return 0;

	/* Reset the interface. */
	aue_reset(sc);

	eaddr = CLLADDR(ifp->if_sadl);
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		aue_csr_write_1(sc, AUE_PAR0 + i, eaddr[i]);

	 /* If we want promiscuous mode, set the allframes bit. */
	if (ifp->if_flags & IFF_PROMISC)
		AUE_SETBIT(sc, AUE_CTL2, AUE_CTL2_RX_PROMISC);
	else
		AUE_CLRBIT(sc, AUE_CTL2, AUE_CTL2_RX_PROMISC);

	rv = usbnet_init_rx_tx(un);

	/* Load the multicast filter. */
	aue_uno_mcast(ifp);

	/* Enable RX and TX */
	aue_csr_write_1(sc, AUE_CTL0, AUE_CTL0_RXSTAT_APPEND | AUE_CTL0_RX_ENB);
	AUE_SETBIT(sc, AUE_CTL0, AUE_CTL0_TX_ENB);
	AUE_SETBIT(sc, AUE_CTL2, AUE_CTL2_EP3_CLR);

	//mii_mediachg(mii);

	return rv;
}

static void
aue_uno_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const	un = ifp->if_softc;
	struct aue_softc * const sc = usbnet_softc(un);

	AUEHIST_FUNC();
	AUEHIST_CALLARGSN(5, "aue%jd: enter", device_unit(un->un_dev), 0, 0, 0);

	aue_csr_write_1(sc, AUE_CTL0, 0);
	aue_csr_write_1(sc, AUE_CTL1, 0);
	aue_reset(sc);
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(aue)
