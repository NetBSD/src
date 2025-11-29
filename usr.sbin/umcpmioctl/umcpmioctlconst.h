/*	$NetBSD: umcpmioctlconst.h,v 1.2 2025/11/29 18:39:15 brad Exp $	*/

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

#ifndef _UMCPMIOCTLCONST_H_
#define _UMCPMIOCTLCONST_H_

/* These structures describe the command line command structure */

static const struct umcpmioctlcmd umcpmioctlcmds[] = {
	{
		.cmd = "get",
		.id = UMCPMIO_GET,
		.helpargs = "sram|gp|flash cs|flash gp|flash usbman|flash usbprod|flash usbsn|flash chipsn|type"

	},
	{
		.cmd = "put",
		.id = UMCPMIO_PUT,
		.helpargs = "flash gp"

	},
	{
		.cmd = "status",
		.id = UMCPMIO_STATUS,
		.helpargs = ""
	},
	{
		.cmd = "send",
		.id = UMCPMIO_SEND,
		.helpargs = "spi_cancel"
	}
};

static const struct umcpmioctlcmd getsubcmds[] = {
	{
		.cmd = "sram",
		.id = UMCPMIO_IOCTL_GET_SRAM,
		.helpargs = ""
	},
	{
		.cmd = "gp",
		.id = MCP2221_IOCTL_GET_GP_CFG,
		.helpargs = ""
	},
	{
		.cmd = "flash",
		.id = UMCPMIO_IOCTL_GET_FLASH,
		.helpargs = "cs | gp | usbman | usbprod | usbsn | chipsn | spi | usbkeyparams"
	},
	{
		.cmd = "type",
		.id = UMCPMIO_IOCTL_GET_TYPE,
		.helpargs = ""
	}
};

static const struct umcpmioctlcmd getsubsubcmds[] = {
	{
		.cmd = "gpio",
		.id = MCP2210_IOCTL_GET_SRAM_GPIO,
		.helpargs = ""
	},
	{
		.cmd = "gpio_values",
		.id = MCP2210_IOCTL_GET_SRAM_GPIO_VAL,
		.helpargs = ""
	},
	{
		.cmd = "gpio_direction",
		.id = MCP2210_IOCTL_GET_SRAM_GPIO_DIR,
		.helpargs = ""
	},
	{
		.cmd = "spi",
		.id = MCP2210_IOCTL_GET_SRAM_SPI,
		.helpargs = ""
	}
};

static const struct umcpmioctlcmd getflashsubcmds[] = {
	{
		.cmd = "cs",
		.id = UMCPMIO_IOCTL_GET_FLASH_CS,
		.helpargs = ""
	},
	{
		.cmd = "gp",
		.id = UMCPMIO_IOCTL_GET_FLASH_GP,
		.helpargs = ""
	},
	{
		.cmd = "usbman",
		.id = UMCPMIO_IOCTL_GET_FLASH_USBMAN,
		.helpargs = ""
	},
	{
		.cmd = "usbprod",
		.id = UMCPMIO_IOCTL_GET_FLASH_USBPROD,
		.helpargs = ""
	},
	{
		.cmd = "usbsn",
		.id = UMCPMIO_IOCTL_GET_FLASH_USBSN,
		.helpargs = ""
	},
	{
		.cmd = "chipsn",
		.id = UMCPMIO_IOCTL_GET_FLASH_CHIPSN,
		.helpargs = ""
	},
	{
		.cmd = "spi",
		.id = UMCPMIO_IOCTL_GET_NVRAM_SPI,
		.helpargs = ""
	},
	{
		.cmd = "usbkeyparams",
		.id = UMCPMIO_IOCTL_GET_NVRAM_USBKEYPARAMS,
		.helpargs = ""
	},
};

static const struct umcpmioctlcmd putsubcmds[] = {
	{
		.cmd = "flash",
		.id = UMCPMIO_IOCTL_PUT_FLASH,
		.helpargs = ""
	}
};

static const struct umcpmioctlcmd putflashsubcmds[] = {
	{
		.cmd = "gp",
		.id = UMCPMIO_IOCTL_PUT_FLASH_GP,
		.helpargs = "GPn (n is 0-8) GPIO_PIN_INPUT\n\t\t\t\tGPIO_PIN_OUTPUT\n\t\t\t\tGPIO_PIN_ALTn (n is 0-6)\n\t\t\t\tDEFAULT_OUTPUT_ZERO\n\t\t\t\tDEFAULT_OUTPUT_ONE....",
	},
	{
		.cmd = "cs",
		.id = UMCPMIO_IOCTL_PUT_FLASH_GP,
		.helpargs = "\n\t\t\t\talias for 'put flash gp'"
	},
	{
		.cmd = "spi",
		.id = MCP2210_IOCTL_PUT_NVRAM_SPI,
		.helpargs = "CS_TO_DATA_DELAY <num>\n\t\t\t\tLAST_BYTE_TO_CS_DELAY <num>\n\t\t\t\tDELAY_BETWEEN_BYTES <num>"
	},
	{
		.cmd = "usbkeyparams",
		.id = MCP2210_IOCTL_PUT_NVRAM_USBKEYPARAMS,
		.helpargs = "POWERED SELF|BUS\n\t\t\t\t\t\tMA <num 1 - 511>"
	}
};

static const struct umcpmioctlcmd sendsubcmds[] = {
	{
		.cmd = "spi_cancel",
		.id = MCP2210_IOCTL_SEND_CANCEL,
		.helpargs = "spi_cancel"
	}
};

#endif
