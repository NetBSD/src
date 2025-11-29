/*	$NetBSD: umcpmio_io.h,v 1.3 2025/11/29 18:39:14 brad Exp $	*/

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

#define UMCPMIO_CHIP_TYPE_2210 0x01
#define UMCPMIO_CHIP_TYPE_2221 0x02

union umcpmio_ioctl_get_status {
	struct mcp2210_status_res mcp2210_status_res;
	struct mcp2221_status_res mcp2221_status_res;
	uint8_t status_blob[UMCPMIO_RES_BUFFER_SIZE];
};

struct umcpmio_ioctl_get_flash {
	uint8_t subcode;
	union {
		struct mcp2210_get_nvram_res mcp2210_get_nvram_res;
		struct mcp2221_get_flash_res mcp2221_get_flash_res;
		uint8_t get_blob[UMCPMIO_RES_BUFFER_SIZE];
	} res;
};

struct umcpmio_ioctl_put_flash {
	uint8_t subcode;
	union {
		struct mcp2210_set_nvram_req mcp2210_set_req;
		struct mcp2221_put_flash_req mcp2221_put_req;
		uint8_t put_req_blob[UMCPMIO_REQ_BUFFER_SIZE];
	} req;
	union {
		struct mcp2210_set_nvram_res mcp2210_set_res;
		struct mcp2221_put_flash_res mcp2221_put_res;
		uint8_t put_res_blob[UMCPMIO_RES_BUFFER_SIZE];
	} res;
};

struct mcp2210_ioctl_get_sram {
	uint8_t cmd;
	uint8_t res[UMCPMIO_RES_BUFFER_SIZE];
};

#define UMCPMIO_GET_STATUS	_IOR('m', 1, union umcpmio_ioctl_get_status)
#define MCP2221_GET_SRAM	_IOR('m', 2, struct mcp2221_get_sram_res)
#define MCP2221_GET_GP_CFG	_IOR('m', 3, struct mcp2221_get_gpio_cfg_res)
#define UMCPMIO_GET_FLASH	_IOWR('m', 4, struct umcpmio_ioctl_get_flash)
#define UMCPMIO_PUT_FLASH	_IOWR('m', 5, struct umcpmio_ioctl_put_flash)
#define UMCPMIO_CHIP_TYPE	_IOR('m', 6, uint8_t)
#define MCP2210_GET_SRAM	_IOWR('m', 7, struct mcp2210_ioctl_get_sram)
#define MCP2210_CANCEL_SPI	_IOR('m', 8, struct mcp2210_cancel_spi_res)

#endif	/* _UMCPMIO_IO_H_ */
