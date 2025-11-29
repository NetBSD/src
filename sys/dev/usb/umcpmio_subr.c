/*	$NetBSD: umcpmio_subr.c,v 1.4 2025/11/29 18:39:15 brad Exp $	*/

/*
 * Copyright (c) 2024 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: umcpmio_subr.c,v 1.4 2025/11/29 18:39:15 brad Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_transport.h>
#include <dev/usb/umcpmio_subr.h>
#include <dev/usb/umcpmio_hid_reports.h>

#define UMCPMIO_DEBUG 1
#ifdef UMCPMIO_DEBUG
#define DPRINTF(x)	do { if (umcpmiodebug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (umcpmiodebug > (n)) printf x; } while (0)
extern int umcpmiodebug;
#else
#define DPRINTF(x)	__nothing
#define DPRINTFN(n,x)	__nothing
#endif

/* Handy functions that do a bunch of things, very often they
 * don't fit anywhere else.
 */

void
WAITMS(int ms)
{
	if (ms > 0)
		delay(ms * 1000);
}

void
umcpmio_dump_buffer(bool enabled, uint8_t *buf, u_int len, const char *name)
{
	int i;

	if (enabled) {
		DPRINTF(("%s:", name));
		for (i = 0; i < len; i++) {
			DPRINTF((" %02x", buf[i]));
		}
		DPRINTF(("\n"));
	}
}


int
mcp2210_decode_errors(uint8_t request, int err, uint8_t general_result)
{
	int r = 0;

	if (err)
		return err;

	switch (general_result) {
	case MCP2210_CMD_COMPLETE_OK:
		break;
	case MCP2210_CMD_UNKNOWN:
		r = EINVAL;
		break;
	case MCP2210_CMD_ACCESS_REJECTED:
	case MCP2210_CMD_ACCESS_DENIED:
	case MCP2210_CMD_EEPROM_LOCKED:
		r = EPERM;
		break;
	case MCP2210_CMD_SPI_BUS_UNAVAIL:
	case MCP2210_CMD_USB_TRANSFER_IP:
		r = EBUSY;
		break;
	case MCP2210_CMD_EEPROM_FAIL:
	default:
		r = EIO;
	}

	return r;
}

int
mcp2221_get_status(struct umcpmio_softc *sc,
    struct mcp2221_status_res *res)
{
	struct mcp2221_status_req req;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	req.cmd = MCP2221_CMD_STATUS;

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	return err;
}

int
mcp2210_get_status(struct umcpmio_softc *sc,
    struct mcp2210_status_res *res)
{
	struct mcp2210_status_req req;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2210_REQ_BUFFER_SIZE);
	req.cmd = MCP2210_CMD_STATUS;

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	err = mcp2210_decode_errors(req.cmd, err, res->completion);

	return err;
}

int
mcp2221_put_status(struct umcpmio_softc *sc,
    struct mcp2221_status_req *req, struct mcp2221_status_res *res)
{
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	req->cmd = MCP2221_CMD_STATUS;

	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	return err;
}

int
mcp2221_get_sram(struct umcpmio_softc *sc,
    struct mcp2221_get_sram_res *res)
{
	struct mcp2221_get_sram_req req;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	req.cmd = MCP2221_CMD_GET_SRAM;

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	return err;
}

int
mcp2221_put_sram(struct umcpmio_softc *sc,
    struct mcp2221_set_sram_req *req, struct mcp2221_set_sram_res *res)
{
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	req->cmd = MCP2221_CMD_SET_SRAM;

	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	return err;
}

int
mcp2210_get_nvram(struct umcpmio_softc *sc, uint8_t subcode,
    struct mcp2210_get_nvram_res *res)
{
	struct mcp2210_get_nvram_req req;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2210_REQ_BUFFER_SIZE);
	req.cmd = MCP2210_CMD_GET_NVRAM;

	if (subcode < MCP2210_NVRAM_SUBCODE_SPI ||
	    subcode > MCP2210_NVRAM_SUBCODE_USBMAN)
		return EINVAL;

	req.subcode = subcode;

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	err = mcp2210_decode_errors(req.cmd, err, res->completion);

	return err;
}

int
mcp2210_set_nvram(struct umcpmio_softc *sc, struct mcp2210_set_nvram_req *req,
    struct mcp2210_set_nvram_res *res)
{
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	req->cmd = MCP2210_CMD_SET_NVRAM;

	if (req->subcode < MCP2210_NVRAM_SUBCODE_SPI ||
	    req->subcode > MCP2210_NVRAM_SUBCODE_USBMAN) {
		DPRINTF(("mcp2210_set_nvram: subcode out of range:"
		    " subcode=%d\n",
		    req->subcode));
		return EINVAL;
	}
	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	err = mcp2210_decode_errors(req->cmd, err, res->completion);

	return err;
}

int
mcp2221_get_flash(struct umcpmio_softc *sc, uint8_t subcode,
    struct mcp2221_get_flash_res *res)
{
	struct mcp2221_get_flash_req req;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	req.cmd = MCP2221_CMD_GET_FLASH;

	if (subcode < MCP2221_FLASH_SUBCODE_CS ||
	    subcode > MCP2221_FLASH_SUBCODE_CHIPSN)
		return EINVAL;

	req.subcode = subcode;

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	return err;
}

int
mcp2221_put_flash(struct umcpmio_softc *sc, struct mcp2221_put_flash_req *req,
    struct mcp2221_put_flash_res *res)
{
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	req->cmd = MCP2221_CMD_SET_FLASH;

	if (req->subcode < MCP2221_FLASH_SUBCODE_CS ||
	    req->subcode > MCP2221_FLASH_SUBCODE_CHIPSN) {
		DPRINTF(("mcp2221_put_flash: subcode out of range:"
		    " subcode=%d\n",
		    req->subcode));
		return EINVAL;
	}
	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	return err;
}

int
mcp2210_read_eeprom(struct umcpmio_softc *sc, uint8_t addr, uint8_t *buf)
{
	struct mcp2210_read_eeprom_req req;
	struct mcp2210_read_eeprom_res res;
	int err = 0;

	req.cmd = MCP2210_CMD_READ_EEPROM;
	req.addr = addr;

	mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) & res, sc->sc_cv_wait);
	mutex_exit(&sc->sc_action_mutex);

	err = mcp2210_decode_errors(req.cmd, err, res.completion);

	if (!err)
		*buf = res.value;

	return err;
}

int
mcp2210_write_eeprom(struct umcpmio_softc *sc, uint8_t addr, uint8_t buf)
{
	struct mcp2210_write_eeprom_req req;
	struct mcp2210_write_eeprom_res res;
	int err = 0;

	req.cmd = MCP2210_CMD_WRITE_EEPROM;
	req.addr = addr;
	req.value = buf;

	mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) & res, sc->sc_cv_wait);
	mutex_exit(&sc->sc_action_mutex);

	err = mcp2210_decode_errors(req.cmd, err, res.completion);

	return err;
}
