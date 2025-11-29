/*	$NetBSD: umcpmio_subr.h,v 1.3 2025/11/29 18:39:14 brad Exp $	*/

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

#ifndef	_UMCPMIO_SUBR_H_
#define	_UMCPMIO_SUBR_H_

#include <sys/types.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_hid_reports.h>

struct umcpmio_mapping_put {
	const char	*tname;
	const uint8_t	mask;
};

void WAITMS(int);
void umcpmio_dump_buffer(bool, uint8_t *, u_int, const char *);
int mcp2210_decode_errors(uint8_t, int, uint8_t);
int mcp2210_get_status(struct umcpmio_softc *, struct mcp2210_status_res *);
int mcp2221_get_status(struct umcpmio_softc *, struct mcp2221_status_res *);
int mcp2221_put_status(struct umcpmio_softc *, struct mcp2221_status_req *,
    struct mcp2221_status_res *);
int mcp2221_get_sram(struct umcpmio_softc *, struct mcp2221_get_sram_res *);
int mcp2221_put_sram(struct umcpmio_softc *, struct mcp2221_set_sram_req *,
    struct mcp2221_set_sram_res *);
int mcp2210_get_nvram(struct umcpmio_softc *, uint8_t,
    struct mcp2210_get_nvram_res *);
int mcp2210_set_nvram(struct umcpmio_softc *, struct mcp2210_set_nvram_req *,
    struct mcp2210_set_nvram_res *);
int mcp2221_get_flash(struct umcpmio_softc *, uint8_t,
    struct mcp2221_get_flash_res *);
int mcp2221_put_flash(struct umcpmio_softc *, struct mcp2221_put_flash_req *,
    struct mcp2221_put_flash_res *);
int mcp2210_read_eeprom(struct umcpmio_softc *, uint8_t, uint8_t *);
int mcp2210_write_eeprom(struct umcpmio_softc *, uint8_t, uint8_t);

#endif	/* _UMCPMIO_SUBR_H_ */
