/*	$NetBSD: umcpmio_io.h,v 1.2 2025/03/17 18:24:08 riastradh Exp $	*/

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

#ifndef _UMCPMIO_IO_H_
#define _UMCPMIO_IO_H_

#include <sys/types.h>

#include <sys/ioccom.h>

#include <dev/usb/umcpmio_hid_reports.h>

struct umcpmio_ioctl_get_flash {
	uint8_t subcode;
	struct mcp2221_get_flash_res get_flash_res;
};

struct umcpmio_ioctl_put_flash {
	uint8_t subcode;
	struct mcp2221_put_flash_req put_flash_req;
	struct mcp2221_put_flash_res put_flash_res;
};

#define UMCPMIO_GET_STATUS	_IOR('m', 1, struct mcp2221_status_res)
#define UMCPMIO_GET_SRAM	_IOR('m', 2, struct mcp2221_get_sram_res)
#define UMCPMIO_GET_GP_CFG	_IOR('m', 3, struct mcp2221_get_gpio_cfg_res)
#define UMCPMIO_GET_FLASH	_IOWR('m', 4, struct umcpmio_ioctl_get_flash)
#define UMCPMIO_PUT_FLASH	_IOWR('m', 5, struct umcpmio_ioctl_put_flash)

#endif	/* _UMCPMIO_IO_H_ */
