/* $NetBSD: ssdfb_i2c.c,v 1.11 2021/08/05 22:31:20 tnn Exp $ */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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
__KERNEL_RCSID(0, "$NetBSD: ssdfb_i2c.c,v 1.11 2021/08/05 22:31:20 tnn Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/i2c/i2cvar.h>
#include <dev/ic/ssdfbvar.h>

struct ssdfb_i2c_softc {
	struct		ssdfb_softc sc;
	i2c_tag_t	sc_i2c_tag;
	i2c_addr_t	sc_i2c_addr;
	size_t		sc_transfer_size;
};

static int	ssdfb_i2c_match(device_t, cfdata_t, void *);
static void	ssdfb_i2c_attach(device_t, device_t, void *);
static int	ssdfb_i2c_detach(device_t, int);

static int	ssdfb_i2c_probe_transfer_size(struct ssdfb_i2c_softc *, bool);
static int	ssdfb_i2c_transfer(struct ssdfb_i2c_softc *, uint8_t, uint8_t *,
				size_t, int);
static int	ssdfb_i2c_cmd(void *, uint8_t *, size_t, bool);
static int	ssdfb_i2c_transfer_rect(void *, uint8_t, uint8_t, uint8_t,
				uint8_t, uint8_t *, size_t, bool);
static int	ssdfb_i2c_transfer_rect_ssd1306(void *, uint8_t, uint8_t,
				uint8_t, uint8_t, uint8_t *, size_t, bool);
static int	ssdfb_i2c_transfer_rect_sh1106(void *, uint8_t, uint8_t,
				uint8_t, uint8_t, uint8_t *, size_t, bool);
static int	ssdfb_smbus_transfer_rect(void *, uint8_t, uint8_t, uint8_t,
				uint8_t, uint8_t *, size_t, bool);

CFATTACH_DECL_NEW(ssdfb_iic, sizeof(struct ssdfb_i2c_softc),
    ssdfb_i2c_match, ssdfb_i2c_attach, ssdfb_i2c_detach, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "solomon,ssd1306fb-i2c",
	  .value = SSDFB_PRODUCT_SSD1306_GENERIC },

	{ .compat = "sino,sh1106fb-i2c",
	  .value = SSDFB_PRODUCT_SH1106_GENERIC },

	DEVICE_COMPAT_EOL
};

static int
ssdfb_i2c_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	switch (ia->ia_addr) {
	case SSDFB_I2C_DEFAULT_ADDR:
	case SSDFB_I2C_ALTERNATIVE_ADDR:
		return I2C_MATCH_ADDRESS_ONLY;
	}

	return 0;
}

static void
ssdfb_i2c_attach(device_t parent, device_t self, void *aux)
{
	struct ssdfb_i2c_softc *sc = device_private(self);
	struct cfdata *cf = device_cfdata(self);
	struct i2c_attach_args *ia = aux;
	int flags = cf->cf_flags;

	if ((flags & SSDFB_ATTACH_FLAG_PRODUCT_MASK) == SSDFB_PRODUCT_UNKNOWN) {
		const struct device_compatible_entry *dce =
		    iic_compatible_lookup(ia, compat_data);
		if (dce != NULL) {
			flags |= (int)dce->value;
		}
	}
	if ((flags & SSDFB_ATTACH_FLAG_PRODUCT_MASK) == SSDFB_PRODUCT_UNKNOWN)
		flags |= SSDFB_PRODUCT_SSD1306_GENERIC;

	flags |= SSDFB_ATTACH_FLAG_MPSAFE;
	sc->sc.sc_dev = self;
	sc->sc_i2c_tag = ia->ia_tag;
	sc->sc_i2c_addr = ia->ia_addr;
	sc->sc.sc_cookie = (void *)sc;
	sc->sc.sc_cmd = ssdfb_i2c_cmd;
	sc->sc.sc_transfer_rect = ssdfb_i2c_transfer_rect;

	ssdfb_attach(&sc->sc, flags);
}

static int
ssdfb_i2c_detach(device_t self, int flags)
{
	struct ssdfb_i2c_softc *sc = device_private(self);

	return ssdfb_detach(&sc->sc);
}

static int
ssdfb_i2c_probe_transfer_size(struct ssdfb_i2c_softc *sc, bool usepoll)
{
	int flags = usepoll ? I2C_F_POLL : 0;
	uint8_t cb = SSDFB_I2C_CTRL_BYTE_DATA_MASK;
	int error;
	uint8_t buf[128];
	size_t len;

	error = iic_acquire_bus(sc->sc_i2c_tag, flags);
	if (error)
		return error;
	len = sizeof(buf);
	memset(buf, 0, len);
	while (len > 0) {
		error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_i2c_addr, &cb, sizeof(cb), buf, len, flags);
		if (!error) {
			break;
		}
		len >>= 1;
	}
	if (!error && len < 2) {
		error = E2BIG;
	} else {
		sc->sc_transfer_size = len;
	}
	(void) iic_release_bus(sc->sc_i2c_tag, flags);

	return error;
}

static int
ssdfb_i2c_transfer(struct ssdfb_i2c_softc *sc, uint8_t cb, uint8_t *data,
		   size_t len, int flags)
{
	int error;
	size_t xfer_size = sc->sc_transfer_size;

	while (len >= xfer_size) {
		error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_i2c_addr, &cb, sizeof(cb), data, xfer_size, flags);
		if (error)
			return error;
		len -= xfer_size;
		data += xfer_size;
	}
	if (len > 0) {
		error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_i2c_addr, &cb, sizeof(cb), data, len, flags);
	}

	return error;
}

static int
ssdfb_i2c_cmd(void *cookie, uint8_t *cmd, size_t len, bool usepoll)
{
	struct ssdfb_i2c_softc *sc = (struct ssdfb_i2c_softc *)cookie;
	int flags = usepoll ? I2C_F_POLL : 0;
	uint8_t cb = 0;
	int error;

	error = iic_acquire_bus(sc->sc_i2c_tag, flags);
	if (error)
		return error;
	error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_i2c_addr, &cb, sizeof(cb), cmd, len, flags);
	(void) iic_release_bus(sc->sc_i2c_tag, flags);

	return error;
}

static int
ssdfb_i2c_transfer_rect(void *cookie, uint8_t fromcol, uint8_t tocol,
    uint8_t frompage, uint8_t topage, uint8_t *p, size_t stride, bool usepoll)
{
	struct ssdfb_i2c_softc *sc = (struct ssdfb_i2c_softc *)cookie;
	uint8_t cmd[2];
	int error;

	/*
	 * Test if large transfers are supported by the parent i2c bus and
	 * pick the fastest transfer routine for subsequent invocations.
	 */
	switch (sc->sc.sc_p->p_controller_id) {
	case SSDFB_CONTROLLER_SSD1306:
		sc->sc.sc_transfer_rect = ssdfb_i2c_transfer_rect_ssd1306;
		break;
	case SSDFB_CONTROLLER_SH1106:
		sc->sc.sc_transfer_rect = ssdfb_i2c_transfer_rect_sh1106;
		break;
	default:
		sc->sc.sc_transfer_rect = ssdfb_smbus_transfer_rect;
		break;
	}

	if (sc->sc.sc_transfer_rect != ssdfb_smbus_transfer_rect) {
		error = ssdfb_i2c_probe_transfer_size(sc, usepoll);
		if (error)
			return error;
		aprint_verbose_dev(sc->sc.sc_dev, "%zd-byte transfers\n",
		    sc->sc_transfer_size);
		if (sc->sc_transfer_size == 2) {
			sc->sc.sc_transfer_rect = ssdfb_smbus_transfer_rect;
		}
	}

	/*
	 * Set addressing mode for SSD1306.
	 */
	if (sc->sc.sc_p->p_controller_id == SSDFB_CONTROLLER_SSD1306) {
		cmd[0] = SSD1306_CMD_SET_MEMORY_ADDRESSING_MODE;
		cmd[1] = sc->sc.sc_transfer_rect
		    == ssdfb_i2c_transfer_rect_ssd1306
		    ? SSD1306_MEMORY_ADDRESSING_MODE_HORIZONTAL
		    : SSD1306_MEMORY_ADDRESSING_MODE_PAGE;
		error = ssdfb_i2c_cmd(cookie, cmd, sizeof(cmd), usepoll);
		if (error)
			return error;
	}

	return sc->sc.sc_transfer_rect(cookie, fromcol, tocol, frompage, topage,
	    p, stride, usepoll);
}


static int
ssdfb_i2c_transfer_rect_ssd1306(void *cookie, uint8_t fromcol, uint8_t tocol,
    uint8_t frompage, uint8_t topage, uint8_t *p, size_t stride, bool usepoll)
{
	struct ssdfb_i2c_softc *sc = (struct ssdfb_i2c_softc *)cookie;
	int flags = usepoll ? I2C_F_POLL : 0;
	uint8_t cc = 0;
	uint8_t cb = SSDFB_I2C_CTRL_BYTE_DATA_MASK;
	size_t len = tocol + 1 - fromcol;
	int error;
	/*
	 * SSD1306 does not implement the Continuation bit correctly.
	 * The SH1106 protocol defines that a control byte WITH Co
	 * set must be inserted between each command. But SSD1306
	 * fails to parse the commands if we do that.
	 */
	uint8_t cmds[] = {
		SSD1306_CMD_SET_COLUMN_ADDRESS,
		fromcol, tocol,
		SSD1306_CMD_SET_PAGE_ADDRESS,
		frompage, topage
	};

	error = iic_acquire_bus(sc->sc_i2c_tag, flags);
	if (error)
		return error;
	error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_i2c_addr, &cc, sizeof(cc), cmds, sizeof(cmds), flags);
	if (error)
		goto out;
	while (frompage <= topage) {
		error = ssdfb_i2c_transfer(sc, cb, p, len, flags);
		if (error)
			goto out;
		frompage++;
		p += stride;
	}
out:
	(void) iic_release_bus(sc->sc_i2c_tag, flags);

	return error;
}

static int
ssdfb_i2c_transfer_rect_sh1106(void *cookie, uint8_t fromcol, uint8_t tocol,
    uint8_t frompage, uint8_t topage, uint8_t *p, size_t stride, bool usepoll)
{
	struct ssdfb_i2c_softc *sc = (struct ssdfb_i2c_softc *)cookie;
	int flags = usepoll ? I2C_F_POLL : 0;
	uint8_t cb = SSDFB_I2C_CTRL_BYTE_DATA_MASK;
	uint8_t cc = SSDFB_I2C_CTRL_BYTE_CONTINUATION_MASK;
	size_t len = tocol + 1 - fromcol;
	int error;
	uint8_t cmds[] = {
		SSDFB_CMD_SET_PAGE_START_ADDRESS_BASE + frompage,
		SSDFB_I2C_CTRL_BYTE_CONTINUATION_MASK,
		SSDFB_CMD_SET_HIGHER_COLUMN_START_ADDRESS_BASE + (fromcol >> 4),
		SSDFB_I2C_CTRL_BYTE_CONTINUATION_MASK,
		SSDFB_CMD_SET_LOWER_COLUMN_START_ADDRESS_BASE + (fromcol & 0xf)
	};

	error = iic_acquire_bus(sc->sc_i2c_tag, flags);
	if (error)
		return error;
	while (frompage <= topage) {
		cmds[0] = SSDFB_CMD_SET_PAGE_START_ADDRESS_BASE + frompage;
		error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_i2c_addr, &cc, sizeof(cc), cmds, sizeof(cmds), flags);
		if (error)
			goto out;
		error = ssdfb_i2c_transfer(sc, cb, p, len, flags);
		if (error)
			goto out;
		frompage++;
		p += stride;
	}
out:
	(void) iic_release_bus(sc->sc_i2c_tag, flags);

	return error;
}

/*
 * If the parent is an SMBus, then we can only send 2 bytes
 * of payload per txn. The SSD1306 triple byte commands are
 * not available so we have to use PAGE addressing mode
 * and split data into multiple txns.
 * This is ugly and slow but it's the best we can do.
 */
static int
ssdfb_smbus_transfer_rect(void *cookie, uint8_t fromcol, uint8_t tocol,
    uint8_t frompage, uint8_t topage, uint8_t *p, size_t stride, bool usepoll)
{
	struct ssdfb_i2c_softc *sc = (struct ssdfb_i2c_softc *)cookie;
	int flags = usepoll ? I2C_F_POLL : 0;
	uint8_t cb = SSDFB_I2C_CTRL_BYTE_DATA_MASK;
	uint8_t cc = 0;
	size_t len = tocol + 1 - fromcol;
	uint8_t cmd_higher_col =
	    SSDFB_CMD_SET_HIGHER_COLUMN_START_ADDRESS_BASE + (fromcol >> 4);
	uint8_t cmd_lower_col =
	    SSDFB_CMD_SET_LOWER_COLUMN_START_ADDRESS_BASE + (fromcol & 0xf);
	uint8_t cmd_page;
	uint8_t data[2];
	uint8_t *colp;
	uint8_t *endp;
	int error;

	error = iic_acquire_bus(sc->sc_i2c_tag, flags);
	if (error)
		return error;
	while (frompage <= topage) {
		error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_i2c_addr, &cc, sizeof(cc),
		    &cmd_higher_col, sizeof(cmd_higher_col), flags);
		if (error)
			goto out;
		error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_i2c_addr, &cc, sizeof(cc),
		    &cmd_lower_col, sizeof(cmd_lower_col), flags);
		if (error)
			goto out;
		cmd_page = SSDFB_CMD_SET_PAGE_START_ADDRESS_BASE + frompage;
		error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
		    sc->sc_i2c_addr, &cc, sizeof(cc),
		    &cmd_page, sizeof(cmd_page), flags);
		if (error)
			goto out;
		colp = p;
		endp = colp + len;
		if (len & 1) {
			data[0] = *colp++;
			error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
			    sc->sc_i2c_addr, &cb, sizeof(cb), data, 1, flags);
			if (error)
				goto out;
		}
		while (colp < endp) {
			/*
			 * Send two bytes at a time. We can't use colp directly
			 * because i2c controllers sometimes have data alignment
			 * requirements.
			 */
			data[0] = *colp++;
			data[1] = *colp++;
			error = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
			    sc->sc_i2c_addr, &cb, sizeof(cb), data, 2, flags);
			if (error)
				goto out;
		}
		frompage++;
		p += stride;
	}
out:
	(void) iic_release_bus(sc->sc_i2c_tag, flags);

	return error;
}
