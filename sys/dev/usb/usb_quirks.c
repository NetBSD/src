/*	$NetBSD: usb_quirks.c,v 1.92.2.3 2022/01/18 20:02:23 snj Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/usb_quirks.c,v 1.30 2003/01/02 04:15:55 imp Exp $	*/

/*
 * Copyright (c) 1998, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: usb_quirks.c,v 1.92.2.3 2022/01/18 20:02:23 snj Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbhist.h>
#include <dev/usb/usb_quirks.h>

#ifdef USB_DEBUG
extern int usbdebug;
#endif

#define DPRINTF(FMT,A,B,C,D)    USBHIST_LOG(usbdebug,FMT,A,B,C,D)

#define ANY 0xffff
#define _USETW(w) { (w) & 0x00ff, ((w) & 0xff00) >> 8 }

/*
 * NXP PN533 NFC chip descriptors
 */
static const usb_endpoint_descriptor_t desc_ep_pn533_in = {
	/* bLength */		sizeof(desc_ep_pn533_in),
	/* bDescriptorType */	UDESC_ENDPOINT,
	/* bEndpointAddress */	UE_DIR_IN | 0x04,
	/* bmAttributes */	UE_BULK,
	/* wMaxPacketSize */	_USETW(0x0040),
	/* bInterval */		0x04, /* 255ms */
};

static const usb_endpoint_descriptor_t desc_ep_pn533_out = {
	/* bLength */		sizeof(desc_ep_pn533_in),
	/* bDescriptorType */	UDESC_ENDPOINT,
	/* bEndpointAddress */	UE_DIR_OUT | 0x04,
	/* bmAttributes */	UE_BULK,
	/* wMaxPacketSize */	_USETW(0x0040),
	/* bInterval */		0x04, /* 255ms */
};

static const usb_interface_descriptor_t desc_iface_pn533 = {
	/* bLength */		sizeof(desc_iface_pn533),
	/* bDescriptorType */	 UDESC_INTERFACE,
	/* bInterfaceNumber */	 0,
	/* bAlternateSetting */	 0,
	/* bNumEndpoints */	 2,
	/* bInterfaceClass */	 0xff,
	/* bInterfaceSubClass */ 0xff,
	/* bInterfaceProtocol */ 0xff,
	/* iInterface */	 0,
};

static const usb_config_descriptor_t desc_conf_pn533 = {
	/* bLength */		 sizeof(desc_conf_pn533),
	/* bDescriptorType */	 UDESC_CONFIG,
	/* wTotalLength	 */	 _USETW(sizeof(desc_conf_pn533) +
					sizeof(desc_iface_pn533) +
					sizeof(desc_ep_pn533_in) +
					sizeof(desc_ep_pn533_out)
				 ),
	/* bNumInterfac	*/	 1,
	/* bConfigurationValue */1,
	/* iConfiguration */	 0,
	/* bmAttributes	*/	 UC_ATTR_MBO,
	/* bMaxPower */		 0x32, /* 100mA */
};

static const usb_descriptor_t *desc_pn533[] = {
	(const usb_descriptor_t *)&desc_conf_pn533,
	(const usb_descriptor_t *)&desc_iface_pn533,
	(const usb_descriptor_t *)&desc_ep_pn533_out,
	(const usb_descriptor_t *)&desc_ep_pn533_in,
	NULL
};


usbd_status
usbd_get_desc_fake(struct usbd_device *dev, int type, int index,
		   int len, void *desc)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
#ifdef USB_DEBUG
	const usb_device_descriptor_t *dd = usbd_get_device_descriptor(dev);
#endif
	const usb_descriptor_t *ub;
	int i = 0;
	int j = 0;
	usbd_status err = USBD_INVAL;

	if (dev->ud_quirks == NULL || dev->ud_quirks->desc == NULL) {
		DPRINTF("%04x/%04x: no fake descriptors",
		        UGETW(dd->idVendor), UGETW(dd->idProduct), 0, 0);
		goto out;
	}

	for (j = 0; dev->ud_quirks->desc[j]; j++) {
		ub = dev->ud_quirks->desc[j];
		if (ub->bDescriptorType == type && i++ == index)
			break;
	}

	if (dev->ud_quirks->desc[j] == NULL) {
		DPRINTF("%04x/%04x: no fake descriptor type = %d, len = %d",
		       UGETW(dd->idVendor), UGETW(dd->idProduct), type, len);
		goto out;
	}

	do {
		ub = dev->ud_quirks->desc[j];

		if (ub->bLength > len) {
			DPRINTF("%04x/%04x: short buf len = %d, bLength = %d",
			        UGETW(dd->idVendor), UGETW(dd->idProduct),
			        type, ub->bLength);
			goto out;
		}

		memcpy(desc, ub, ub->bLength);
		DPRINTF("%04x/%04x: Use fake descriptor type %d",
			UGETW(dd->idVendor), UGETW(dd->idProduct),
			type, 0);

		desc = (char *)desc + ub->bLength;
		len -= ub->bLength;
		j++;
	} while (len && dev->ud_quirks->desc[j] &&
		 dev->ud_quirks->desc[j]->bDescriptorType != type);

	err = USBD_NORMAL_COMPLETION;

	DPRINTF("%04x/%04x: Using fake USB descriptors\n",
	        UGETW(dd->idVendor), UGETW(dd->idProduct), 0, 0);
out:
	DPRINTF("return err = %d", err, 0, 0, 0);
	return err;
}

Static const struct usbd_quirk_entry {
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	struct usbd_quirks quirks;
} usb_quirks[] = {
 /* Devices which should be ignored by uhid */
 { USB_VENDOR_APC,		USB_PRODUCT_APC_UPS,			ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_APC,		USB_PRODUCT_APC_UPS3,			ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_CYBERPOWER,	USB_PRODUCT_CYBERPOWER_UPS0,		ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_CYBERPOWER,	USB_PRODUCT_CYBERPOWER_UPS,		ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_CYBERPOWER,	USB_PRODUCT_CYBERPOWER_UPS2,		ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_GRETAGMACBETH,	ANY,					ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_MGE,		USB_PRODUCT_MGE_UPS1,			ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_MGE,		USB_PRODUCT_MGE_UPS2,			ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_MICROCHIP,	USB_PRODUCT_MICROCHIP_PICKIT1,		ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_MICROCHIP,	USB_PRODUCT_MICROCHIP_PICKIT2,		ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_MICROCHIP,	USB_PRODUCT_MICROCHIP_PICKIT3,		ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_TRIPPLITE2,	ANY,					ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_MISC,		USB_PRODUCT_MISC_WISPY_24X,		ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_WELTREND,	USB_PRODUCT_WELTREND_HID,		ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_SILABS,		USB_PRODUCT_SILABS_EC3,			ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_TI,		USB_PRODUCT_TI_MSP430,			ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_XRITE,		ANY,					ANY,
	{ UQ_HID_IGNORE, NULL }},
 { USB_VENDOR_KYE,		USB_PRODUCT_KYE_NICHE,			0x100,
	{ UQ_NO_SET_PROTO, NULL }},
 { USB_VENDOR_INSIDEOUT,	USB_PRODUCT_INSIDEOUT_EDGEPORT4,	0x094,
	{ UQ_SWAP_UNICODE, NULL }},
 { USB_VENDOR_DALLAS,		USB_PRODUCT_DALLAS_J6502,		0x0a2,
	{ UQ_BAD_ADC, NULL }},
 { USB_VENDOR_DALLAS,		USB_PRODUCT_DALLAS_J6502,		0x0a2,
	{ UQ_AU_NO_XU, NULL }},
 { USB_VENDOR_ALTEC,		USB_PRODUCT_ALTEC_ADA70,		0x103,
	{ UQ_BAD_ADC, NULL }},
 { USB_VENDOR_ALTEC,		USB_PRODUCT_ALTEC_ASC495,		0x000,
	{ UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_SONY,		USB_PRODUCT_SONY_PS2EYETOY4,		0x000,
	{ UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_SONY,		USB_PRODUCT_SONY_PS2EYETOY5,		0x000,
	{ UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_PHILIPS,		USB_PRODUCT_PHILIPS_PCVC740K,		ANY,
	{ UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_LOGITECH,		USB_PRODUCT_LOGITECH_QUICKCAMPRONB,	0x000,
	{ UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_LOGITECH,		USB_PRODUCT_LOGITECH_QUICKCAMPRO4K,	0x000,
	{ UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_LOGITECH,		USB_PRODUCT_LOGITECH_QUICKCAMMESS,	0x100,
	{ UQ_BAD_ADC, NULL }},
 { USB_VENDOR_QTRONIX,		USB_PRODUCT_QTRONIX_980N,		0x110,
	{ UQ_SPUR_BUT_UP, NULL }},
 { USB_VENDOR_ALCOR2,		USB_PRODUCT_ALCOR2_KBD_HUB,		0x001,
	{ UQ_SPUR_BUT_UP, NULL }},
 { USB_VENDOR_METRICOM,		USB_PRODUCT_METRICOM_RICOCHET_GS,	0x100,
	{ UQ_ASSUME_CM_OVER_DATA, NULL }},
 { USB_VENDOR_SANYO,		USB_PRODUCT_SANYO_SCP4900,		0x000,
	{ UQ_ASSUME_CM_OVER_DATA, NULL }},
 { USB_VENDOR_MOTOROLA2,	USB_PRODUCT_MOTOROLA2_T720C,		0x001,
	{ UQ_ASSUME_CM_OVER_DATA, NULL }},
 { USB_VENDOR_EICON,		USB_PRODUCT_EICON_DIVA852,		0x100,
	{ UQ_ASSUME_CM_OVER_DATA, NULL }},
 { USB_VENDOR_SIEMENS2,		USB_PRODUCT_SIEMENS2_MC75,		0x000,
	{ UQ_ASSUME_CM_OVER_DATA, NULL }},
 { USB_VENDOR_TELEX,		USB_PRODUCT_TELEX_MIC1,			0x009,
	{ UQ_AU_NO_FRAC, NULL }},
 { USB_VENDOR_SILICONPORTALS,	USB_PRODUCT_SILICONPORTALS_YAPPHONE,	0x100,
	{ UQ_AU_INP_ASYNC, NULL }},
 { USB_VENDOR_AVANCELOGIC,	USB_PRODUCT_AVANCELOGIC_USBAUDIO,	0x101,
	{ UQ_AU_INP_ASYNC, NULL }},
 { USB_VENDOR_PLANTRONICS,	USB_PRODUCT_PLANTRONICS_HEADSET,	0x004,
	{ UQ_AU_INP_ASYNC, NULL }},
 { USB_VENDOR_CMEDIA,		USB_PRODUCT_CMEDIA_USBAUDIO,		ANY,
	{ UQ_AU_INP_ASYNC, NULL }},

 /* XXX These should have a revision number, but I don't know what they are. */
 { USB_VENDOR_HP,		USB_PRODUCT_HP_895C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_HP,		USB_PRODUCT_HP_880C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_HP,		USB_PRODUCT_HP_815C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_HP,		USB_PRODUCT_HP_810C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_HP,		USB_PRODUCT_HP_830C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_HP,		USB_PRODUCT_HP_885C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_HP,		USB_PRODUCT_HP_840C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_HP,		USB_PRODUCT_HP_816C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_HP,		USB_PRODUCT_HP_959C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_MTK,		USB_PRODUCT_MTK_GPS_RECEIVER,		ANY,
	{ UQ_NO_UNION_NRM, NULL }},
 { USB_VENDOR_NEC,		USB_PRODUCT_NEC_PICTY900,		ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_NEC,		USB_PRODUCT_NEC_PICTY760,		ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_NEC,		USB_PRODUCT_NEC_PICTY920,		ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_NEC,		USB_PRODUCT_NEC_PICTY800,		ANY,
	{ UQ_BROKEN_BIDIR, NULL }},
 { USB_VENDOR_HP,		USB_PRODUCT_HP_1220C,			ANY,
	{ UQ_BROKEN_BIDIR, NULL }},

 /* Apple internal notebook ISO keyboards have swapped keys */
 { USB_VENDOR_APPLE,		USB_PRODUCT_APPLE_FOUNTAIN_ISO,		ANY,
	{ UQ_APPLE_ISO, NULL }},
 { USB_VENDOR_APPLE,		USB_PRODUCT_APPLE_GEYSER_ISO,		ANY,
	{ UQ_APPLE_ISO, NULL }},

 /* HID and audio are both invalid on iPhone/iPod Touch */
 { USB_VENDOR_APPLE,		USB_PRODUCT_APPLE_IPHONE,		ANY,
	{ UQ_HID_IGNORE | UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_APPLE,		USB_PRODUCT_APPLE_IPOD_TOUCH,		ANY,
	{ UQ_HID_IGNORE | UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_APPLE,		USB_PRODUCT_APPLE_IPOD_TOUCH_4G,	ANY,
	{ UQ_HID_IGNORE | UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_APPLE,		USB_PRODUCT_APPLE_IPHONE_3G,		ANY,
	{ UQ_HID_IGNORE | UQ_BAD_AUDIO, NULL }},
 { USB_VENDOR_APPLE,		USB_PRODUCT_APPLE_IPHONE_3GS,		ANY,
	{ UQ_HID_IGNORE | UQ_BAD_AUDIO, NULL }},

 { USB_VENDOR_LG,		USB_PRODUCT_LG_CDMA_MSM,		ANY,
	{ UQ_ASSUME_CM_OVER_DATA, NULL }},
 { USB_VENDOR_QUALCOMM2,	USB_PRODUCT_QUALCOMM2_CDMA_MSM,		ANY,
	{ UQ_ASSUME_CM_OVER_DATA, NULL }},
 { USB_VENDOR_HYUNDAI,		USB_PRODUCT_HYUNDAI_UM175,		ANY,
	{ UQ_ASSUME_CM_OVER_DATA, NULL }},
 { USB_VENDOR_ZOOM,		USB_PRODUCT_ZOOM_3095,			ANY,
	{ UQ_LOST_CS_DESC, NULL }},

 /*
  * NXP PN533 bugs
  * 
  * 1. It corrupts its USB descriptors. The quirk is to provide hardcoded
  *    descriptors instead of getting them from the device.
  * 2. It mishandles the USB toggle bit. This causes some replies to be
  *    filered out by the USB host controller and be reported as timed out.
  *    NFC tool's libnfc workaround this bug by sending a dummy frame to
  *    resync the toggle bit, but in order to succeed, that operation must
  *    not be reported as failed. The quirk is therefore to pretend to 
  *    userland that output timeouts are successes.
  */
 { USB_VENDOR_PHILIPSSEMI,	USB_PRODUCT_PHILIPSSEMI_PN533,		ANY,
	{ UQ_DESC_CORRUPT | UQ_MISS_OUT_ACK, desc_pn533 }},
 { USB_VENDOR_SHUTTLE,		USB_PRODUCT_SHUTTLE_SCL3711,		ANY,
	{ UQ_DESC_CORRUPT | UQ_MISS_OUT_ACK, desc_pn533 }},
 { USB_VENDOR_SHUTTLE,		USB_PRODUCT_SHUTTLE_SCL3712,		ANY,
	{ UQ_DESC_CORRUPT | UQ_MISS_OUT_ACK, desc_pn533 }},
 { 0, 0, 0, { 0, NULL } }
};

const struct usbd_quirks usbd_no_quirk = { 0 };

const struct usbd_quirks *
usbd_find_quirk(usb_device_descriptor_t *d)
{
	const struct usbd_quirk_entry *t;
	uint16_t vendor = UGETW(d->idVendor);
	uint16_t product = UGETW(d->idProduct);
	uint16_t revision = UGETW(d->bcdDevice);

	for (t = usb_quirks; t->idVendor != 0; t++) {
		if (t->idVendor == vendor &&
		    (t->idProduct == ANY || t->idProduct == product) &&
		    (t->bcdDevice == ANY || t->bcdDevice == revision))
			break;
	}
#ifdef USB_DEBUG
	if (usbdebug && t->quirks.uq_flags)
		printf("usbd_find_quirk 0x%04x/0x%04x/%x: %d\n",
			  UGETW(d->idVendor), UGETW(d->idProduct),
			  UGETW(d->bcdDevice), t->quirks.uq_flags);
#endif
	return &t->quirks;
}
