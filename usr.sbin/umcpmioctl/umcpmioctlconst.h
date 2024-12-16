/*	$NetBSD: umcpmioctlconst.h,v 1.1 2024/12/16 16:37:40 brad Exp $	*/

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
		.helpargs = "sram|gp|flash cs|flash gp|flash usbman|flash usbprod|flash usbsn|flash chipsn"

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
		.id = UMCPMIO_IOCTL_GET_GP_CFG,
		.helpargs = ""
	},
	{
		.cmd = "flash",
		.id = UMCPMIO_IOCTL_GET_FLASH,
		.helpargs = "cs | gp | usbman | usbprod | usbsn | chipsn"
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
	}
};

static const struct umcpmioctlcmd putsubcmds[] = {
	{
		.cmd = "flash",
		.id = UMCPMIO_IOCTL_PUT_FLASH,
		.helpargs = "gp GPn GPIO_PIN_INPUT\n\t\t\t\tGPIO_PIN_OUTPUT\n\t\t\t\tGPIO_PIN_ALT0\n\t\t\t\tGPIO_PIN_ALT1\n\t\t\t\tGPIO_PIN_ALT2\n\t\t\t\tGPIO_PIN_ALT3\n\t\t\t\tDEFAULT_OUTPUT_ZERO\n\t\t\t\tDEFAULT_OUTPUT_ONE....",
	}
};

static const struct umcpmioctlcmd putflashsubcmds[] = {
	{
		.cmd = "gp",
		.id = UMCPMIO_IOCTL_PUT_FLASH_GP,
		.helpargs = ""
	}
};


#endif
