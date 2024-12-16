/*	$NetBSD: printumcpmio.h,v 1.1 2024/12/16 16:37:40 brad Exp $	*/

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


#ifndef _PRINTUMCPMIO_H_
#define _PRINTUMCPMIO_H_

#include <dev/usb/umcpmio_hid_reports.h>

EXTERN void print_status(struct mcp2221_status_res *);
EXTERN void print_sram(struct mcp2221_get_sram_res *);
EXTERN void print_gpio_cfg(struct mcp2221_get_gpio_cfg_res *);
EXTERN void print_flash(struct mcp2221_get_flash_res *,int);

#endif
