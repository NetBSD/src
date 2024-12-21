/*	$NetBSD: umcpmioctl.h,v 1.2 2024/12/21 13:48:32 brad Exp $	*/

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

#ifndef _UMCPMIOCTL_H_
#define _UMCPMIOCTL_H_

/* Top level commands */
#define UMCPMIO_GET 1
#define UMCPMIO_PUT 2
#define UMCPMIO_STATUS 3

/* Second level commands */

#define UMCPMIO_IOCTL_GET_SRAM 1
#define UMCPMIO_IOCTL_GET_GP_CFG 2
#define UMCPMIO_IOCTL_GET_FLASH 3

#define UMCPMIO_IOCTL_PUT_FLASH 1

/* Third level commands */

#define UMCPMIO_IOCTL_GET_FLASH_CS 0
#define UMCPMIO_IOCTL_GET_FLASH_GP 1
#define UMCPMIO_IOCTL_GET_FLASH_USBMAN 2
#define UMCPMIO_IOCTL_GET_FLASH_USBPROD 3
#define UMCPMIO_IOCTL_GET_FLASH_USBSN 4
#define UMCPMIO_IOCTL_GET_FLASH_CHIPSN 5

#define UMCPMIO_IOCTL_PUT_FLASH_GP 1

struct umcpmioctlcmd {
	const char	*cmd;
	const int	id;
	const char	*helpargs;
};

#endif
