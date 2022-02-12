/* $NetBSD: ihidev.c,v 1.28 2022/02/12 03:24:35 riastradh Exp $ */
/* $OpenBSD ihidev.c,v 1.13 2017/04/08 02:57:23 deraadt Exp $ */

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

/*
 * HID-over-i2c driver
 *
 * https://msdn.microsoft.com/en-us/library/windows/hardware/dn642101%28v=vs.85%29.aspx
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ihidev.c,v 1.28 2022/02/12 03:24:35 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/workqueue.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ihidev.h>

#include <dev/hid/hid.h>

#if defined(__i386__) || defined(__amd64__)
#  include "acpica.h"
#endif
#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#endif

#include "locators.h"

/* #define IHIDEV_DEBUG */

#ifdef IHIDEV_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

/* 7.2 */
enum {
	I2C_HID_CMD_DESCR	= 0x0,
	I2C_HID_CMD_RESET	= 0x1,
	I2C_HID_CMD_GET_REPORT	= 0x2,
	I2C_HID_CMD_SET_REPORT	= 0x3,
	I2C_HID_CMD_GET_IDLE	= 0x4,
	I2C_HID_CMD_SET_IDLE	= 0x5,
	I2C_HID_CMD_GET_PROTO	= 0x6,
	I2C_HID_CMD_SET_PROTO	= 0x7,
	I2C_HID_CMD_SET_POWER	= 0x8,

	/* pseudo commands */
	I2C_HID_REPORT_DESCR	= 0x100,
};

static int I2C_HID_POWER_ON	= 0x0;
static int I2C_HID_POWER_OFF	= 0x1;

static int	ihidev_match(device_t, cfdata_t, void *);
static void	ihidev_attach(device_t, device_t, void *);
static int	ihidev_detach(device_t, int);
CFATTACH_DECL_NEW(ihidev, sizeof(struct ihidev_softc),
    ihidev_match, ihidev_attach, ihidev_detach, NULL);

static bool	ihidev_intr_init(struct ihidev_softc *);
static void	ihidev_intr_fini(struct ihidev_softc *);

static bool	ihidev_suspend(device_t, const pmf_qual_t *);
static bool	ihidev_resume(device_t, const pmf_qual_t *);
static int	ihidev_hid_command(struct ihidev_softc *, int, void *, bool);
static int	ihidev_intr(void *);
static void	ihidev_work(struct work *, void *);
static int	ihidev_reset(struct ihidev_softc *, bool);
static int	ihidev_hid_desc_parse(struct ihidev_softc *);

static int	ihidev_maxrepid(void *, int);
static int	ihidev_print(void *, const char *);
static int	ihidev_submatch(device_t, cfdata_t, const int *, void *);

static bool	ihidev_acpi_get_info(struct ihidev_softc *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "PNP0C50" },
	{ .compat = "ACPI0C50" },
	{ .compat = "hid-over-i2c" },
	DEVICE_COMPAT_EOL
};

static int
ihidev_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args * const ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return I2C_MATCH_DIRECT_COMPATIBLE;

	return 0;
}

static void
ihidev_attach(device_t parent, device_t self, void *aux)
{
	struct ihidev_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	struct ihidev_attach_arg iha;
	device_t dev;
	int repid, repsz;
	int isize;
	int locs[IHIDBUSCF_NLOCS];

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_phandle = ia->ia_cookie;
	if (ia->ia_cookietype != I2C_COOKIE_ACPI) {
		aprint_error(": unsupported device tree type\n");
		return;
	}
	if (!ihidev_acpi_get_info(sc)) {
		return;
	}

	if (ihidev_hid_command(sc, I2C_HID_CMD_DESCR, NULL, false) ||
	    ihidev_hid_desc_parse(sc)) {
		aprint_error(": failed fetching initial HID descriptor\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": vendor 0x%x product 0x%x, %s\n",
	    le16toh(sc->hid_desc.wVendorID), le16toh(sc->hid_desc.wProductID),
	    ia->ia_name);

	sc->sc_nrepid = ihidev_maxrepid(sc->sc_report, sc->sc_reportlen);
	if (sc->sc_nrepid < 0)
		return;

	aprint_normal_dev(self, "%d report id%s\n", sc->sc_nrepid,
	    sc->sc_nrepid > 1 ? "s" : "");

	sc->sc_nrepid++;
	sc->sc_subdevs = kmem_zalloc(sc->sc_nrepid * sizeof(struct ihidev *),
	    KM_SLEEP);

	/* find largest report size and allocate memory for input buffer */
	sc->sc_isize = le16toh(sc->hid_desc.wMaxInputLength);
	for (repid = 0; repid < sc->sc_nrepid; repid++) {
		repsz = hid_report_size(sc->sc_report, sc->sc_reportlen,
		    hid_input, repid);

		isize = repsz + 2; /* two bytes for the length */
		isize += (sc->sc_nrepid != 1); /* one byte for the report ID */
		if (isize > sc->sc_isize)
			sc->sc_isize = isize;

		DPRINTF(("%s: repid %d size %d\n",
		    device_xname(sc->sc_dev), repid, repsz));
	}
	sc->sc_ibuf = kmem_zalloc(sc->sc_isize, KM_SLEEP);
	if (!ihidev_intr_init(sc)) {
		return;
	}

	iha.iaa = ia;
	iha.parent = sc;

	/* Look for a driver claiming all report IDs first. */
	iha.reportid = IHIDEV_CLAIM_ALLREPORTID;
	locs[IHIDBUSCF_REPORTID] = IHIDEV_CLAIM_ALLREPORTID;
	dev = config_found(self, &iha, ihidev_print,
	    CFARGS(.submatch = ihidev_submatch,
		   .locators = locs));
	if (dev != NULL) {
		for (repid = 0; repid < sc->sc_nrepid; repid++)
			sc->sc_subdevs[repid] = device_private(dev);
		return;
	}

	for (repid = 0; repid < sc->sc_nrepid; repid++) {
		if (hid_report_size(sc->sc_report, sc->sc_reportlen, hid_input,
		    repid) == 0 &&
		    hid_report_size(sc->sc_report, sc->sc_reportlen,
		    hid_output, repid) == 0 &&
		    hid_report_size(sc->sc_report, sc->sc_reportlen,
		    hid_feature, repid) == 0)
			continue;

		iha.reportid = repid;
		locs[IHIDBUSCF_REPORTID] = repid;
		dev = config_found(self, &iha, ihidev_print,
		    CFARGS(.submatch = ihidev_submatch,
			   .locators = locs));
		sc->sc_subdevs[repid] = device_private(dev);
	}

	/* power down until we're opened */
	if (ihidev_hid_command(sc, I2C_HID_CMD_SET_POWER, &I2C_HID_POWER_OFF,
		false)) {
		aprint_error_dev(sc->sc_dev, "failed to power down\n");
		return;
	}
	if (!pmf_device_register(self, ihidev_suspend, ihidev_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
ihidev_detach(device_t self, int flags)
{
	struct ihidev_softc *sc = device_private(self);
	int error;

	error = config_detach_children(self, flags);
	if (error)
		return error;

	pmf_device_deregister(self);
	ihidev_intr_fini(sc);

	if (ihidev_hid_command(sc, I2C_HID_CMD_SET_POWER, &I2C_HID_POWER_OFF,
		true))
		aprint_error_dev(sc->sc_dev, "failed to power down\n");

	if (sc->sc_ibuf != NULL) {
		kmem_free(sc->sc_ibuf, sc->sc_isize);
		sc->sc_ibuf = NULL;
	}

	if (sc->sc_report != NULL)
		kmem_free(sc->sc_report, sc->sc_reportlen);

	mutex_destroy(&sc->sc_lock);

	return 0;
}

static bool
ihidev_suspend(device_t self, const pmf_qual_t *q)
{
	struct ihidev_softc *sc = device_private(self);

	mutex_enter(&sc->sc_lock);
	if (sc->sc_refcnt > 0) {
		printf("ihidev power off\n");
		if (ihidev_hid_command(sc, I2C_HID_CMD_SET_POWER,
		    &I2C_HID_POWER_OFF, true))
		aprint_error_dev(sc->sc_dev, "failed to power down\n");
	}
	mutex_exit(&sc->sc_lock);
	return true;
}

static bool
ihidev_resume(device_t self, const pmf_qual_t *q)
{
	struct ihidev_softc *sc = device_private(self);

	mutex_enter(&sc->sc_lock);
	if (sc->sc_refcnt > 0) {
		printf("ihidev power reset\n");
		ihidev_reset(sc, true);
	}
	mutex_exit(&sc->sc_lock);
	return true;
}

static int
ihidev_hid_command(struct ihidev_softc *sc, int hidcmd, void *arg, bool poll)
{
	int i, res = 1;
	int flags = poll ? I2C_F_POLL : 0;

	iic_acquire_bus(sc->sc_tag, flags);

	switch (hidcmd) {
	case I2C_HID_CMD_DESCR: {
		/*
		 * 5.2.2 - HID Descriptor Retrieval
		 * register is passed from the controller
		 */
		uint8_t cmd[] = {
			htole16(sc->sc_hid_desc_addr) & 0xff,
			htole16(sc->sc_hid_desc_addr) >> 8,
		};

		DPRINTF(("%s: HID command I2C_HID_CMD_DESCR at 0x%x\n",
		    device_xname(sc->sc_dev), htole16(sc->sc_hid_desc_addr)));

		/* 20 00 */
		res = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
		    &cmd, sizeof(cmd), &sc->hid_desc_buf,
		    sizeof(struct i2c_hid_desc), flags);

		DPRINTF(("%s: HID descriptor:", device_xname(sc->sc_dev)));
		for (i = 0; i < sizeof(struct i2c_hid_desc); i++)
			DPRINTF((" %.2x", sc->hid_desc_buf[i]));
		DPRINTF(("\n"));

		break;
	}
	case I2C_HID_CMD_RESET: {
		uint8_t cmd[] = {
			htole16(sc->hid_desc.wCommandRegister) & 0xff,
			htole16(sc->hid_desc.wCommandRegister) >> 8,
			0,
			I2C_HID_CMD_RESET,
		};

		DPRINTF(("%s: HID command I2C_HID_CMD_RESET\n",
		    device_xname(sc->sc_dev)));

		/* 22 00 00 01 */
		res = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
		    &cmd, sizeof(cmd), NULL, 0, flags);

		break;
	}
	case I2C_HID_CMD_GET_REPORT: {
		struct i2c_hid_report_request *rreq =
		    (struct i2c_hid_report_request *)arg;

		uint8_t cmd[] = {
			htole16(sc->hid_desc.wCommandRegister) & 0xff,
			htole16(sc->hid_desc.wCommandRegister) >> 8,
			0,
			I2C_HID_CMD_GET_REPORT,
			0, 0, 0,
		};
		int cmdlen = 7;
		int dataoff = 4;
		int report_id = rreq->id;
		int report_id_len = 1;
		int report_len = rreq->len + 2;
		int d;
		uint8_t *tmprep;

		DPRINTF(("%s: HID command I2C_HID_CMD_GET_REPORT %d "
		    "(type %d, len %d)\n", device_xname(sc->sc_dev), report_id,
		    rreq->type, rreq->len));

		/*
		 * 7.2.2.4 - "The protocol is optimized for Report < 15.  If a
		 * report ID >= 15 is necessary, then the Report ID in the Low
		 * Byte must be set to 1111 and a Third Byte is appended to the
		 * protocol.  This Third Byte contains the entire/actual report
		 * ID."
		 */
		if (report_id >= 15) {
			cmd[dataoff++] = report_id;
			report_id = 15;
			report_id_len = 2;
		} else
			cmdlen--;

		cmd[2] = report_id | rreq->type << 4;

		cmd[dataoff++] = sc->hid_desc.wDataRegister & 0xff;
		cmd[dataoff] = sc->hid_desc.wDataRegister >> 8;

		/*
		 * 7.2.2.2 - Response will be a 2-byte length value, the report
		 * id with length determined above, and then the report.
		 * Allocate rreq->len + 2 + 2 bytes, read into that temporary
		 * buffer, and then copy only the report back out to
		 * rreq->data.
		 */
		report_len += report_id_len;
		tmprep = kmem_zalloc(report_len, KM_NOSLEEP);
		if (tmprep == NULL) {
			/* XXX pool or preallocate? */
			DPRINTF(("%s: out of memory\n",
				device_xname(sc->sc_dev)));
			res = ENOMEM;
			break;
		}

		/* type 3 id 8: 22 00 38 02 23 00 */
		res = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
		    &cmd, cmdlen, tmprep, report_len, flags);

		d = tmprep[0] | tmprep[1] << 8;
		if (d != report_len) {
			DPRINTF(("%s: response size %d != expected length %d\n",
			    device_xname(sc->sc_dev), d, report_len));
		}

		if (report_id_len == 2)
			d = tmprep[2] | tmprep[3] << 8;
		else
			d = tmprep[2];

		if (d != rreq->id) {
			DPRINTF(("%s: response report id %d != %d\n",
			    device_xname(sc->sc_dev), d, rreq->id));
			iic_release_bus(sc->sc_tag, 0);
			kmem_free(tmprep, report_len);
			return (1);
		}

		DPRINTF(("%s: response:", device_xname(sc->sc_dev)));
		for (i = 0; i < report_len; i++)
			DPRINTF((" %.2x", tmprep[i]));
		DPRINTF(("\n"));

		memcpy(rreq->data, tmprep + 2 + report_id_len, rreq->len);
		kmem_free(tmprep, report_len);

		break;
	}
	case I2C_HID_CMD_SET_REPORT: {
		struct i2c_hid_report_request *rreq =
		    (struct i2c_hid_report_request *)arg;

		uint8_t cmd[] = {
			htole16(sc->hid_desc.wCommandRegister) & 0xff,
			htole16(sc->hid_desc.wCommandRegister) >> 8,
			0,
			I2C_HID_CMD_SET_REPORT,
			0, 0, 0, 0, 0, 0,
		};
		int cmdlen = 10;
		int report_id = rreq->id;
		int report_len = 2 + (report_id ? 1 : 0) + rreq->len;
		int dataoff;
		uint8_t *finalcmd;

		DPRINTF(("%s: HID command I2C_HID_CMD_SET_REPORT %d "
		    "(type %d, len %d):", device_xname(sc->sc_dev), report_id,
		    rreq->type, rreq->len));
		for (i = 0; i < rreq->len; i++)
			DPRINTF((" %.2x", ((uint8_t *)rreq->data)[i]));
		DPRINTF(("\n"));

		/*
		 * 7.2.2.4 - "The protocol is optimized for Report < 15.  If a
		 * report ID >= 15 is necessary, then the Report ID in the Low
		 * Byte must be set to 1111 and a Third Byte is appended to the
		 * protocol.  This Third Byte contains the entire/actual report
		 * ID."
		 */
		dataoff = 4;
		if (report_id >= 15) {
			cmd[dataoff++] = report_id;
			report_id = 15;
		} else
			cmdlen--;

		cmd[2] = report_id | rreq->type << 4;

		if (rreq->type == I2C_HID_REPORT_TYPE_FEATURE) {
			cmd[dataoff++] = htole16(sc->hid_desc.wDataRegister)
			    & 0xff;
			cmd[dataoff++] = htole16(sc->hid_desc.wDataRegister)
			    >> 8;
		} else {
			cmd[dataoff++] = htole16(sc->hid_desc.wOutputRegister)
			    & 0xff;
			cmd[dataoff++] = htole16(sc->hid_desc.wOutputRegister)
			    >> 8;
		}

		cmd[dataoff++] = report_len & 0xff;
		cmd[dataoff++] = report_len >> 8;
		cmd[dataoff] = rreq->id;

		finalcmd = kmem_zalloc(cmdlen + rreq->len, KM_NOSLEEP);
		if (finalcmd == NULL) {
			res = ENOMEM;
			break;
		}

		memcpy(finalcmd, cmd, cmdlen);
		memcpy(finalcmd + cmdlen, rreq->data, rreq->len);

		/* type 3 id 4: 22 00 34 03 23 00 04 00 04 03 */
		res = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
		    finalcmd, cmdlen + rreq->len, NULL, 0, flags);
		kmem_free(finalcmd, cmdlen + rreq->len);

 		break;
 	}

	case I2C_HID_CMD_SET_POWER: {
		int power = *(int *)arg;
		uint8_t cmd[] = {
			htole16(sc->hid_desc.wCommandRegister) & 0xff,
			htole16(sc->hid_desc.wCommandRegister) >> 8,
			power,
			I2C_HID_CMD_SET_POWER,
		};

		DPRINTF(("%s: HID command I2C_HID_CMD_SET_POWER(%d)\n",
		    device_xname(sc->sc_dev), power));

		/* 22 00 00 08 */
		res = iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
		    &cmd, sizeof(cmd), NULL, 0, flags);

		break;
	}
	case I2C_HID_REPORT_DESCR: {
		uint8_t cmd[] = {
			htole16(sc->hid_desc.wReportDescRegister) & 0xff,
			htole16(sc->hid_desc.wReportDescRegister) >> 8,
		};

		DPRINTF(("%s: HID command I2C_HID_REPORT_DESCR at 0x%x with "
		    "size %d\n", device_xname(sc->sc_dev), cmd[0],
		    sc->sc_reportlen));

		/* 20 00 */
		res = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
		    &cmd, sizeof(cmd), sc->sc_report, sc->sc_reportlen, flags);

		DPRINTF(("%s: HID report descriptor:",
		    device_xname(sc->sc_dev)));
		for (i = 0; i < sc->sc_reportlen; i++)
			DPRINTF((" %.2x", sc->sc_report[i]));
		DPRINTF(("\n"));

		break;
	}
	default:
		aprint_error_dev(sc->sc_dev, "unknown command %d\n",
		    hidcmd);
	}

	iic_release_bus(sc->sc_tag, flags);

	return (res);
}

static int
ihidev_reset(struct ihidev_softc *sc, bool poll)
{
	DPRINTF(("%s: resetting\n", device_xname(sc->sc_dev)));

	if (ihidev_hid_command(sc, I2C_HID_CMD_SET_POWER,
	    &I2C_HID_POWER_ON, poll)) {
		aprint_error_dev(sc->sc_dev, "failed to power on\n");
		return (1);
	}

	DELAY(1000);

	if (ihidev_hid_command(sc, I2C_HID_CMD_RESET, 0, poll)) {
		aprint_error_dev(sc->sc_dev, "failed to reset hardware\n");

		ihidev_hid_command(sc, I2C_HID_CMD_SET_POWER,
		    &I2C_HID_POWER_OFF, poll);

		return (1);
	}

	DELAY(1000);

	return (0);
}

/*
 * 5.2.2 - HID Descriptor Retrieval
 *
 * parse HID Descriptor that has already been read into hid_desc with
 * I2C_HID_CMD_DESCR
 */
static int
ihidev_hid_desc_parse(struct ihidev_softc *sc)
{
	int retries = 3;

	/* must be v01.00 */
	if (le16toh(sc->hid_desc.bcdVersion) != 0x0100) {
		aprint_error_dev(sc->sc_dev,
		    "bad HID descriptor bcdVersion (0x%x)\n",
		    le16toh(sc->hid_desc.bcdVersion));
		return (1);
	}

	/* must be 30 bytes for v1.00 */
	if (le16toh(sc->hid_desc.wHIDDescLength !=
	    sizeof(struct i2c_hid_desc))) {
		aprint_error_dev(sc->sc_dev,
		    "bad HID descriptor size (%d != %zu)\n",
		    le16toh(sc->hid_desc.wHIDDescLength),
		    sizeof(struct i2c_hid_desc));
		return (1);
	}

	if (le16toh(sc->hid_desc.wReportDescLength) <= 0) {
		aprint_error_dev(sc->sc_dev,
		    "bad HID report descriptor size (%d)\n",
		    le16toh(sc->hid_desc.wReportDescLength));
		return (1);
	}

	while (retries-- > 0) {
		if (ihidev_reset(sc, false)) {
			if (retries == 0)
				return(1);

			DELAY(1000);
		}
		else
			break;
	}

	sc->sc_reportlen = le16toh(sc->hid_desc.wReportDescLength);
	sc->sc_report = kmem_zalloc(sc->sc_reportlen, KM_SLEEP);

	if (ihidev_hid_command(sc, I2C_HID_REPORT_DESCR, 0, false)) {
		aprint_error_dev(sc->sc_dev, "failed fetching HID report\n");
		return (1);
	}

	return (0);
}

static bool
ihidev_intr_init(struct ihidev_softc *sc)
{
#if NACPICA > 0
	ACPI_HANDLE hdl = (void *)(uintptr_t)sc->sc_phandle;
	struct acpi_resources res;
	ACPI_STATUS rv;
	char buf[100];

	rv = acpi_resource_parse(sc->sc_dev, hdl, "_CRS", &res,
	    &acpi_resource_parse_ops_quiet);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "can't parse '_CRS'\n");
		return false;
	}

	const struct acpi_irq * const irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(sc->sc_dev, "no IRQ resource\n");
		acpi_resource_cleanup(&res);
		return false;
	}

	sc->sc_intr_type =
	    irq->ar_type == ACPI_EDGE_SENSITIVE ? IST_EDGE : IST_LEVEL;

	acpi_resource_cleanup(&res);

	sc->sc_ih = acpi_intr_establish(sc->sc_dev, sc->sc_phandle, IPL_TTY,
	    false, ihidev_intr, sc, device_xname(sc->sc_dev));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev, "can't establish interrupt\n");
		return false;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting at %s\n",
	    acpi_intr_string(sc->sc_ih, buf, sizeof(buf)));

	if (workqueue_create(&sc->sc_wq, device_xname(sc->sc_dev), ihidev_work,
		sc, PRI_NONE, IPL_TTY, WQ_MPSAFE)) {
		aprint_error_dev(sc->sc_dev,
		    "can't establish workqueue\n");
		return false;
	}
	sc->sc_work_pending = 0;

	return true;
#else
	aprint_error_dev(sc->sc_dev, "can't establish interrupt\n");
	return false;
#endif
}

static void
ihidev_intr_fini(struct ihidev_softc *sc)
{
#if NACPICA > 0
	if (sc->sc_ih != NULL) {
		acpi_intr_disestablish(sc->sc_ih);
	}
	if (sc->sc_wq != NULL) {
		workqueue_destroy(sc->sc_wq);
	}
#endif
}

static void
ihidev_intr_mask(struct ihidev_softc * const sc)
{

	if (sc->sc_intr_type == IST_LEVEL) {
#if NACPICA > 0
		acpi_intr_mask(sc->sc_ih);
#endif
	}
}

static void
ihidev_intr_unmask(struct ihidev_softc * const sc)
{

	if (sc->sc_intr_type == IST_LEVEL) {
#if NACPICA > 0
		acpi_intr_unmask(sc->sc_ih);
#endif
	}
}

static int
ihidev_intr(void *arg)
{
	struct ihidev_softc * const sc = arg;

	/*
	 * Schedule our work.  If we're using a level-triggered
	 * interrupt, we have to mask it off while we wait for service.
	 */
	if (atomic_swap_uint(&sc->sc_work_pending, 1) == 0)
		workqueue_enqueue(sc->sc_wq, &sc->sc_work, NULL);
	ihidev_intr_mask(sc);

	return 1;
}

static void
ihidev_work(struct work *wk, void *arg)
{
	struct ihidev_softc * const sc = arg;
	struct ihidev *scd;
	u_int psize;
	int res, i;
	u_char *p;
	u_int rep = 0;

	atomic_store_relaxed(&sc->sc_work_pending, 0);

	mutex_enter(&sc->sc_lock);

	iic_acquire_bus(sc->sc_tag, 0);
	res = iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr, NULL, 0,
	    sc->sc_ibuf, sc->sc_isize, 0);
	iic_release_bus(sc->sc_tag, 0);
	if (res != 0)
		goto out;

	/*
	 * 6.1.1 - First two bytes are the packet length, which must be less
	 * than or equal to wMaxInputLength
	 */
	psize = sc->sc_ibuf[0] | sc->sc_ibuf[1] << 8;
	if (!psize || psize > sc->sc_isize) {
		DPRINTF(("%s: %s: invalid packet size (%d vs. %d)\n",
		    device_xname(sc->sc_dev), __func__, psize, sc->sc_isize));
		goto out;
	}

	/* 3rd byte is the report id */
	p = sc->sc_ibuf + 2;
	psize -= 2;
	if (sc->sc_nrepid != 1)
		rep = *p++, psize--;

	if (rep >= sc->sc_nrepid) {
		aprint_error_dev(sc->sc_dev, "%s: bad report id %d\n",
		    __func__, rep);
		goto out;
	}

	DPRINTF(("%s: %s: hid input (rep %d):", device_xname(sc->sc_dev),
	    __func__, rep));
	for (i = 0; i < sc->sc_isize; i++)
		DPRINTF((" %.2x", sc->sc_ibuf[i]));
	DPRINTF(("\n"));

	scd = sc->sc_subdevs[rep];
	if (scd == NULL || !(scd->sc_state & IHIDEV_OPEN))
		goto out;

	scd->sc_intr(scd, p, psize);

 out:
	mutex_exit(&sc->sc_lock);

	/*
	 * If our interrupt is level-triggered, re-enable it now.
	 */
	ihidev_intr_unmask(sc);
}

static int
ihidev_maxrepid(void *buf, int len)
{
	struct hid_data *d;
	struct hid_item h;
	int maxid;

	maxid = -1;
	h.report_ID = 0;
	for (d = hid_start_parse(buf, len, hid_none); hid_get_item(d, &h); )
		if ((int)h.report_ID > maxid)
			maxid = h.report_ID;
	hid_end_parse(d);

	return (maxid);
}

static int
ihidev_print(void *aux, const char *pnp)
{
	struct ihidev_attach_arg *iha = aux;

	if (iha->reportid == IHIDEV_CLAIM_ALLREPORTID)
		return (QUIET);

	if (pnp)
		aprint_normal("hid at %s", pnp);

	if (iha->reportid != 0)
		aprint_normal(" reportid %d", iha->reportid);

	return (UNCONF);
}

static int
ihidev_submatch(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	struct ihidev_attach_arg *iha = aux;

	if (cf->ihidevcf_reportid != IHIDEV_UNK_REPORTID &&
	    cf->ihidevcf_reportid != iha->reportid)
		return (0);

	return config_match(parent, cf, aux);
}

int
ihidev_open(struct ihidev *scd)
{
	struct ihidev_softc *sc = scd->sc_parent;
	int error;

	DPRINTF(("%s: %s: state=%d refcnt=%d\n", device_xname(sc->sc_dev),
	    __func__, scd->sc_state, sc->sc_refcnt));

	mutex_enter(&sc->sc_lock);

	if (scd->sc_state & IHIDEV_OPEN || sc->sc_refcnt == INT_MAX) {
		error = EBUSY;
		goto out;
	}

	scd->sc_state |= IHIDEV_OPEN;

	if (sc->sc_refcnt++ || sc->sc_isize == 0) {
		error = 0;
		goto out;
	}

	/* power on */
	ihidev_reset(sc, false);
	error = 0;

out:	mutex_exit(&sc->sc_lock);
	return error;
}

void
ihidev_close(struct ihidev *scd)
{
	struct ihidev_softc *sc = scd->sc_parent;

	DPRINTF(("%s: %s: state=%d refcnt=%d\n", device_xname(sc->sc_dev),
	    __func__, scd->sc_state, sc->sc_refcnt));

	mutex_enter(&sc->sc_lock);

	KASSERTMSG(scd->sc_state & IHIDEV_OPEN,
	    "%s: closing %s when not open",
	    device_xname(scd->sc_idev),
	    device_xname(sc->sc_dev));
	scd->sc_state &= ~IHIDEV_OPEN;

	if (--sc->sc_refcnt)
		goto out;

	if (ihidev_hid_command(sc, I2C_HID_CMD_SET_POWER,
	    &I2C_HID_POWER_OFF, false))
		aprint_error_dev(sc->sc_dev, "failed to power down\n");

out:	mutex_exit(&sc->sc_lock);
}

void
ihidev_get_report_desc(struct ihidev_softc *sc, void **desc, int *size)
{
	*desc = sc->sc_report;
	*size = sc->sc_reportlen;
}

/* convert hid_* constants used throughout HID code to i2c HID equivalents */
int
ihidev_report_type_conv(int hid_type_id)
{
	switch (hid_type_id) {
	case hid_input:
		return I2C_HID_REPORT_TYPE_INPUT;
	case hid_output:
		return I2C_HID_REPORT_TYPE_OUTPUT;
	case hid_feature:
		return I2C_HID_REPORT_TYPE_FEATURE;
	default:
		return -1;
	}
}

int
ihidev_get_report(struct device *dev, int type, int id, void *data, int len)
{
	struct ihidev_softc *sc = (struct ihidev_softc *)dev;
	struct i2c_hid_report_request rreq;
	int ctype;

	if ((ctype = ihidev_report_type_conv(type)) < 0)
		return (1);

	rreq.type = ctype;
	rreq.id = id;
	rreq.data = data;
	rreq.len = len;

	if (ihidev_hid_command(sc, I2C_HID_CMD_GET_REPORT, &rreq, false)) {
		aprint_error_dev(sc->sc_dev, "failed fetching report\n");
		return (1);
	}

	return 0;
}

int
ihidev_set_report(struct device *dev, int type, int id, void *data,
    int len)
{
	struct ihidev_softc *sc = (struct ihidev_softc *)dev;
	struct i2c_hid_report_request rreq;
	int ctype;

	if ((ctype = ihidev_report_type_conv(type)) < 0)
		return (1);

	rreq.type = ctype;
	rreq.id = id;
	rreq.data = data;
	rreq.len = len;

	if (ihidev_hid_command(sc, I2C_HID_CMD_SET_REPORT, &rreq, false)) {
		aprint_error_dev(sc->sc_dev, "failed setting report\n");
		return (1);
	}

	return 0;
}

static bool
ihidev_acpi_get_info(struct ihidev_softc *sc)
{
	ACPI_HANDLE hdl = (void *)(uintptr_t)sc->sc_phandle;
	ACPI_STATUS status;
	ACPI_INTEGER val;

	/* 3cdff6f7-4267-4555-ad05-b30a3d8938de */
	uint8_t i2c_hid_guid[] = {
		0xF7, 0xF6, 0xDF, 0x3C,
		0x67, 0x42,
		0x55, 0x45,
		0xAD, 0x05,
		0xB3, 0x0A, 0x3D, 0x89, 0x38, 0xDE,
	};

	status = acpi_dsm_integer(hdl, i2c_hid_guid, 1, 1, NULL, &val);
	if (ACPI_FAILURE(status)) {
		aprint_error_dev(sc->sc_dev,
		    "failed to get HidDescriptorAddress: %s\n",
		    AcpiFormatException(status));
		return false;
	}

	sc->sc_hid_desc_addr = (u_int)val;

	return true;
}
