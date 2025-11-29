/*	$NetBSD: putflash.h,v 1.2 2025/11/29 18:39:15 brad Exp $	*/

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


#ifndef _PUTFLASH_H_
#define _PUTFLASH_H_

#include <dev/usb/umcpmio_hid_reports.h>

EXTERN int mcp2221_parse_flash_gp_req(int, struct mcp2221_put_flash_req *, char **, int, int, bool);
EXTERN int mcp2210_parse_flash_gp_req(int, struct mcp2210_set_nvram_req *, char **, int, int, bool);
EXTERN int mcp2210_parse_flash_spi_req(int, struct mcp2210_set_nvram_req *, char **, int, int, bool);
EXTERN int mcp2210_parse_flash_usbkeyparams_req(int, struct mcp2210_set_nvram_req *, char **, int, int, bool);

#endif
