/* $NetBSD: ihidev.h,v 1.1 2017/12/10 17:05:54 bouyer Exp $ */
/* $OpenBSD ihidev.h,v 1.4 2016/01/31 18:24:35 jcs Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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

/*
 * HID-over-i2c driver
 *
 * Copyright (c) 2015, 2016 joshua stein <jcs@openbsd.org>
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

/* from usbdi.h: Match codes. */
/* First five codes is for a whole device. */
#define IMATCH_VENDOR_PRODUCT_REV			14
#define IMATCH_VENDOR_PRODUCT				13
#define IMATCH_VENDOR_DEVCLASS_DEVPROTO			12
#define IMATCH_DEVCLASS_DEVSUBCLASS_DEVPROTO		11
#define IMATCH_DEVCLASS_DEVSUBCLASS			10
/* Next six codes are for interfaces. */
#define IMATCH_VENDOR_PRODUCT_REV_CONF_IFACE		 9
#define IMATCH_VENDOR_PRODUCT_CONF_IFACE		 8
#define IMATCH_VENDOR_IFACESUBCLASS_IFACEPROTO		 7
#define IMATCH_VENDOR_IFACESUBCLASS			 6
#define IMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO	 5
#define IMATCH_IFACECLASS_IFACESUBCLASS			 4
#define IMATCH_IFACECLASS				 3
#define IMATCH_IFACECLASS_GENERIC			 2
/* Generic driver */
#define IMATCH_GENERIC					 1
/* No match */
#define IMATCH_NONE					 0

#define IHIDBUSCF_REPORTID				0
#define IHIDBUSCF_REPORTID_DEFAULT			-1

#define ihidevcf_reportid cf_loc[IHIDBUSCF_REPORTID]
#define IHIDEV_UNK_REPORTID IHIDBUSCF_REPORTID_DEFAULT

/* 5.1.1 - HID Descriptor Format */
struct i2c_hid_desc {
	uint16_t wHIDDescLength;
	uint16_t bcdVersion;
	uint16_t wReportDescLength;
	uint16_t wReportDescRegister;
	uint16_t wInputRegister;
	uint16_t wMaxInputLength;
	uint16_t wOutputRegister;
	uint16_t wMaxOutputLength;
	uint16_t wCommandRegister;
	uint16_t wDataRegister;
	uint16_t wVendorID;
	uint16_t wProductID;
	uint16_t wVersionID;
	uint32_t reserved;
} __packed;

struct ihidev_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_tag;
	i2c_addr_t	sc_addr;
	uint64_t	sc_phandle;

	void *		sc_ih;
	kmutex_t	sc_intr_lock;

	u_int		sc_hid_desc_addr;
	union {
		uint8_t	hid_desc_buf[sizeof(struct i2c_hid_desc)];
		struct i2c_hid_desc hid_desc;
	};

	uint8_t		*sc_report;
	int		sc_reportlen;

	int		sc_nrepid;
	struct		ihidev **sc_subdevs;

	u_int		sc_isize;
	u_char		*sc_ibuf;

	int		sc_refcnt;
};

struct ihidev {
	device_t	sc_idev;
	struct ihidev_softc *sc_parent;
	uint8_t		sc_report_id;
	uint8_t		sc_state;
#define	IHIDEV_OPEN	0x01	/* device is open */
	void		(*sc_intr)(struct ihidev *, void *, u_int);

	int		sc_isize;
	int		sc_osize;
	int		sc_fsize;
};

struct ihidev_attach_arg {
	struct i2c_attach_args	*iaa;
	struct ihidev_softc	*parent;
	uint8_t			 reportid;
#define	IHIDEV_CLAIM_ALLREPORTID	255
};

struct i2c_hid_report_request {
	u_int id;
	u_int type;
#define I2C_HID_REPORT_TYPE_INPUT	0x1
#define I2C_HID_REPORT_TYPE_OUTPUT	0x2
#define I2C_HID_REPORT_TYPE_FEATURE	0x3
	void *data;
	u_int len;
};

void ihidev_get_report_desc(struct ihidev_softc *, void **, int *);
int ihidev_open(struct ihidev *);
void ihidev_close(struct ihidev *);

int ihidev_set_report(device_t, int, int, void *, int);
int ihidev_get_report(device_t, int, int, void *, int);
int ihidev_report_type_conv(int);

